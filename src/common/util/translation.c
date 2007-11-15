/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/
#include <config.h>

#include "translation.h"
#include "log.h"
#include "bsdstring.h"

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
	variant * begin;
  variant * top;
  int size;
} opstack;

variant
opstack_pop(opstack ** stackp)
{
  opstack * stack = *stackp;
  
  assert(stack);
  assert(stack->top>stack->begin);
  return *(--stack->top);
}

void
opstack_push(opstack ** stackp, variant data)
{
  opstack * stack = *stackp;
  if (stack==NULL) {
    stack = (opstack*)malloc(sizeof(opstack));
    stack->size = 1;
    stack->begin = malloc(sizeof(variant) * stack->size);
    stack->top = stack->begin;
    *stackp = stack;
  }
  if (stack->top - stack->begin == stack->size) {
    size_t pos = stack->top - stack->begin;
    stack->size += stack->size;
    stack->begin = realloc(stack->begin, sizeof(variant) * stack->size);
    stack->top = stack->begin + pos;
	}
  *stack->top++ = data;
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
	variant value;
} variable;

static variable * variables;

static void
free_variables(void)
{
	variables = NULL;
}

static void
add_variable(const char * symbol, variant value)
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
  boolean braces = false;
  char symbol[32];
  char *cp = symbol; /* current position */

  if (*in=='{') {
    braces = true;
    ++in;
  }
  while (isalnum(*in) || *in=='.') *cp++ = *in++;
  *cp = '\0';
  /* symbol will now contain the symbol name */
  if (*in=='(') {
    /* it's a function we need to parse, start by reading the parameters */
    function * foo;
    while (*in != ')') {
      in = parse(stack, ++in, userdata); /* will push the result on the stack */
      if (in==NULL) return NULL;
    }
    ++in;
    foo = find_function(symbol);
    if (foo==NULL) {
      log_error(("parser does not know about \"%s\" function.\n", symbol));
      return NULL;
    }
    foo->parse(stack, userdata); /* will pop parameters from stack (reverse order!) and push the result */
  } else {
    variable * var = find_variable(symbol);
    if (braces && *in=='}') {
      ++in;
    }
    /* it's a constant (variable is a misnomer, but heck, const was taken;)) */
    if (var==NULL) {
      log_error(("parser does not know about \"%s\" variable.\n", symbol));
      return NULL;
    }
    opush(stack, var->value);
  }
  return in;
}

#define TOKENSIZE 4096
static const char *
parse_string(opstack ** stack, const char* in, const void * userdata) /* (char*) -> char* */
{
  char * c;
  char * buffer = balloc(TOKENSIZE);
  size_t size = TOKENSIZE - 1;
  const char * ic = in;
  char * oc = buffer;
  /* mode flags */
  boolean f_escape = false;
  boolean bDone = false;
  variant var;

  while (*ic && !bDone) {
    if (f_escape) {
      f_escape = false;
      switch (*ic) {
        case 'n':
          if (size>0) { *oc++='\n'; --size; }
          break;
        case 't':
          if (size>0) { *oc++='\t'; --size; }
          break;
        default:
          if (size>0) { *oc++=*ic++; --size; }
      }
    } else {
      int ch = (unsigned char)(*ic);
      int bytes;

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
          c = (char*)opop_v(stack);
          bytes = (int)strlcpy(oc, c, size);
          if (bytes<(int)size) oc += bytes;
          else oc += size;
          bfree(c);
          break;
        default:
          if (size>0) { *oc++=*ic++; --size; }
          else ++ic;
      }
    }
  }
  *oc++ = '\0';
  bfree(oc);
  var.v = buffer;
  opush(stack, var);
  return ic;
}

static const char *
parse_int(opstack ** stack, const char * in)
{
  int k = 0;
  int vz = 1;
  boolean ok = false;
  variant var;
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
  while (isdigit(*(unsigned char*)in)) {
    k = k * 10 + (*in++)-'0';
  }
  var.i = k*vz;
  opush(stack, var);
  return in;
}


static const char *
parse(opstack ** stack, const char* inn, const void * userdata)
{
  const char * b = inn;
  while (*b) {
    switch (*b) {
      case '"':
        return parse_string(stack, ++b, userdata);
        break;
      case '$':
        return parse_symbol(stack, ++b, userdata);
        break;
      default:
        if (isdigit(*(unsigned char*)b) || *b=='-' || *b=='+') {
          return parse_int(stack, b);
        }
        else ++b;
    }
  }
  log_error(("could not parse \"%s\". Probably invalid message syntax.", inn));
  return NULL;
}

const char * 
translate(const char* format, const void * userdata, const char* vars, variant args[])
{
  int i = 0;
  const char *ic = vars;
  char symbol[32];
  char *oc = symbol;
  opstack * stack = NULL;
  const char * rv;

  brelease();
  free_variables();

  assert(format);
  assert(*ic == 0 || isalnum(*ic));
  while (*ic) {
    *oc++ = *ic++;
    if (!isalnum(*ic)) {
      variant x = args[i++];
      *oc = '\0';
      oc = symbol;
      add_variable(strcpy(balloc(strlen(symbol)+1), symbol), x);
      while (*ic && !isalnum(*ic)) ++ic;
    }
  }

  if (parse(&stack, format, userdata)==NULL) return NULL;
  rv = (const char*)opop(&stack).v;
  free(stack->begin);
  free(stack);
  return rv;
}

static void
eval_lt(opstack ** stack, const void * userdata) /* (int, int) -> int */
{
  int a = opop_i(stack);
  int b = opop_i(stack);
  int rval = (b<a)?1:0;
  opush_i(stack, rval);
  unused(userdata);
}

static void
eval_eq(opstack ** stack, const void * userdata) /* (int, int) -> int */
{
  int a = opop_i(stack);
  int b = opop_i(stack);
  int rval = (a==b)?1:0;
  opush_i(stack, rval);
  unused(userdata);
}

static void
eval_add(opstack ** stack, const void * userdata) /* (int, int) -> int */
{
	int a = opop_i(stack);
	int b = opop_i(stack);
	opush_i(stack, a+b);
	unused(userdata);
}

static void
eval_isnull(opstack ** stack, const void * userdata) /* (int, int) -> int */
{
	void * a = opop_v(stack);
	opush_i(stack, (a==NULL)?1:0);
	unused(userdata);
}

static void
eval_if(opstack ** stack, const void * userdata) /* (int, int) -> int */
{
	void * a = opop_v(stack);
	void * b = opop_v(stack);
	int cond = opop_i(stack);
	opush_v(stack, cond?b:a);
	unused(userdata);
}

static void
eval_strlen(opstack ** stack, const void * userdata) /* string -> int */
{
	const char * c = (const char *)opop_v(stack);
	opush_i(stack, c?(int)strlen(c):0);
	unused(userdata);
}

#include "base36.h"
static void
eval_int(opstack ** stack, const void * userdata)
{
	int i = opop_i(stack);
	const char * c = itoa10(i);
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

void
translation_init(void)
{
  add_function("lt", &eval_lt);
  add_function("eq", &eval_eq);
  add_function("int", &eval_int);
  add_function("add", &eval_add);
  add_function("strlen", &eval_strlen);
  add_function("if", &eval_if);
  add_function("isnull", &eval_isnull);
}

void
translation_done(void)
{
	free_functions();
	free(buffer.begin);
}
