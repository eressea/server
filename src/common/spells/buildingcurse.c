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
#include "buildingcurse.h"

/* kernel includes */
#include <kernel/message.h>
#include <kernel/objtypes.h>
#include <kernel/building.h>
#include <kernel/ship.h>
#include <kernel/curse.h>

/* util includes */
#include <util/nrmessage.h>
#include <util/base36.h>
#include <util/functions.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>


static int
cinfo_building(const struct locale * lang, const void * obj, typ_t typ, const curse *c, int self)
{
  message * msg;

  unused(typ);
  assert(typ == TYP_BUILDING);

  if (self != 0){ /* owner or inside */
    msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
  } else { /* outside */
    msg = msg_message(mkname("curseinfo", "buildingunknown"), "id", c->no);
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

/* C_MAGICWALLS*/
static int
cinfo_magicrunes(const struct locale* lang, const void * obj, typ_t typ, const curse *c, int self)
{
  message * msg = NULL;
  if (typ == TYP_BUILDING){
    building * b;
    b = (building*)obj;
    if (self != 0) {
      msg = msg_message(mkname("curseinfo", "magicrunes_building"), "building id", b, c->no);
    }
  } else if (typ == TYP_SHIP) {
    ship *sh;
    sh = (ship*)obj;
    if (self != 0){
      msg = msg_message(mkname("curseinfo", "magicrunes_ship"), "ship id", sh, c->no);
    }
  }

  if (msg!=NULL) {
    nr_render(msg, lang, buf, sizeof(buf), NULL);
    msg_release(msg);
    return 1;
  }
  return 0;
}
static struct curse_type ct_magicrunes = { "magicrunes",
  CURSETYP_NORM, 0, M_SUMEFFECT,
  "Dieser Zauber verstärkt die natürliche Widerstandskraft gegen eine "
  "Verzauberung.",
  cinfo_magicrunes
};

/* Heimstein-Zauber */
static struct curse_type ct_magicwalls = { "magicwalls",
  CURSETYP_NORM, 0, NO_MERGE,
  "Die Macht dieses Zaubers ist fast greifbar und tief in die Mauern "
  "gebunden. Starke elementarmagische Kräfte sind zu spüren. "
  "Vieleicht wurde gar ein Erdelementar in diese Mauern gebannt. "
  "Ausser ebenso starkter Antimagie wird nichts je diese Mauern "
  "gefährden können.",
  cinfo_building
};

/* Feste Mauer - Präkampfzauber, wirkt nur 1 Runde */
static struct curse_type ct_strongwall = { "strongwall",
  CURSETYP_NORM, 0, NO_MERGE,
  "",
  NULL
};

/* Ewige Mauern-Zauber */
static struct curse_type ct_nocostbuilding = { "nocostbuilding",
  CURSETYP_NORM, 0, NO_MERGE,
  "Die Macht dieses Zaubers ist fast greifbar und tief in die Mauern "
  "gebunden. Unbeeindruck vom Zahn der Zeit wird dieses Gebäude wohl "
  "auf Ewig stehen.",
  cinfo_building
};


void
register_buildingcurse(void)
{
  ct_register(&ct_magicwalls);
  ct_register(&ct_strongwall);
  ct_register(&ct_magicrunes);
  ct_register(&ct_nocostbuilding);
}
