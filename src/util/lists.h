#ifndef LISTS_H
#define LISTS_H
#ifdef __cplusplus
extern "C" {
#endif

    typedef struct strlist {
        struct strlist *next;
        char *s;
    } strlist;

    void addstrlist(strlist ** SP, const char *s);
    void freestrlist(strlist * s);
    void addlist(void *l1, void *p1);
    void translist(void *l1, void *l2, void *p);
    void freelist(void *p1);
    unsigned int listlen(void *l);

#ifdef __cplusplus
}
#endif
#endif
