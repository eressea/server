#include "movement.h"

#include <kernel/attrib.h>
#include <kernel/gamedata.h>
#include <util/macros.h>

#include <storage.h>

#include <assert.h>
#include <limits.h>

static int read_movement(variant *var, void *owner, gamedata *data)
{
    READ_INT(data->store, &var->i);
    if (var->i != 0)
        return AT_READ_OK;
    else
        return AT_READ_FAIL;
}

attrib_type at_movement = {
    "movement", NULL, NULL, NULL, a_writeint, read_movement
};

bool get_movement(attrib * const *alist, int type)
{
    const attrib *a = a_find(*alist, &at_movement);
    if (a == NULL)
        return false;
    if (a->data.i & type)
        return true;
    return false;
}

void set_movement(attrib ** alist, int type)
{
    attrib *a = a_find(*alist, &at_movement);
    if (a == NULL)
        a = a_add(alist, a_new(&at_movement));
    a->data.i |= type;
}

static int age_speedup(attrib * a, void *owner)
{
    UNUSED_ARG(owner);
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

