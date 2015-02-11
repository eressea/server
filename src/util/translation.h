/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#ifndef H_UTIL_TRANSLATION
#define H_UTIL_TRANSLATION
#ifdef __cplusplus
extern "C" {
#endif

#include "variant.h"
    struct opstack;
    extern void opstack_push(struct opstack **stack, variant data);
#define opush_i(stack, x) { variant localvar; localvar.i = x; opstack_push(stack, localvar); }
#define opush_v(stack, x) { variant localvar; localvar.v = x; opstack_push(stack, localvar); }
#define opush(stack, i) opstack_push(stack, i)

    extern variant opstack_pop(struct opstack **stack);
#define opop_v(stack) opstack_pop(stack).v
#define opop_i(stack) opstack_pop(stack).i
#define opop(stack) opstack_pop(stack)

    extern void translation_init(void);
    extern void translation_done(void);
    extern const char *translate(const char *format, const void *userdata,
        const char *vars, variant args[]);

    /* eval_x functions */
    typedef void(*evalfun) (struct opstack ** stack, const void *);
    extern void add_function(const char *symbol, evalfun parse);

    /* transient memory blocks */
    extern char *balloc(size_t size);

#ifdef __cplusplus
}
#endif
#endif
