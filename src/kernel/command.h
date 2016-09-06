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
#ifndef H_UTIL_COMMAND_H
#define H_UTIL_COMMAND_H
#ifdef __cplusplus
extern "C" {
#endif

    struct locale;
    struct order;
    struct unit;
    struct tnode;

    typedef struct syntaxtree {
        const struct locale *lang;
        struct tnode *root;
        struct syntaxtree *next;
    } syntaxtree;

    typedef void(*parser) (const void *nodes, struct unit * u, struct order *);
    void add_command(void **troot, void *tnext,
        const char *str, parser fun);
    void do_command(const void *troot, struct unit *u, struct order *);

    struct syntaxtree *stree_create(void);
    void stree_free(struct syntaxtree *);
    void *stree_find(const struct syntaxtree *stree,
        const struct locale *lang);

#ifdef __cplusplus
}
#endif
#endif
