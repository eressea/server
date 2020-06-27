#include <platform.h>
#include "targetregion.h"

#include <kernel/config.h>
#include <kernel/region.h>

#include <kernel/attrib.h>
#include <kernel/gamedata.h>
#include <util/resolve.h>

#include <storage.h>

static void
write_targetregion(const variant *var, const void *owner, struct storage *store)
{
    write_region_reference((region *)var->v, store);
}

static int read_targetregion(variant *var, void *owner, gamedata *data)
{
    if (read_region_reference(data, (region **)&var->v) <= 0) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

attrib_type at_targetregion = {
    "targetregion",
    NULL,
    NULL,
    NULL,
    write_targetregion,
    read_targetregion,
    NULL,
    ATF_UNIQUE
};

attrib *make_targetregion(struct region * r)
{
    attrib *a = a_new(&at_targetregion);
    a->data.v = r;
    return a;
}
