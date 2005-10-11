/* vi: set ts=2:
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
#include <eressea.h>

/* misc includes */
#include <attributes/key.h>
#include <attributes/otherfaction.h>
#include <attributes/targetregion.h>
#include <modules/autoseed.h>
#include <modules/xecmd.h>

/* gamecode includes */
#include <gamecode/economy.h>
#include <gamecode/monster.h>

/* kernel includes */
#include <kernel/alchemy.h>
#include <kernel/alliance.h>
#include <kernel/border.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/movement.h>
#include <kernel/names.h>
#include <kernel/pathfinder.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/resources.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/spell.h>
#include <kernel/study.h>
#include <kernel/teleport.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/cvector.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/resolve.h>
#include <util/sql.h>
#include <util/vset.h>

/* libc includes */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#undef XMAS1999
#undef XMAS2000
#undef XMAS2001
#undef XMAS2002

extern void reorder_owners(struct region * r);
extern int incomplete_data;

static int
curse_emptiness(void)
{
	const curse_type * ct = ct_find("godcursezone");
	region * r;
	for (r=regions;r!=NULL;r=r->next) {
		unit * u = r->units;
		if (r->land==NULL) continue;
		if (fval(r, RF_CHAOTIC)) continue;
		if (r->terrain==T_GLACIER) continue;
		if (r->age<=200) continue;
		if (get_curse(r->attribs, ct)) continue;
		while (u && u->faction->no==MONSTER_FACTION) u=u->next;
		if (u==NULL) fset(r, FL_MARK);
	}
	for (r=regions;r!=NULL;r=r->next) {
		if (fval(r, FL_MARK)) {
			direction_t d;
			for (d=0;d!=MAXDIRECTIONS;++d) {
				region * rn = rconnect(r,d);
				if (rn==NULL) continue;
				if (fval(rn, FL_MARK) || get_curse(rn->attribs, ct)) {
					break;
				}
			}
			if (d!=MAXDIRECTIONS) {
        variant effect;
				curse * c;
        effect.i = 0;
        c = create_curse(NULL, &r->attribs, ct, 100, 100, effect, 0);
				curse_setflag(c, CURSE_ISNEW|CURSE_IMMUNE);
			}
			freset(r, FL_MARK);
		}
	}
	return 0;
}

void
french_testers(void)
{
	faction * f = factions;
	const struct locale * french = find_locale("fr");
	while (f!=NULL) {
		if (f->locale==french) fset(f, FFL_NOTIMEOUT);
		f = f->next;
	}
}

static void
verify_owners(boolean bOnce)
{
	region * r;

	for (r=regions;r;r=r->next) {
		unit * u;
		boolean bFail = false;

		for (u=r->units;u;u=u->next) {
			if (u->building) {
				unit * bo = buildingowner(r, u->building);
				if (!fval(bo, UFL_OWNER)) {
					log_error(("[verify_owners] %u ist Besitzer von %s, hat aber UFL_OWNER nicht.\n", unitname(bo), buildingname(u->building)));
					bFail = true;
					if (bOnce) break;
				}
				if (bo!=u && fval(u, UFL_OWNER)) {
					log_error(("[verify_owners] %u ist NICHT Besitzer von %s, hat aber UFL_OWNER.\n", unitname(u), buildingname(u->building)));
					bFail = true;
					if (bOnce) break;
				}
			}
			if (u->ship) {
				unit * bo = shipowner(u->ship);
				if (!fval(bo, UFL_OWNER)) {
					log_error(("[verify_owners] %u ist Besitzer von %s, hat aber UFL_OWNER nicht.\n", unitname(bo), shipname(u->ship)));
					bFail = true;
					if (bOnce) break;
				}
				if (bo!=u && fval(u, UFL_OWNER)) {
					log_error(("[verify_owners] %u ist NICHT Besitzer von %s, hat aber UFL_OWNER.\n", unitname(u), shipname(u->ship)));
					bFail = true;
					if (bOnce) break;
				}
			}
		}
		if (bFail) reorder_owners(r);
	}
}

/* make sure that this is done only once! */
#define do_once(magic, fun) \
{ \
	attrib * a = find_key(global.attribs, atoi36(magic)); \
	if (!a) { \
		log_warning(("[do_once] a unique fix %d=\"%s\" was applied.\n", atoi36(magic), magic)); \
		if (fun == 0) a_add(&global.attribs, make_key(atoi36(magic))); \
	} \
}

int
warn_items(void)
{
	boolean found = 0;
	region * r;
	const item_type * it_money = it_find("money");
	for (r=regions;r;r=r->next) {
		unit * u;
		for (u=r->units;u;u=u->next) {
			item * itm;
			for (itm=u->items;itm;itm=itm->next) {
				if (itm->number>100000 && itm->type!=it_money) {
					found = 1;
					log_error(("Einheit %s hat %u %s\n",
						unitid(u), itm->number,
						resourcename(itm->type->rtype, 0)));
				}
			}
		}
	}
	return found;
}

static boolean
kor_teure_talente(unit *u)
{
	if(effskill(u, SK_TACTICS) >= 1 ||
		 effskill(u, SK_MAGIC) >= 1 ||
		 effskill(u, SK_ALCHEMY) >= 1 ||
		 effskill(u, SK_HERBALISM) >= 1 ||
		 effskill(u, SK_SPY) >= 1) {
				return true;
	}
	return false;
}

static void
no_teurefremde(boolean convert)
{
  const curse_type * slave_ct = ct_find("slavery");
  region *r;

  for(r=regions;r;r=r->next) {
    unit *u;

    for(u=r->units;u;u=u->next) {
      if (u->faction->no != MONSTER_FACTION && playerrace(u->faction->race)
        && is_migrant(u) && kor_teure_talente(u))
      {
        if (slave_ct && curse_active(get_curse(u->attribs, slave_ct))) 
          continue;
        log_warning(("Teurer Migrant: %s, Partei %s\n",
          unitname(u), factionname(u->faction)));
        if (convert) {
          u->race = u->faction->race;
          u->irace = u->faction->race;
          sprintf(buf, "Die Götter segnen %s mit der richtigen Rasse",
            unitname(u));
          addmessage(0, u->faction, buf, MSG_MESSAGE, ML_IMPORTANT);
        }
      }
    }
  }
}

attrib_type at_roadfix = {
	"roadfix",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
#if RELEASE_VERSION<DISABLE_ROADFIX
	DEFAULT_WRITE,
#else
	NULL, /* disable writing them so they will disappear */
#endif
	DEFAULT_READ,
	ATF_UNIQUE
};

/* ************************************************************ */
/* GANZ WICHTIG! ALLE GEÄNDERTEN SPRÜCHE NEU ANZEIGEN */
/* GANZ WICHTIG! FÜGT AUCH NEUE ZAUBER IN DIE LISTE DER BEKANNTEN EIN */
/* ************************************************************ */
static void
show_newspells(void)
{
  region *r;
  /* Alle geänderten Zauber in das array newspellids[]. mit SPL_NOSPELL
  * terminieren */

  spellid_t newspellids[] = {
    SPL_NOSPELL
  };

  /* die id's der neuen oder veränderten Sprüche werden in newspellids[]
  * abgelegt */

  for(r=regions; r; r=r->next) {
    unit *u;
    for(u=r->units;u;u=u->next) {
      sc_mage *m = get_mage(u);
      if (u->faction->no == MONSTER_FACTION) continue;
      if (m != NULL) {
        int i;

        if (m->magietyp == M_GRAU) continue;

        for (i = 0; newspellids[i] != SPL_NOSPELL; i++) {
          spell *sp = find_spellbyid(newspellids[i]);

          if (!sp) continue;

          if (m->magietyp == sp->magietyp || has_spell(u, sp)) {
            attrib * a = a_find(u->faction->attribs, &at_reportspell);
            while (a && a->data.i != (int)sp->id) a = a->nexttype;
            if (!a) {
              /* spell is not being shown yet. if seen before, remove to show again */
              a = a_find(u->faction->attribs, &at_seenspell);
              while (a && a->data.i != (int)sp->id) a = a->nexttype;
              if (a) a_remove(&u->faction->attribs, a);
            }
          }
        }
      }
    }
  }
}

extern plane * arena;

static void
fix_age(void)
{
	faction * f;
	const race * oldorc = rc_find("orc");
	const race * uruk = rc_find("uruk");
	for (f=factions;f;f=f->next) {
		if (f->no!=MONSTER_FACTION && playerrace(f->race)) continue;
		if (f->race==oldorc) f->race= uruk;
		else if (f->age!=turn) {
			log_printf("Alter von Partei %s auf %d angepasst.\n", factionid(f), turn);
			f->age = turn;
		}
	}
}


static void
fix_firewalls(void)
{
  region * r = regions;
  while (r) {
    direction_t d;
    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * r2 = rconnect(r, d);
      if (r2) {
        border * b = get_borders(r, r2);
        while (b) {
          if (b->type==&bt_firewall) {
            attrib * a = a_find(b->attribs, &at_countdown);
            if (a==NULL || a->data.i <= 0) {
              erase_border(b);
              log_warning(("firewall between regions %s and %s was bugged. removed.\n",
                regionname(r, NULL), regionname(r2, NULL)));
              b = get_borders(r, r2);
            } else {
              b = b->next;
            }
          } else {
            b = b->next;
          }
        }
      }
    }
    r = r->next;
  }
}

static void
fix_otherfaction(void)
{
  int count = 0;
  region * r;
  for (r=regions;r;r=r->next) {
    unit * u;
    for (u=r->units;u;u=u->next) {
      attrib * a = a_find(u->attribs, &at_otherfaction);
			if (a!=NULL) {
				faction * f = (faction*)a->data.v;
				if (f==u->faction) {
					a_remove(&u->attribs, a);
					++count;
				}
			}
    }
  }
  if (count) log_warning(("%u units had otherfaction=own faction.\n", count));
}

extern attrib * make_atgmcreate(const struct item_type * itype);
extern attrib * make_atpermissions(void);
extern struct attrib_type at_permissions;
extern struct attrib_type at_gmcreate;


static void
update_gms(void)
{
	faction * f;
	for (f=factions;f;f=f->next) {
		attrib * permissions = a_find(f->attribs, &at_permissions);
		if (permissions) {
			const char * keys[] = { "gmgate", "gmmsgr", "gmkill", "gmmsgu", NULL };
			int k;
			item_t i;
			for (k=0;keys[k];++k) {
				add_key((attrib**)&permissions->data.v, atoi36(keys[k]));
			}
			for (i=I_LAENSWORD;i!=I_DRACHENBLUT;++i) {
				attrib * a = a_find((attrib*)permissions->data.v, &at_gmcreate);
				while (a && a->data.v!=(void*)olditemtype[i]) a=a->nexttype;
				if (!a) a_add((attrib**)&permissions->data.v, make_atgmcreate(olditemtype[i]));
			}
		}
	}
}

static int
fix_demands(void)
{
  region *r;

  for (r=regions; r; r=r->next) {
    if (r->land!=NULL && r->land->demands==NULL) {
      fix_demand(r);
    }
  }
  return 0;
}

#include "group.h"
static void
fix_allies(void)
{
	faction * f;
	for (f=factions;f;f=f->next) {
		group * g;
		for (g=f->groups;g;g=g->next) {
			ally ** ap=&g->allies;
			while (*ap) {
				ally * an, * a = *ap;
				for (an = a->next;an;an=an->next) {
					if (a->faction==an->faction) {
						*ap = a->next;
						free(a);
						break;
					}
				}
				if (an==NULL) ap = &(*ap)->next;
			}
		}
	}
}

#ifdef FUZZY_BASE36
extern boolean enable_fuzzy;
#endif

static void
fix_icastles(void)
{
	region * r;
	for (r=regions; r; r=r->next) {
		building * b;
		for (b=r->buildings; b; b=b->next) {
			attrib * a;
			const building_type * btype = bt_find("castle");
			icastle_data * data;
			a = a_find(b->attribs, &at_icastle);
			if (b->type!=bt_find("illusion") && !a) continue;

			if (!a) {
				/* attribut hat gefehle */
				a = a_add(&b->attribs, a_new(&at_icastle));
			}
			if (b->type!=bt_find("illusion")) {
				/* gebäudetyp war falsch */
				btype = b->type;
				b->type = bt_find("illusion");
			}
			data = (icastle_data*)a->data.v;
			if (data->time<=0) {
				/* zeit war kaputt oder abgelaufen */
				data->time = 1;
			}
			if (!data->type) {
				/* typ muss gesetzt werden, weil er nicht geladen wurde */
				data->type = btype;
			}
			if (data->building!=b) {
				/* rückwärtszeiger auf das gebäude reparieren */
				data->building=b;
			}
		}
	}
}

#if 0
static void
fix_herbs(void)
{
	const char * plain_herbs[] = {"Flachwurz", "Würziger Wagemut", "Eulenauge", "Grüner Spinnerich", "Blauer Baumringel", "Elfenlieb"};
	const herb_type * htypes[6];
	int herbs[6];
	int hneed[6];
	region *r;
	int i, hsum = 0, left = 0;

	for (i=0;i!=6;++i) {
		htypes[i] = resource2herb(finditemtype(plain_herbs[i], NULL)->rtype);
		herbs[i] = 0;
	}

	for (r=regions; r; r=r->next) if (rterrain(r) == T_PLAIN) {
		const herb_type *htype = rherbtype(r);
		if (htype==NULL) {
			htype = htypes[i];
			rsetherbtype(r, htype);
		}
		for (i=0;i!=6;++i) if (htypes[i]==htype) break;
		assert(i!=6);
		herbs[i]++;
		hsum++;
	}

	for (i=0;i!=6;++i) {
		int hwant = hsum / (6-i);
		hneed[i] = hwant - herbs[i];
		if (hneed[i]>0) left += hneed[i];
		hsum -= hwant;
	}

	for (r=regions; r; r=r->next) if (rterrain(r) == T_PLAIN) {
		const herb_type *htype = rherbtype(r);
		assert(htype);
		for (i=0;i!=6;++i) if (htypes[i]==htype) break;
		assert(i!=6);
		if (hneed[i]<0) {
			int p;
			int k = rand() % left;
			for (p=0;p!=6;++p) if (hneed[p]>0) {
				k-=hneed[p];
				if (k<0) break;
			}
			assert(p!=6);
			hneed[p]--;
			hneed[i]++;
			left--;
			rsetherbtype(r, htypes[p]);
		}
		hsum++;
	}

	for (i=0;i!=6;++i) herbs[i] = 0;

	for (r=regions; r; r=r->next) if (rterrain(r) == T_PLAIN) {
		const herb_type *htype = rherbtype(r);
		assert(htype);
		for (i=0;i!=6;++i) if (htypes[i]==htype) break;
		assert(i!=6);
		herbs[i]++;
	}
	for (i=0;i!=6;++i) {
		fprintf(stderr, "%s : %d\n", locale_string(NULL, resourcename(htypes[i]->itype->rtype, 0)), herbs[i]);
	}
}
#endif

#include <triggers/timeout.h>
#include <triggers/changerace.h>
#include <triggers/changefaction.h>
#include <triggers/createcurse.h>
#include <triggers/createunit.h>
#include <triggers/killunit.h>
#include <triggers/giveitem.h>

typedef struct handler_info {
	char * event;
	trigger * triggers;
} handler_info;


typedef struct timeout_data {
	trigger * triggers;
	int timer;
	variant trigger_data;
} timeout_data;

trigger *
get_timeout(trigger * td, trigger * tfind)
{
	trigger * t = td;
	while (t) {
		if (t->type==&tt_timeout) {
			timeout_data * tdata = (timeout_data *)t->data.v;
			trigger * tr = tdata->triggers;
			while (tr) {
				if (tr==tfind) break;
				tr=tr->next;
			}
			if (tr==tfind) break;
		}
		t=t->next;
	}
	return t;
}

#include <modules/gmcmd.h>

static int
secondfaction(faction * pf)
{
	unit * u = findunit(atoi36("5q9w"));
	if (u!=NULL) {
		plane * p = rplane(u->region);
		if (p!=NULL) {
			region * center = findregion((p->maxx-p->minx)/2, (p->maxy-p->miny)/2);
			if (center!=NULL) {
				gm_addfaction(pf->email, p, center);
				return 0;
			}
		}
	}
	return -1;
}

static void
update_gmquests(void)
{
	faction * f = findfaction(atoi36("gm04"));
	if (f) {
		unit * u = f->units;
		potion_t p;
		attrib * permissions = a_find(f->attribs, &at_permissions);

		if (u!=NULL) {
			plane * p = rplane(u->region);
			/* gm04 will keine Orks */
			if (p!=NULL) p->flags |= PFL_NOORCGROWTH;
			/* gm04 will keine Monster */
			if (p!=NULL) p->flags |= PFL_NOMONSTERS;
		}
		for (p=0;p!=MAX_POTIONS;++p) {
			attrib * a = a_find((attrib*)permissions->data.v, &at_gmcreate);
			while (a && a->data.v!=(void*)oldpotiontype[p]->itype) a=a->nexttype;
			if (!a) a_add((attrib**)&permissions->data.v, make_atgmcreate(oldpotiontype[p]->itype));
		}
		do_once("et02", secondfaction(f));
	}
}

#define TEST_LOCALES 0
#if TEST_LOCALES
static void
setup_locales(void)
{
	locale * lang = find_locale("en");
	faction * f = factions;
	while (f) {
		f->locale = lang;
		f = f->next;
	}
}
#endif

#include <triggers/shock.h>
#include <triggers/killunit.h>


#if RESOURCE_CONVERSION
extern struct attrib_type at_resources;
void
init_resourcefix(void)
{
	at_register(&at_resources);
}

#endif

int
growing_trees(void)
{
	region *r;

	for(r=regions; r; r=r->next) {
		if(rtrees(r, 2)) {
			rsettrees(r, 1, rtrees(r, 2)/4);
			rsettrees(r, 0, rtrees(r, 2)/2);
		}
	}
	return 0;
}

#include <triggers/gate.h>
#include <triggers/unguard.h>
typedef struct gate_data {
	struct building * gate;
	struct region * target;
} gate_data;

static void
fix_gates(void)
{
	region * r;
	for (r=regions;r;r=r->next) {
		unit * u;
		building * b;
		for (u=r->units;u;u=u->next) {
			trigger ** triggers = get_triggers(u->attribs, "timer");
			if (triggers) {
				trigger * t = *triggers;
				while (t && t->type!= &tt_gate) t=t->next;
				if (t!=NULL) {
					gate_data * gd = (gate_data*)t->data.v;
					struct building * b = gd->gate;
					struct region * rtarget = gd->target;
					if (r!=b->region) {
						add_trigger(&b->attribs, "timer", trigger_gate(b, rtarget));
						add_trigger(&b->attribs, "create", trigger_unguard(b));
						fset(b, BLD_UNGUARDED);
					}
				}
				remove_triggers(&u->attribs, "timer", &tt_gate);
				remove_triggers(&u->attribs, "create", &tt_unguard);
			}
		}
		for (b=r->buildings;b;b=b->next) {
			trigger ** triggers = get_triggers(b->attribs, "timer");
			if (triggers) {
				trigger * t = *triggers;
				while (t && t->type!= &tt_gate) t=t->next;
				if (t!=NULL) {
					gate_data * gd = (gate_data*)t->data.v;
					struct building * b = gd->gate;
					struct region * rtarget = gd->target;
					remove_triggers(&b->attribs, "timer", &tt_gate);
					remove_triggers(&b->attribs, "create", &tt_unguard);
					if (r!=b->region) {
						add_trigger(&b->attribs, "timer", trigger_gate(b, rtarget));
						add_trigger(&b->attribs, "create", trigger_unguard(b));
						fset(b, BLD_UNGUARDED);
					}
				}
			}
		}
	}
}

static int
road_decay(void)
{
  const struct building_type * bt_caravan, * bt_dam, * bt_tunnel;
  region * r;

  bt_caravan = bt_find("caravan");
  bt_dam = bt_find("dam");
  bt_tunnel = bt_find("tunnel");

  for (r=regions;r;r=r->next) {
    boolean half = false;
    if (rterrain(r) == T_SWAMP) {
      /* wenn kein Damm existiert */
      if (!buildingtype_exists(r, bt_dam)) {
        half = true;
      }
    }
    else if (rterrain(r) == T_DESERT) {
      /* wenn keine Karawanserei existiert */
      if (!buildingtype_exists(r, bt_caravan)) {
        half = true;
      }
    }
    else if (rterrain(r) == T_GLACIER) {
      /* wenn kein Tunnel existiert */
      if (!buildingtype_exists(r, bt_tunnel)) {
        half = true;
      }
    }

    if (half) {
      direction_t d;
      short maxt = terrain[rterrain(r)].roadreq;
      /* Falls Karawanserei, Damm oder Tunnel einstürzen, wird die schon
       * gebaute Straße zur Hälfte vernichtet */
      for (d=0;d!=MAXDIRECTIONS;++d) {
        if (rroad(r, d) > maxt) {
          rsetroad(r, d, maxt);
        }
      }
    }
  }
  return 0;
}

static void
frame_regions(void)
{
#ifdef AGE_FIX
  unsigned short ocean_age = (unsigned short)turn;
endif
  region * r = regions;
  for (r=regions;r;r=r->next) {
    direction_t d;
#ifdef AGE_FIX
    if (rterrain(r) == T_OCEAN && r->age==0) {
      unsigned short age = 0;
      direction_t d;
      for (d=0;d!=MAXDIRECTIONS;++d) {
        region * rn = rconnect(r, d);
        if (rn && rn->age>age) {
          age = rn->age;
        }
      }
      if (age!=0 && age < ocean_age) {
        ocean_age = age;
      }
      r->age = ocean_age;
    } else if (r->age>ocean_age) {
      ocean_age = r->age;
    }
#endif
    if (r->age<16) continue;
    if (r->planep) continue;
    if (r->terrain==T_FIREWALL) continue;

    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * rn = rconnect(r, d);
      if (rn==NULL) {
        rn = new_region(r->x+delta_x[d], r->y+delta_y[d]);
        terraform(rn, T_FIREWALL);
        rn->age=r->age;
      }
    }
  }
}

#define GLOBAL_WARMING 200

#ifdef GLOBAL_WARMING

static void
iceberg(region * r)
{
  direction_t d;
  for (d=0;d!=MAXDIRECTIONS;++d) {
    region * rn = rconnect(r, d);
    if (rn!=NULL) {
      terrain_t rt = rn->terrain;
      if (rt!=T_ICEBERG && rt!=T_ICEBERG_SLEEP && rt!=T_GLACIER && rt!=T_OCEAN) {
        break;
      }
    }
  }
  if (d==MAXDIRECTIONS) {
    terraform(r, T_ICEBERG_SLEEP);
  }
}

static void
global_warming(void)
{
  region * r;
  for (r=regions;r;r=r->next) {
    if (r->age<GLOBAL_WARMING) continue;
    if (r->terrain==T_GLACIER) {
      /* 1% chance that an existing glacier gets unstable */
      if (chance(0.01)) {
        iceberg(r);
      }
    } else if (r->terrain==T_ICEBERG || r->terrain==T_ICEBERG_SLEEP) {
      direction_t d;
      for (d=0;d!=MAXDIRECTIONS;++d) {
        region * rn = rconnect(r, d);
        if (rn && rn->terrain==T_GLACIER && chance(0.10)) {
          /* 10% chance that a glacier next to an iceberg gets unstable */
          iceberg(rn);
        }
      }
    }
  }
}
#endif

static int
fix_astralplane(void)
{
  plane * astralplane = get_astralplane();
  region * r;
  region_list * rlist = NULL;
  faction * monsters = findfaction(MONSTER_FACTION);

  if (astralplane==NULL || monsters==NULL) return 0;

  freset(astralplane, PFL_NOCOORDS);
  freset(astralplane, PFL_NOFEED);
  set_ursprung(monsters, astralplane->id, 0, 0);

  for (r=regions;r;r=r->next) if (rplane(r)==astralplane) {
    region * ra = r_standard_to_astral(r);
    if (ra==NULL) continue;
    if (r->terrain!=T_FIREWALL) continue;
    if (ra->terrain==T_ASTRALB) continue;
    if (ra->units!=NULL) {
      add_regionlist(&rlist, ra);
    }
    log_printf("protecting firewall in %s by blocking astral space in %s.\n", regionname(r, NULL), regionname(ra, NULL));
    terraform(ra, T_ASTRALB);
  }
  while (rlist!=NULL) {
    region_list * rnew = rlist;
    region * r = rnew->data;
    direction_t dir;
    rlist = rlist->next;
    for (dir=0;dir!=MAXDIRECTIONS;++dir) {
      region * rnext = rconnect(r, dir);
      if (rnext==NULL) continue;
      if (rnext->terrain!=T_ASTRAL) continue;
      while (r->units) {
        unit * u = r->units;
        move_unit(u, rnext, NULL);
        if (u->faction->no==MONSTER_FACTION) {
          set_number(u, 0);
        }
      }
      break;
    }
    if (r->units!=NULL) {
      unit * u;
      for (u=r->units;u;u=u->next) {
        if (u->faction->no!=MONSTER_FACTION) {
          log_error(("Einheit %s in %u, %u steckt im Nebel fest.\n", 
            unitname(u), r->x, r->y));
        }
      }
    }
    free(rnew);
  }
  return 0;
}

static int
warn_password(void)
{
	faction * f = factions;
	while (f) {
		boolean pwok = true;
		const char * c = f->passw;
		while (*c) {
			if (!isalnum((unsigned char)*c)) pwok = false;
			c++;
		}
		if (pwok == false) {
			ADDMSG(&f->msgs, msg_message("msg_errors", "string",
				"Dein Passwort enthält Zeichen, die bei der Nachsendung "
				"von Reports Probleme bereiten können. Bitte wähle ein neues "
				"Passwort, bevorzugt nur aus Buchstaben und Zahlen bestehend."));
		}
		f = f->next;
	}
	return 0;
}

extern border *borders[];

static void
fix_road_borders(void)
{
#define MAXDEL 10000
  border *deleted[MAXDEL];
  int hash;
  int i = 0;

  if (incomplete_data) return;
  for(hash=0; hash<BMAXHASH && i!=MAXDEL; hash++) {
    border * bhash;
    for (bhash=borders[hash];bhash && i!=MAXDEL;bhash=bhash->nexthash) {
      border * b;
      for (b=bhash;b && i!=MAXDEL;b=b->next) {
        if (b->type == &bt_road) {
          short x1, x2, y1, y2;
          region *r1, *r2;

          x1 = b->from->x;
          y1 = b->from->y;
          x2 = b->to->x;
          y2 = b->to->y;

          r1 = findregion(x1, y1);
          r2 = findregion(x2, y2);

          if (r1->land == NULL || r2->land == NULL
            || terrain[r1->terrain].roadreq == 0
            || terrain[r2->terrain].roadreq == 0) 
          {
            deleted[i++] = b;
          }
        }
      }
    }
  }

  while (i>0) {
    i--;
    erase_border(deleted[i]);
  }
}

#ifdef WDW_PHOENIX
static region *
random_land_region(void)
{
	region *r;
	int c = 0;

	/* count the available regions */
	for(r=regions; r; r=r->next) {
		if (r->land && rplane(r)==NULL) ++c;
	}

	/* choose one */
	c = rand()%c;

	/* this is a bit obfuscated, but should be correct */
	for(r=regions; c > 0; r=r->next) {
		if (r->land && rplane(r)==NULL) --c;
	}

	return(r);
}

static void
check_phoenix(void)
{
	const race * phoenix_race = rc_find("phoenix");
	unit * phoenix;
	region * r;
	faction *f;

	/* check if there is a phoenix in the world */
	for(f=factions; f; f=f->next) {
		unit *u;

		for(u=f->units; u; u=u->nextF) {
			if(u->race == phoenix_race) {
				return;
			}
		}
  }

  f = findfaction(MONSTER_FACTION);
  if (f==NULL) return;

	/* it is not, so we create it */
	r = random_land_region();
	phoenix = createunit(r, f, 1, phoenix_race);
	phoenix->name = strdup("Der Phönix");

	/* TODO: generate an appropriate region message */
}
#endif

static void
fix_dissolve(unit * u, int value, char mode)
{
  attrib * a = a_find(u->attribs, &at_unitdissolve);

  if (a!=NULL) return;
  a = a_add(&u->attribs, a_new(&at_unitdissolve));
  a->data.ca[0] = mode;
  a->data.ca[1] = (char)value;
  log_warning(("unit %s has race %s and no dissolve-attrib\n", unitname(u), rc_name(u->race, 0)));
}

static void
check_dissolve(void)
{
  region * r;
  for (r=regions;r!=NULL;r=r->next) {
    unit * u;
    for (u=r->units;u!=NULL;u=u->next) if (u->faction->no!=MONSTER_FACTION) {
      if (u->race==new_race[RC_STONEGOLEM]) {
        fix_dissolve(u, STONEGOLEM_CRUMBLE, 0);
        continue;
      } 
      if (u->race==new_race[RC_IRONGOLEM]) {
        fix_dissolve(u, IRONGOLEM_CRUMBLE, 0);
        continue;
      } 
      if (u->race==new_race[RC_PEASANT]) {
        fix_dissolve(u, 15, 1);
        continue;
      }
      if (u->race==new_race[RC_TREEMAN]) {
        fix_dissolve(u, 5, 2);
        continue;
      }
    }
  }
}

static int 
fix_familiars(void)
{
	const struct locale * lang = find_locale("en");
  region * r;
  for (r=regions;r;r=r->next) {
    unit * u;
    for (u=r->units;u;u=u->next) if (u->faction->no!=MONSTER_FACTION) {
      if (u->race->init_familiar && u->race->maintenance==0) {
        /* this is a familiar */
        unit * mage = get_familiar_mage(u);
        if (mage==0) {
          log_error(("%s is a %s familiar with no mage for faction %s\n",
										 unitid(u), racename(lang, u, u->race), 
										 factionid(u->faction)));
        } else if (!is_mage(mage)) {
          log_error(("%s is a %s familiar, but %s is not a mage for faction %s\n",
										 unitid(u), racename(lang, u, u->race), unitid(mage), 
										 factionid(u->faction)));
        }
      }
    }
  }
  return 0;
}

#define REPORT_NR (1 << O_REPORT)
#define REPORT_CR (1 << O_COMPUTER)
#define REPORT_ZV (1 << O_ZUGVORLAGE)

static int
disable_templates(void)
{
  faction * f;
  for (f=factions;f;f=f->next) {
    if (f->options & REPORT_CR) {
      f->options &= ~REPORT_ZV;
    }
  }
  return 0;
}

static int
nothing(void)
{
  return 0;
}

static int
fix_attribflags(void)
{
  region * r;
  for (r = regions; r; r=r->next) {
    unit * u = r->units;
    for (u=r->units;u!=NULL;u=u->next) {
      const attrib *a = u->attribs;
      while (a) {
        if (a->type==&at_guard) {
          fset(u, UFL_GUARD);
        }
        else if (a->type==&at_group) {
          fset(u, UFL_GROUP);
        }
        else if (a->type==&at_stealth) {
          fset(u, UFL_STEALTH);
        }
        a = a->next;
      }
    }
  }
  return 0;
}

static int 
fix_astral_firewalls(void)
{
  region * r;
  for (r = regions; r; r=r->next) {
    if (r->planep==get_astralplane() && r->terrain==T_FIREWALL) {
      terraform(r, T_ASTRALB);
    }
  }
  return 0;
}

static int 
fix_chaosgates(void)
{
  region * r;
  for (r = regions; r; r=r->next) {
    const attrib *a = a_findc(r->attribs, &at_direction);

    while (a!=NULL) {
      spec_direction * sd = (spec_direction *)a->data.v;
      region * r2 = findregion(sd->x, sd->y);
	  if (r2!=NULL) {
        border * b = get_borders(r, r2);
        while (b) {
          if (b->type==&bt_chaosgate) break;
          b = b->next;
        }
        if (b==NULL) {
          b = new_border(&bt_chaosgate, r, r2);
        }
      }
      a = a->nexttype;
    }
  }
  return 0;
}

void
korrektur(void)
{
  check_dissolve();
	french_testers();
#if TEST_LOCALES
	setup_locales();
#endif
  if (turn>400) {
    do_once("zvrm", disable_templates());
  } else {
    do_once("zvrm", nothing());
  }
  do_once("rdec", road_decay());

  do_once("chgt", fix_chaosgates());
  do_once("atrx", fix_attribflags());
  do_once("asfi", fix_astral_firewalls());
  frame_regions();
#ifdef GLOBAL_WARMING
  global_warming();
#endif
	fix_astralplane();
	fix_firewalls();
	fix_gates();
	update_gms();
	verify_owners(false);
	/* fix_herbtypes(); */
	/* In Vin 3+ können Parteien komplett übergeben werden. */
  if (!ExpensiveMigrants()) {
    no_teurefremde(true);
  }
  fix_allies();
  update_gmquests(); /* test gm quests */
  /* fix_unitrefs(); */
  warn_password();
  fix_road_borders();
  if (turn>1000) curse_emptiness(); /*** disabled ***/
  /* seems something fishy is going on, do this just
   * to be on the safe side:
   */
  fix_demands();
  fix_otherfaction();
  fix_familiars();
  /* trade_orders(); */
  
  /* immer ausführen, wenn neue Sprüche dazugekommen sind, oder sich
   * Beschreibungen geändert haben */
  show_newspells();
  fix_age();
  
  /* Immer ausführen! Erschafft neue Teleport-Regionen, wenn nötig */
  create_teleport_plane();
  
#ifdef WDW_PHOENIX
	check_phoenix();
#endif

	if (global.data_version<TYPES_VERSION) fix_icastles();
#ifdef FUZZY_BASE36
	enable_fuzzy = true;
#endif
}
