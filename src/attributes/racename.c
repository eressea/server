#include "racename.h"

#include <kernel/attrib.h>
#include <util/strings.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

attrib_type at_racename = {
    "racename", NULL, a_finalizestring, NULL, a_writestring, a_readstring
};

const char *get_racename(attrib * alist)
{
    attrib *a = a_find(alist, &at_racename);
    if (a)
        return (const char *)a->data.v;
    return NULL;
}

void set_racename(attrib ** palist, const char *name)
{
    attrib *a = a_find(*palist, &at_racename);

    if (a) {
        if (name) {
            if (strcmp(a->data.v, name) != 0) {
                free(a->data.v);
                a->data.v = str_strdup(name);
            }
        }
        else {
            a_remove(palist, a);
        }
    }
    else if (name) {
        a = a_add(palist, a_new(&at_racename));
        a->data.v = str_strdup(name);
    }
}
