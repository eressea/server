#ifndef H_UTIL_COMMAND_H
#define H_UTIL_COMMAND_H
#ifdef __cplusplus
extern "C" {
#endif

    struct locale;
    struct order;
    struct unit;
    struct tnode;
    struct command;

    typedef struct syntaxtree {
        const struct locale *lang;
        struct tnode *root;
        struct syntaxtree *next;
        struct command *cmds;
    } syntaxtree;

    typedef void(*parser) (const void *nodes, struct unit * u, struct order *);
    void do_command(const struct tnode *troot, struct unit *u, struct order *);

    struct syntaxtree *stree_create(void);
    void stree_add(struct syntaxtree *, const char *str, parser fun);
    void stree_free(struct syntaxtree *);
    void *stree_find(const struct syntaxtree *stree,
        const struct locale *lang);

#ifdef __cplusplus
}
#endif
#endif
