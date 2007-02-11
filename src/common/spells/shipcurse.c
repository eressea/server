/* vi: set ts=2:
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include "eressea.h"
#include "shipcurse.h"

/* kernel includes */
#include <message.h>
#include <ship.h>
#include <nrmessage.h>
#include <objtypes.h>
#include <curse.h>

/* util includes */
#include <functions.h>
#include <base36.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>


int
cinfo_ship(const struct locale * lang, const void * obj, typ_t typ, curse *c, int self)
{
  message * msg;

  unused(typ);
  unused(obj);
  assert(typ == TYP_SHIP);

  if (self != 0){ /* owner or inside */
    msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
  } else {
    msg = msg_message(mkname("curseinfo", "shipunknown"), "id", c->no);
  }
  if (msg) {
    nr_render(msg, lang, buf, sizeof(buf), NULL);
    msg_release(msg);
    return 1;
  }
  log_warning(("There is no curseinfo for %s.\n", c->type->cname));
  return 0;
}

/* CurseInfo mit Spezialabfragen */

/* C_SHIP_NODRIFT */
static int
cinfo_shipnodrift(const struct locale * lang, const void * obj, typ_t typ, curse *c, int self)
{
  ship * sh;
  unused(typ);

  assert(typ == TYP_SHIP);
  sh = (ship*)obj;

  if (self != 0){
    sprintf(buf, "%s ist mit guten Wind gesegnet", sh->name);
    if (c->duration <= 2){
      scat(", doch der Zauber beginnt sich bereits aufzulösen");
    }
    scat(".");
  } else {
    sprintf(buf, "Ein silberner Schimmer umgibt das Schiff.");
  }
  scat(" (");
  scat(itoa36(c->no));
  scat(")");
  return 1;
}

static struct curse_type ct_stormwind = { "stormwind",
  CURSETYP_NORM, 0, NO_MERGE,
  "", cinfo_ship
};
static struct curse_type ct_flyingship = { "flyingship",
  CURSETYP_NORM, 0, NO_MERGE,
  "", cinfo_ship
};
static struct curse_type ct_nodrift = { "nodrift",
  CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR ),
  "Der Zauber auf diesem Schiff ist aus den elementaren Magien der Luft "
  "und des Wassers gebunden. Der dem Wasser verbundene Teil des Zaubers "
  "läßt es leichter durch die Wellen gleiten und der der Luft verbundene "
  "Teil scheint es vor widrigen Winden zu schützen.",
  cinfo_shipnodrift
};
static struct curse_type ct_shipdisorientation = { "shipdisorientation",
  CURSETYP_NORM, 0, NO_MERGE,
  "Dieses Schiff hat sich verfahren.", cinfo_ship
};
static struct curse_type ct_shipspeedup = { "shipspeedup",
  CURSETYP_NORM, 0, 0,
  NULL, cinfo_ship
};

void
register_shipcurse(void)
{
  ct_register(&ct_stormwind);
  ct_register(&ct_flyingship);
  ct_register(&ct_nodrift);
  ct_register(&ct_shipdisorientation);
  ct_register(&ct_shipspeedup);
}

