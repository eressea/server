#include <config.h>

#include "translation.h"

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/**
 ** simple operand stack
 **/

typedef struct opstack {
	struct opstack * next;
	void * data;
} opstack;

void * 
opstack_pop(opstack ** stack)
{
	opstack * os = *stack;
	void * data;
	assert(os);
	data = os->data;
	*stack = os->next;
	free(os);
	return data;
}

void
opstack_push(opstack ** stack, void * data)
{
	opstack * os = (opstack*)malloc(sizeof(opstack));
	os->next = *stack;
	os->data = data;
	*stack = os;
}

/**
 ** static buffer malloc 
 **/

#define BUFSIZE 128*1024
static struct {
	char * begin;
	char * end;
	char * last;
	char * current;
} buffer;

char * 
balloc(size_t size)
{
	static int init = 0;
	if (!init) {
		init = 1;
		buffer.current = buffer.begin = malloc(BUFSIZE);
		buffer.end = buffer.begin + BUFSIZE;
	}
	if (buffer.current + size > buffer.end) {
		/* out of memory! */
		return NULL;
	}
	buffer.last = buffer.current;
	buffer.current +=size;
	return buffer.last;
}

void
bfree(char * c)
/* only release this memory if it was part of the last allocation 
 * that's a joke, but who cares.
 */
{
	if (c>=buffer.last && c<buffer.current) buffer.current = c;
}

void 
brelease(void)
{
	buffer.last = buffer.current = buffer.begin;
}


/**
 ** constant values
 **/

typedef struct variable {
	struct variable * next;
	const char * symbol;
	const void * value;
} variable;

static variable * variables;

static void
free_variables(void)
{
	variables = NULL;
}

static void
add_variable(const char * symbol, const void * value)
{
	variable * var = (variable*)balloc(sizeof(variable));

	var->value  = value;
	var->symbol = symbol;

	var->next   = variables;
	variables   = var;
}

static variable *
find_variable(const char * symbol)
{
	variable * var = variables;
	while (var) {
		if (!strcmp(var->symbol, symbol)) break;
		var = var->next;
	}
	return var;
}

/**
 ** constant values
 **/

typedef struct function {
	struct function * next;
	const char * symbol;
	evalfun parse;
} function;

static function * functions;

static void
free_functions(void)
{
	while (functions) {
		function * fun = functions;
		functions = fun->next;
		free(fun);
	}
}

void
add_function(const char * symbol, evalfun parse)
{
	function * fun = (function*)malloc(sizeof(function));

	fun->parse  = parse;
	fun->symbol = symbol;

	fun->next   = functions;
	functions   = fun;
}

static function *
find_function(const char * symbol)
{
	function * fun = functions;
	while (fun) {
		if (!strcmp(fun->symbol, symbol)) break;
		fun = fun->next;
	}
	return fun;
}

static const char * parse(opstack **, const char* in, const void *);
/* static const char * sample = "\"enno and $bool($if($eq($i,0),\"noone else\",\"$i other people\"))\""; */

static const char *
parse_symbol(opstack ** stack, const char* in, const void * userdata)
	/* in is the symbol name and following text, starting after the $ 
	 * result goes on the stack
	 */
{
	char symbol[32];
	char *cp = symbol; /* current position */

	while (isalnum(*in)) *cp++ = *in++;
	*cp = '\0';
	/* symbol will now contain the symbol name */
	if (*in=='(') {
		/* it's a function we need to parse 
		 * start by reading the parameters
		 */
		function * foo;
		while (*in != ')') {
			in = parse(stack, ++in, userdata); /* will push the result on the stack */
			if (in==NULL) return NULL;
		}
		++in;
		foo = find_function(symbol);
		if (foo==NULL) return NULL;
		foo->parse(stack, userdata); /* will pop parameters from stack (reverse order!) and push the result */
	} else {
		/* it's a constant (variable is a misnomer, but heck, const was taken;)) */
		variable * var = find_variable(symbol);
		if (var==NULL) return NULL;
		opush(stack, var->value);
	}
	return in;
}

static const char *
parse_string(opstack ** stack, const char* in, const void * userdata) /* (char*) -> char* */
{
	char * c;
	char * buffer = balloc(4*1024);
	const char * ic = in;
	char * oc = buffer;
	/* mode flags */
	boolean f_escape = false;
	boolean bDone = false;

	while (*ic && !bDone) {
		if (f_escape) {
			f_escape = false;
			switch (*ic) {
			case 'n':
				*oc++='\n';
				break;
			case 't':
				*oc++='\t';
				break;
			default:
				*oc++=*ic;
			}
		} else {
			int ch = (unsigned char)(*ic);
			switch (ch) {
			case '\\':
				f_escape = true;
				++ic;
				break;
			case '"':
				bDone = true;
				++ic;
				break;
			case '$':
				ic = parse_symbol(stack, ++ic, userdata);
				if (ic==NULL) return NULL;
				c = opop(stack, char*);
				oc += strlen(strcpy(oc, c));
				bfree(c);
				break;
			default:
				*oc++=*ic++;
			}
		}
	}
	*oc++ = '\0';
	bfree(oc);
	opush(stack, buffer);
	return ic;
}

static const char *
parse_int(opstack ** stack, const char * in)
{
	int k = 0;
	int vz = 1;
	boolean ok = false;
	do {
		switch (*in) {
		case '+':
			++in;
			break;
		case '-':
			++in;
			vz=vz*-1;
			break;
		default:
			ok = true;
		}
	} while (!ok);
	while (isdigit(*in)) {
		k = k * 10 + (*in++)-'0';
	}
	opush(stack, k*vz);
	return in;
}


static const char *
parse(opstack ** stack, const char* in, const void * userdata)
{
	while (*in) {
		switch (*in) {
		case '"':
			return parse_string(stack, ++in, userdata);
			break;
		case '$':
			return parse_symbol(stack, ++in, userdata);
			break;
		default:
			if (isdigit(*in) || *in=='-' || *in=='+') {
				return parse_int(stack, in);
			}
			else ++in;
		}
	}
	return in;
}

char * 
translate(const char* format, const void * userdata, const char* vars, const void* args[])
{
	int i = 0;
	char * retval;
	const char *ic = vars;
	char symbol[32];
	char *oc = symbol;
	opstack * stack = NULL;
	brelease();
	free_variables();

	assert(isalnum(*ic));
   while (*ic) {
		*oc++ = *ic++;
		if (!isalnum(*ic)) {
			const void * x = args[i++];
			*oc = '\0';
			oc = symbol;
			add_variable(strcpy(balloc(strlen(symbol)+1), symbol), x);
			while (*ic && !isalnum(*ic)) ++ic;
		}
   }

	if (parse(&stack, format, userdata)==NULL) return NULL;
	retval = strdup(opop(&stack, const char*));

	return retval;
}

char *
translate_va(const char* format, const void * userdata, const char* vars, ...)
{
	char * retval;
	va_list marker;
	const char *ic = vars;
	char symbol[32];
	char *oc = symbol;
	opstack * stack = NULL;
	brelease();
	free_variables();

   va_start(marker, vars);     /* Initialize variable arguments. */
	assert(isalnum(*ic));
   while (*ic) {
		*oc++ = *ic++;
		if (!isalnum(*ic)) {
			void * x = va_arg(marker, void *);
			*oc = '\0';
			oc = symbol;
			add_variable(strcpy(balloc(strlen(symbol)+1), symbol), x);
			while (*ic && !isalnum(*ic)) ++ic;
		}
   }
   va_end(marker);              /* Reset variable arguments.      */

	if (parse(&stack, format, userdata)==NULL) return NULL;
	retval = strdup(opop(&stack, const char*));

	return retval;
}

static void
eval_eq(opstack ** stack, const void * userdata) /* (int, int) -> int */
{
	int a = opop(stack, int);
	int b = opop(stack, int);
	int rval = (a==b)?1:0;
	opush(stack, rval);
	unused(userdata);
}

static void
eval_add(opstack ** stack, const void * userdata) /* (int, int) -> int */
{
	int a = opop(stack, int);
	int b = opop(stack, int);
	opush(stack, a+b);
	unused(userdata);
}

static void
eval_if(opstack ** stack, const void * userdata) /* (int, int) -> int */
{
	void * a = opop(stack, void *);
	void * b = opop(stack, void *);
	int cond = opop(stack, int);
	opush(stack, cond?b:a);
	unused(userdata);
}

static void
eval_strlen(opstack ** stack, const void * userdata) /* string -> int */
{
	const char * c = opop(stack, const char *);
	opush(stack, c?strlen(c):0);
	unused(userdata);
}

#include "base36.h"
static void
eval_int(opstack ** stack, const void * userdata)
{
	int i = opop(stack, int);
	const char * c = itoa10(i);
	opush(stack, strcpy(balloc(strlen(c)+1), c));
	unused(userdata);
}

#include "language.h"
static void
eval_localize(opstack ** stack, const void * userdata) /* (string, locale) -> string */
{
	const struct locale *lang = opop(stack, const struct locale *);
	const char *c = opop(stack, const char *);
	c = locale_string(lang, c);
	opush(stack, strcpy(balloc(strlen(c)+1), c));
	unused(userdata);
}

static void
eval_locale(opstack ** stack, const void * userdata) /* (string) -> locale */
{
	const char *c = opop(stack, const char *);
	const struct locale *lang = find_locale(c);
	opush(stack, lang);
	unused(userdata);
}

void
translation_init(void)
{
	add_function("eq", &eval_eq);
	add_function("int", &eval_int);
	add_function("add", &eval_add);
	add_function("strlen", &eval_strlen);
	add_function("if", &eval_if);
	add_function("localize", &eval_localize);
	add_function("locale", &eval_locale);
}

void
translation_done(void)
{
	free_functions();
	free(buffer.begin);
}
