/* vi: set ts=2:
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2005   |  
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 * 
 * This file contains spells that aren't referenced by the kernel. That should
 * really be most if not all the spells, but they are in spells/spells.c
 * because it's so much work to get them out of the big array.
 */

#include <config.h>
#include "eressea.h"
#include "spy.h"

#include <kernel/faction.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/region.h>
#include <kernel/objtypes.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

#include <util/functions.h>


/* ------------------------------------------------------------- */
/* Name:       Plappermaul
 * Stufe:      4
 * Gebiet:     Cerddor
 * Kategorie:  Einheit
 *
 * Wirkung:
 *  Einheit ausspionieren. Gibt auch Zauber und Kampfstatus aus.  Wirkt
 *  gegen Magieresistenz. Ist diese zu hoch, so wird der Zauber entdeckt
 *  (Meldung) und der Zauberer erhält nur die Talente, nicht die Werte
 *  der Einheit und auch keine Zauber.
 *
 * Flag:
 *  (UNITSPELL | TESTCANSEE)
 */
static int
sp_babbler(castorder *co)
{
  unit *target;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;
  message * msg;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  target = pa->param[0]->data.u;

  if (target->faction == mage->faction) {
    /* Die Einheit ist eine der unsrigen */
    cmistake(mage, co->order, 45, MSG_MAGIC);
  }

  /* Magieresistenz Unit */
  if (target_resists_magic(mage, target, TYP_UNIT, 0)) {
    spy_message(5, mage, target);
    msg = msg_message("babbler_resist", "unit mage", target, mage);
  } else {
    spy_message(100, mage, target);
    msg = msg_message("babbler_effect", "unit", target);
  }
  r_addmessage(r, target->faction, msg);
  msg_release(msg);
  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Traumdeuten
 * Stufe:   7
 * Kategorie:      Einheit
 *
 * Wirkung:
 *  Wirkt gegen Magieresistenz.  Spioniert die Einheit aus. Gibt alle
 *  Gegenstände, Talente mit Stufe, Zauber und Kampfstatus an.
 *
 *  Magieresistenz hier prüfen, wegen Fehlermeldung
 *
 * Flag:
 * (UNITSPELL)
 */
static int
sp_readmind(castorder *co)
{
  unit *target;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  target = pa->param[0]->data.u;

  if (target->faction == mage->faction) {
    /* Die Einheit ist eine der unsrigen */
    cmistake(mage, co->order, 45, MSG_MAGIC);
  }

  /* Magieresistenz Unit */
  if (target_resists_magic(mage, target, TYP_UNIT, 0)) {
    cmistake(mage, co->order, 180, MSG_MAGIC);
    /* "Fühlt sich beobachtet"*/
    ADDMSG(&target->faction->msgs, msg_message("stealdetect", "unit", target));
    return 0;
  }
  spy_message(2, mage, target);

  return cast_level;
}

void
register_gcspells(void)
{
  register_function((pf_generic)&sp_babbler, "cast_babbler");
  register_function((pf_generic)&sp_readmind, "cast_readmind");
}
