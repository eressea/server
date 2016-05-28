/*
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#include <platform.h>
#include <kernel/config.h>
#include "gmcmd.h"
#include <kernel/command.h>

/* misc includes */
#include <items/demonseye.h>
#include <attributes/key.h>
#include <triggers/gate.h>
#include <triggers/unguard.h>

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/gamedata.h>

#include <storage.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

static int read_permissions(attrib * a, void *owner, struct gamedata *data)
{
    assert(!a);
    read_attribs(data, &a, owner);
    a_remove(&a, a);
    return AT_READ_OK;
}

static int read_gmcreate(attrib * a, void *owner, struct gamedata *data)
{
    char zText[32];
    READ_TOK(data->store, zText, sizeof(zText));
    return AT_READ_OK;
}

void register_gmcmd(void)
{
    at_deprecate("GM:create", read_gmcreate);
    at_deprecate("GM:permissions", read_permissions);
}
