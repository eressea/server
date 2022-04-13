#include "lists.h"
#include "strings.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct void_list {
    struct void_list *next;
    void *data;
} void_list;

void addlist(void *l1, void *p1)
{

    /* add entry p to the end of list l */

    void_list **l;
    void_list *p, *q;

    l = (void_list **)l1;
    p = (void_list *)p1;
    assert(p->next == 0);

    if (*l) {
        for (q = *l; q->next; q = q->next)
            assert(q);
        q->next = p;
    }
    else
        *l = p;
}

static void choplist(void *a, void *b)
{
    void_list **l = (void_list **)a, *p = (void_list *)b;
    /* remove entry p from list l - when called, a pointer to p must be
     * kept in order to use (and free) p; if omitted, this will be a
     * memory leak */

    void_list **q;

    for (q = l; *q; q = &((*q)->next)) {
        if (*q == p) {
            *q = p->next;
            p->next = 0;
            break;
        }
    }
}

void translist(void *l1, void *l2, void *p)
{

    /* remove entry p from list l1 and add it at the end of list l2 */

    choplist(l1, p);
    addlist(l2, p);
}

void freelist(void *p1)
{

    /* remove all entries following and including entry p from a listlist */

    void_list *p, *p2;

    p = (void_list *)p1;

    while (p) {
        p2 = p->next;
        free(p);
        p = p2;
    }
}

unsigned int listlen(void *l)
{

    /* count entries p in list l */

    unsigned int i;
    void_list *p;

    for (p = (void_list *)l, i = 0; p; p = p->next, i++);
    return i;
}
/* - String Listen --------------------------------------------- */
void addstrlist(strlist ** SP, char *s)
{
    strlist *slist = malloc(sizeof(strlist));
    if (!slist) abort();
    slist->next = NULL;
    slist->s = s;
    addlist(SP, slist);
}

void freestrlist(strlist * s)
{
    strlist *q, *p = s;
    while (p) {
        q = p->next;
        free(p->s);
        free(p);
        p = q;
    }
}
