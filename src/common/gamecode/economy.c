/* vi: set ts=2:
*
*
*	Eressea PB(E)M host Copyright (C) 1998-2003
*      Christian Schlittchen (corwin@amber.kn-bremen.de)
*      Katja Zedel (katze@felidae.kn-bremen.de)
*      Henning Peters (faroul@beyond.kn-bremen.de)
*      Enno Rehling (enno@eressea-pbem.de)
*      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
*
*  based on:
*
* Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
* Atlantis v1.7                    Copyright 1996 by Alex Schröder
*
* This program may not be used, modified or distributed without
* prior permission by the authors of Eressea.
* This program may not be sold or used commercially without prior written
* permission from the authors.
*/

#include <config.h>
#include "eressea.h"
#include "economy.h"

/* gamecode includes */
#include "laws.h"
#include "randenc.h"

/* kernel includes */
#include <kernel/alchemy.h>
#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/faction.h>
#include <kernel/give.h>
#include <kernel/item.h>
#include <kernel/karma.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/movement.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/resources.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/spy.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/message.h>

/* libs includes */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include <attributes/reduceproduction.h>
#include <attributes/racename.h>
#include <attributes/orcification.h>

#include <items/seed.h>

typedef struct request {
  struct request * next;
  struct unit *unit;
  int qty;
  int no;
  union {
    boolean goblin; /* stealing */
    const struct luxury_type * ltype; /* trading */
  } type;
} request;

static request workers[1024];
static request *nextworker;
static int working;

static request entertainers[1024];
static request *nextentertainer;
static int entertaining;

static int norders;
static request *oa;

int
income(const unit * u)
{
	switch(old_race(u->race)) {
	case RC_FIREDRAGON:
		return 150 * u->number;
	case RC_DRAGON:
		return 1000 * u->number;
	case RC_WYRM:
		return 5000 * u->number;
	}
	return 20 * u->number;
}

static void
scramble(void *data, int n, size_t width)
{
  int j;
  char temp[64];
  assert(width<=sizeof(temp));
  for (j=0;j!=n;++j) {
    int k = rand() % n;
	if (k==j) continue;
    memcpy(temp, (char*)data+j*width, width);
    memcpy((char*)data+j*width, (char*)data+k*width, width);
    memcpy((char*)data+k*width, temp, width);
  }
}

static void
expandorders(region * r, request * requests)
{
	unit *u;
	request *o;

	/* Alle Units ohne request haben ein -1, alle units mit orders haben ein
	* 0 hier stehen */

	for (u = r->units; u; u = u->next)
		u->n = -1;

	norders = 0;

  for (o = requests; o; o = o->next) {
    if (o->qty > 0) {
      norders += o->qty;
    }
	}

	if (norders > 0) {
    int i = 0;
		oa = (request *) calloc(norders, sizeof(request));
		for (o = requests; o; o = o->next) {
			if (o->qty > 0) {
        int j;
				for (j = o->qty; j; j--) {
					oa[i] = *o;
					oa[i].unit->n = 0;
					i++;
				}
			}
		}
		scramble(oa, norders, sizeof(request));
	} else {
		oa = NULL;
	}
	while (requests) {
		request * o = requests->next;
		free(requests);
		requests = o;
	}
}
/* ------------------------------------------------------------- */

static void
change_level(unit * u, skill_t sk, int bylevel)
{
	skill * sv = get_skill(u, sk);
	assert(bylevel>0);
	if (sv==0) sv = add_skill(u, sk);
	sk_set(sv, sv->level+bylevel);
}

typedef struct recruitment {
  struct recruitment * next;
  faction * f;
  request * requests;
  int total, assigned;
} recruitment;

static recruitment *
select_recruitment(request ** rop, int (*quantify)(const struct race*, int), int * total)
{
  recruitment * recruits = NULL;

  while (*rop) {
    recruitment * rec = recruits;
    request * ro = *rop;
    unit * u = ro->unit;
    const race * rc = u->number?u->race:u->faction->race;
    int qty = quantify(rc, ro->qty);

    if (qty<0) {
      rop = &ro->next; /* skip this one */
    } else {
      *rop = ro->next; /* remove this one */
      while (rec && rec->f!=u->faction) rec = rec->next;
      if (rec==NULL) {
        rec = malloc(sizeof(recruitment));
        rec->f = u->faction;
        rec->total = 0;
        rec->assigned = 0;
        rec->requests = NULL;
        rec->next = recruits;
        recruits = rec;
      }
      *total += qty;
      rec->total += qty;
      ro->next = rec->requests;
      rec->requests = ro;
    }
  }
  return recruits;
}

static void
add_recruits(unit * u, int number, int wanted)
{
  assert(number<=wanted);
  if (number > 0) {
    unit * unew;
    int i = fspecial(u->faction, FS_MILITIA);

    if (u->number==0) {
      set_number(u, number);
      u->hp = number * unit_max_hp(u);
      unew = u;
    } else {
      unew = create_unit(u->region, u->faction, number, u->race, 0, NULL, u);
    }

    if (unew->race == new_race[RC_URUK]) {
      change_level(unew, SK_MELEE, 1);
      change_level(unew, SK_SPEAR, 1);
    }
    if (unew->race->ec_flags & ECF_REC_HORSES) {
      change_level(unew, SK_RIDING, 1);
    }

    if (i > 0) {
      if (unew->race->bonus[SK_SPEAR] >= 0)
        change_level(unew, SK_SPEAR, i);
      if (unew->race->bonus[SK_MELEE] >= 0)
        change_level(unew, SK_MELEE, i);
      if (unew->race->bonus[SK_LONGBOW] >= 0)
        change_level(unew, SK_LONGBOW, i);
      if (unew->race->bonus[SK_CROSSBOW] >= 0)
        change_level(unew, SK_CROSSBOW, i);
      if (unew->race->bonus[SK_RIDING] >= 0)
        change_level(unew, SK_RIDING, i);
      if (unew->race->bonus[SK_AUSDAUER] >= 0)
        change_level(unew, SK_AUSDAUER, i);
    }
    if (unew!=u) {
      transfermen(unew, u, unew->number);
      destroy_unit(unew);
    }
  }
  if (number < wanted) {
    ADDMSG(&u->faction->msgs, msg_message("recruit",
      "unit region amount want", u, u->region, number, wanted));
  }
}

static int
any_recruiters(const struct race * rc, int qty)
{
  if (rc==new_race[RC_URUK]) return qty;
  return qty * 2;
}

static int
peasant_recruiters(const struct race * rc, int qty)
{
  if (rc->ec_flags & ECF_REC_UNLIMITED) return -1;
  if (rc->ec_flags & ECF_REC_HORSES) return -1;
  if (rc==new_race[RC_URUK]) return qty;
  return qty * 2;
}

static int
horse_recruiters(const struct race * rc, int qty)
{
  if (rc->ec_flags & ECF_REC_UNLIMITED) return -1;
  if (rc->ec_flags & ECF_REC_HORSES) return qty * 2;
  return -1;
}

static int
do_recruiting(recruitment * recruits, int available)
{
  recruitment * rec;
  int recruited = 0;

  while (available>0) {
    int n = 0;
    int rest, mintotal = INT_MAX;

    for (rec=recruits;rec!=NULL;rec=rec->next) {
      int want = rec->total - rec->assigned;
      if (want>0) {
        if (mintotal>want) mintotal = want;
        ++n;
      }
    }
    if (n==0) break;
    if (mintotal*n>available) {
      mintotal = available/n;
    }
    rest = available - mintotal*n;

    for (rec=recruits;rec!=NULL;rec=rec->next) {
      int want = rec->total - rec->assigned;

      if (want>0) {
        int get = mintotal;
        if (want>mintotal && rest<n && (rand() % n) < rest) {
          --rest;
          ++get;
        }
        assert(get<=want);
        available -= get;
        rec->assigned += get;
      }
    }
  }

  for (rec=recruits;rec!=NULL;rec=rec->next) {
    request * req;
    int get = rec->assigned;

    for (req=rec->requests;req;req=req->next) {
      unit * u = req->unit;
      const race * rc = u->faction->race;
      int number, multi = 2;

      if (rc==new_race[RC_URUK]) multi = 1;

      number = min(req->qty, get / multi);
      if (rc->recruitcost) {
        int afford = get_pooled(u, oldresourcetype[R_SILVER], GET_DEFAULT) / rc->recruitcost;
        number = min(number, afford);
        use_pooled(u, oldresourcetype[R_SILVER], GET_DEFAULT, rc->recruitcost*number);
      }
      add_recruits(u, number, req->qty);
      if ((rc->ec_flags & ECF_REC_ETHEREAL)==0) {
        recruited += number * multi;
      }

      get -= number * multi;
    }
  }
  return recruited;
}

void
free_recruitments(recruitment * recruits)
{
  while (recruits) {
    recruitment * rec = recruits;
    recruits = rec->next;
    while (rec->requests) {
      request * req = rec->requests;
      rec->requests = req->next;
      free(req);
    }
    free(rec);
  }
}

/* Rekrutierung */
static void
expandrecruit(region * r, request * recruitorders)
{
  recruitment * recruits = NULL;

  int orc_total = 0;

  /* centaurs: */
  recruits = select_recruitment(&recruitorders, horse_recruiters, &orc_total);
  if (recruits) {
    int recruited, horses = rhorses(r) * 2;
    if (orc_total<horses) horses = orc_total;
    recruited = do_recruiting(recruits, horses);
    rsethorses(r, (horses - recruited) / 2);
    free_recruitments(recruits);
  }

  /* peasant limited: */
  recruits = select_recruitment(&recruitorders, peasant_recruiters, &orc_total);
  if (recruits) {
    int orc_recruited, orc_peasants = rpeasants(r) * 2;
    int orc_frac = orc_peasants / RECRUITFRACTION; /* anzahl orks. 2 ork = 1 bauer */
    if (orc_total<orc_frac) orc_frac = orc_total;
    orc_recruited = do_recruiting(recruits, orc_frac);
    assert(orc_recruited<=orc_frac);
    rsetpeasants(r, (orc_peasants - orc_recruited)/2);
    free_recruitments(recruits);
  }

  /* no limit: */
  recruits = select_recruitment(&recruitorders, any_recruiters, &orc_total);
  if (recruits) {
    do_recruiting(recruits, INT_MAX);
    free_recruitments(recruits);
  }

  assert(recruitorders==NULL);
}

static void
recruit(unit * u, struct order * ord, request ** recruitorders)
{
	int n;
  region * r = u->region;
	plane * pl;
	request *o;
	int recruitcost;
  const faction * f = u->faction;
  const struct race * rc = f->race;

  init_tokens(ord);
  skip_token();
  n = geti();
  if (f->no==MONSTER_FACTION) {
    /* Monster dürfen REKRUTIERE 15 dracoid machen */
    const char * str = getstrtoken();
    if (str!=NULL && *str) {
      for (rc = races;rc;rc=rc->next) {
        if (strncasecmp(LOC(f->locale, rc->_name[0]), str, strlen(str))==0)
          break;
        if (strncasecmp(LOC(f->locale, rc->_name[1]), str, strlen(str))==0)
          break;
      }
    }
    if (rc==NULL) rc = f->race;
  }

#if GUARD_DISABLES_RECRUIT == 1
	/* this is a very special case because the recruiting unit may be empty
	* at this point and we have to look at the creating unit instead. This
	* is done in cansee, which is called indirectly by is_guarded(). */
	if (is_guarded(r, u, GUARD_RECRUIT)) {
    cmistake(u, ord, 70, MSG_EVENT);
		return;
	}
#endif

	if (rc == new_race[RC_INSECT]) {
		if (get_gamedate(turn, 0)->season == 0 && rterrain(r) != T_DESERT) {
#ifdef INSECT_POTION
			boolean usepotion = false;
			unit *u2;

			for (u2 = r->units; u2; u2 = u2->next)
				if (fval(u2, UFL_WARMTH)) {
					usepotion = true;
					break;
				}
				if (!usepotion)
#endif
				{
        cmistake(u, ord, 98, MSG_EVENT);
					return;
				}
		}
		/* in Gletschern, Eisbergen gar nicht rekrutieren */
		if (r_insectstalled(r)) {
      cmistake(u, ord, 97, MSG_EVENT);
			return;
		}
	}
	if (is_cursed(r->attribs, C_RIOT, 0)) {
		/* Die Region befindet sich in Aufruhr */
    cmistake(u, ord, 237, MSG_EVENT);
		return;
	}

  if (!(rc->ec_flags & ECF_REC_HORSES) && fval(r, RF_ORCIFIED)) {
#if RACE_ADJUSTMENTS
    if (rc != new_race[RC_URUK])
#else
    if (rc != new_race[RC_ORC])
#endif
    {
      cmistake(u, ord, 238, MSG_EVENT);
      return;
    }
  }

	recruitcost = rc->recruitcost;
	if (recruitcost) {
		pl = getplane(r);
		if (pl && fval(pl, PFL_NORECRUITS)) {
			add_message(&u->faction->msgs,
				msg_feedback(u, ord, "error_pflnorecruit", ""));
			return;
		}

		if (get_pooled(u, oldresourcetype[R_SILVER], GET_DEFAULT) < recruitcost) {
      cmistake(u, ord, 142, MSG_EVENT);
			return;
		}
	}
	if (!playerrace(u->race) || idle(u->faction)) {
		cmistake(u, ord, 139, MSG_EVENT);
		return;
	}
	/* snotlinge sollten hiermit bereits abgefangen werden, die
	* parteirasse ist uruk oder ork*/
	if (u->race != rc) {
		if (u->number != 0) {
			cmistake(u, ord, 139, MSG_EVENT);
			return;
		}
		else u->race = rc;
	}

	if (has_skill(u, SK_MAGIC)) {
		/* error158;de;{unit} in {region}: '{command}' - Magier arbeiten
		* grundsätzlich nur alleine! */
		cmistake(u, ord, 158, MSG_EVENT);
		return;
	}
	if (has_skill(u, SK_ALCHEMY)
		&& count_skill(u->faction, SK_ALCHEMY) + n >
		max_skill(u->faction, SK_ALCHEMY))
	{
		cmistake(u, ord, 156, MSG_EVENT);
		return;
	}
	if (recruitcost) n = min(n, get_pooled(u, oldresourcetype[R_SILVER], GET_DEFAULT) / recruitcost);

	u->wants = n;

	if (!n) {
		cmistake(u, ord, 142, MSG_EVENT);
		return;
	}
	o = (request *) calloc(1, sizeof(request));
	o->qty = n;
	o->unit = u;
	addlist(recruitorders, o);
}
/* ------------------------------------------------------------- */

static void
give_cmd(unit * u, order * ord)
{
  region * r = u->region;
	unit *u2;
	const char *s;
	int i, n;
	const item_type * itype;
	int notfound_error = 63;

  init_tokens(ord);
  skip_token();
	u2 = getunit(r, u->faction);

	if (!u2 && !getunitpeasants) {
		cmistake(u, ord, notfound_error, MSG_COMMERCE);
		return;
	}

	/* Damit Tarner nicht durch die Fehlermeldung enttarnt werden können */
	if (u2 && !alliedunit(u2, u->faction, HELP_GIVE) && !cansee(u->faction,r,u2,0) && !ucontact(u2, u) && !fval(u2, UFL_TAKEALL)) {
		cmistake(u, ord, notfound_error, MSG_COMMERCE);
		return;
	}
	if (u == u2) {
		cmistake(u, ord, 8, MSG_COMMERCE);
		return;
	}

	/* UFL_TAKEALL ist ein grober Hack. Generalisierung tut not, ist aber nicht
	* wirklich einfach. */
	if (r->planep && fval(r->planep, PFL_NOGIVE) && (!u2 || !fval(u2, UFL_TAKEALL))) {
		cmistake(u, ord, 268, MSG_COMMERCE);
		return;
	}

	s = getstrtoken();

	if (findparam(s, u->faction->locale) == P_CONTROL) {
		if (!u2) {
			cmistake(u, ord, notfound_error, MSG_EVENT);
			return;
		}
		if (!u->building && !u->ship) {
			cmistake(u, ord, 140, MSG_EVENT);
			return;
		}
		if (u->building && u2->building != u->building) {
			cmistake(u, ord, 33, MSG_EVENT);
			return;
		}
		if (u->ship && u2->ship != u->ship) {
			cmistake(u, ord, 32, MSG_EVENT);
			return;
		}
		if (!fval(u, UFL_OWNER)) {
			cmistake(u, ord, 49, MSG_EVENT);
			return;
		}
		if (!alliedunit(u2, u->faction, HELP_GIVE) && !ucontact(u2, u)) {
			cmistake(u, ord, 40, MSG_EVENT);
			return;
		}
		freset(u, UFL_OWNER);
		fset(u2, UFL_OWNER);

		ADDMSG(&u->faction->msgs,
			msg_message("givecommand", "unit receipient", u, u2));
		if (u->faction != u2->faction) {
			ADDMSG(&u2->faction->msgs,
				msg_message("givecommand", "unit receipient",
				ucansee(u2->faction, u, u_unknown()), u2));
		}
		return;
	}
	if (u2 && u2->race == new_race[RC_SPELL]) {
		cmistake(u, ord, notfound_error, MSG_COMMERCE);
		return;
	}

	/* if ((u->race->ec_flags & NOGIVE) || fval(u,UFL_LOCKED)) {*/
	if (u->race->ec_flags & NOGIVE && u2!=NULL) {
		ADDMSG(&u->faction->msgs,
			msg_feedback(u, ord, "race_nogive", "race", u->race));
		return;
	}
	/* sperrt hier auch personenübergaben!
	if (u2 && !(u2->race->ec_flags & GETITEM)) {
	ADDMSG(&u->faction->msgs,
	msg_feedback(u, ord, "race_notake", "race", u2->race));
	return;
	}
	*/

	/* Übergabe aller Kräuter */
	if (findparam(s, u->faction->locale) == P_HERBS) {
		boolean given = false;
		if (!(u->race->ec_flags & GIVEITEM) && u2!=NULL) {
			ADDMSG(&u->faction->msgs,
				msg_feedback(u, ord, "race_nogive", "race", u->race));
			return;
		}
		if (u2 && !(u2->race->ec_flags & GETITEM)) {
			ADDMSG(&u->faction->msgs,
				msg_feedback(u, ord, "race_notake", "race", u2->race));
			return;
		}
		if (!u2) {
			if (!getunitpeasants) {
				cmistake(u, ord, notfound_error, MSG_COMMERCE);
				return;
			}
		}
		if (u->items) {
			item **itmp=&u->items;
			while (*itmp) {
				const item_type * itype = (*itmp)->type;
				if (fval(itype, ITF_HERB) && (*itmp)->number>0) {
					/* give_item ändert im fall,das man alles übergibt, die
					* item-liste der unit, darum continue vor pointerumsetzten */
					if (give_item((*itmp)->number, (*itmp)->type, u, u2, ord)==0) {
						given = true;
						continue;
					}
				}
				itmp = &(*itmp)->next;
			}
		}
		if (!given) cmistake(u, ord, 38, MSG_COMMERCE);
		return;
	}
	if (findparam(s, u->faction->locale) == P_ZAUBER) {
		cmistake(u, ord, 7, MSG_COMMERCE);
		/* geht nimmer */
		return;
	}
	if (findparam(s, u->faction->locale) == P_UNIT) {	/* Einheiten uebergeben */
		if (!(u->race->ec_flags & GIVEUNIT)) {
			cmistake(u, ord, 167, MSG_COMMERCE);
			return;
		}

		if (u2 && !alliedunit(u2, u->faction, HELP_GIVE) && !ucontact(u2, u)) {
			cmistake(u, ord, 40, MSG_COMMERCE);
			return;
		}
		give_unit(u, u2, ord);
		return;
	}
	if (findparam(s, u->faction->locale) == P_ANY) { /* Alle Gegenstände übergeben */
		const char * s = getstrtoken();

    if (u2 && !alliedunit(u2, u->faction, HELP_GIVE) && !ucontact(u2, u)) {
			cmistake(u, ord, 40, MSG_COMMERCE);
			return;
		}

		if (*s == 0) {
			if (!(u->race->ec_flags & GIVEITEM) && u2!=NULL) {
				ADDMSG(&u->faction->msgs,
					msg_feedback(u, ord, "race_nogive", "race", u->race));
				return;
			}
			if (u2 && !(u2->race->ec_flags & GETITEM)) {
				ADDMSG(&u->faction->msgs,
					msg_feedback(u, ord, "race_notake", "race", u2->race));
				return;
			}

			/* für alle items einmal prüfen, ob wir mehr als von diesem Typ
			* reserviert ist besitzen und diesen Teil dann übergeben */
			if (u->items) {
				item **itmp=&u->items;
				while (*itmp) {
					if ((*itmp)->number > 0
						&& (*itmp)->number - get_reservation(u, (*itmp)->type->rtype) > 0) {
							n = (*itmp)->number - get_reservation(u, (*itmp)->type->rtype);
							if (give_item(n, (*itmp)->type, u, u2, ord)==0) continue;
						}
						itmp = &(*itmp)->next;
				}
			}
			return;
		}
		i = findparam(s, u->faction->locale);
		if (i == P_PERSON) {
			if (!(u->race->ec_flags & GIVEPERSON)) {
				ADDMSG(&u->faction->msgs,
					msg_feedback(u, ord, "race_noregroup", "race", u->race));
				return;
			}
			n = u->number;
			give_men(n, u, u2, ord);
			return;
		}

		if (!(u->race->ec_flags & GIVEITEM) && u2!=NULL) {
			ADDMSG(&u->faction->msgs,
				msg_feedback(u, ord, "race_nogive", "race", u->race));
			return;
		}
		if (u2 && !(u2->race->ec_flags & GETITEM)) {
			ADDMSG(&u->faction->msgs,
				msg_feedback(u, ord, "race_notake", "race", u2->race));
			return;
		}

		itype = finditemtype(s, u->faction->locale);
		if (itype!=NULL) {
			item * i = *i_find(&u->items, itype);
			if (i!=NULL) {
				n = i->number - get_reservation(u, itype->rtype);
				give_item(n, itype, u, u2, ord);
				return;
			}
		}
	}

	n = atoip(s);		/* n: anzahl */
	s = getstrtoken();

	if (s == NULL) {
		cmistake(u, ord, 113, MSG_COMMERCE);
		return;
	}

  if (u2 && !alliedunit(u2, u->faction, HELP_GIVE) && !ucontact(u2, u)) {
		const resource_type * rtype = findresourcetype(s, u->faction->locale);
		if (rtype==NULL || !fval(rtype, RTF_SNEAK))
		{
			cmistake(u, ord, 40, MSG_COMMERCE);
			return;
		}
	}
	i = findparam(s, u->faction->locale);
	if (i == P_PERSON) {
		if (!(u->race->ec_flags & GIVEPERSON)) {
			ADDMSG(&u->faction->msgs,
				msg_feedback(u, ord, "race_noregroup", "race", u->race));
			return;
		}
		give_men(n, u, u2, ord);
		return;
	}

	if (u2!=NULL) {
		if (!(u->race->ec_flags & GIVEITEM) && u2!=NULL) {
			ADDMSG(&u->faction->msgs,
				msg_feedback(u, ord, "race_nogive", "race", u->race));
			return;
		}
		if (!(u2->race->ec_flags & GETITEM)) {
			ADDMSG(&u->faction->msgs,
				msg_feedback(u, ord, "race_notake", "race", u2->race));
			return;
		}
	}

	itype = finditemtype(s, u->faction->locale);
	if (itype!=NULL) {
		give_item(n, itype, u, u2, ord);
		return;
	}
	cmistake(u, ord, 123, MSG_COMMERCE);
}

static int
forget_cmd(unit * u, order * ord)
{
	skill_t talent;
	const char *s;
  
  init_tokens(ord);
  skip_token();
  s = getstrtoken();

	if ((talent = findskill(s, u->faction->locale)) != NOSKILL) {
		ADDMSG(&u->faction->msgs,
			msg_message("forget", "unit skill", u, talent));
		set_level(u, talent, 0);
	}
  return 0;
}

void
add_spende(faction * f1, faction * f2, int amount, region * r)
{
	donation *sp;

	sp = r->donations;

	while (sp) {
		if (sp->f1 == f1 && sp->f2 == f2) {
			sp->amount += amount;
			return;
		}
		sp = sp->next;
	}

	sp = calloc(1, sizeof(donation));
	sp->f1 = f1;
	sp->f2 = f2;
	sp->amount = amount;
	sp->next = r->donations;
	r->donations = sp;
}

static boolean
maintain(building * b, boolean first)
/* first==false -> take money from wherever you can */
{
	int c;
	region * r = b->region;
	boolean paid = true, work = first;
	unit * u;
	if (fval(b, BLD_MAINTAINED) || b->type==NULL || b->type->maintenance==NULL || is_cursed(b->attribs, C_NOCOST, 0)) {
		fset(b, BLD_MAINTAINED);
		fset(b, BLD_WORKING);
		return true;
	}
	u = buildingowner(r, b);
	if (u==NULL) return false;
	for (c=0;b->type->maintenance[c].number;++c) {
		const maintenance * m = b->type->maintenance+c;
		int need = m->number;

		if (fval(m, MTF_VARIABLE)) need = need * b->size;
		if (u) {
			/* first ist im ersten versuch true, im zweiten aber false! Das
			* bedeutet, das in der Runde in die Region geschafften Resourcen
			* nicht genutzt werden können, weil die reserviert sind! */
			if (!first) need -= get_pooled(u, m->rtype, GET_ALL);
			else need -= get_pooled(u, m->rtype, GET_DEFAULT);
			if (!first && need > 0) {
				unit * ua;
				for (ua=r->units;ua;ua=ua->next) freset(ua->faction, FL_DH);
				fset(u->faction, FL_DH); /* hat schon */
				for (ua=r->units;ua;ua=ua->next) {
					if (!fval(ua->faction, FL_DH) && (ua->faction == u->faction || alliedunit(ua, u->faction, HELP_MONEY))) {
						need -= get_pooled(ua, m->rtype, GET_ALL);
						fset(ua->faction, FL_DH);
						if (need<=0) break;
					}
				}
			}
			if (need > 0) {
				if (!fval(m, MTF_VITAL)) work = false;
				else {
					paid = false;
					break;
				}
			}
		}
	}
	if (paid && c>0) {
		/* TODO: wieviel von was wurde bezahlt */
		if (first) {
			ADDMSG(&u->faction->msgs, msg_message("maintenance", "unit building", u, b));
		} else {
			ADDMSG(&u->faction->msgs, msg_message("maintenance_late", "building", b));
		}
		fset(b, BLD_MAINTAINED);
    if (work) {
      fset(b, BLD_WORKING);
    }
		for (c=0;b->type->maintenance[c].number;++c) {
			const maintenance * m = b->type->maintenance+c;
			int cost = m->number;

			if (!fval(m, MTF_VITAL) && !work) continue;
			if (fval(m, MTF_VARIABLE)) cost = cost * b->size;

			if (!first) cost -= use_pooled(u, m->rtype, GET_ALL, cost);
			else cost -= use_pooled(u, m->rtype, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, cost);
			if (!first && cost > 0) {
				unit * ua;
				for (ua=r->units;ua;ua=ua->next) freset(ua->faction, FL_DH);
				fset(u->faction, FL_DH); /* hat schon */
				for (ua=r->units;ua;ua=ua->next) {
					if (!fval(ua->faction, FL_DH) && alliedunit(ua, u->faction, HELP_MONEY)) {
						int give = use_pooled(ua, m->rtype, GET_ALL, cost);
						if (!give) continue;
						cost -= give;
						fset(ua->faction, FL_DH);
						if (m->rtype==r_silver) add_spende(ua->faction, u->faction, give, r);
						if (cost<=0) break;
					}
				}
			}
			assert(cost==0);
		}
	} else {
		message * msg = add_message(&u->faction->msgs,
			msg_message("maintenancefail", "unit building", u, b));
		msg_release(msg);
		return false;
	}
	return true;
}

static void
gebaeude_stuerzt_ein(region * r, building * b)
{
	unit *u;
	int n, i;
	int opfer = 0;
	int road = 0;
	struct message * msg;

	for (u = r->units; u; u = u->next) {
		if (u->building == b) {
			int loss = 0;

			fset(u->faction, FL_MARK);
			freset(u, UFL_OWNER);
			leave(r,u);
			n = u->number;
			for (i = 0; i < n; i++) {
				if (rand() % 100 >= EINSTURZUEBERLEBEN) {
					++loss;
				}
			}
			scale_number(u, u->number - loss);
			opfer += loss;
		}
	}

	msg = msg_message("buildingcrash", "region building opfer road", r, b, opfer, road);
	add_message(&r->msgs, msg);
	for (u=r->units; u; u=u->next) {
		faction * f = u->faction;
		if (fval(f, FL_MARK)) {
			freset(u->faction, FL_MARK);
			add_message(&f->msgs, msg);
		}
	}
	msg_release(msg);
	destroy_building(b);
}

void
maintain_buildings(boolean crash)
{
	region * r;
	for (r = regions; r; r = r->next) {
		building **bp = &r->buildings;
		while (*bp) {
			building * b = *bp;
      boolean maintained = maintain(b, !crash);

      /* the second time, send a message */
      if (crash) {
        if (!maintained && (rand() % 100 < EINSTURZCHANCE)) {
  				gebaeude_stuerzt_ein(r, b);
	  			continue;
        } else if (!fval(b, BLD_WORKING)) {
          unit * u = buildingowner(r, b);
          const char * msgtype = maintained?"maintenance_nowork":"maintenance_none";
  	  		struct message * msg = msg_message(msgtype, "building", b);

          if (u) {
						add_message(&u->faction->msgs, msg);
						r_addmessage(r, u->faction, msg);
          } else {
            add_message(&r->msgs, msg);
          }
          msg_release(msg);
        }
      }
			bp=&b->next;
		}
	}
}


void
economics(void)
{
  region *r;
  unit *u;

  /* Geben vor Selbstmord (doquit)! Hier alle unmittelbaren Befehle.
  * Rekrutieren vor allen Einnahmequellen. Bewachen JA vor Steuern
  * eintreiben. */

  for (r = regions; r; r = r->next) if (r->units) {
    request *recruitorders = NULL;

    for (u = r->units; u; u = u->next) {
      order * ord;
      boolean destroyed = false;
      for (ord = u->orders; ord; ord = ord->next) {
        switch (get_keyword(ord)) {
          case K_DESTROY:
            if (!destroyed) {
              if (destroy_cmd(u, ord)!=0) ord = NULL;
              destroyed = true;
            }
            break;

          case K_GIVE:
          case K_LIEFERE:
            give_cmd(u, ord);
            break;

          case K_FORGET:
            forget_cmd(u, ord);
            break;

        }
	    	if (u->orders==NULL) break;
      }
    }
    /* RECRUIT orders */

    for (u = r->units; u; u = u->next) {
			order * ord;
			for (ord = u->orders; ord; ord = ord->next) {
        if (get_keyword(ord) == K_RECRUIT) {
          recruit(u, ord, &recruitorders);
          break;
        }
      }
    }

    if (recruitorders) expandrecruit(r, recruitorders);

  }
}
/* ------------------------------------------------------------- */


static void
manufacture(unit * u, const item_type * itype, int want)
{
  int n;
  int skill;
  int minskill = itype->construction->minskill;
  skill_t sk = itype->construction->skill;

  skill = effskill(u, sk);
  skill = skillmod(itype->rtype->attribs, u, u->region, sk, skill, SMF_PRODUCTION);

  if (skill < 0) {
    /* an error occured */
    int err = -skill;
    cmistake(u, u->thisorder, err, MSG_PRODUCE);
    return;
  }

  if(want==0)
    want=maxbuild(u, itype->construction);

  n = build(u, itype->construction, 0, want);
  switch (n) {
    case ENEEDSKILL:
      ADDMSG(&u->faction->msgs,
        msg_feedback(u, u->thisorder, "skill_needed", "skill", sk));
      return;
    case ELOWSKILL:
      ADDMSG(&u->faction->msgs,
        msg_feedback(u, u->thisorder, "manufacture_skills", "skill minskill product",
        sk, minskill, itype->rtype, 1));
      return;
    case ENOMATERIALS:
      /* something missing from the list of materials */
      strcpy(buf, "Dafür braucht man mindestens:");
      {
        int c, n;
        const construction * cons = itype->construction;
        char * ch = buf+strlen(buf);
        assert(cons);
        for (c=0;cons->materials[c].number; c++) {
	  requirement * m = cons->materials+c;
          if (c!=0)
            strcat(ch++, ",");
          n = m->number / cons->reqsize;
          sprintf(ch, " %d %s", n?n:1,
            locale_string(u->faction->locale,
            resourcename(m->rtype, m->number!=1)));
          ch = ch+strlen(ch);
        }
        mistake(u, u->thisorder, buf, MSG_PRODUCE);
        return;
      }
  }
  if (n>0) {
    i_change(&u->items, itype, n);
    if (want==INT_MAX) want = n;
    ADDMSG(&u->faction->msgs, msg_message("manufacture",
      "unit region amount wanted resource", u, u->region, n, want, itype->rtype));
  } else {
    cmistake(u, u->thisorder, 125, MSG_PRODUCE);
  }
}

typedef struct allocation {
	struct allocation * next;
	int want, get;
	double save;
	unsigned int flags;
	unit * unit;
} allocation;
#define new_allocation() calloc(sizeof(allocation), 1)
#define free_allocation(a) free(a)

typedef struct allocation_list {
	struct allocation_list * next;
	allocation * data;
	const resource_type * type;
} allocation_list;

static allocation_list * allocations;


static void
allocate_resource(unit * u, const resource_type * rtype, int want)
{
	const item_type * itype = resource2item(rtype);
	region * r = u->region;
	int busy = u->number;
	int dm = 0;
	allocation_list * alist;
	allocation * al;
	unit *u2;
	int amount, skill;

	/* momentan kann man keine ressourcen abbauen, wenn man dafür
	* Materialverbrauch hat: */
	assert(itype!=NULL && (itype->construction==NULL || itype->construction->materials==NULL));

	if (itype == olditemtype[I_WOOD] && fval(r, RF_MALLORN)) {
		cmistake(u, u->thisorder, 92, MSG_PRODUCE);
		return;
	}
	if (itype == olditemtype[I_MALLORN] && !fval(r, RF_MALLORN)) {
		cmistake(u, u->thisorder, 91, MSG_PRODUCE);
		return;
	}
	if (itype == olditemtype[I_LAEN]) {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (btype != bt_find("mine")) {
			cmistake(u, u->thisorder, 104, MSG_PRODUCE);
			return;
		}
	}

	if (besieged(u)) {
		cmistake(u, u->thisorder, 60, MSG_PRODUCE);
		return;
	}
	/* Elfen können Holzfällen durch Bewachen verhindern, wenn sie
	die Holzfäller sehen. */
	if (itype == olditemtype[I_WOOD] || itype == olditemtype[I_MALLORN]) {
		for (u2 = r->units; u2; u2 = u2->next) {
			if (getguard(u2) & GUARD_TREES
				&& u2->number
				&& cansee(u2->faction,r,u,0)
				&& !besieged(u2)
				&& !alliedunit(u2, u->faction, HELP_GUARD)
				&& armedmen(u2)
				) {
					add_message(&u->faction->msgs,
						msg_feedback(u, u->thisorder, "region_guarded", "guard", u2));
					return;
				}
		}
	}

	/* Bergwächter können Abbau von Eisen/Laen durch Bewachen verhindern.
	* Als magische Wesen 'sehen' Bergwächter alles und werden durch
	* Belagerung nicht aufgehalten.  (Ansonsten wie oben bei Elfen anpassen).
	*/
	if (itype == olditemtype[I_IRON] || itype == olditemtype[I_LAEN])
	{
		for (u2 = r->units; u2; u2 = u2->next ) {
			if (getguard(u) & GUARD_MINING
				&& !fval(u2, UFL_ISNEW)
				&& u2->number
				&& !alliedunit(u2, u->faction, HELP_GUARD))
			{
				add_message(&u->faction->msgs,
					msg_feedback(u, u->thisorder, "region_guarded", "guard", u2));
				return;
			}
		}
	}

	assert(itype->construction->skill!=0 || "limited resource needs a required skill for making it");
	skill = eff_skill(u, itype->construction->skill, u->region);
	if (skill == 0) {
		skill_t sk = itype->construction->skill;
		add_message(&u->faction->msgs,
			msg_feedback(u, u->thisorder, "skill_needed", "skill", sk));
		return;
	}
	if (skill < itype->construction->minskill) {
		skill_t sk = itype->construction->skill;
		add_message(&u->faction->msgs,
			msg_feedback(u, u->thisorder, "manufacture_skills", "skill minskill product",
			sk, itype->construction->minskill, itype->rtype));
		return;
	} else {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (itype == olditemtype[I_IRON] && btype == bt_find("mine")) {
			++skill;
		}
		else if (itype == olditemtype[I_STONE] && btype == bt_find("quarry")) {
			++skill;
		}
		else if (itype == olditemtype[I_WOOD] && btype == bt_find("sawmill")) {
			++skill;
		}
		else if (itype == olditemtype[I_MALLORN] && btype == bt_find("sawmill")) {
			++skill;
		}
	}
	amount = skill * u->number;
	/* nun ist amount die Gesamtproduktion der Einheit (in punkten) */

	/* mit Flinkfingerring verzehnfacht sich die Produktion */
	amount += skill * min(u->number, get_item(u,I_RING_OF_NIMBLEFINGER)) * 9;

	/* Schaffenstrunk: */
	if ((dm = get_effect(u, oldpotiontype[P_DOMORE])) != 0) {
		dm = min(dm, u->number);
		change_effect(u, oldpotiontype[P_DOMORE], -dm);
		amount += dm * skill;	/* dm Personen produzieren doppelt */
	}

	amount /= itype->construction->minskill;

	/* Limitierung durch Parameter m. */
	if (want > 0 && want < amount) amount = want;

	busy = (amount + skill - 1) / skill; /* wieviel leute tun etwas? */

	alist = allocations;
	while (alist && alist->type!=rtype) alist = alist->next;
	if (!alist) {
		alist = calloc(sizeof(struct allocation_list), 1);
		alist->next = allocations;
		alist->type = rtype;
		allocations = alist;
	}
	al = new_allocation();
	al->want = amount;
	al->save = 1.0;
	al->next = alist->data;
	al->unit = u;
	alist->data = al;
	if (itype==olditemtype[I_IRON]) {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (btype==bt_find("mine"))
			al->save *= 0.5;
		if (u->race == new_race[RC_DWARF]) {
#if RACE_ADJUSTMENTS
			al->save *= 0.75;
#else
			al->save *= 0.5;
#endif
		}
	} else if (itype==olditemtype[I_STONE]) {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (btype==bt_find("quarry"))
			al->save = al->save*0.5;
#if RACE_ADJUSTMENTS
		if (u->race == new_race[RC_TROLL])
			al->save = al->save*0.75;
#endif
	} else if (itype==olditemtype[I_MALLORN]) {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (btype==bt_find("sawmill"))
			al->save *= 0.5;
	} else if (itype==olditemtype[I_WOOD]) {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (btype==bt_find("sawmill"))
			al->save *= 0.5;
	}
}

typedef struct allocator {
	const struct resource_type * type;
	void (*allocate)(const struct allocator *, region *, allocation *);

	struct allocator * next;
} allocator;

static struct allocator * allocators;

static const allocator *
get_allocator(const struct resource_type * type)
{
	const struct allocator * alloc = allocators;
	while (alloc && alloc->type!=type) alloc = alloc->next;
	return alloc;
}

static allocator *
make_allocator(const struct resource_type * type, void (*allocate)(const struct allocator *, region *, allocation *))
{
	allocator * alloc = (allocator *)malloc(sizeof(allocator));
	alloc->type = type;
	alloc->allocate = allocate;
	return alloc;
}

static void
add_allocator(allocator * alloc)
{
	alloc->next = allocators;
	allocators = alloc;
}

enum {
	AFL_DONE = 1<<0,
	AFL_LOWSKILL = 1<<1
};

static int
required(int want, double save)
{
	int norders = (int)(want * save);
	if (norders < want*save) ++norders;
	return norders;
}

static void
leveled_allocation(const allocator * self, region * r, allocation * alist)
{
	const resource_type * rtype = self->type;
	const item_type * itype = resource2item(rtype);
	rawmaterial * rm = rm_get(r, rtype);
	int need;
	boolean first = true;

	if (rm!=NULL) {
		do {
			int avail = rm->amount;
			int norders = 0;
			allocation * al;

			if(avail <= 0) {
				for (al=alist;al;al=al->next) {
					al->get = 0;
				}
				break;
			}

			assert(avail>0);

			for (al=alist;al;al=al->next) if (!fval(al, AFL_DONE)) {
				int req = required(al->want-al->get, al->save);
				assert(al->get<=al->want && al->get >= 0);
				if (eff_skill(al->unit, itype->construction->skill, r)
					>= rm->level+itype->construction->minskill-1) {
						if (req) {
							norders += req;
						} else {
							fset(al, AFL_DONE);
						}
					} else {
						fset(al, AFL_DONE);
						if (first) fset(al, AFL_LOWSKILL);
					}
			}
			need = norders;

			avail = min(avail, norders);
			if (need>0) {
				int use = 0;
				for (al=alist;al;al=al->next) if (!fval(al, AFL_DONE)) {
					if (avail > 0) {
						int want = required(al->want-al->get, al->save);
						int x = avail*want/norders;
						/* Wenn Rest, dann würfeln, ob ich was bekomme: */
						if (rand() % norders < (avail*want) % norders)
							++x;
						avail -= x;
						use += x;
						norders -= want;
						need -= x;
						al->get = min(al->want, al->get+(int)(x/al->save));
					}
				}
				if (use) {
					assert(use<=rm->amount);
					rm->type->use(rm, r, use);
				}
				assert(avail==0 || norders==0);
			}
			first = false;
		} while (need>0);
	}
}

static void
attrib_allocation(const allocator * self, region * r, allocation * alist)
{
	allocation * al;
	const resource_type * rtype = self->type;
	int avail = 0;
	int norders = 0;
	attrib * a = a_find(rtype->attribs, &at_resourcelimit);
	resource_limit * rdata = (resource_limit*)a->data.v;

	for (al=alist;al;al=al->next) {
		norders += required(al->want, al->save);
	}

	if (rdata->limit) {
		avail = rdata->limit(r, rtype);
		if (avail < 0) avail = 0;
	}
	else avail = rdata->value;

	avail = min(avail, norders);
	for (al=alist;al;al=al->next) {
		if (avail > 0) {
			int want = required(al->want, al->save);
			int x = avail*want/norders;
			/* Wenn Rest, dann würfeln, ob ich was bekomme: */
			if (rand() % norders < (avail*want) % norders)
				++x;
			avail -= x;
			norders -= want;
			al->get = min(al->want, (int)(x/al->save));
			if (rdata->use) {
				int use = required(al->get, al->save);
				if (use) rdata->use(r, rtype, use);
			}
		}
	}
	assert(avail==0 || norders==0);
}

static void
split_allocations(region * r)
{
	allocation_list ** p_alist=&allocations;
	freset(r, RF_DH);
	while (*p_alist) {
		allocation_list * alist = *p_alist;
		const resource_type * rtype = alist->type;
		const allocator * alloc = get_allocator(rtype);
		const item_type * itype = resource2item(rtype);
		allocation ** p_al = &alist->data;

		freset(r, RF_DH);
		alloc->allocate(alloc, r, alist->data);

		while (*p_al) {
			allocation * al = *p_al;
			if (al->get) {
				assert(itype || !"not implemented for non-items");
				i_change(&al->unit->items, itype, al->get);
				produceexp(al->unit, itype->construction->skill, al->unit->number);
				fset(r, RF_DH);
			}
			if (al->want==INT_MAX) al->want = al->get;
			if (fval(al, AFL_LOWSKILL)) {
				add_message(&al->unit->faction->msgs,
					msg_message("produce_lowskill", "unit region resource",
					al->unit, al->unit->region, rtype));
			} else {
				add_message(&al->unit->faction->msgs,
					msg_message("produce", "unit region amount wanted resource",
					al->unit, al->unit->region, al->get, al->want, rtype));
			}
			*p_al=al->next;
			free_allocation(al);
		}
		*p_alist=alist->next;
		free(alist);
	}
	allocations = NULL;
}

static void
create_potion(unit * u, const potion_type * ptype, int want)
{
	int built;

	if(want==0)
		want=maxbuild(u, ptype->itype->construction);
	built = build(u, ptype->itype->construction, 0, want);
	switch (built) {
		case ELOWSKILL:
		case ENEEDSKILL:
			/* no skill, or not enough skill points to build */
			cmistake(u, u->thisorder, 50, MSG_PRODUCE);
			break;
		case ECOMPLETE:
			assert(0);
			break;
		case ENOMATERIALS:
			/* something missing from the list of materials */
			strcpy(buf, "Dafür braucht man mindestens:");
			{
				int c, n;
				const construction * cons = ptype->itype->construction;
				char * ch = buf+strlen(buf);
				assert(cons);
				for (c=0;cons->materials[c].number; c++) {
				  const requirement * m = cons->materials+c;
					if (c!=0)
						strcat(ch++, ",");
					n = m->number / cons->reqsize;
					sprintf(ch, " %d %s", n?n:1,
						LOC(u->faction->locale,
						resourcename(m->rtype, m->number!=1)));
					ch = ch+strlen(ch);
				}
				strcat(ch,".");
				mistake(u, u->thisorder, buf, MSG_PRODUCE);
				return;
			}
			break;
		default:
			i_change(&u->items, ptype->itype, built);
			if (want==INT_MAX) want = built;
			ADDMSG(&u->faction->msgs, msg_message("manufacture",
				"unit region amount wanted resource", u, u->region, built, want, ptype->itype->rtype));
			break;
	}
}

static void
create_item(unit * u, const item_type * itype, int want)
{
	if (fval(itype->rtype, RTF_LIMITED)) {
#if GUARD_DISABLES_PRODUCTION == 1
		if(is_guarded(u->region, u, GUARD_PRODUCE)) {
			cmistake(u, u->thisorder, 70, MSG_EVENT);
			return;
		}
#endif
		allocate_resource(u, itype->rtype, want);
	} else {
		const potion_type * ptype = resource2potion(itype->rtype);
		if (ptype!=NULL) create_potion(u, ptype, want);
		else if (itype->construction && itype->construction->materials) manufacture(u, itype, want);
		else cmistake(u, u->thisorder, 125, MSG_PRODUCE);
	}
}

static void
make_cmd(unit * u, struct order * ord)
{
  region * r = u->region;
	const building_type * btype;
	const ship_type * stype;
	param_t p;
	int m;
	const item_type * itype;
	const char *s;
  const struct locale * lang = u->faction->locale;
  
  init_tokens(ord);
  skip_token();
  s = getstrtoken();

	m = atoi(s);
	sprintf(buf, "%d", m);
	if (!strcmp(buf, s)) {
		/* first came a want-paramter */
		s = getstrtoken();
	} else {
		m = INT_MAX;
	}

	p = findparam(s, u->faction->locale);

	/* MACHE TEMP kann hier schon gar nicht auftauchen, weil diese nicht in
	* thisorder abgespeichert werden - und auf den ist getstrtoken() beim
	* aufruf von make geeicht */

	if (p == P_ROAD) {
		direction_t d;
		if(r->planep && fval(r->planep, PFL_NOBUILD)) {
			cmistake(u, ord, 275, MSG_PRODUCE);
			return;
		}
		d = finddirection(getstrtoken(), u->faction->locale);
		if (d!=NODIRECTION) {
			if(r->planep && fval(r->planep, PFL_NOBUILD)) {
				cmistake(u, ord, 94, MSG_PRODUCE);
				return;
			}
			build_road(r, u, m, d);
		} else cmistake(u, ord, 71, MSG_PRODUCE);
		return;
	} else if (p == P_SHIP) {
		if(r->planep && fval(r->planep, PFL_NOBUILD)) {
			cmistake(u, ord, 276, MSG_PRODUCE);
			return;
		}
		continue_ship(r, u, m);
		return;
	} else if (p == P_HERBS) {
		herbsearch(r, u, m);
		return;
	}

  /* since the string can match several objects, like in 'academy' and
   * 'academy of arts', we need to figure out what the player meant.
   * This is not 100% safe.
   */
	stype = findshiptype(s, lang);
	btype = findbuildingtype(s, lang);
	itype = finditemtype(s, lang);
  
  if (itype!=NULL && (btype!=NULL || stype!=NULL)) {
    if (itype->construction==NULL) {
      /* if the item cannot be made, we probably didn't mean to make it */
      itype = NULL;
    } else if (stype!=NULL) {
      const char * sname = LOC(lang, stype->name[0]);
      const char * iname = LOC(lang, resourcename(itype->rtype, 0));
      if (strlen(iname)<strlen(sname)) stype = NULL;
      else itype = NULL;
    } else {
      const char * bname = LOC(lang, btype->_name);
      const char * iname = LOC(lang, resourcename(itype->rtype, 0));
      if (strlen(iname)<strlen(bname)) btype = NULL;
      else itype = NULL;
    }
  }

  if (btype!=NULL && stype!=NULL) {
    const char * bname = LOC(lang, btype->_name);
    const char * sname = LOC(lang, stype->name[0]);
    if (strlen(sname)<strlen(bname)) btype = NULL;
    else stype = NULL;
  }
  
	if (stype != NOSHIP) {
		if(r->planep && fval(r->planep, PFL_NOBUILD)) {
			cmistake(u, ord, 276, MSG_PRODUCE);
			return;
		}
		create_ship(r, u, stype, m, ord);
		return;
	}

	if (btype != NOBUILDING) {
		if(r->planep && fval(r->planep, PFL_NOBUILD)) {
			cmistake(u, ord, 94, MSG_PRODUCE);
			return;
		}
		build_building(u, btype, m, ord);
		return;
	}

	if (itype!=NULL) {
		create_item(u, itype, m);
		return;
	}

	cmistake(u, ord, 125, MSG_PRODUCE);
}
/* ------------------------------------------------------------- */

static void 
free_luxuries(struct attrib * a)
{
  i_freeall((item*)a->data.v);
}

const attrib_type at_luxuries = {
	"luxuries", NULL, free_luxuries, NULL, NULL, NULL
};

static void
expandbuying(region * r, request * buyorders)
{
	int max_products;
	unit *u;
	static struct trade {
		const luxury_type * type;
		int number;
		int multi;
	} *trades, *trade;
	static int ntrades=0;
	int i, j;
	const luxury_type * ltype;

	if (ntrades==0) {
		for (ltype=luxurytypes;ltype;ltype=ltype->next)
			++ntrades;
		trades = gc_add(calloc(sizeof(struct trade), ntrades));
		for (i=0, ltype=luxurytypes;i!=ntrades;++i, ltype=ltype->next)
			trades[i].type = ltype;
	}
	for (i=0;i!=ntrades;++i) {
		trades[i].number = 0;
		trades[i].multi = 1;
	}

	if (!buyorders) return;

	/* Initialisation. multiplier ist der Multiplikator auf den
	* Verkaufspreis. Für max_products Produkte kauft man das Produkt zum
	* einfachen Verkaufspreis, danach erhöht sich der Multiplikator um 1.
	* counter ist ein Zähler, der die gekauften Produkte zählt. money
	* wird für die debug message gebraucht. */

	max_products = rpeasants(r) / TRADE_FRACTION;

	/* Kauf - auch so programmiert, daß er leicht erweiterbar auf mehrere
	* Güter pro Monat ist. j sind die Befehle, i der Index des
	* gehandelten Produktes. */
  if (max_products>0) {
	expandorders(r, buyorders);
	if (!norders) return;

	for (j = 0; j != norders; j++) {
		int price, multi;
		ltype = oa[j].type.ltype;
		trade = trades;
		while (trade->type!=ltype) ++trade;
		multi = trade->multi;
		if (trade->number + 1 > max_products) ++multi;
		price = ltype->price * multi;

		if (get_pooled(oa[j].unit, oldresourcetype[R_SILVER], GET_DEFAULT) >= price) {
			unit * u = oa[j].unit;

			/* litems zählt die Güter, die verkauft wurden, u->n das Geld, das
			* verdient wurde. Dies muß gemacht werden, weil der Preis ständig sinkt,
			* man sich also das verdiente Geld und die verkauften Produkte separat
			* merken muß. */
			attrib * a = a_find(u->attribs, &at_luxuries);
			if (a==NULL) a = a_add(&u->attribs, a_new(&at_luxuries));
			i_change((item**)&a->data.v, ltype->itype, 1);
			i_change(&oa[j].unit->items, ltype->itype, 1);
			use_pooled(u, oldresourcetype[R_SILVER], GET_DEFAULT, price);
			if (u->n < 0)
				u->n = 0;
			u->n += price;

			rsetmoney(r, rmoney(r) + price);

			/* Falls mehr als max_products Bauern ein Produkt verkauft haben, steigt
			* der Preis Multiplikator für das Produkt um den Faktor 1. Der Zähler
			* wird wieder auf 0 gesetzt. */
			if (++trade->number > max_products) {
				trade->number = 0;
				++trade->multi;
			}
      fset(u, UFL_LONGACTION);
		}
	}
	free(oa);

	/* Ausgabe an Einheiten */

	for (u = r->units; u; u = u->next) {
		attrib * a = a_find(u->attribs, &at_luxuries);
		item * itm;
		if (a==NULL) continue;
		add_message(&u->faction->msgs, new_message(u->faction,
			"buy%u:unit%i:money", u, u->n));
		for (itm=(item*)a->data.v; itm; itm=itm->next) {
			if (itm->number) {
				add_message(&u->faction->msgs, new_message(u->faction,
					"buyamount%u:unit%i:amount%X:resource", u, itm->number, itm->type->rtype));
			}
		}
		a_remove(&u->attribs, a);
	}
  }
}

attrib_type at_trades = {
	"trades",
		DEFAULT_INIT,
		DEFAULT_FINALIZE,
		DEFAULT_AGE,
		NO_WRITE,
		NO_READ
};

static void
buy(unit * u, request ** buyorders, struct order * ord)
{
  region * r = u->region;
	int n, k;
	request *o;
	attrib * a;
	const item_type * itype = NULL;
	const luxury_type * ltype = NULL;
	if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
		cmistake(u, ord, 69, MSG_INCOME);
		return;
	}
	if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
		cmistake(u, ord, 69, MSG_INCOME);
		return;
	}
	/* Im Augenblick kann man nur 1 Produkt kaufen. expandbuying ist aber
	* schon dafür ausgerüstet, mehrere Produkte zu kaufen. */

  init_tokens(ord);
  skip_token();
	n = geti();
	if (!n) {
		cmistake(u, ord, 26, MSG_COMMERCE);
		return;
	}
	if (besieged(u)) {
		/* Belagerte Einheiten können nichts kaufen. */
		cmistake(u, ord, 60, MSG_COMMERCE);
		return;
	}

	if (u->race == new_race[RC_INSECT]) {
		/* entweder man ist insekt, oder... */
		if (rterrain(r) != T_SWAMP && rterrain(r) != T_DESERT && !rbuildings(r)) {
			cmistake(u, ord, 119, MSG_COMMERCE);
			return;
		}
	} else {
		/* ...oder in der Region muß es eine Burg geben. */
    building * b;
    static const struct building_type * bt_castle;
    if (!bt_castle) bt_castle = bt_find("castle");
    for (b=r->buildings;b;b=b->next) {
      if (b->type==bt_castle && b->size>=2) break;
    }
		if (b==NULL) {
			cmistake(u, ord, 119, MSG_COMMERCE);
			return;
		}
	}

	/* Ein Händler kann nur 10 Güter pro Talentpunkt handeln. */
	k = u->number * 10 * eff_skill(u, SK_TRADE, r);

	/* hat der Händler bereits gehandelt, muss die Menge der bereits
	* verkauften/gekauften Güter abgezogen werden */
	a = a_find(u->attribs, &at_trades);
	if (!a) {
		a = a_add(&u->attribs, a_new(&at_trades));
	} else {
		k -= a->data.i;
	}

	n = min(n, k);

	if (!n) {
		cmistake(u, ord, 102, MSG_COMMERCE);
		return;
	}

	assert(n>=0);
	/* die Menge der verkauften Güter merken */
	a->data.i += n;

	itype = finditemtype(getstrtoken(), u->faction->locale);
	if (itype!=NULL) {
		ltype = resource2luxury(itype->rtype);
		if (ltype==NULL) {
			cmistake(u, ord, 124, MSG_COMMERCE);
			return;
		}
	}
	if (r_demand(r, ltype)) {
		add_message(&u->faction->msgs,
			msg_feedback(u, ord, "luxury_notsold", ""));
		return;
	}
	o = (request *) calloc(1, sizeof(request));
	o->type.ltype = ltype; /* sollte immer gleich sein */

	o->unit = u;
	o->qty = n;
	addlist(buyorders, o);
}
/* ------------------------------------------------------------- */

/* Steuersätze in % bei Burggröße */
static int tax_per_size[7] =
{0, 6, 12, 18, 24, 30, 36};

static void
expandselling(region * r, request * sellorders, int limit)
{
	int money, price, j, max_products;
	/* int m, n = 0; */
	int maxsize = 0, maxeffsize = 0;
	int taxcollected = 0;
	int hafencollected = 0;
	unit *maxowner = (unit *) NULL;
	building *maxb = (building *) NULL;
	building *b;
	unit *u;
	unit *hafenowner;
	static int *counter;
	static int ncounter = 0;

	if (ncounter==0) {
		const luxury_type * ltype;
		for (ltype=luxurytypes;ltype;ltype=ltype->next) ++ncounter;
		counter=(int*)gc_add(calloc(sizeof(int), ncounter));
	} else {
		memset(counter, 0, sizeof(int)*ncounter);
	}

	if (!sellorders)	/* NEIN, denn Insekten können in	|| !r->buildings) */
		return;					/* Sümpfen und Wüsten auch so handeln */

	/* Stelle Eigentümer der größten Burg fest. Bekommt Steuern aus jedem
	* Verkauf. Wenn zwei Burgen gleicher Größe bekommt gar keiner etwas. */

	for (b = rbuildings(r); b; b = b->next) {
		if (b->size > maxsize && buildingowner(r, b) != NULL
			&& b->type == bt_find("castle")) {
				maxb = b;
				maxsize = b->size;
				maxowner = buildingowner(r, b);
			} else if (b->size == maxsize && b->type == bt_find("castle")) {
				maxb = (building *) NULL;
				maxowner = (unit *) NULL;
			}
	}

	hafenowner = owner_buildingtyp(r, bt_find("harbour"));

	if (maxb != (building *) NULL && maxowner != (unit *) NULL) {
		maxeffsize = buildingeffsize(maxb, false);
		if (maxeffsize == 0) {
			maxb = (building *) NULL;
			maxowner = (unit *) NULL;
		}
	}
	/* Die Region muss genug Geld haben, um die Produkte kaufen zu können. */

	money = rmoney(r);

	/* max_products sind 1/100 der Bevölkerung, falls soviele Produkte
	* verkauft werden - counter[] - sinkt die Nachfrage um 1 Punkt.
	* multiplier speichert r->demand für die debug message ab. */

	max_products = rpeasants(r) / TRADE_FRACTION;
	if (max_products <= 0) return;

	if (rterrain(r) == T_DESERT && buildingtype_exists(r, bt_find("caravan"))) {
		max_products = rpeasants(r) * 2 / TRADE_FRACTION;
	}
	/* Verkauf: so programmiert, dass er leicht auf mehrere Gueter pro
	* Runde erweitert werden kann. */

	expandorders(r, sellorders);
	if (!norders) return;

	for (j = 0; j != norders; j++) {
		static const luxury_type * search=NULL;
		const luxury_type * ltype = oa[j].type.ltype;
		int multi = r_demand(r, ltype);
		static int i=-1;
		int use = 0;
		if (search!=ltype) {
			i=0;
			for (search=luxurytypes;search!=ltype;search=search->next) ++i;
		}
    if (counter[i]>=limit) continue;
		if (counter[i]+1 > max_products && multi > 1) --multi;
		price = ltype->price * multi;

		if (money >= price) {
			int abgezogenhafen = 0;
			int abgezogensteuer = 0;
			unit * u = oa[j].unit;
			attrib * a = a_find(u->attribs, &at_luxuries);
			if (a==NULL) a = a_add(&u->attribs, a_new(&at_luxuries));
			i_change((item**)&a->data.v, ltype->itype, 1);
			++use;
			if (u->n < 0)
				u->n = 0;

			if (hafenowner != NULL) {
				if (hafenowner->faction != u->faction) {
					abgezogenhafen = price / 10;
					hafencollected += abgezogenhafen;
					price -= abgezogenhafen;
					money -= abgezogenhafen;
				}
			}
			if (maxb != NULL) {
				if (maxowner->faction != u->faction) {
					abgezogensteuer = price * tax_per_size[maxeffsize] / 100;
					taxcollected += abgezogensteuer;
					price -= abgezogensteuer;
					money -= abgezogensteuer;
				}
			}
			u->n += price;
			change_money(u, price);
      fset(u, UFL_LONGACTION);

			/* r->money -= price; --- dies wird eben nicht ausgeführt, denn die
			* Produkte können auch als Steuern eingetrieben werden. In der Region
			* wurden Silberstücke gegen Luxusgüter des selben Wertes eingetauscht!
			* Falls mehr als max_products Kunden ein Produkt gekauft haben, sinkt
			* die Nachfrage für das Produkt um 1. Der Zähler wird wieder auf 0
			* gesetzt. */

			if (++counter[i] > max_products) {
				int d = r_demand(r, ltype);
				if (d > 1)
					r_setdemand(r, ltype, d-1);
				counter[i] = 0;
			}
		}
		if (use>0) {
#ifdef NDEBUG
			use_pooled(oa[j].unit, ltype->itype->rtype, GET_DEFAULT, use);
#else
			/* int i = */ use_pooled(oa[j].unit, ltype->itype->rtype, GET_DEFAULT, use);
			/* assert(i==use); */
#endif
		}
	}
	free(oa);

	/* Steuern. Hier werden die Steuern dem Besitzer der größten Burg gegeben. */

	if (maxowner) {
		if (taxcollected > 0) {
			change_money(maxowner, (int) taxcollected);
			add_income(maxowner, IC_TRADETAX, taxcollected, taxcollected);
			/* TODO: Meldung
       * "%s verdient %d Silber durch den Handel in %s.",
       * unitname(maxowner), (int) taxcollected, regionname(r)); */
		}
	}
	if (hafenowner) {
		if (hafencollected > 0) {
			change_money(hafenowner, (int) hafencollected);
			add_income(hafenowner, IC_TRADETAX, hafencollected, hafencollected);
		}
	}
	/* Berichte an die Einheiten */

	for (u = r->units; u; u = u->next) {

		attrib * a = a_find(u->attribs, &at_luxuries);
		item * itm;
		if (a==NULL) continue;
		for (itm=(item*)a->data.v; itm; itm=itm->next) {
			if (itm->number) {
				ADDMSG(&u->faction->msgs, msg_message("sellamount", 
          "unit amount resource", u, itm->number, itm->type->rtype));
			}
		}
		a_remove(&u->attribs, a);
		add_income(u, IC_TRADE, u->n, u->n);
	}
}

static boolean
sell(unit * u, request ** sellorders, struct order * ord)
{
  boolean unlimited = true;
	const item_type * itype;
	const luxury_type * ltype=NULL;
	int n;
  region * r = u->region;
	const char *s;

	if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
		cmistake(u, ord, 69, MSG_INCOME);
		return false;
	}
	/* sellorders sind KEIN array, weil für alle items DIE SELBE resource
	* (das geld der region) aufgebraucht wird. */

  init_tokens(ord);
  skip_token();
	s = getstrtoken();

  if (findparam(s, u->faction->locale) == P_ANY) {
    unlimited = false;
		n = rpeasants(r) / TRADE_FRACTION;
		if (rterrain(r) == T_DESERT && buildingtype_exists(r, bt_find("caravan")))
			n *= 2;
		if (n==0) {
			cmistake(u, ord, 303, MSG_COMMERCE);
			return false;
		}
	} else {
		n = atoi(s);
		if (n==0) {
			cmistake(u, ord, 27, MSG_COMMERCE);
			return false;
		}
	}
	/* Belagerte Einheiten können nichts verkaufen. */

	if (besieged(u)) {
		add_message(&u->faction->msgs,
			msg_feedback(u, ord, "error60", ""));
		return false;
	}
	/* In der Region muß es eine Burg geben. */

	if (u->race == new_race[RC_INSECT]) {
		if (rterrain(r) != T_SWAMP && rterrain(r) != T_DESERT && !rbuildings(r)) {
			cmistake(u, ord, 119, MSG_COMMERCE);
			return false;
		}
	} else {
    /* ...oder in der Region muß es eine Burg geben. */
    building * b;
    static const struct building_type * bt_castle;
    if (!bt_castle) bt_castle = bt_find("castle");
    for (b=r->buildings;b;b=b->next) {
      if (b->type==bt_castle && b->size>=2) break;
    }
		if (b==NULL) {
			cmistake(u, ord, 119, MSG_COMMERCE);
			return false;
		}
	}

	/* Ein Händler kann nur 10 Güter pro Talentpunkt verkaufen. */

	n = min(n, u->number * 10 * eff_skill(u, SK_TRADE, r));

	if (!n) {
		cmistake(u, ord, 54, MSG_COMMERCE);
		return false;
	}
	s=getstrtoken();
	itype = finditemtype(s, u->faction->locale);
	if (itype!=NULL) ltype = resource2luxury(itype->rtype);
	if (ltype==NULL) {
		cmistake(u, ord, 126, MSG_COMMERCE);
		return false;
	}
	else {
		attrib * a;
    request *o;
		int k, available;

		if (!r_demand(r, ltype)) {
			cmistake(u, ord, 263, MSG_COMMERCE);
			return false;
		}
		available = get_pooled(u, itype->rtype, GET_DEFAULT);

		/* Wenn andere Einheiten das selbe verkaufen, muß ihr Zeug abgezogen
		* werden damit es nicht zweimal verkauft wird: */
		for (o=*sellorders;o;o=o->next) {
			if (o->type.ltype==ltype && o->unit->faction == u->faction) {
				int fpool = o->qty - get_pooled(o->unit, itype->rtype, GET_RESERVE);
				available -= max(0, fpool);
			}
		}

		n = min(n, available);

		if (n <= 0) {
			cmistake(u, ord, 264, MSG_COMMERCE);
			return false;
		}
		/* Hier wird request->type verwendet, weil die obere limit durch
		* das silber gegeben wird (region->money), welches für alle
		* (!) produkte als summe gilt, als nicht wie bei der
		* produktion, wo für jedes produkt einzeln eine obere limite
		* existiert, so dass man arrays von orders machen kann. */

		/* Ein Händler kann nur 10 Güter pro Talentpunkt handeln. */
		k = u->number * 10 * eff_skill(u, SK_TRADE, r);

		/* hat der Händler bereits gehandelt, muss die Menge der bereits
		* verkauften/gekauften Güter abgezogen werden */
		a = a_find(u->attribs, &at_trades);
		if (!a) {
			a = a_add(&u->attribs, a_new(&at_trades));
		} else {
			k -= a->data.i;
		}

		n = min(n, k);
		assert(n>=0);
		/* die Menge der verkauften Güter merken */
		a->data.i += n;
		o = (request *) calloc(1, sizeof(request));
		o->unit = u;
		o->qty = n;
		o->type.ltype = ltype;
		addlist(sellorders, o);

    return unlimited;
	}
}
/* ------------------------------------------------------------- */

static void
expandstealing(region * r, request * stealorders)
{
	int i;

	expandorders(r, stealorders);
	if (!norders) return;

	/* Für jede unit in der Region wird Geld geklaut, wenn sie Opfer eines
	* Beklauen-Orders ist. Jedes Opfer muß einzeln behandelt werden.
	*
	* u ist die beklaute unit. oa.unit ist die klauende unit.
	*/

	for (i = 0; i != norders && oa[i].unit->n <= oa[i].unit->wants; i++) {
		unit *u = findunitg(oa[i].no, r);
		int n = 0;
		if (u && u->region==r) n = get_pooled(u, r_silver, GET_ALL);
#ifndef GOBLINKILL
		if (oa[i].type.goblin) { /* Goblin-Spezialklau */
			int uct = 0;
			unit *u2;
			assert(effskill(oa[i].unit, SK_STEALTH)>=4 || !"this goblin\'s talent is too low");
			for (u2 = r->units; u2; u2 = u2->next)
				if (u2->faction == u->faction)
					uct += u2->number;
			n -= uct * 2 * MAINTENANCE;
		}
#endif
		if (n>10 && rplane(r) && (rplane(r)->flags & PFL_NOALLIANCES)) {
			/* In Questen nur reduziertes Klauen */
			n = 10;
		}
		if (n > 0) {
			n = min(n, oa[i].unit->wants);
			use_pooled(u, r_silver, GET_ALL, n);
			oa[i].unit->n = n;
			change_money(oa[i].unit, n);
			ADDMSG(&u->faction->msgs, msg_message("stealeffect", "unit region amount", u, u->region, n));
		}
		add_income(oa[i].unit, IC_STEAL, oa[i].unit->wants, oa[i].unit->n);
    fset(oa[i].unit, UFL_LONGACTION);
	}
	free(oa);
}

/* ------------------------------------------------------------- */
static void
plant(region *r, unit *u, int raw)
{
	int n, i, skill, planted = 0;
  const item_type * itype;
  static const resource_type * rt_water = NULL;
  if (rt_water==NULL) rt_water = rt_find("p2");

  assert(rt_water!=NULL);
	if (!fval(r->terrain, LAND_REGION)) {
		return;
	}
	if (rherbtype(r) == NULL) {
		cmistake(u, u->thisorder, 108, MSG_PRODUCE);
		return;
	}

	/* Skill prüfen */
	skill = eff_skill(u, SK_HERBALISM, r);
	itype = rherbtype(r);
	if (skill < 6) {
		add_message(&u->faction->msgs,
			msg_feedback(u, u->thisorder, "plant_skills",
			"skill minskill product", SK_HERBALISM, 6, itype->rtype, 1));
		return;
	}
	/* Wasser des Lebens prüfen */
	if (get_pooled(u, rt_water, GET_DEFAULT) == 0) {
		add_message(&u->faction->msgs,
			msg_feedback(u, u->thisorder, "resource_missing", "missing", rt_water));
		return;
	}
	n = get_pooled(u, itype->rtype, GET_DEFAULT);
	/* Kräuter prüfen */
	if (n==0) {
		add_message(&u->faction->msgs,
			msg_feedback(u, u->thisorder, "resource_missing", "missing",
			itype->rtype));
		return;
	}

	n = min(skill*u->number, n);
	n = min(raw, n);
	/* Für jedes Kraut Talent*10% Erfolgschance. */
	for(i = n; i>0; i--) {
		if (rand()%10 < skill) planted++;
	}
	produceexp(u, SK_HERBALISM, u->number);

	/* Alles ok. Abziehen. */
	use_pooled(u, rt_water, GET_DEFAULT, 1);
	use_pooled(u, itype->rtype, GET_DEFAULT, n);
	rsetherbs(r, rherbs(r)+planted);
	ADDMSG(&u->faction->msgs, msg_message("plant", "unit region amount herb", 
    u, r, planted, itype->rtype));
}

static void
planttrees(region *r, unit *u, int raw)
{
	int n, i, skill, planted = 0;
	const resource_type * rtype;

	if (!fval(r->terrain, LAND_REGION)) {
		return;
	}

	/* Mallornbäume kann man nur in Mallornregionen züchten */
	if (fval(r, RF_MALLORN)) {
		rtype = rt_mallornseed;
	} else {
		rtype = rt_seed;
	}

	/* Skill prüfen */
	skill = eff_skill(u, SK_HERBALISM, r);
	if (skill < 6) {
		add_message(&u->faction->msgs,
			msg_feedback(u, u->thisorder, "plant_skills",
			"skill minskill product", SK_HERBALISM, 6, rtype, 1));
		return;
	}
	if (fval(r, RF_MALLORN) && skill < 7 ) {
		add_message(&u->faction->msgs,
			msg_feedback(u, u->thisorder, "plant_skills",
			"skill minskill product", SK_HERBALISM, 7, rtype, 1));
		return;
	}

	n = get_pooled(u, rtype, GET_DEFAULT);
	/* Samen prüfen */
	if (n==0) {
		add_message(&u->faction->msgs,
			msg_feedback(u, u->thisorder, "resource_missing", "missing", rtype));
		return;
	}

	/* wenn eine Anzahl angegeben wurde, nur soviel verbrauchen */
	n = min(raw, n);
	n = min(skill*u->number, n);

	/* Für jeden Samen Talent*10% Erfolgschance. */
	for(i = n; i>0; i--) {
		if (rand()%10 < skill) planted++;
	}
	rsettrees(r, 0, rtrees(r, 0)+planted);

	/* Alles ok. Abziehen. */
	produceexp(u, SK_HERBALISM, u->number);
	use_pooled(u, rtype, GET_DEFAULT, n);

	ADDMSG(&u->faction->msgs, msg_message("plant",
		"unit region amount herb", u, r, planted, rtype));
}

/* züchte bäume */
static void
breedtrees(region *r, unit *u, int raw)
{
	int n, i, skill, planted = 0;
	const resource_type * rtype;
	static int current_season = -1;
  
  if (current_season<0) current_season = get_gamedate(turn, NULL)->season;

	/* Bäume züchten geht nur im Frühling */
	if (current_season != SEASON_SPRING){
		planttrees(r, u, raw);
		return;
	}

	if (!fval(r->terrain, LAND_REGION)) {
		return;
	}

	/* Mallornbäume kann man nur in Mallornregionen züchten */
	if (fval(r, RF_MALLORN)) {
		rtype = rt_mallornseed;
	} else {
		rtype = rt_seed;
	}

	/* Skill prüfen */
	skill = eff_skill(u, SK_HERBALISM, r);
	if (skill < 12) {
		planttrees(r, u, raw);
		return;
	}
	n = get_pooled(u, rtype, GET_DEFAULT);
	/* Samen prüfen */
	if (n==0) {
		add_message(&u->faction->msgs,
			msg_feedback(u, u->thisorder, "resource_missing", "missing", rtype));
		return;
	}

	/* wenn eine Anzahl angegeben wurde, nur soviel verbrauchen */
	n = min(raw, n);
	n = min(skill*u->number, n);

	/* Für jeden Samen Talent*5% Erfolgschance. */
	for(i = n; i>0; i--) {
		if (rand()%100 < skill*5) planted++;
	}
	rsettrees(r, 1, rtrees(r, 1)+planted);

	/* Alles ok. Abziehen. */
	produceexp(u, SK_HERBALISM, u->number);
	use_pooled(u, rtype, GET_DEFAULT, n);

	add_message(&u->faction->msgs, new_message(u->faction,
		"plant%u:unit%r:region%i:amount%X:herb", u, r, planted, rtype));
}

static void
plant_cmd(unit *u, struct order * ord)
{
  region * r = u->region;
	int m;
	const char *s;
	param_t p;
	const resource_type * rtype = NULL;

  if (r->land==NULL) {
    /* TODO: error message here */
    return;
  }

  /* pflanze [<anzahl>] <parameter> */
  init_tokens(ord);
  skip_token();
	s = getstrtoken();
	m = atoi(s);
	sprintf(buf, "%d", m);
	if (!strcmp(buf, s)) {
		/* first came a want-paramter */
		s = getstrtoken();
	} else {
		m = INT_MAX;
	}

	if (!s[0]) {
		p = P_ANY;
	} else {
		p = findparam(s, u->faction->locale);
		rtype = findresourcetype(s, u->faction->locale);
	}

	if (p==P_HERBS){
		plant(r, u, m);
		return;
	}
	else if (p==P_TREES){
		breedtrees(r, u, m);
		return;
	}
	else if (rtype!=NULL){
		if (rtype==rt_mallornseed || rtype==rt_seed) {
			breedtrees(r, u, m);
			return;
		}
	}
}


/* züchte pferde */
static void
breedhorses(region *r, unit *u)
{
	int n, c;
	int gezuechtet = 0;
	struct building * b = inside_building(u);
	const struct building_type * btype = b?b->type:NULL;

	if (btype!=bt_find("stables")) {
		cmistake(u, u->thisorder, 122, MSG_PRODUCE);
		return;
	}
	if (get_item(u, I_HORSE) < 2) {
		cmistake(u, u->thisorder, 107, MSG_PRODUCE);
		return;
	}
	n = min(u->number * eff_skill(u, SK_HORSE_TRAINING, r), get_item(u, I_HORSE));

	for (c = 0; c < n; c++) {
		if (rand() % 100 < eff_skill(u, SK_HORSE_TRAINING, r)) {
			i_change(&u->items, olditemtype[I_HORSE], 1);
			gezuechtet++;
		}
	}

	if (gezuechtet > 0) {
		produceexp(u, SK_HORSE_TRAINING, u->number);
	}

	add_message(&u->faction->msgs, new_message(u->faction,
		"raised%u:unit%i:amount", u, gezuechtet));
}

static void
breed_cmd(unit *u, struct order * ord)
{
	int m;
	const char *s;
	param_t p;
  region *r = u->region;

	/* züchte [<anzahl>] <parameter> */
  init_tokens(ord);
  skip_token();
	s = getstrtoken();

  m = atoi(s);
	sprintf(buf, "%d", m);
	if (!strcmp(buf, s)) {
		/* first came a want-paramter */
		s = getstrtoken();
	} else {
		m = INT_MAX;
	}

	if(!s[0]){
		p = P_ANY;
	} else {
		p = findparam(s, u->faction->locale);
	}

	switch (p) {
		case P_HERBS:
			plant(r, u, m);
			return;
		case P_TREES:
			breedtrees(r, u, m);
			return;
		default:
			breedhorses(r, u);
			return;
	}
}

static const char *
rough_amount(int a, int m)

{
	int p = (a * 100)/m;

	if (p < 10) {
		return "sehr wenige";
	} else if (p < 30) {
		return "wenige";
	} else if (p < 60) {
		return "relativ viele";
	} else if (p < 90) {
		return "viele";
	}
	return "sehr viele";
}

static void
research_cmd(unit *u, struct order * ord)
{
  region *r = u->region;

  init_tokens(ord);
  skip_token();
  /*
  const char *s = getstrtoken();

  if (findparam(s, u->faction->locale) == P_HERBS) { */

  if (eff_skill(u, SK_HERBALISM, r) < 7) {
    cmistake(u, ord, 227, MSG_EVENT);
    return;
  }

  produceexp(u, SK_HERBALISM, u->number);

  if (rherbs(r) > 0) {
    const item_type *itype = rherbtype(r);

    if (itype != NULL) {
      add_message(&u->faction->msgs, new_message(u->faction,
        "researchherb%u:unit%r:region%s:amount%X:herb", u, r,
        rough_amount(rherbs(r), 100), itype->rtype));
    } else {
      add_message(&u->faction->msgs, new_message(u->faction,
        "researchherb_none%u:unit%r:region", u, r));
    }
  } else {
    add_message(&u->faction->msgs, new_message(u->faction,
      "researchherb_none%u:unit%r:region", u, r));
  }
}

static int
wahrnehmung(region * r, faction * f)
{
	unit *u;
	int w = 0;

	for (u = r->units; u; u = u->next) {
		if (u->faction == f) {
				if (eff_skill(u, SK_OBSERVATION, r) > w) {
					w = eff_skill(u, SK_OBSERVATION, r);
				}
			}
	}

	return w;
}

static void
steal_cmd(unit * u, struct order * ord, request ** stealorders)
{
	int n, i, id;
	boolean goblin = false;
	request * o;
	unit * u2 = NULL;
  region * r = u->region;
	faction * f = NULL;

  if (fval(r->terrain, SEA_REGION) && u->race != new_race[RC_AQUARIAN]) {
    cmistake(u, ord, 242, MSG_INCOME);
    return;
  }

	if (r->planep && fval(r->planep, PFL_NOATTACK)) {
		cmistake(u, ord, 270, MSG_INCOME);
		return;
	}

  init_tokens(ord);
  skip_token();
  id = read_unitid(u->faction, r);
	u2 = findunitr(r, id);

	if (u2 && u2->region==u->region) {
		f = u2->faction;
	} else {
		f = dfindhash(id);
	}

	for (u2=r->units;u2;u2=u2->next) {
		if (u2->faction == f && cansee(u->faction, r, u2, 0)) break;
	}

	if (!u2) {
		cmistake(u, ord, 63, MSG_INCOME);
		return;
	}

	if (u2->faction->age < NewbieImmunity()) {
		add_message(&u->faction->msgs,
			msg_feedback(u, ord, "newbie_immunity_error",
			"turns", NewbieImmunity()));
		return;
	}

	if (u->faction->alliance!=NULL && u->faction->alliance == u2->faction->alliance) {
		cmistake(u, ord, 47, MSG_INCOME);
		return;
	}

	assert(u->region==u2->region);
	if (!can_contact(r, u, u2)) {
		add_message(&u->faction->msgs, msg_feedback(u, ord, "error60", ""));
		return;
	}

	n = eff_skill(u, SK_STEALTH, r) - wahrnehmung(r, f);

	if (n == 0) {
		/* Wahrnehmung == Tarnung */
		if (u->race != new_race[RC_GOBLIN] || eff_skill(u, SK_STEALTH, r) <= 3) {
			add_message(&u->faction->msgs, new_message(u->faction,
				"stealfail%u:unit%u:target", u, u2));
			add_message(&u2->faction->msgs, new_message(u2->faction,
				"stealdetect%u:unit", u2));
			return;
		} else {
			add_message(&u2->faction->msgs, new_message(u2->faction,
				"thiefdiscover%u:unit%u:target", u, u2));
			add_message(&u->faction->msgs, new_message(u->faction,
				"stealfatal%u:unit%u:target", u, u2));
			n = 1;
			goblin = true;
		}
	} else if (n < 0) {
		/* Wahrnehmung > Tarnung */
		if (u->race != new_race[RC_GOBLIN] || eff_skill(u, SK_STEALTH, r) <= 3) {
			add_message(&u->faction->msgs, new_message(u->faction,
				"stealfatal%u:unit%u:target", u, u2));

			add_message(&u2->faction->msgs, new_message(u2->faction,
				"thiefdiscover%u:unit%u:target", u, u2));
			return;
		} else {	/* Goblin-Spezialdiebstahl, Meldung an Beklauten */
			add_message(&u2->faction->msgs, new_message(u2->faction,
				"thiefdiscover%u:unit%u:target", u, u2));
			n = 1;
			goblin = true;
		}
	}
	n = max(0, n);

	i = min(u->number, get_item(u,I_RING_OF_NIMBLEFINGER));
	if (i > 0) {
		n *= STEALINCOME * (u->number + i * 9);
	} else {
		n *= u->number * STEALINCOME;
	}

	u->wants = n;

	/* wer dank unsichtbarkeitsringen klauen kann, muss nicht unbedingt ein
	* guter dieb sein, schliesslich macht man immer noch sehr viel laerm */

	o = (request *) calloc(1, sizeof(request));
	o->unit = u;
	o->qty = 1; /* Betrag steht in u->wants */
	o->no = u2->no;
	o->type.goblin = goblin; /* Merken, wenn Goblin-Spezialklau */
	addlist(stealorders, o);

	/* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */

	produceexp(u, SK_STEALTH, min(n, u->number));
}
/* ------------------------------------------------------------- */


static void
expandentertainment(region * r)
{
	unit *u;
	int m = entertainmoney(r);
	request *o;

	for (o = &entertainers[0]; o != nextentertainer; ++o) {
		double part = m / (double) entertaining;
		u = o->unit;
		if (entertaining <= m)
			u->n = o->qty;
		else
			u->n = (int) (o->qty * part);
		change_money(u, u->n);
		rsetmoney(r, rmoney(r) - u->n);
		m -= u->n;
		entertaining -= o->qty;

		/* Nur soviel PRODUCEEXP wie auch tatsächlich gemacht wurde */
		produceexp(u, SK_ENTERTAINMENT, min(u->n, u->number));
		add_income(u, IC_ENTERTAIN, o->qty, u->n);
    fset(u, UFL_LONGACTION);
	}
}

void
entertain_cmd(unit * u, struct order * ord)
{
  region * r = u->region;
  int max_e;
  request *o;
  static int entertainbase = 0;
  static int entertainperlevel = 0;

  if (!entertainbase) {
    const char * str = get_param(global.parameters, "entertain.base");
    entertainbase = str?atoi(str):0;
  }
  if (!entertainperlevel) {
    const char * str = get_param(global.parameters, "entertain.perlevel");
    entertainperlevel = str?atoi(str):0;
  }
  if (fval(u, UFL_WERE)) {
    cmistake(u, ord, 58, MSG_INCOME);
    return;
  }
  if (!effskill(u, SK_ENTERTAINMENT)) {
    cmistake(u, ord, 58, MSG_INCOME);
    return;
  }
  if (besieged(u)) {
    cmistake(u, ord, 60, MSG_INCOME);
    return;
  }
  if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
    cmistake(u, ord, 69, MSG_INCOME);
    return;
  }
  if (is_cursed(r->attribs, C_DEPRESSION, 0)) {
    cmistake(u, ord, 28, MSG_INCOME);
    return;
  }

  u->wants = u->number * (entertainbase + effskill(u, SK_ENTERTAINMENT)
                            * entertainperlevel);

  init_tokens(ord);
  skip_token();
  max_e = geti();
  if (max_e != 0) {
    u->wants = min(u->wants,max_e);
  }
  o = nextentertainer++;
  o->unit = u;
  o->qty = u->wants;
  entertaining += o->qty;
}

/* ------------------------------------------------------------- */


static void
expandwork(region * r)
{
	int n, earnings;
	/* n: verbleibende Einnahmen */
	/* m: maximale Arbeiter */
	int m = maxworkingpeasants(r);
	int p_wage = wage(r, NULL, NULL);
	int verdienst = 0;
	request *o;

	for (o = &workers[0]; o != nextworker; ++o) {
		unit * u = o->unit;
		int workers;

		if (u->number == 0) continue;

		if (m>=working) workers = u->number;
		else {
			workers = u->number * m / working;
			if (rand() % working < (u->number * m) % working) workers++;
		}

		assert(workers>=0);

		u->n = workers * wage(u->region, u->faction, u->race);

		m -= workers;
		assert(m>=0);

		change_money(u, u->n);
		working -= o->unit->number;
		add_income(u, IC_WORK, o->qty, u->n);
    fset(u, UFL_LONGACTION);
  }

	n = m * p_wage;

	/* Der Rest wird von den Bauern verdient. n ist das uebriggebliebene
	* Geld. */
#if AFFECT_SALARY
	{
		attrib * a = a_find(r->attribs, &at_salary);
		if (a) verdienst = a->data.i;
	}
#endif
  
	earnings = min(n, rpeasants(r) * p_wage) + verdienst;
	/* Mehr oder weniger durch Trank "Riesengrass" oder "Faulobstschnaps" */

	if (verdienst) {
		sprintf(buf, "%d Bauern verdienen ein Silber %s durch %s.",
			abs(verdienst), (verdienst > 0) ? "mehr" : "weniger",
			(verdienst > 0) ? "Riesengras" : "Faulobstschnaps");
	}
	rsetmoney(r, rmoney(r) + earnings);
}

static void
work(unit * u, order * ord)
{
  region * r = u->region;
	request *o;
	int w;

	if(fval(u, UFL_WERE)) {
		cmistake(u, ord, 313, MSG_INCOME);
		return;
	}
	if (besieged(u)) {
		cmistake(u, ord, 60, MSG_INCOME);
		return;
	}
	if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
		cmistake(u, ord, 69, MSG_INCOME);
		return;
	}
	w = wage(r, u->faction, u->race);
	u->wants = u->number * w;
	o = nextworker++;
	o->unit = u;
	o->qty = u->number * w;
	working += u->number;
}
/* ------------------------------------------------------------- */

static void
expandtax(region * r, request * taxorders)
{
	unit *u;
	int i;

	expandorders(r, taxorders);
	if (!norders) return;

	for (i = 0; i != norders && rmoney(r) > TAXFRACTION; i++) {
		change_money(oa[i].unit, TAXFRACTION);
		oa[i].unit->n += TAXFRACTION;
		rsetmoney(r, rmoney(r) -TAXFRACTION);
	}
	free(oa);

  for (u = r->units; u; u = u->next) {
    if (u->n >= 0) {
      add_income(u, IC_TAX, u->wants, u->n);
      fset(u, UFL_LONGACTION);
    }
	}
}

void
tax_cmd(unit * u, struct order * ord, request ** taxorders)
{
	/* Steuern werden noch vor der Forschung eingetrieben */
  region * r = u->region;
	unit *u2;
	int n;
	request *o;
	int max;

	if (!humanoidrace(u->race) && u->faction != findfaction(MONSTER_FACTION)) {
		cmistake(u, ord, 228, MSG_INCOME);
		return;
	}

	if (fval(u, UFL_WERE)) {
		cmistake(u, ord, 228, MSG_INCOME);
		return;
	}

	if (besieged(u)) {
		cmistake(u, ord, 60, MSG_INCOME);
		return;
	}
	n = armedmen(u);

	if (!n) {
		cmistake(u, ord, 48, MSG_INCOME);
		return;
	}

  init_tokens(ord);
  skip_token();
  max = geti();

  if (max == 0) max = INT_MAX;
	if (!playerrace(u->race)) {
		u->wants = min(income(u), max);
	} else {
		u->wants = min(n * eff_skill(u, SK_TAXING, r) * 20, max);
	}

	u2 = is_guarded(r, u, GUARD_TAX);
	if (u2) {
		add_message(&u->faction->msgs,
			msg_feedback(u, ord, "region_guarded", "guard", u2));
		return;
	}

	/* die einnahmen werden in fraktionen von 10 silber eingeteilt: diese
	* fraktionen werden dann bei eintreiben unter allen eintreibenden
	* einheiten aufgeteilt. */

	o = (request *) calloc(1, sizeof(request));
	o->qty = u->wants / TAXFRACTION;
	o->unit = u;
	addlist(taxorders, o);
	return;
}
/* ------------------------------------------------------------- */

void
produce(void)
{
  region *r;
  request *taxorders, *sellorders, *stealorders, *buyorders;
  unit *u;
  int todo;

  /* das sind alles befehle, die 30 tage brauchen, und die in thisorder
  * stehen! von allen 30-tage befehlen wird einfach der letzte verwendet
  * (dosetdefaults).
  *
  * kaufen vor einnahmequellen. da man in einer region dasselbe produkt
  * nicht kaufen und verkaufen kann, ist die reihenfolge wegen der
  * produkte egal. nicht so wegen dem geld.
  *
  * lehren vor lernen. */

  for (r = regions; r; r = r->next) {
    boolean limited = true;

    assert(rmoney(r) >= 0);
    assert(rpeasants(r) >= 0);

    buyorders = 0;
    sellorders = 0;
    nextworker = &workers[0];
    working = 0;
    nextentertainer = &entertainers[0];
    entertaining = 0;
    taxorders = 0;
    stealorders = 0;

    for (u = r->units; u; u = u->next) {
      if (u->race == new_race[RC_SPELL] || fval(u, UFL_LONGACTION))
        continue;

      if (u->race == new_race[RC_INSECT] && r_insectstalled(r) &&
        !is_cursed(u->attribs, C_KAELTESCHUTZ,0))
        continue;

      if (attacked(u) && u->thisorder==NULL) {
        cmistake(u, u->thisorder, 52, MSG_PRODUCE);
        continue;
      }

      if (!TradeDisabled()) {
        order * ord;
        boolean trader = false;
        for (ord = u->orders;ord;ord=ord->next) {
          switch (get_keyword(ord)) {
          case K_BUY:
            buy(u, &buyorders, ord);
            trader = true;
            break;
          case K_SELL:
            /* sell returns true if the sale is not limited
             * by the region limit */
            limited &= !sell(u, &sellorders, ord);
            trader = true;
            break;
          }
        }
        if (trader) {
          attrib * a = a_find(u->attribs, &at_trades);
          if (a && a->data.i) {
            produceexp(u, SK_TRADE, u->number);
          }
          fset(u, UFL_LONGACTION);
          continue;
        }
      }

      todo = get_keyword(u->thisorder);
      if (todo == NOKEYWORD) continue;

      if (fval(r->terrain, SEA_REGION) && u->race != new_race[RC_AQUARIAN]
      && !(u->race->flags & RCF_SWIM)
        && todo != K_STEAL && todo != K_SPY && todo != K_SABOTAGE)
        continue;

      switch (todo) {
        case K_MAKE:
          make_cmd(u, u->thisorder);
          break;

        case K_ENTERTAIN:
          entertain_cmd(u, u->thisorder);
          break;

        case K_WORK:
          if (playerrace(u->race)) work(u, u->thisorder);
          else if (playerrace(u->faction->race)) {
            add_message(&u->faction->msgs,
              msg_feedback(u, u->thisorder, "race_cantwork", "race", u->race));
          }
          break;

        case K_TAX:
          tax_cmd(u, u->thisorder, &taxorders);
          break;

        case K_STEAL:
          steal_cmd(u, u->thisorder, &stealorders);
          break;

        case K_SPY:
          spy_cmd(u, u->thisorder);
          break;

        case K_SABOTAGE:
          sabotage_cmd(u, u->thisorder);
          break;

        case K_BREED:
          breed_cmd(u, u->thisorder);
          break;

        case K_PLANT:
          plant_cmd(u, u->thisorder);
          break;

        case K_RESEARCH:
          research_cmd(u, u->thisorder);
          break;
      }
    }

    split_allocations(r);
    /* Entertainment (expandentertainment) und Besteuerung (expandtax) vor den
    * Befehlen, die den Bauern mehr Geld geben, damit man aus den Zahlen der
    * letzten Runde berechnen kann, wieviel die Bauern für Unterhaltung
    * auszugeben bereit sind. */
    if (entertaining) expandentertainment(r);
    expandwork(r);
    if (taxorders) expandtax(r, taxorders);

    /* An erster Stelle Kaufen (expandbuying), die Bauern so Geld bekommen, um
    * nachher zu beim Verkaufen (expandselling) den Spielern abkaufen zu
    * können. */

    if (buyorders) expandbuying(r, buyorders);

    if (sellorders) {
      int limit = rpeasants(r) / TRADE_FRACTION;
      if (rterrain(r) == T_DESERT && buildingtype_exists(r, bt_find("caravan")))
        limit *= 2;
      expandselling(r, sellorders, limited?limit:INT_MAX);
    }

    /* Die Spieler sollen alles Geld verdienen, bevor sie beklaut werden
    * (expandstealing). */

    if (stealorders) expandstealing(r, stealorders);

    assert(rmoney(r) >= 0);
    assert(rpeasants(r) >= 0);

  }
}

void
init_economy(void)
{
  add_allocator(make_allocator(item2resource(olditemtype[I_HORSE]), attrib_allocation));
  add_allocator(make_allocator(item2resource(olditemtype[I_WOOD]), attrib_allocation));
  add_allocator(make_allocator(item2resource(olditemtype[I_MALLORN]), attrib_allocation));
  add_allocator(make_allocator(item2resource(olditemtype[I_STONE]), leveled_allocation));
  add_allocator(make_allocator(item2resource(olditemtype[I_IRON]), leveled_allocation));
  add_allocator(make_allocator(item2resource(olditemtype[I_LAEN]), leveled_allocation));
  add_allocator(make_allocator(rt_seed, attrib_allocation));
  add_allocator(make_allocator(rt_mallornseed, attrib_allocation));
}
