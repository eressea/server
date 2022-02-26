#include <kernel/config.h>
#include "hate.h"

#include <kernel/unit.h>

#include <kernel/attrib.h>
#include <kernel/gamedata.h>
#include <util/macros.h>
#include <util/resolve.h>

#include <storage.h>

static int verify_hate(attrib * a, void *owner)
{
    UNUSED_ARG(owner);
    if (a->data.v == NULL) {
        return 0;
    }
    return 1;
}

static void
write_hate(const variant *var, const void *owner, struct storage *store)
{
    write_unit_reference((unit *)var->v, store);
}

static int read_hate(variant *var, void *owner, gamedata *data)
{
    if (read_unit_reference(data, (unit **)&var->v, NULL) <= 0) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

attrib_type at_hate = {
    "hates",
    NULL,
    NULL,
    verify_hate,
    write_hate,
    read_hate,
};

attrib *make_hate(struct unit * u)
{
    attrib *a = a_new(&at_hate);
    a->data.v = u;
    return a;
}
