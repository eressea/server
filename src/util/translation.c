#ifdef _MSC_VER
#include <platform.h>
#endif

#include "translation.h"

#include "strings.h"
#include "critbit.h"
#include "log.h"
#include "macros.h"
#include "assert.h"

 /* libc includes */
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

/**
 ** simple operand stack
 **/

typedef struct opstack {
    variant *begin;
    variant *top;
    unsigned int size;
} opstack;

variant opstack_pop(opstack ** stackp)
{
    opstack *stack = *stackp;

    assert(stack);
    assert(stack->top > stack->begin);
    return *(--stack->top);
}

void opstack_push(opstack ** stackp, variant data)
{
    opstack *stack = *stackp;
    if (stack == NULL) {
        stack = (opstack *)malloc(sizeof(opstack));
        assert(stack);
        stack->size = 2;
        stack->begin = malloc(sizeof(variant) * stack->size);
        stack->top = stack->begin;
        *stackp = stack;
    }
    if (stack->top == stack->begin + stack->size) {
        size_t pos = stack->top - stack->begin;
        void *tmp;
        stack->size += stack->size;
        tmp = realloc(stack->begin, sizeof(variant) * stack->size);
        assert(tmp);
        stack->begin = (variant *)tmp;
        stack->top = stack->begin + pos;
    }
    *stack->top++ = data;
}

/**
 ** static buffer malloc
 **/

#define BBUFSIZE 0x10000
static struct {
    char *begin;
    char *handle_end;
    char *last;
    char *current;
} buffer;

char *balloc(size_t size)
{
    static int init = 0;
    if (!init) {
        init = 1;
        buffer.current = buffer.begin = malloc(BBUFSIZE * sizeof(char));
        buffer.handle_end = buffer.begin + BBUFSIZE;
    }
    assert(buffer.current + size <= buffer.handle_end || !"balloc is out of memory");
    buffer.last = buffer.current;
    buffer.current += size;
    return buffer.last;
}

void bfree(char *c)
/* only release this memory if it was part of the last allocation
 * that's a joke, but who cares.
 * I'm afraid I don't get the joke.
 */
{
    if (c >= buffer.last && c < buffer.current)
        buffer.current = c;
}

void brelease(void)
{
    buffer.last = buffer.current = buffer.begin;
}

/**
 ** constant values
 **/

typedef struct variable {
    struct variable *next;
    const char *symbol;
    variant value;
} variable;

static variable *variables;

static void free_variables(void)
{
    variables = NULL;
}

static void add_variable(const char *symbol, variant value)
{
    variable *var = (variable *)balloc(sizeof(variable));

    var->value = value;
    var->symbol = symbol;

    var->next = variables;
    variables = var;
}

static variable *find_variable(const char *symbol)
{
    variable *var = variables;
    while (var) {
        if (!strcmp(var->symbol, symbol))
            break;
        var = var->next;
    }
    return var;
}

/**
 ** constant values
 **/

static struct critbit_tree functions = { 0 };

static void free_functions(void)
{
    cb_clear(&functions);
    functions.root = 0;
}

void add_function(const char *symbol, evalfun parse)
{
    char token[64]; /* Flawfinder: ignore */
    size_t len = strlen(symbol);

    assert(len + sizeof(parse) < sizeof(token));
    len = cb_new_kv(symbol, len, &parse, sizeof(parse), token);
    cb_insert(&functions, token, len);
}

static evalfun find_function(const char *symbol)
{
    void * matches;
    if (cb_find_prefix(&functions, symbol, strlen(symbol) + 1, &matches, 1, 0)) {
        evalfun result;
        cb_get_kv(matches, &result, sizeof(result));
        return result;
    }
    return 0;
}

static const char *parse(opstack **, const char *in, const void *);
/* static const char * sample = "\"enno and $bool($if($eq($i,0),\"noone else\",\"$i other people\"))\""; */

static const char *parse_symbol(opstack ** stack, const char *in,
    const void *userdata)
    /* in is the symbol name and following text, starting after the $
     * result goes on the stack
     */
{
    bool braces = false;
    char symbol[32]; /* Flawfinder: ignore */
    char *cp = symbol;            /* current position */

    if (*in == '{') {
        braces = true;
        ++in;
    }
    while (isalnum(*in) || *in == '.') {
        *cp++ = *in++;
        assert(cp < symbol + sizeof(symbol));
    }
    *cp = '\0';
    /* symbol will now contain the symbol name */
    if (*in == '(') {
        /* it's a function we need to parse, start by reading the parameters */
        evalfun foo;

        while (*in != ')') {
            in = parse(stack, ++in, userdata);        /* will push the result on the stack */
            if (in == NULL)
                return NULL;
        }
        ++in;
        foo = find_function(symbol);
        if (foo == NULL) {
            log_error("parser does not know about \"%s\" function.\n", symbol);
            return NULL;
        }
        foo(stack, userdata);        /* will pop parameters from stack (reverse order!) and push the result */
    }
    else {
        variable *var = find_variable(symbol);
        if (braces && *in == '}') {
            ++in;
        }
        /* it's a constant (variable is a misnomer, but heck, const was taken;)) */
        if (var == NULL) {
            log_error("parser does not know about \"%s\" variable.\n", symbol);
            return NULL;
        }
        opush(stack, var->value);
    }
    return in;
}

#define TOKENSIZE 4096
static const char *parse_string(opstack ** stack, const char *in,
    const void *userdata)
{                               /* (char*) -> char* */
    char *c;
    char *buffer = balloc(TOKENSIZE);
    size_t size = TOKENSIZE - 1;
    const char *ic = in;
    char *oc = buffer;
    /* mode flags */
    bool f_escape = false;
    bool bDone = false;
    variant var;

    while (*ic && !bDone) {
        if (f_escape) {
            f_escape = false;
            switch (*ic) {
            case 'n':
                if (size > 0) {
                    *oc++ = '\n';
                    --size;
                }
                break;
            case 't':
                if (size > 0) {
                    *oc++ = '\t';
                    --size;
                }
                break;
            default:
                if (size > 0) {
                    *oc++ = *ic++;
                    --size;
                }
            }
        }
        else {
            int ch = (unsigned char)(*ic);
            size_t bytes;

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
                if (ic == NULL)
                    return NULL;
                c = (char *)opop_v(stack);
                bytes = (c ? str_strlcpy(oc, c, size) : 0);
                if (bytes < size)
                    oc += bytes;
                else
                    oc += size;
                bfree(c);
                break;
            default:
                if (size > 0) {
                    *oc++ = *ic++;
                    --size;
                }
                else
                    ++ic;
            }
        }
    }
    *oc++ = '\0';
    bfree(oc);
    var.v = buffer;
    opush(stack, var);
    return ic;
}

static const char *parse_int(opstack ** stack, const char *in)
{
    int k = 0;
    int vz = 1;
    bool ok = false;
    variant var;
    do {
        switch (*in) {
        case '+':
            ++in;
            break;
        case '-':
            ++in;
            vz = vz * -1;
            break;
        default:
            ok = true;
        }
    } while (!ok);
    while (isdigit(*(unsigned char *)in)) {
        k = k * 10 + (*in++) - '0';
    }
    var.i = k * vz;
    opush(stack, var);
    return in;
}

static const char *parse(opstack ** stack, const char *inn,
    const void *userdata)
{
    const char *b = inn;
    while (*b) {
        switch (*b) {
        case '"':
            return parse_string(stack, ++b, userdata);
            break;
        case '$':
            return parse_symbol(stack, ++b, userdata);
            break;
        default:
            if (isdigit(*(const unsigned char *)b) || *b == '-' || *b == '+') {
                return parse_int(stack, b);
            }
            else
                ++b;
        }
    }
    log_error("could not parse \"%s\". Probably invalid message syntax.", inn);
    return NULL;
}

const char *translate(const char *format, const void *userdata,
    const char *vars, variant args[])
{
    unsigned int i = 0;
    const char *ic = vars;
    char symbol[32]; /* Flawfinder: ignore */
    char *oc = symbol;
    opstack *stack = NULL;
    const char *rv;

    brelease();
    free_variables();

    assert(format);
    assert(*ic == 0 || isalnum(*ic));
    while (*ic) {
        *oc++ = *ic++;
        assert(oc < symbol + sizeof(symbol));
        if (!isalnum(*ic)) {
            size_t len;
            variant x = args[i++];
            *oc = '\0';
            len = oc - symbol + 1;
            str_strlcpy(oc = balloc(len), symbol, len);
            add_variable(oc, x);
            oc = symbol;
            while (*ic && !isalnum(*ic))
                ++ic;
        }
    }

    if (format[0] == '"') {
        rv = parse(&stack, format, userdata);
    }
    else {
        rv = parse_string(&stack, format, userdata);
    }
    if (rv != NULL) {
        if (rv[0]) {
            log_error("residual data after parsing: %s\n", rv);
        }
        rv = (const char *)opop(&stack).v;
        free(stack->begin);
        free(stack);
    }
    return rv;
}

static void eval_lt(opstack ** stack, const void *userdata)
{                               /* (int, int) -> int */
    int a = opop_i(stack);
    int b = opop_i(stack);
    int rval = (b < a) ? 1 : 0;
    opush_i(stack, rval);
    UNUSED_ARG(userdata);
}

static void eval_eq(opstack ** stack, const void *userdata)
{                               /* (int, int) -> int */
    int a = opop_i(stack);
    int b = opop_i(stack);
    int rval = (a == b) ? 1 : 0;
    opush_i(stack, rval);
    UNUSED_ARG(userdata);
}

static void eval_add(opstack ** stack, const void *userdata)
{                               /* (int, int) -> int */
    int a = opop_i(stack);
    int b = opop_i(stack);
    opush_i(stack, a + b);
    UNUSED_ARG(userdata);
}

static void eval_isnull(opstack ** stack, const void *userdata)
{                               /* (int, int) -> int */
    void *a = opop_v(stack);
    opush_i(stack, (a == NULL) ? 1 : 0);
    UNUSED_ARG(userdata);
}

static void eval_if(opstack ** stack, const void *userdata)
{                               /* (int, int) -> int */
    void *a = opop_v(stack);
    void *b = opop_v(stack);
    int cond = opop_i(stack);
    opush_v(stack, cond ? b : a);
    UNUSED_ARG(userdata);
}

#include "base36.h"
static void eval_int(opstack ** stack, const void *userdata)
{
    int i = opop_i(stack);
    const char *c = itoa10(i);
    size_t size = strlen(c) + 1; /* Flawfinder: ignore */
    variant var;

    str_strlcpy(var.v = balloc(size), c, size);
    opush(stack, var);
}

void translation_init(void)
{
    add_function("lt", &eval_lt);
    add_function("eq", &eval_eq);
    add_function("int", &eval_int);
    add_function("add", &eval_add);
    add_function("if", &eval_if);
    add_function("isnull", &eval_isnull);
}

void translation_done(void)
{
    free_functions();
    free(buffer.begin);
}
