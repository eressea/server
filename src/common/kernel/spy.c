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
#include "spy.h"

/* kernel includes */
#include <kernel/build.h>
#include <kernel/reports.h>
#include <kernel/item.h>
#include <kernel/karma.h>
#include <kernel/faction.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/movement.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

/* attributes includes */
#include <attributes/racename.h>

/* util includes */
#include <util/vset.h>
#include <util/rand.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <attributes/otherfaction.h>

/* in spy steht der Unterschied zwischen Wahrnehmung des Opfers und
 * Spionage des Spions */
void
spy_message(int spy, const unit *u, const unit *target)
{
	const char *c;

	/* Infos:
	 * - Kampfstatus
	 * - verborgene Gegenstände: Amulette, Ringe, Phiolen, Geld
	 * - Partei
	 * - Talentinfo
	 * - Zaubersprüche
	 * - Zauberwirkungen
	 */
	/* mit spy=100 (magische Spionage) soll alles herausgefunden werden */

	buf[0]='\0';
	if (spy > 99){
		/* magische Spionage */
		/* Zauberwirkungen */
	}
	if (spy > 20){
    sc_mage * m = get_mage(target);
		/* bei Magiern Zaubersprüche und Magiegebiet */
		if (m) {
			spell_list *slist = m->spells;
			boolean first = true;

			scat("Magiegebiet: ");
			scat(LOC(u->faction->locale, magietypen[find_magetype(target)]));
			scat(", Sprüche: ");

			for (;slist; slist=slist->next) {
				spell * sp = slist->data;
				if (first) {
					first = false;
				} else {
					scat(", ");
				}
				scat(spell_name(sp, u->faction->locale));
			}
			if (first) scat("Keine");
			scat(". ");
		}
	}
	if (spy > 6){
		/* wahre Partei */
		scat("Partei '");
		scat(factionname(target->faction));
		scat("'. ");
	} else {
		/* ist die Einheit in Spionage nicht gut genug, glaubt sie die
		 * Parteitarnung */
		faction *fv = visible_faction(u->faction,target);

		if (fv != target->faction){
			scat("Partei '");
			scat(factionname(fv));
			scat("'. ");
		} else if (!fval(target, UFL_PARTEITARNUNG)){
			scat("Partei '");
			scat(factionname(target->faction));
			scat("'. ");
		}
	}
	if (spy > 0){
    int first = 1;
    int found = 0;
    skill * sv;

    scat("Talente: ");
    for (sv = target->skills;sv!=target->skills+target->skill_size;++sv) {
      if (sv->level>0) {
        found++;
        if (first == 1) {
          first = 0;
        } else {
          scat(", ");
        }
        scat(skillname(sv->id, u->faction->locale));
        scat(" ");
        icat(eff_skill(target, sv->id, target->region));
      }

			if (found == 0) {
				scat("Keine");
			}
			scat(". ");
		}

		scat("Im Gepäck sind");
		{
			boolean first = true;
			int found = 0;
			item * itm;
			for (itm=target->items;itm;itm=itm->next) {
				if (itm->number>0) {
					resource_type * rtype = itm->type->rtype;
					++found;
					if (first) {
						first = false;
						scat(": ");
					} else {
						scat(", ");
					}

					if (itm->number == 1) {
						scat("1 ");
						scat(locale_string(u->faction->locale, resourcename(rtype, 0)));
					} else {
						icat(itm->number);
						scat(" ");
						scat(locale_string(u->faction->locale, resourcename(rtype, NMF_PLURAL)));
					}
				}
			}
			if (found == 0) {
				scat(" keine verborgenen Gegenstände");
			}
			scat(". ");
		}
	}
	/* spion ist gleich gut wie Wahrnehmung Opfer */
	/* spion ist schlechter als Wahrnehmung Opfer */
	{ /* immer */
		scat("Kampfstatus: ");
		scat(report_kampfstatus(target, u->faction->locale));
		c = locale_string(u->faction->locale, hp_status(target));
		if (c && strlen(c))
			sprintf(buf, "%s (%s)", buf, c);
		scat(".");
	}

	ADDMSG(&u->faction->msgs, msg_message("spyreport",
		"spy target report", u, target, strdup(buf)));
}


int
spy_cmd(unit * u, struct order * ord)
{
  unit *target;
  int spy, observe;
  double spychance, observechance;
  region * r = u->region;

  init_tokens(ord);
  skip_token();
  target = getunit(r, u->faction);

  if (!target) {
    cmistake(u, u->thisorder, 63, MSG_EVENT);
    return 0;
  }
  if (!can_contact(r, u, target)) {
    cmistake(u, u->thisorder, 24, MSG_EVENT);
    return 0;
  }
  if (eff_skill(u, SK_SPY, r) < 1) {
    cmistake(u, u->thisorder, 39, MSG_EVENT);
    return 0;
  }
  /* Die Grundchance für einen erfolgreichen Spionage-Versuch ist 10%.
  * Für jeden Talentpunkt, den das Spionagetalent das Tarnungstalent
  * des Opfers übersteigt, erhöht sich dieses um 5%*/
  spy = eff_skill(u, SK_SPY, r) - eff_skill(target, SK_STEALTH, r);
  spychance = 0.1 + max(spy*0.05, 0.0);

  if (chance(spychance)) {
    produceexp(u, SK_SPY, u->number);
    spy_message(spy, u, target);
  } else {
    ADDMSG(&u->faction->msgs, msg_message("spyfail", "spy target", u, target));
  }

  /* der Spion kann identifiziert werden, wenn das Opfer bessere
  * Wahrnehmung als das Ziel Tarnung + Spionage/2 hat */
  observe = eff_skill(target, SK_OBSERVATION, r)
    - (effskill(u, SK_STEALTH) + eff_skill(u, SK_SPY, r)/2);

  if (invisible(u, target) >= u->number) {
    observe = min(observe, 0);
  }

  /* Anschließend wird - unabhängig vom Erfolg - gewürfelt, ob der
  * Spionageversuch bemerkt wurde. Die Wahrscheinlich dafür ist (100 -
  * SpionageSpion*5 + WahrnehmungOpfer*2)%. */
  observechance = 1.0 - (eff_skill(u, SK_SPY, r) * 0.05)
    + (eff_skill(target, SK_OBSERVATION, r) * 0.02);

  if (chance(observechance)) {
    ADDMSG(&target->faction->msgs, msg_message("spydetect", 
      "spy target", observe>0?u:NULL, target));
  }
  return 0;
}

int
setwere_cmd(unit *u, struct order * ord)
{
#ifdef KARMA_MODULE
  int level = fspecial(u->faction, FS_LYCANTROPE);
  const char *s;

  if (!level) {
    cmistake(u, ord, 311, MSG_EVENT);
    return 0;
  }

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (s == NULL || *s == '\0') {
    if(fval(u, UFL_WERE)) {
      cmistake(u, ord, 309, MSG_EVENT);
    } else if(rng_int()%100 < 35+(level-1)*20) { /* 35, 55, 75, 95% */
      fset(u, UFL_WERE);
    } else {
      cmistake(u, ord, 311, MSG_EVENT);
    }
  } else if (findparam(s, u->faction->locale) == P_NOT) {
    if(fval(u, UFL_WERE)) {
      cmistake(u, ord, 310, MSG_EVENT);
    } else if(rng_int()%100 < 90-level*20) {	/* 70, 50, 30, 10% */
      freset(u, UFL_WERE);
    } else {
      cmistake(u, ord, 311, MSG_EVENT);
    }

  }
#endif /* KARMA_MODULE */
  return 0;
}

int
setstealth_cmd(unit * u, struct order * ord)
{
  const char *s;
  char level;
  const race * trace;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  /* Tarne ohne Parameter: Setzt maximale Tarnung */

  if (s == NULL || *s == 0) {
    u_seteffstealth(u, -1);
    return 0;
  }

  trace = findrace(s, u->faction->locale);
  if (trace) {
    /* Dämonen können sich nur als andere Spielerrassen tarnen */
    if (u->race == new_race[RC_DAEMON]) {
      race_t allowed[] = { RC_DWARF, RC_ELF, RC_ORC, RC_GOBLIN, RC_HUMAN, 
        RC_TROLL, RC_DAEMON, RC_INSECT, RC_HALFLING, RC_CAT, RC_AQUARIAN,
        RC_URUK, NORACE };
      int i;
      for (i=0;allowed[i]!=NORACE;++i) if (new_race[allowed[i]]==trace) break;
      if (new_race[allowed[i]]==trace) {
        u->irace = trace;
        if (u->race->flags & RCF_SHAPESHIFTANY && get_racename(u->attribs))
          set_racename(&u->attribs, NULL);
      }
      return 0;
    }

    /* Singdrachen können sich nur als Drachen tarnen */
    if (u->race == new_race[RC_SONGDRAGON] || u->race == new_race[RC_BIRTHDAYDRAGON]) {
      if (trace==new_race[RC_SONGDRAGON]||trace==new_race[RC_FIREDRAGON]||trace==new_race[RC_DRAGON]||trace==new_race[RC_WYRM]) {
        u->irace = trace;
        if (u->race->flags & RCF_SHAPESHIFTANY && get_racename(u->attribs))
          set_racename(&u->attribs, NULL);
      }
      return 0;
    }

    /* Dämomen und Illusionsparteien können sich als andere race tarnen */
    if (u->race->flags & RCF_SHAPESHIFT) {
      if (playerrace(trace)) {
        u->irace = trace;
        if ((u->race->flags & RCF_SHAPESHIFTANY) && get_racename(u->attribs))
          set_racename(&u->attribs, NULL);
      }
    }
    return 0;
  }

  switch(findparam(s, u->faction->locale)) {
  case P_FACTION:
    /* TARNE PARTEI [NICHT|NUMMER abcd] */
    s = getstrtoken();
    if(!s || *s == 0) {
      fset(u, UFL_PARTEITARNUNG);
    } else if (findparam(s, u->faction->locale) == P_NOT) {
      freset(u, UFL_PARTEITARNUNG);
    } else if (findkeyword(s, u->faction->locale) == K_NUMBER) {
      const char *s2 = getstrtoken();
      int nr = -1;

      if(s2) nr = atoi36(s2);
      if(!s2 || *s2 == 0 || nr == u->faction->no) {
        a_removeall(&u->attribs, &at_otherfaction);
      } else {
        struct faction * f = findfaction(nr);
        if(f==NULL) {
          cmistake(u, ord, 66, MSG_EVENT);
        } else {
          region * lastr = NULL;
          /* for all units mu of our faction, check all the units in the region
           * they are in, if their visible faction is f, it's ok. use lastr to
           * avoid testing the same region twice in a row. */
          unit * mu = u->faction->units;
          while (mu!=NULL) {
            unit * ru = mu->region->units;
            if (mu->region!=lastr) {
              lastr = mu->region;
              while (ru!=NULL) {
                faction * fv = visible_faction(f, ru);
                if (fv==f) {
                  if (cansee(f, lastr, ru, 0)) break;
                }
                ru = ru->next;
              }
              if (ru!=NULL) break;
            }
            mu = mu->nextF;
          }
          if (mu!=NULL) {
            attrib * a = a_find(u->attribs, &at_otherfaction);
            if (!a) a = a_add(&u->attribs, make_otherfaction(f));
            else a->data.v = f;
          }
        }
      }
    } else {
      cmistake(u, ord, 289, MSG_EVENT);
    }
    break;
  case P_ANY:
    /* TARNE ALLES (was nicht so alles geht?) */
    u_seteffstealth(u, -1);
    break;
#ifdef KARMA_MODULE
  case P_NUMBER:
    /* TARNE ANZAHL [NICHT] */
    if (!fspecial(u->faction, FS_HIDDEN)) {
      cmistake(u, ord, 277, MSG_EVENT);
      return 0;
    }
    s = getstrtoken();
    if (findparam(s, u->faction->locale) == P_NOT) {
      attrib * a = a_find(u->attribs, &at_fshidden);
      if (a==NULL) a->data.ca[0] = 0;
      if (a->data.i == 0) a_remove(&u->attribs, a);
    } else {
      attrib * a = a_find(u->attribs, &at_fshidden);
      if (a!=NULL) a = a_add(&u->attribs, a_new(&at_fshidden));
      a->data.ca[0] = 1;
    }
    break;
#endif /* KARMA_MODULE */
#ifdef KARMA_MODULE
  case P_ITEMS:
    /* TARNE GEGENSTÄNDE [NICHT] */
    if(!fspecial(u->faction, FS_HIDDEN)) {
      cmistake(u, ord, 277, MSG_EVENT);
      return 0;
    }
    if (findparam(s, u->faction->locale) == P_NOT) {
      attrib * a = a_find(u->attribs, &at_fshidden);
      if (a!=NULL) a->data.ca[1] = 0;
      if (a->data.i == 0) a_remove(&u->attribs, a);
    } else {
      attrib * a = a_find(u->attribs, &at_fshidden);
      if (a==NULL) a = a_add(&u->attribs, a_new(&at_fshidden));
      a->data.ca[1] = 1;
    }
    break;
#endif /* KARMA_MODULE */
  case P_NOT:
    u_seteffstealth(u, -1);
    break;
  default:
    if (isdigit(s[0])) {
      /* Tarnungslevel setzen */
      level = (char) atoip(s);
      if (level > effskill(u, SK_STEALTH)) {
        sprintf(buf, "%s kann sich nicht so gut tarnen.", unitname(u));
        mistake(u, ord, buf, MSG_EVENT);
        return 0;
      }
      u_seteffstealth(u, level);
    } else if (u->race->flags & RCF_SHAPESHIFTANY) {
      set_racename(&u->attribs, s);
    }
  }
  return 0;
}

static int
faction_skill(region * r, faction * f, skill_t sk)
{
	int value = 0;
	unit *u;

	list_foreach(unit, r->units, u)
		if (u->faction == f)
		{
			int s = eff_skill(u, sk, r);
			value = max(value, s);
		}
	list_next(u);
	return value;
}

static int
crew_skill(region * r, faction * f, ship * sh, skill_t sk)
{
	int value = 0;
	unit *u;

	for (u=r->units;u;u=u->next) {
		if (u->ship == sh && u->faction == f) {
			int s = eff_skill(u, sk, r);
			value = max(s, value);
		}
	}
	return value;
}

static int
try_destruction(unit * u, unit * u2, const char *name, int skilldiff)
{
	const char *destruction_success_msg = "%s wurde von %s zerstoert.";
	const char *destruction_failed_msg = "%s konnte %s nicht zerstoeren.";
	const char *destruction_detected_msg = "%s wurde beim Versuch, %s zu zerstoeren, entdeckt.";
	const char *detect_failure_msg = "Es wurde versucht, %s zu zerstoeren.";
	const char *object_destroyed_msg = "%s wurde zerstoert.";

	if (skilldiff == 0) {
		/* tell the unit that the attempt failed: */
		sprintf(buf, destruction_failed_msg, unitname(u), name);
		addmessage(0, u->faction, buf, MSG_EVENT, ML_WARN);
		/* tell the enemy about the attempt: */
		sprintf(buf, detect_failure_msg, name);
		addmessage(0, u2->faction, buf, MSG_EVENT, ML_IMPORTANT);
		return 0;
	}
	if (skilldiff < 0) {
		/* tell the unit that the attempt was detected: */
		sprintf(buf, destruction_detected_msg, unitname(u), name);
		addmessage(0, u->faction, buf, MSG_EVENT, ML_WARN);
		/* tell the enemy whodunit: */
		sprintf(buf, detect_failure_msg, unitname(u2), unitname(u), name);
		addmessage(0, u2->faction, buf, MSG_EVENT, ML_IMPORTANT);
		return 0;
	}
	/* tell the unit that the attempt succeeded */
	sprintf(buf, destruction_success_msg, name, unitname(u));
	addmessage(0, u->faction, buf, MSG_EVENT, ML_INFO);
	sprintf(buf, object_destroyed_msg, name);
	addmessage(0, u2->faction, buf, MSG_EVENT, ML_IMPORTANT);
	return 1;					/* success */
}

static void
sink_ship(region * r, ship * sh, const char *name, char spy, unit * saboteur)
{
	const char *person_lost_msg = "- %d Person von %s ertrinkt; %s.";
	const char *persons_lost_msg = "- %d Personen von %s ertrinken; %s.";
	const char *unit_dies_msg = "Die Einheit wird ausgeloescht";
	const char *unit_lives_msg = "Die Einheit rettet sich nach ";
	const char *unit_intact_msg = "%s ueberlebt unbeschadet und rettet sich nach %s.";
	const char *ship_sinks_msg = "%s versinkt im Ozean.";
	const char *enemy_discovers_spy_msg = "%s wurde beim versenken von %s entdeckt.";
	const char *spy_discovered_msg = "%s entdeckte %s beim versenken von %s.";
	unit **ui;
	region *safety = r;
	int i;
	direction_t d;
	unsigned int index;
	double probability = 0.0;
	char buffer[DISPLAYSIZE + 1];
	vset informed;
	vset survivors;

	vset_init(&informed);
	vset_init(&survivors);

	/* figure out what a unit's chances of survival are: */
  if (!fval(r->terrain, SEA_REGION)) {
    probability = CANAL_SWIMMER_CHANCE;
  } else {
    for (d = 0; d != MAXDIRECTIONS; ++d) {
      region * rn = rconnect(r, d);
      if (!fval(rn->terrain, SEA_REGION) && !move_blocked(NULL, r, rn)) {
        safety = rn;
        probability = OCEAN_SWIMMER_CHANCE;
        break;
      }
    }
  }
	for (ui = &r->units; *ui; ui = &(*ui)->next) {
		unit *u = *ui;

		/* inform this faction about the sinking ship: */
		vset_add(&informed, u->faction);
		if (u->ship == sh) {
			int dead = 0;

			/* if this fails, I misunderstood something: */
			for (i = 0; i != u->number; ++i)
				if (chance(probability))
					++dead;

			if (dead != u->number)
				/* she will live. but her items get stripped */
			{
				vset_add(&survivors, u);
				if (dead > 0) {
					strcat(strcpy(buffer, unit_lives_msg), regionname(safety, u->faction));
					sprintf(buf, (dead == 1) ? person_lost_msg : persons_lost_msg,
							dead, unitname(u), buffer);
				} else
					sprintf(buf, unit_intact_msg, unitname(u), regionname(safety, u->faction));
				addmessage(0, u->faction, buf, MSG_EVENT, ML_WARN);
				set_leftship(u, u->ship);
				u->ship = 0;
				if (r != safety)
					setguard(u, GUARD_NONE);
				while (u->items) i_remove(&u->items, u->items);
				move_unit(u, safety, NULL);
			} else {
				sprintf(buf, (dead == 1) ? person_lost_msg : persons_lost_msg,
						dead, unitname(u), unit_dies_msg);
			}
			if (dead == u->number) {
        /* the poor creature, she dies */
				*ui = u->next;
				destroy_unit(u);
			}
		}
	}

	/* inform everyone, and reduce money to the absolutely necessary
	 * amount: */
	while (informed.size != 0) {
		unit *lastunit = 0;
    int money = 0, maintain = 0;
		faction * f = (faction *) informed.data[0];

    /* find out how much money this faction still has: */
		for (index = 0; index != survivors.size; ++index) {
			unit *u = (unit *) survivors.data[index];

			if (u->faction == f) {
				maintain += maintenance_cost(u);
				money += get_money(u);
				lastunit = u;
			}
		}
		/* 'money' shall be the maintenance-surplus of the survivng
		 * units: */
		money = money - maintain;
		for (index = 0; money > 0; ++index) {
			int remove;
			unit *u = (unit *) survivors.data[index];

			assert(index < survivors.size);
			if (u->faction == f && playerrace(u->race)) {
				remove = min(get_money(u), money);
				money -= remove;
				change_money(u, -remove);
			}
		}
		/* finally, report to this faction that the ship sank: */
		sprintf(buf, ship_sinks_msg, name);
		addmessage(0, f, buf, MSG_EVENT, ML_WARN);
		vset_erase(&informed, f);
		if (spy == 1 && f != saboteur->faction &&
			faction_skill(r, f, SK_OBSERVATION) - eff_skill(saboteur, SK_STEALTH, r) > 0) {
			/* the unit is discovered */
			sprintf(buf, spy_discovered_msg, lastunit, unitname(saboteur), name);
			addmessage(0, f, buf, MSG_EVENT, ML_IMPORTANT);
			sprintf(buf, enemy_discovers_spy_msg, unitname(saboteur), name);
			addmessage(0, saboteur->faction, buf, MSG_EVENT, ML_MISTAKE);
		}
	}
	/* finally, get rid of the ship */
	destroy_ship(sh);
	vset_destroy(&informed);
	vset_destroy(&survivors);
}

int
sabotage_cmd(unit * u, struct order * ord)
{
	const char *s;
	int i;
	ship *sh;
	unit *u2;
	char buffer[DISPLAYSIZE];
  region * r = u->region;

  init_tokens(ord);
  skip_token();
	s = getstrtoken();

	i = findparam(s, u->faction->locale);

	switch (i) {
	case P_SHIP:
		sh = u->ship;
		if (!sh) {
			cmistake(u, u->thisorder, 144, MSG_EVENT);
			return 0;
		}
		u2 = shipowner(sh);
		strcat(strcpy(buffer, shipname(sh)), sh->type->name[0]);
		if (try_destruction(u, u2, buffer, eff_skill(u, SK_SPY, r)
						  - crew_skill(r, u2->faction, sh, SK_OBSERVATION))) {
			sink_ship(r, sh, buffer, 1, u);
		}
		break;
	default:
		cmistake(u, u->thisorder, 9, MSG_EVENT);
		return 0;
	}

	return 0;
}

