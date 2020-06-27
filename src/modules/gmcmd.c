#include <platform.h>
#include <kernel/config.h>
#include "gmcmd.h"
#include <kernel/command.h>

/* misc includes */
#include <attributes/key.h>
#include <triggers/gate.h>

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

/* util includes */
#include <kernel/attrib.h>
#include <kernel/gamedata.h>
#include <util/macros.h>

#include <storage.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

static int read_permissions(variant *var, void *owner, struct gamedata *data)
{
    attrib *a = NULL;
    UNUSED_ARG(var);
    read_attribs(data, &a, owner);
    a_remove(&a, a);
    return AT_READ_OK;
}

static int read_gmcreate(variant *var, void *owner, struct gamedata *data)
{
    char zText[32];
    UNUSED_ARG(var);
    READ_TOK(data->store, zText, sizeof(zText));
    return AT_READ_OK;
}

void register_gmcmd(void)
{
    at_deprecate("GM:create", read_gmcreate);
    at_deprecate("GM:permissions", read_permissions);
}
