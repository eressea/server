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
#ifndef H_UTIL_COMMAND_H
#define H_UTIL_COMMAND_H
#ifdef __cplusplus
extern "C" {
#endif

struct tnode;
struct locale;

typedef struct syntaxtree {
	const struct locale * lang;
	struct tnode * root;
	struct syntaxtree * next;
} syntaxtree;

typedef void (*parser)(const struct tnode *, const char*, void *, const char*);
extern void add_command(struct tnode * troot, 
								struct tnode * tnext, 
								const char * str, 
                        parser fun);
extern void do_command(const struct tnode * troot, void * u, 
							 const char * cmd);

extern struct syntaxtree * stree_create(void);
extern struct tnode * stree_find(const struct syntaxtree * stree, const struct locale * lang);

#ifdef __cplusplus
}
#endif
#endif
