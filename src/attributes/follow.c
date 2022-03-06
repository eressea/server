#include "follow.h"

#include <kernel/unit.h>

#include <kernel/attrib.h>
#include <kernel/gamedata.h>
#include <util/variant.h>

#include <storage.h>

static int read_follow(variant * var, void *owner, gamedata *data)
{
    READ_INT(data->store, NULL);   /* skip it */
    return AT_READ_FAIL;
}

attrib_type at_follow = {
    "follow", NULL, NULL, NULL, NULL, read_follow
};

attrib *make_follow(struct unit * u)
{
    attrib *a = a_new(&at_follow);
    a->data.v = u;
    return a;
}
