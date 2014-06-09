/* vi: set ts=2:
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
#include <kernel/reports.h>
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
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/umlaut.h>
#include <util/parser.h>
#include <util/rng.h>

#include <storage.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

static int read_permissions(attrib * a, void *owner, struct storage *store)
{
  attrib *attr = NULL;
  a_read(store, &attr, NULL);
  a_free(attr);
  return AT_READ_OK;
}

struct attrib_type at_permissions = {
  "GM:permissions",
  NULL, NULL, NULL,
  NULL, read_permissions,
  ATF_UNIQUE
};

static int read_gmcreate(attrib * a, void *owner, struct storage *store)
{
  char zText[32];
  READ_TOK(store, zText, sizeof(zText));
  return AT_READ_OK;
}

/* at_gmcreate specifies that the owner can create items of a particular type */
attrib_type at_gmcreate = {
  "GM:create",
  NULL, NULL, NULL,
  NULL, read_gmcreate
};

void register_gmcmd(void)
{
  at_register(&at_gmcreate);
  at_register(&at_permissions);
}
