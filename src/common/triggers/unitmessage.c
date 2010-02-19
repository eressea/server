/* vi: set ts=2:
+-------------------+  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
+-------------------+  
This program may not be used, modified or distributed 
without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <kernel/eressea.h>
#include "unitmessage.h"

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/message.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/storage.h>

/* ansi includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/***
** give an item to someone
**/

typedef struct unitmessage_data {
  struct unit * target;
  char * string;
  int type;
  int level;
} unitmessage_data;

static void
unitmessage_init(trigger * t)
{
  t->data.v = calloc(sizeof(unitmessage_data), 1);
}

static void
unitmessage_free(trigger * t)
{
  unitmessage_data * sd = (unitmessage_data*)t->data.v;
  free(sd->string);
  free(t->data.v);
}

static int
unitmessage_handle(trigger * t, void * data)
{
  /* call an event handler on unitmessage.
  * data.v -> ( variant event, int timer )
  */
  unitmessage_data * td = (unitmessage_data*)t->data.v;
  if (td->target && td->target->no) {
    struct faction * f = td->target->faction;
    addmessage(td->target->region, f, LOC(f->locale, td->string), td->type, td->level);
  }
  unused(data);
  return 0;
}

static void
unitmessage_write(const trigger * t, struct storage * store)
{
  unitmessage_data * td = (unitmessage_data*)t->data.v;
  write_unit_reference(td->target, store);
  store->w_tok(store, td->string);
  store->w_int(store, td->type);
  store->w_int(store, td->level);
}

static int
unitmessage_read(trigger * t, struct storage * store)
{
  unitmessage_data * td = (unitmessage_data*)t->data.v;
  char zText[256];

  int result = read_reference(&td->target, store, read_unit_reference, resolve_unit);

  td->string = store->r_tok(store);
  td->type = store->r_int(store);
  td->level = store->r_int(store);
  td->string = strdup(zText);

  if (result==0 && td->target==NULL) {
    return AT_READ_FAIL;
  }
  return AT_READ_OK;
}

trigger_type tt_unitmessage = {
  "unitmessage",
  unitmessage_init,
  unitmessage_free,
  unitmessage_handle,
  unitmessage_write,
  unitmessage_read
};

trigger *
trigger_unitmessage(unit * target, const char * string, int type, int level)
{
  trigger * t = t_new(&tt_unitmessage);
  unitmessage_data * td = (unitmessage_data*)t->data.v;
  td->target = target;
  td->string = strdup(string);
  td->type = type;
  td->level = level;
  return t;
}
