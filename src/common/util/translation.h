/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
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

struct opstack;
extern void * opstack_pop(struct opstack ** stack);
extern void opstack_push(struct opstack ** stack, void * data);
#define opush(stack, i) opstack_push(stack, (void *)(i))
#define opop(stack, T) (T)opstack_pop(stack)

extern void translation_init(void);
extern void translation_done(void);
extern const char * translate_va(const char* format, const void * userdata, const char* vars, ...);
extern const char * translate(const char* format, const void * userdata, const char* vars, void* args[]);

/* eval_x functions */
typedef void (*evalfun)(struct opstack ** stack, const void *);
extern void add_function(const char * symbol, evalfun parse);

/* transient memory blocks */
extern char * balloc(size_t size);

#ifdef __cplusplus
}
#endif
#endif
