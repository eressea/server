#pragma once
#ifndef H_UTIL_TRANSLATION
#define H_UTIL_TRANSLATION

#include <stddef.h>

#include "variant.h"

struct opstack;

/* transient memory blocks */
extern char *balloc(size_t size);

#define opush_i(stack, x) { variant localvar; localvar.i = x; opstack_push(stack, localvar); }
#define opush_v(stack, x) { variant localvar; localvar.v = x; opstack_push(stack, localvar); }
#define opush(stack, i) opstack_push(stack, i)
#define opop_v(stack) opstack_pop(stack).v
#define opop_i(stack) opstack_pop(stack).i
#define opop(stack) opstack_pop(stack)

void opstack_push(struct opstack **stack, variant data);

variant opstack_pop(struct opstack **stack);

void translation_init(void);
void translation_done(void);
const char *translate(const char *format, const void *userdata,
    const char *vars, variant args[]);

/* eval_x functions */
typedef void(*evalfun) (struct opstack ** stack, const void *);
void add_function(const char *symbol, evalfun parse);

#endif
