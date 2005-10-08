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
#include "eressea.h"
#include "reports.h"

/* kernel includes */
#include "building.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "karma.h"
#include "magic.h"
#include "message.h"
#include "order.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "ship.h"
#include "skill.h"
#include "unit.h"
#ifdef USE_UGROUPS
# include "ugroup.h"
# include <attributes/ugroup.h>
#endif

/* util includes */
#include <util/bsdstring.h>
#include <util/base36.h>
#include <util/goodies.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

/* attributes includes */
#include <attributes/follow.h>
#include <attributes/otherfaction.h>
#include <attributes/racename.h>

const char * g_reportdir;

const char *neue_gebiete[] = {
	"none",
	"illaun",
	"tybied",
	"cerddor",
	"gwyrrd",
	"draig"
};

const char *coasts[MAXDIRECTIONS] =
{
	"coast::nw",
	"coast::ne",
	"coast::e",
	"coast::se",
	"coast::sw",
	"coast::w"
};

const char *
reportpath(void)
{
	static char zText[MAX_PATH];
	if (g_reportdir) return g_reportdir;
	return strcat(strcpy(zText, basepath()), "/reports");
}

static char *
groupid(const struct group * g, const struct faction * f)
{
	typedef char name[OBJECTIDSIZE + 1];
	static name idbuf[8];
	static int nextbuf = 0;
	char *buf = idbuf[(++nextbuf) % 8];
	sprintf(buf, "%s (%s)", g->name, factionid(f));
	return buf;
}

static const char *
report_kampfstatus(const unit * u, const struct locale * lang)
{
	static char fsbuf[64];
	static const char * azstatus[] = {
		"status_aggressive", "status_front",
		"status_rear", "status_defensive",
		"status_avoid", "status_flee" };

	strcpy(fsbuf, LOC(lang, azstatus[u->status]));
	if (fval(u, UFL_NOAID)) {
		strcat(fsbuf, ", ");
		strcat(fsbuf, LOC(lang, "status_noaid"));
	}

	return fsbuf;
}

const char *
hp_status(const unit * u)
{
	double p = (double) ((double) u->hp / (double) (u->number * unit_max_hp(u)));

	if (p > 2.00) return mkname("damage", "critical");
	if (p > 1.50) return mkname("damage", "heavily");
	if (p < 0.50) return mkname("damage", "badly");
	if (p < 0.75) return mkname("damage", "wounded");
	if (p < 0.99) return mkname("damage", "exhausted");

	return NULL;
}

void
report_item(const unit * owner, const item * i, const faction * viewer, const char ** name, const char ** basename, int * number, boolean singular)
{
	if (owner->faction == viewer) {
		if (name) *name = locale_string(viewer->locale, resourcename(i->type->rtype, ((i->number!=1 && !singular)?GR_PLURAL:0)));
		if (basename) *basename = resourcename(i->type->rtype, 0);
		if (number) *number = i->number;
	} else if (i->type->rtype==r_silver) {
		int pp = i->number/owner->number;
		if (number) *number = 1;
		if (pp > 50000 && dragonrace(owner->race)) {
			if (name) *name = locale_string(viewer->locale, "dragonhoard");
			if (basename) *basename = "dragonhoard";
		} else if (pp > 5000) {
			if (name) *name = locale_string(viewer->locale, "moneychest");
			if (basename) *basename = "moneychest";
		} else if (pp > 500) {
			if (name) *name = locale_string(viewer->locale, "moneybag");
			if (basename) *basename = "moneybag";
		} else {
			if (number) *number = 0;
			if (name) *name = NULL;
			if (basename) *basename = NULL;
		}
	} else {
		if (name) *name = locale_string(viewer->locale, resourcename(i->type->rtype, NMF_APPEARANCE|((i->number!=1 && !singular)?GR_PLURAL:0)));
		if (basename) *basename = resourcename(i->type->rtype, NMF_APPEARANCE);
		if (number) {
			const herb_type * htype = resource2herb(i->type->rtype);
			if (htype!=NULL) *number = 1;
			else *number = i->number;
		}
	}
}


#define ORDERS_IN_NR 1
static size_t
buforder(char * bufp, size_t size, const order * ord, int mode)
{
  size_t tsize = 0, rsize;

  rsize = strlcpy(bufp, ", \"", size);
  tsize += rsize;
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;
  if (mode<ORDERS_IN_NR) {
    char * cmd = getcommand(ord);
    rsize = strlcpy(bufp, cmd, size);
    free(cmd);
  } else {
    rsize = strlcpy(bufp, "...", size);
  }
  tsize += rsize;
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  if (size>1) {
    strcpy(bufp, "\"");
    ++tsize;
  }
  return tsize;
}

int
bufunit(const faction * f, const unit * u, int indent, int mode)
{
  int i, dh;
  skill_t sk;
  int getarnt = fval(u, UFL_PARTEITARNUNG);
  const char *pzTmp;
  building * b;
  boolean isbattle = (boolean)(mode == see_battle);
  int telepath_see = fspecial(f, FS_TELEPATHY);
  attrib *a_fshidden = NULL;
  item * itm;
  item * show;
  faction *fv = visible_faction(f, u);
  char * bufp = buf;
  boolean itemcloak = false;
  static const curse_type * itemcloak_ct = 0;
  static boolean init = false;
  size_t size = sizeof(buf), rsize;

  if (!init) {
    init = true;
    itemcloak_ct = ct_find("itemcloak");
  }
  if (itemcloak_ct!=NULL) {
    itemcloak = curse_active(get_curse(u->attribs, itemcloak_ct));
  }

  if (fspecial(u->faction, FS_HIDDEN))
    a_fshidden = a_find(u->attribs, &at_fshidden);

  rsize = strlcpy(bufp, unitname(u), size);
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  if (!isbattle) {
    attrib *a_otherfaction = a_find(u->attribs, &at_otherfaction);
    if (u->faction == f) {
      if (fval(u, UFL_GROUP)) {
        attrib *a = a_find(u->attribs, &at_group);
        if (a) {
          group * g = (group*)a->data.v;
          rsize = strlcpy(bufp, ", ", size);
          if (rsize>size) rsize = size-1;
          size -= rsize;
          bufp += rsize;
          rsize = strlcpy(bufp, groupid(g, f), size);
          if (rsize>size) rsize = size-1;
          size -= rsize;
          bufp += rsize;
        }
      }
      if (getarnt) {
        rsize = strlcpy(bufp, ", ", size);
        if (rsize>size) rsize = size-1;
        size -= rsize;
        bufp += rsize;
        rsize = strlcpy(bufp, LOC(f->locale, "anonymous"), size);
        if (rsize>size) rsize = size-1;
        size -= rsize;
        bufp += rsize;
      } else if (a_otherfaction) {
        faction * otherfaction = get_otherfaction(a_otherfaction);
        if (otherfaction) {
          rsize = strlcpy(bufp, ", ", size);
          if (rsize>size) rsize = size-1;
          size -= rsize;
          bufp += rsize;
          rsize = strlcpy(bufp, factionname(otherfaction), size);
          if (rsize>size) rsize = size-1;
          size -= rsize;
          bufp += rsize;
        }
      }
    } else {
      if (getarnt) {
        rsize = strlcpy(bufp, ", ", size);
        if (rsize>size) rsize = size-1;
        size -= rsize;
        bufp += rsize;
        rsize = strlcpy(bufp, LOC(f->locale, "anonymous"), size);
        if (rsize>size) rsize = size-1;
        size -= rsize;
        bufp += rsize;
      } else {
        if (a_otherfaction && alliedunit(u, f, HELP_FSTEALTH)) {
          faction * f = get_otherfaction(a_otherfaction);
          bufp += snprintf(bufp, size, ", %s (%s)", factionname(f), factionname(u->faction));
          size = sizeof(buf)-(bufp-buf);
        } else {
          rsize = strlcpy(bufp, ", ", size);
          if (rsize>size) rsize = size-1;
          size -= rsize;
          bufp += rsize;
          rsize = strlcpy(bufp, factionname(fv), size);
          if (rsize>size) rsize = size-1;
          size -= rsize;
          bufp += rsize;
        }
      }
    }
  }
#ifdef USE_UGROUPS
  if (u->faction == f) {
    attrib *a = a_find(u->attribs, &at_ugroup);
    if (a) {
      ugroup *ug = findugroupid(u->faction, a->data.i);
      if (is_ugroupleader(u, ug)) {
        if (size>1) {
          strcpy(bufp++, "*");
          --size;
        }
      }
      rsize = strlcpy(bufp, itoa36(ug->id), size);
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
    }
  }
#endif

  rsize = strlcpy(bufp, ", ", size);
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  if (u->faction != f && a_fshidden && a_fshidden->data.ca[0] == 1 && effskill(u, SK_STEALTH) >= 6) {
    rsize = strlcpy(bufp, "? ", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
  } else {
    bufp += snprintf(bufp, size, "%d ", u->number);
    size = sizeof(buf)-(bufp-buf);
  }

  pzTmp = get_racename(u->attribs);
  if (pzTmp) {
    rsize = strlcpy(bufp, pzTmp, size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
    if (u->faction==f && fval(u->race, RCF_SHAPESHIFTANY)) {
      rsize = strlcpy(bufp, " (", size);
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
      rsize = strlcpy(bufp, racename(f->locale, u, u->race), size);
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
      if (size>1) {
        strcpy(bufp++, ")");
        --size;
      }
    }
  } else {
    rsize = strlcpy(bufp, racename(f->locale, u, u->irace), size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
    if (u->faction==f && u->irace!=u->race) {
      rsize = strlcpy(bufp, " (", size);
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
      rsize = strlcpy(bufp, racename(f->locale, u, u->race), size);
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
      if (size>1) {
        strcpy(bufp++, ")");
        --size;
      }
    }
  }

#ifdef HEROES
  if (fval(u, UFL_HERO) && (u->faction == f || omniscient(f))) {
    rsize = strlcpy(bufp, ", ", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
    rsize = strlcpy(bufp, LOC(f->locale, "hero"), size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
  }
#endif
  /* status */

  if (u->number && (u->faction == f || telepath_see || isbattle)) {
    const char * c = locale_string(f->locale, hp_status(u));
    rsize = strlcpy(bufp, ", ", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
    rsize = strlcpy(bufp, report_kampfstatus(u, f->locale), size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
    if (c || fval(u, UFL_HUNGER)) {
      rsize = strlcpy(bufp, " (", size);
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
      if (c) {
        rsize = strlcpy(bufp, c, size);
        if (rsize>size) rsize = size-1;
        size -= rsize;
        bufp += rsize;
      }
      if (fval(u, UFL_HUNGER)) {
        if (c) rsize = strlcpy(bufp, ", hungert", size);
        else rsize = strlcpy(bufp, "hungert", size);
        if (rsize>size) rsize = size-1;
        size -= rsize;
        bufp += rsize;
      }
      if (size>1) {
        strcpy(bufp++, ")");
        --size;
      }
    }
  }
  if (getguard(u)) {
    rsize = strlcpy(bufp, ", bewacht die Region", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
  }

  if ((b = usiege(u))!=NULL) {
    rsize = strlcpy(bufp, ", belagert ", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
    rsize = strlcpy(bufp, buildingname(b), size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
  }

  dh = 0;
  if (u->faction == f || telepath_see) {
    for (sk = 0; sk != MAXSKILLS; sk++) {
      rsize = spskill(bufp, size, f->locale, u, sk, &dh, 1);
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
    }
  }

  dh = 0;
  if (f == u->faction || telepath_see || omniscient(f)) {
    show = u->items;
  } else if (!itemcloak && mode >= see_unit && !(a_fshidden
    && a_fshidden->data.ca[1] == 1 && effskill(u, SK_STEALTH) >= 3)) 
  {
    show = NULL;
    for (itm=u->items;itm;itm=itm->next) {
      item * ishow;
      const char * ic;
      int in;
      report_item(u, itm, f, NULL, &ic, &in, false);
      if (ic && *ic && in>0) {
        for (ishow = show; ishow; ishow=ishow->next) {
          const char * sc;
          int sn;
          if (ishow->type==itm->type) sc=ic;
          else report_item(u, ishow, f, NULL, &sc, &sn, false);
          if (sc==ic || strcmp(sc, ic)==0) {
            ishow->number+=itm->number;
            break;
          }
        }
        if (ishow==NULL) {
          ishow = i_add(&show, i_new(itm->type, itm->number));
        }
      }
    }
  } else {
    show = NULL;
  }
  for (itm=show; itm; itm=itm->next) {
    const char * ic;
    int in;
    report_item(u, itm, f, &ic, NULL, &in, false);
    if (in==0 || ic==NULL) continue;
    rsize = strlcpy(bufp, ", ", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;

    if (!dh) {
      bufp += snprintf(bufp, size, "%s: ", LOC(f->locale, "nr_inventory"));
      size = sizeof(buf)-(bufp-buf);
      dh = 1;
    }
    if (in == 1) {
      rsize = strlcpy(bufp, ic, size);
    } else {
      rsize = snprintf(bufp, size, "%d %s", in, ic);
    }
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
  }
  if (show!=u->items) while (show) i_free(i_remove(&show, show));

  if (u->faction == f || telepath_see) {
    sc_mage * m = get_mage(u);

    if (m!=NULL) {
      spell_list *slist = m->spells;
      int t = effskill(u, SK_MAGIC);
      bufp += snprintf(bufp, size, ". Aura %d/%d", get_spellpoints(u), max_spellpoints(u->region,u));
      size = sizeof(buf)-(bufp-buf);

      for (dh=0; slist; slist=slist->next) {
        spell * sp = slist->data;
        if (sp->level > t) continue;
        if (!dh) {
          rsize = snprintf(bufp, size, ", %s: ", LOC(f->locale, "nr_spells"));
          dh = 1;
        } else {
          rsize = strlcpy(bufp, ", ", size);
        }
        if (rsize>size) rsize = size-1;
        size -= rsize;
        bufp += rsize;
        rsize = strlcpy(bufp, spell_name(sp, f->locale), size);
        if (rsize>size) rsize = size-1;
        size -= rsize;
        bufp += rsize;
      }

      for (i=0; i!=MAXCOMBATSPELLS; ++i) {
        if (get_combatspell(u, i)) break;
      }
      if (i!=MAXCOMBATSPELLS) {
        bufp += snprintf(bufp, size, ", %s: ", LOC(f->locale, "nr_combatspells"));
        size = sizeof(buf)-(bufp-buf);
        dh = 0;
        for (i = 0; i < MAXCOMBATSPELLS; i++){
          const spell *sp;
          if (!dh){
            dh = 1;
          } else {
            rsize = strlcpy(bufp, ", ", size);
            if (rsize>size) rsize = size-1;
            size -= rsize;
            bufp += rsize;
          }
          sp = get_combatspell(u,i);
          if (sp) {
            int sl = get_combatspelllevel(u, i);
            rsize = strlcpy(bufp, spell_name(sp, u->faction->locale), size);
            if (rsize>size) rsize = size-1;
            size -= rsize;
            bufp += rsize;

            if (sl > 0) {
              bufp += snprintf(bufp, size, " (%d)", sl);
              size = sizeof(buf)-(bufp-buf);
            }
          } else {
            rsize = strlcpy(bufp, LOC(f->locale, "nr_nospells"), size);
            if (rsize>size) rsize = size-1;
            size -= rsize;
            bufp += rsize;
          }
        }
      }
    }
#ifdef LASTORDER
    if (!isbattle && u->lastorder) {
      rsize = buforder(bufp, size, u->lastorder, 0);
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
    }
#else
    if (!isbattle) {
      boolean printed = 0;
      order * ord;;
      for (ord=u->old_orders;ord;ord=ord->next) {
        if (is_repeated(ord)) {
          if (printed<ORDERS_IN_NR) {
            rsize = buforder(bufp, size, ord, printed++);
            size -= rsize;
            bufp += rsize;
          } else break;
        }
      }
      if (printed<ORDERS_IN_NR) for (ord=u->orders;ord;ord=ord->next) {
        if (is_repeated(ord)) {
          if (printed<ORDERS_IN_NR) {
            rsize = buforder(bufp, size, ord, printed++);
            size -= rsize;
            bufp += rsize;
          } else break;
        }
      }
    }
#endif
  }
  i = 0;

  if (u->display && u->display[0]) {
    rsize = strlcpy(bufp, "; ", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;

    rsize = strlcpy(bufp, u->display, size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;

    i = u->display[strlen(u->display) - 1];
  }
  if (i != '!' && i != '?' && i != '.') {
    if (size>1) {
      strcpy(bufp++, ".");
      --size;
    }
  }
  pzTmp = uprivate(u);
  if (u->faction == f && pzTmp) {
    rsize = strlcpy(bufp, " (Bem: ", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
    rsize = strlcpy(bufp, pzTmp, size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
    rsize = strlcpy(bufp, ")", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
  }

  dh=0;
  if (!getarnt && f) {
    if (alliedfaction(getplane(u->region), f, fv, HELP_ALL)) {
      dh = 1;
    }
  }
  return dh;
}

/* TODO: telepath_see wird nicht berücksichtigt: Parteien mit
 * telepath_see sollten immer einzelne Einheiten zu sehen
 * bekommen, alles andere ist darstellungsteschnisch kompliziert.
 */

#ifdef USE_UGROUPS
int
bufunit_ugroupleader(const faction * f, const unit * u, int indent, int mode)
{
	int i, dh;
	int getarnt = fval(u, UFL_PARTEITARNUNG);
	faction *fv;
	const char *pzTmp;
	attrib *a_fshidden = NULL;
	item * itm;
	item * show;
	ugroup *ug = findugroup(u);
	boolean guards = false;
	boolean sieges = false;
  boolean itemcloak = false;
  static const curse_type * itemcloak_ct = 0;
  static boolean init = false;

  if (!init) {
    init = true;
    itemcloak_ct = ct_find("itemcloak");
  }
  if (itemcloak_ct!=NULL) {
    itemcloak = curse_active(get_curse(u->attribs, itemcloak_ct));
  }

	if(fspecial(u->faction, FS_HIDDEN))
		a_fshidden = a_find(u->attribs, &at_fshidden);

	strcpy(buf, u->name);

	fv = visible_faction(f, u);

	if (getarnt) {
		scat(", ");
		scat(LOC(f->locale, "anonymous"));
	} else {
		scat(", ");
		scat(factionname(fv));
	}

	scat(", ");

	for(i = 0; i < ug->members; i++) {
		unit *uc = ug->unit_array[i];
		if(uc->faction != f && a_fshidden
				&& a_fshidden->data.ca[0] == 1 && effskill(uc, SK_STEALTH) >= 6) {
			scat("? ");
		}	 else {
			icat(uc->number);
			scat(" ");
		}
		pzTmp = get_racename(uc->attribs);
		if (pzTmp || u->irace != uc->race) {
			if (pzTmp)
				scat(pzTmp);
			else
				scat(racename(f->locale, u, u->irace));
			scat(" (");
			scat(itoa36(uc->no));
			scat("), ");
		} else {
			scat(racename(f->locale, u, u->race));
			scat(" (");
			scat(itoa36(uc->no));
			scat("), ");
		}
		if(getguard(uc)) guards = true;
		if(usiege(uc)) sieges = true;
	}

	if(guards == true) scat(", bewacht die Region");

	if(sieges == true) {
		scat(", belagert ");
		for(i = 0; i < ug->members; i++) {
			building *b = usiege(ug->unit_array[i]);
			if(b) {
				scat(buildingname(b));
				scat(", ");
			}
		}
	}

	dh = 0;
	show = NULL;
	for(i = 0; i < ug->members; i++) {
		unit *uc = ug->unit_array[i];
		if (!itemcloak && mode >= see_unit && !(a_fshidden
				 && a_fshidden->data.ca[1] == 1 && effskill(u, SK_STEALTH) >= 3)) {
			for (itm=uc->items;itm;itm=itm->next) {
				item *ishow;
				const char * ic;
				int in;
				report_item(u, itm, f, NULL, &ic, &in, false);
				if (ic && *ic && in>0) {
					for (ishow = show; ishow; ishow=ishow->next) {
						const char * sc;
						int sn;
						if (ishow->type==itm->type) sc=ic;
						else report_item(u, ishow, f, NULL, &sc, &sn, false);
						if (sc==ic || strcmp(sc, ic)==0) {
							ishow->number+=itm->number;
							break;
						}
					}
					if (ishow==NULL) {
						ishow = i_add(&show, i_new(itm->type, itm->number));
					}
				}
			}
		}
	}
	for (itm=show; itm; itm=itm->next) {
		const char * ic;
		int in;
		report_item(u, itm, f, &ic, NULL, &in, false);
		if (in==0 || ic==NULL) continue;
		scat(", ");

		if (!dh) {
			sprintf(buf+strlen(buf), "%s: ", LOC(f->locale, "nr_inventory"));
			dh = 1;
		}
		if (in == 1) {
			scat(ic);
		} else {
			icat(in);
			scat(" ");
			scat(ic);
		}
	}
	if(show) while(show) i_free(i_remove(&show, show));

	i = 0;
	if (u->display && u->display[0]) {
		scat("; ");
		scat(u->display);

		i = u->display[strlen(u->display) - 1];
	}
	if (i != '!' && i != '?' && i != '.')
		scat(".");

	dh=0;
	if (!getarnt && f && f->allies) {
		ally *sf;

		for (sf = f->allies; sf && !dh; sf = sf->next) {
			if (sf->status > 0 && sf->status <= HELP_ALL && sf->faction == fv) {
				dh = 1;
			}
		}
	}
	return dh;
}
#endif

size_t
spskill(char * buffer, size_t size, const struct locale * lang, const struct unit * u, skill_t sk, int *dh, int days)
{
  char * bufp = buffer;
  int i, effsk;
  size_t rsize;
  size_t tsize = 0;

  if (!u->number) return 0;

  if (!has_skill(u, sk)) return 0;
  
  rsize = strlcpy(bufp, ", ", size);
  tsize += rsize;
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  if (!*dh) {
    rsize = strlcpy(bufp, LOC(lang, "nr_skills"), size);
    tsize += rsize;
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;

    rsize = strlcpy(bufp, ": ", size);
    tsize += rsize;
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;

    *dh = 1;
  }
  rsize = strlcpy(bufp, skillname(sk, lang), size);
  tsize += rsize;
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  rsize = strlcpy(bufp, " ", size);
  tsize += rsize;
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;
  
  if (sk == SK_MAGIC){
    if (find_magetype(u) != M_GRAU){
      rsize = strlcpy(bufp, LOC(lang, mkname("school", magietypen[find_magetype(u)])), size);
      tsize += rsize;
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;

      rsize = strlcpy(bufp, " ", size);
      tsize += rsize;
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
    }
  }
  
  if (sk == SK_STEALTH && fval(u, UFL_STEALTH)) {
    i = u_geteffstealth(u);
    if (i>=0) {
      rsize = slprintf(bufp, size, "%d/", i);
      tsize += rsize;
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
    }
  }
  
  effsk = effskill(u, sk);
  rsize = slprintf(bufp, size, "%d", effsk);
  tsize += rsize;
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;
  
  if(u->faction->options & Pow(O_SHOWSKCHANGE)) {
    skill *skill = get_skill(u, sk);
    int oldeff = 0;
    int diff;
    
    if (skill->old > 0) {
      oldeff = skill->old + get_modifier(u, sk, skill->old, u->region, false);
    }
    
    oldeff = max(0, oldeff);
    diff   = effsk - oldeff; 
    
    if (diff != 0) {
      rsize = slprintf(bufp, size, " (%s%d)", (diff>0)?"+":"", diff);
      tsize += rsize;
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
    }
  }
  return tsize;
}

void
lparagraph(struct strlist ** SP, char *s, int indent, char mark)
{

	/* Die Liste SP wird mit dem String s aufgefuellt, mit indent und einer
	 * mark, falls angegeben. SP wurde also auf 0 gesetzt vor dem Aufruf.
	 * Vgl. spunit (). */

	char *buflocal = calloc(strlen(s) + indent + 1, sizeof(char));

	if (indent) {
		memset(buflocal, ' ', indent);
		if (mark)
			buflocal[indent - 2] = mark;
	}
	strcpy(buflocal + indent, s);
	addstrlist(SP, buflocal);
	free(buflocal);
}

void
spunit(struct strlist ** SP, const struct faction * f, const unit * u, int indent,
       int mode)
{
  int dh;
#ifdef USE_UGROUPS
  ugroup *ug = findugroup(u);

  if(ug) {
    if(is_ugroupleader(u, ug)) {
      dh = bufunit_ugroupleader(f, u, indent, mode);
    } else {
      return;
    }
  } else
#endif
    dh = bufunit(f, u, indent, mode);
  lparagraph(SP, buf, indent, (char) ((u->faction == f) ? '*' : (dh ? '+' : '-')));
}

/* in spy steht der Unterschied zwischen Wahrnehmung des Opfers und
 * Spionage des Spions */
void
spy_message(int spy, unit *u, unit *target)
{
	const char *c;

	produceexp(u, SK_SPY, u->number);

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
		scat("Talente: ");
		{
			int first = 1;
			int found = 0;
			skill_t sk;

			for (sk = 0; sk != MAXSKILLS; sk++) {
				if (has_skill(target, sk)) {
					found++;
					if (first == 1) {
						first = 0;
					} else {
						scat(", ");
					}
					scat(skillname(sk, u->faction->locale));
					scat(" ");
					icat(eff_skill(target, sk, target->region));
				}
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

const struct unit *
ucansee(const struct faction *f, const struct unit *u, const struct unit *x)
{
	if (cansee(f, u->region, u, 0)) return u;
	return x;
}

faction_list *
get_addresses(faction * f, struct seen_region * seehash[])
{
/* "TODO: travelthru" */
  region *r, *last = lastregion(f);
  const faction * lastf = NULL;
  faction_list * flist = calloc(1, sizeof(faction_list));
  flist->data = f;

  for (r=firstregion(f);r!=last;r=r->next) {
    const unit * u = r->units;
    const seen_region * sr = find_seen(seehash, r);

    if (sr==NULL) continue;
    while (u!=NULL) {
			faction * sf = visible_faction(f, u);
			boolean ballied = sf && sf!=f && sf!=lastf
				&& !fval(u, UFL_PARTEITARNUNG) && cansee(f, r, u, 0);
			if (ballied || ALLIED(f, sf)) {
				faction_list ** fnew = &flist;
				while (*fnew && (*fnew)->data->no < sf->no) {
					fnew =&(*fnew)->next;
				}
				if ((*fnew==NULL) || (*fnew)->data!=sf) {
					faction_list * finsert = malloc(sizeof(faction_list));
					finsert->next = *fnew;
					*fnew = finsert;
					finsert->data = sf;
				}
				lastf = sf;
			}
			u = u->next;
		}
	}

  if (f->alliance != NULL) {
		faction *f2;
		for(f2 = factions; f2; f2 = f2->next) {
			if(f2->alliance != NULL && f2->alliance == f->alliance) {
				faction_list ** fnew = &flist;
				while (*fnew && (*fnew)->data->no < f2->no) {
					fnew =&(*fnew)->next;
				}
				if ((*fnew==NULL) || (*fnew)->data!=f2) {
					faction_list * finsert = malloc(sizeof(faction_list));
					finsert->next = *fnew;
					*fnew = finsert;
					finsert->data = f2;
				}
			}
		}
	}
  return flist;
}

#define MAXSEEHASH 10007
seen_region * reuse;

seen_region **
seen_init(void)
{
  return (seen_region **)calloc(MAXSEEHASH, sizeof(seen_region*));
}

void
seen_done(seen_region * seehash[])
{
  int i;
  for (i=0;i!=MAXSEEHASH;++i) {
    seen_region * sd = seehash[i];
    if (sd==NULL) continue;
    while (sd->nextHash!=NULL) sd = sd->nextHash;
    sd->nextHash = reuse;
    reuse = seehash[i];
    seehash[i] = NULL;
  }
  free(seehash);
}

void
free_seen(void)
{
  while (reuse) {
    seen_region * r = reuse;
    reuse = reuse->nextHash;
    free(r);
  }
}

seen_region *
find_seen(struct seen_region * seehash[], const region * r)
{
  unsigned int index = reg_hashkey(r) % MAXSEEHASH;
  seen_region * find = seehash[index];
	while (find) {
		if (find->r==r) return find;
		find=find->nextHash;
	}
	return NULL;
}

void
get_seen_interval(struct seen_region ** seen, region ** first, region ** last)
{
  /* this is required to find the neighbour regions of the ones we are in,
   * which may well be outside of [firstregion, lastregion) */
#ifdef ENUM_REGIONS
  int i;
  for (i=0;i!=MAXSEEHASH;++i) {
    seen_region * sr = seen[i];
    while (sr!=NULL) {
      if (*first==NULL || sr->r->index<(*first)->index) {
        *first = sr->r;
      }
      if (*last!=NULL && sr->r->index>=(*last)->index) {
        *last = sr->r->next;
      }
      sr = sr->nextHash;
    }
  }
#else
  region * r = regions;
  while (r!=first) {
    if (find_seen(seen, r)!=NULL) {
      *first = r;
      break;
    }
    r = r->next;
  }
  r = *last;
  while (r!=NULL) {
    if (find_seen(seen, r)!=NULL) {
      *last = r->next;
    }
    r = r->next;
  }
#endif
}

boolean
add_seen(struct seen_region * seehash[], struct region * r, unsigned char mode, boolean dis)
{
	seen_region * find = find_seen(seehash, r);
	if (find==NULL) {
		unsigned int index = reg_hashkey(r) % MAXSEEHASH;
		if (!reuse) reuse = (seen_region*)calloc(1, sizeof(struct seen_region));
		find = reuse;
		reuse = reuse->nextHash;
		find->nextHash = seehash[index];
		seehash[index] = find;
		find->r = r;
	} else if (find->mode >= mode) {
		return false;
	}
	find->mode = mode;
	find->disbelieves |= dis;
	return true;
}

