#include "movement.h"

#include <kernel/attrib.h>

#include <storage.h>

#include <assert.h>
#include <limits.h>

static int age_speedup(attrib * a, void *owner)
{
    (void)owner;
    if (a->data.sa[0] > 0) {
        assert(a->data.sa[0] - a->data.sa[1] >= SHRT_MIN);
        assert(a->data.sa[0] - a->data.sa[1] <= SHRT_MAX);
        a->data.sa[0] = (short)(a->data.sa[0] - a->data.sa[1]);
    }
    return (a->data.sa[0] > 0) ? AT_AGE_KEEP : AT_AGE_REMOVE;
}

attrib_type at_speedup = {
    "speedup",
    NULL, NULL,
    age_speedup,
    a_writeint,
    a_readint
};

int get_speedup(attrib* attribs)
{
    int k = 0;
    attrib *a = a_find(attribs, &at_speedup);
    while (a != NULL && a->type == &at_speedup) {
        k += a->data.sa[0];
        a = a->next;
    }
    return k;
}

bool set_speedup(struct attrib** attribs, int speed, int duration)
{
    attrib* a = a_find(*attribs, &at_speedup);
    if (a == NULL) {
        a = a_add(attribs, a_new(&at_speedup));
        a->data.sa[0] = speed;
        a->data.sa[1] = duration;
        return true;
    }
    return false;
}

bool add_speedup(struct attrib** attribs, int speed, int duration)
{
    attrib* a = a_add(attribs, a_new(&at_speedup));
    if (a != NULL) {
        a->data.sa[0] = speed;
        a->data.sa[1] = duration;
        return true;
    }
    return false;
}
