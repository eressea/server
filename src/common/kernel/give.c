/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.

 */
#include <config.h>
#include "eressea.h"

#include "give.h"

/* kernel includes */
#include "faction.h"
#include "item.h"
#include "magic.h"
#include "message.h"
#include "order.h"
#include "pool.h"
#include "race.h"
#include "region.h"
#include "reports.h"
#include "ship.h"
#include "skill.h"
#include "terrain.h"
#include "unit.h"

/* attributes includes */
#include <attributes/racename.h>
#include <attributes/orcification.h>

/* util includes */
#include <util/event.h>
#include <util/base36.h>

/* libc includes */
#include <limits.h>
#include <stdlib.h>

/* Wieviel Fremde eine Partei pro Woche aufnehmen kann */
#define MAXNEWBIES								5
#define RESERVE_DONATIONS /* shall we reserve objects given to us by other factions? */
#define RESERVE_GIVE /* reserve anything that's given from one unit to another? */


static int 
GiveRestriction(void) {
  static int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "GiveRestriction");
    value = str?atoi(str):0;
  }
  return value;
}

static void
add_give(unit * u, unit * u2, int n, const resource_type * rtype, struct order * ord, int error)
{
  if (error) {
    cmistake(u, ord, error, MSG_COMMERCE);
  }
  else if (!u2 || u2->faction!=u->faction) {
    assert(rtype);
    ADDMSG(&u->faction->msgs,
      msg_message("give", "unit target resource amount",
      u, u2?ucansee(u->faction, u2, u_unknown()):u_peasants(), rtype, n));
    if (u2) {
      ADDMSG(&u2->faction->msgs,
        msg_message("give", "unit target resource amount",
        ucansee(u2->faction, u, u_unknown()), u2, rtype, n));
    }
  }
}

static void
give_peasants(int n, const item_type * itype, unit * src)
{
  region *r = src->region;

  /* horses are given to the region via itype->give! */

  if (itype->rtype==r_silver) {
    rsetmoney(r, rmoney(r) + n);
  }
}

int
give_item(int want, const item_type * itype, unit * src, unit * dest, struct order * ord)
{
  short error = 0;
  int n;

  assert(itype!=NULL);
  n = get_pooled(src, item2resource(itype), GET_DEFAULT, want);
  n = min(want, n);
  if (dest && src->faction != dest->faction && src->faction->age < GiveRestriction()) {
		if (ord!=NULL) {
      ADDMSG(&src->faction->msgs, msg_feedback(src, ord, "giverestriction",
        "turns", GiveRestriction()));
		}
    return -1;
  } else if (n == 0) {
    int reserve = get_reservation(src, itype->rtype);
    if (reserve) {
      msg_feedback(src, ord, "nogive_reserved", "resource reservation", 
        itype->rtype, reserve);
      return -1;
    }
    error = 36;
  } else if (itype->flags & ITF_CURSED) {
    error = 25;
  } else if (itype->give && !itype->give(src, dest, itype, n, ord)) {
    return -1;
  } else {
    int use = use_pooled(src, item2resource(itype), GET_SLACK, n);
    if (use<n) use += use_pooled(src, item2resource(itype), GET_RESERVE|GET_POOLED_SLACK, n-use);
    if (dest) {
      i_change(&dest->items, itype, n);
#ifdef RESERVE_GIVE
#ifdef RESERVE_DONATIONS
      change_reservation(dest, item2resource(itype), n);
#else
      if (src->faction==dest->faction) {
        change_reservation(dest, item2resource(itype), n);
      }
#endif
#endif
      handle_event(src->attribs, "give", dest);
      handle_event(dest->attribs, "receive", src);
#if defined(MUSEUM_MODULE) && defined(TODO)
/* TODO: Einen Trigger für den museums-warden benutzen! */
        if (a_find(dest->attribs, &at_warden)) {
          /* warden_add_give(src, dest, itype, n); */
        }
#endif
    } else {
      /* gib bauern */
      give_peasants(use, itype, src);
    }
  }
  add_give(src, dest, n, item2resource(itype), ord, error);
  if (error) return -1;
  return 0;
}

void
give_men(int n, unit * u, unit * u2, struct order * ord)
{
  ship *sh;
  int k = 0;
  int error = 0;

  if (u2 && u->faction != u2->faction && u->faction->age < GiveRestriction()) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "giverestriction",
      "turns", GiveRestriction()));
    return;
  } else if (u == u2) {
    error = 10;
  } else if (!u2 && u->race == new_race[RC_SNOTLING]) {
    /* Snotlings können nicht an Bauern übergeben werden. */
    error = 307;
#ifdef HEROES
  } else if (u2 && u2->number>0 && fval(u, UFL_HERO)!=fval(u2, UFL_HERO)) {
    error = 75;
#endif
  } else if ((u && unit_has_cursed_item(u)) || (u2 && unit_has_cursed_item(u2))) {
    error = 78;
  } else if (fval(u, UFL_LOCKED) || is_cursed(u->attribs, C_SLAVE, 0)) {
    error = 74;
  } else if (u2 && fval(u, UFL_HUNGER)) {
    error = 73;
  } else if (u2 && (fval(u2, UFL_LOCKED)|| is_cursed(u2->attribs, C_SLAVE, 0))) {
    error = 75;
  } else if (u2 && u2->faction != u->faction && !alliedunit(u2, u->faction, HELP_GIVE) && !ucontact(u2, u)) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_no_contact", "target", u2));
    error = -1;
  } else if (u2 && (has_skill(u, SK_MAGIC) || has_skill(u2, SK_MAGIC))) {
    error = 158;
  } else if (u2 && fval(u, UFL_WERE) != fval(u2, UFL_WERE)) {
    error = 312;
  } else if (u2 && u2->number!=0 && u2->race != u->race) {
    log_warning(("Partei %s versucht %s an %s zu übergeben.\n",
                 itoa36(u->faction->no), u->race->_name[0],
                 u2->race->_name[1]));
    error = 139;
  } else if (u2!=NULL && (get_racename(u2->attribs) || get_racename(u->attribs))) {
    error = 139;
  } else {
    if (n > u->number) n = u->number;
    if (u2 && n+u2->number > UNIT_MAXSIZE) {
      n = UNIT_MAXSIZE-u2->number;
      ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_unit_size", "maxsize", UNIT_MAXSIZE));
      assert(n>=0);
    }
    if (n == 0) {
      error = 96;
    } else if (u2 && u->faction != u2->faction) {
      if (u2->faction->newbies + n > MAXNEWBIES) {
        error = 129;
      } else if (u->race != u2->faction->race) {
        if (u2->faction->race != new_race[RC_HUMAN]) {
          error = 120;
        } else if (count_migrants(u2->faction) + n > count_maxmigrants(u2->faction)) {
          error = 128;
        }
        else if (teure_talente(u) || teure_talente(u2)) {
          error = 154;
        } else if (u2->number!=0) {
          error = 139;
        }
      }
    }
  }

  if (u2 && (has_skill(u, SK_ALCHEMY) || has_skill(u2, SK_ALCHEMY))) {
    k = count_skill(u2->faction, SK_ALCHEMY);

    /* Falls die Zieleinheit keine Alchemisten sind, werden sie nun
    * welche. */
    if (!has_skill(u2, SK_ALCHEMY) && has_skill(u, SK_ALCHEMY))
      k += u2->number;

    /* Wenn in eine Alchemisteneinheit Personen verschoben werden */
    if (has_skill(u2, SK_ALCHEMY) && !has_skill(u, SK_ALCHEMY))
      k += n;

    /* Wenn Parteigrenzen überschritten werden */
    if (u2->faction != u->faction)
      k += n;

    /* wird das Alchemistenmaximum ueberschritten ? */

    if (k > max_skill(u2->faction, SK_ALCHEMY)) {
      error = 156;
    }
  }

  if (error==0) {
    if (u2 && u2->number == 0) {
      set_racename(&u2->attribs, get_racename(u->attribs));
      u2->race = u->race;
      u2->irace = u->irace;
#ifdef HEROES
      if (fval(u, UFL_HERO)) fset(u2, UFL_HERO);
      else freset(u2, UFL_HERO);
#endif
    }
    
    if (u2) {
      /* Einheiten von Schiffen können nicht NACH in von
      * Nicht-alliierten bewachten Regionen ausführen */
      sh = leftship(u);
      if (sh) set_leftship(u2, sh);

      transfermen(u, u2, n);
      if (u->faction != u2->faction) {
        u2->faction->newbies += n;
      }
    } else {
      if (getunitpeasants) {
#ifdef ORCIFICATION
        if (u->race == new_race[RC_ORC] && !fval(u->region, RF_ORCIFIED)) {
          attrib *a = a_find(u->region->attribs, &at_orcification);
          if (!a) a = a_add(&u->region->attribs, a_new(&at_orcification));
          a->data.i += n;
        }
#endif
        transfermen(u, NULL, n);
      } else {
        error = 159;
      }
    }
  }
  if (error>0) {
    cmistake(u, ord, error, MSG_COMMERCE);
  }
  else if (!u2 || u2->faction!=u->faction) {
    ADDMSG(&u->faction->msgs, msg_message("give_person", "unit target amount",
      u, u2?ucansee(u->faction, u2, u_unknown()):u_peasants(), n));
    if (u2) {
      ADDMSG(&u2->faction->msgs, msg_message("give_person", "unit target amount",
        ucansee(u2->faction, u, u_unknown()), u2, n));
    }
  }
}

void
give_unit(unit * u, unit * u2, order * ord)
{
  region * r = u->region;
  int n = u->number;

  if (u && unit_has_cursed_item(u)) {
    cmistake(u, ord, 78, MSG_COMMERCE);
    return;
  }

#ifdef HEROES
  if (fval(u, UFL_HERO)) {
    cmistake(u, ord, 75, MSG_COMMERCE);
    return;
  }
#endif
  if (fval(u, UFL_LOCKED) || fval(u, UFL_HUNGER)) {
    cmistake(u, ord, 74, MSG_COMMERCE);
    return;
  }

  if (u2 == NULL) {
    if (fval(r->terrain, SEA_REGION)) {
      cmistake(u, ord, 152, MSG_COMMERCE);
    } else if (getunitpeasants) {
      unit *u3;

      for(u3 = r->units; u3; u3 = u3->next)
        if(u3->faction == u->faction && u != u3) break;

      if(u3) {
        while (u->items) {
          item * iold = i_remove(&u->items, u->items);
          item * inew = *i_find(&u3->items, iold->type);
          if (inew==NULL) i_add(&u3->items, iold);
          else {
            inew->number += iold->number;
            i_free(iold);
          }
        }
      }
      give_men(u->number, u, NULL, ord);
      cmistake(u, ord, 153, MSG_COMMERCE);
    } else {
      ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found", ""));
    }
    return;
  }

  if (!alliedunit(u2, u->faction, HELP_GIVE) && ucontact(u2, u) == 0) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_no_contact", "target", u2));
    return;
  }
  if (u->number == 0) {
    cmistake(u, ord, 105, MSG_COMMERCE);
    return;
  }
  if (u2->faction->newbies + n > MAXNEWBIES) {
    cmistake(u, ord, 129, MSG_COMMERCE);
    return;
  }
  if (u->race != u2->faction->race) {
    if (u2->faction->race != new_race[RC_HUMAN]) {
      cmistake(u, ord, 120, MSG_COMMERCE);
      return;
    }
    if (count_migrants(u2->faction) + u->number > count_maxmigrants(u2->faction)) {
      cmistake(u, ord, 128, MSG_COMMERCE);
      return;
    }
    if (teure_talente(u)) {
      cmistake(u, ord, 154, MSG_COMMERCE);
      return;
    }
  }
  if (has_skill(u, SK_MAGIC)) {
    if (count_skill(u2->faction, SK_MAGIC) + u->number >
      max_skill(u2->faction, SK_MAGIC))
    {
      cmistake(u, ord, 155, MSG_COMMERCE);
      return;
    }
    if (u2->faction->magiegebiet != find_magetype(u)) {
      cmistake(u, ord, 157, MSG_COMMERCE);
      return;
    }
  }
  if (has_skill(u, SK_ALCHEMY)
    && count_skill(u2->faction, SK_ALCHEMY) + u->number >
    max_skill(u2->faction, SK_ALCHEMY))
  {
    cmistake(u, ord, 156, MSG_COMMERCE);
    return;
  }
  add_give(u, u2, 1, r_unit, ord, 0);
  u_setfaction(u, u2->faction);
  u2->faction->newbies += n;
}
