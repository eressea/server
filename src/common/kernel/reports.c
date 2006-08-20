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
#include <kernel/building.h>
#include <kernel/border.h>
#include <kernel/terrain.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/karma.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/unit.h>

/* util includes */
#include <util/bsdstring.h>
#include <util/base36.h>
#include <util/functions.h>
#include <util/goodies.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* attributes includes */
#include <attributes/follow.h>
#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/viewrange.h>

const char * g_reportdir;
const char * visibility[] = {
  "none",
  "neighbour",
  "lighthouse",
  "travel",
  "far",
  "unit",
  "battle"
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
			if (fval(i->type, ITF_HERB)) *number = 1;
			else *number = i->number;
		}
	}
}


int * nmrs = NULL;

int
update_nmrs(void)
{
  int i, newplayers =0;
  faction *f;
  int turn = global.data_turn;

  if (nmrs==NULL) nmrs = malloc(sizeof(int)*(NMRTimeout()+1));
  for (i = 0; i <= NMRTimeout(); ++i) {
    nmrs[i] = 0;
  }

  for (f = factions; f; f = f->next) {
    if (fval(f, FFL_ISNEW)) {
      ++newplayers;
    } else if (f->no != MONSTER_FACTION) {
      int nmr = turn-f->lastorders+1;
      if (nmr<0 || nmr>NMRTimeout()) {
        log_error(("faction %s has %d NMRS\n", factionid(f), nmr));
        nmr = max(0, nmr);
        nmr = min(nmr, NMRTimeout());
      }
      ++nmrs[nmr];
    }
  }
  return newplayers;
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
  int getarnt = fval(u, UFL_PARTEITARNUNG);
  const char *pzTmp;
  building * b;
  boolean isbattle = (boolean)(mode == see_battle);
  int telepath_see = 0;
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

#ifdef KARMA_MODULE
  if (fspecial(u->faction, FS_HIDDEN)) {
    a_fshidden = a_find(u->attribs, &at_fshidden);
  }
  telepath_see = fspecial(f, FS_TELEPATHY);
#endif /* KARMA_MODULE */

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
    skill * sv;
    for (sv = u->skills;sv!=u->skills+u->skill_size;++sv) {
      rsize = spskill(bufp, size, f->locale, u, sv, &dh, 1);
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

size_t
spskill(char * buffer, size_t size, const struct locale * lang, const struct unit * u, struct skill * sv, int *dh, int days)
{
  char * bufp = buffer;
  int i, effsk;
  size_t rsize;
  size_t tsize = 0;

  if (!u->number) return 0;
  if (sv->level<=0) {
    if (sv->old<=0 || (u->faction->options & want(O_SHOWSKCHANGE))==0) {
      return 0;
    }
  }

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
  rsize = strlcpy(bufp, skillname(sv->id, lang), size);
  tsize += rsize;
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  rsize = strlcpy(bufp, " ", size);
  tsize += rsize;
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;
  
  if (sv->id == SK_MAGIC){
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
  
  if (sv->id == SK_STEALTH && fval(u, UFL_STEALTH)) {
    i = u_geteffstealth(u);
    if (i>=0) {
      rsize = slprintf(bufp, size, "%d/", i);
      tsize += rsize;
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
    }
  }
  
  effsk = effskill(u, sv->id);
  rsize = slprintf(bufp, size, "%d", effsk);
  tsize += rsize;
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;
  
  if (u->faction->options & want(O_SHOWSKCHANGE)) {
    int oldeff = 0;
    int diff;
    
    if (sv->old > 0) {
      oldeff = sv->old + get_modifier(u, sv->id, sv->old, u->region, false);
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
  int dh = bufunit(f, u, indent, mode);
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
    int first = 1;
    int found = 0;
    skill * sv;

    scat("Talente: ");
    for (sv = u->skills;sv!=u->skills+u->skill_size;++sv) {
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

const struct unit *
ucansee(const struct faction *f, const struct unit *u, const struct unit *x)
{
  if (cansee(f, u->region, u, 0)) return u;
  return x;
}

static void
get_addresses(report_context * ctx)
{
/* "TODO: travelthru" */
  seen_region * sr = NULL;
  region *r;
  const faction * lastf = NULL;
  faction_list * flist = calloc(1, sizeof(faction_list));
  
  flist->data = ctx->f;

  for (r=ctx->first;sr==NULL && r!=ctx->last;r=r->next) {
    sr = find_seen(ctx->seen, r);
  }
  
  for (;sr!=NULL;sr=sr->next) {
    const region * r = sr->r;
    const unit * u = r->units;
    
    while (u!=NULL) {
      faction * sf = visible_faction(ctx->f, u);
      boolean ballied = sf && sf!=ctx->f && sf!=lastf
          && !fval(u, UFL_PARTEITARNUNG) && cansee(ctx->f, r, u, 0);
      if (ballied || ALLIED(ctx->f, sf)) {
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
  
  if (ctx->f->alliance != NULL) {
    faction *f2;
    for(f2 = factions; f2; f2 = f2->next) {
      if(f2->alliance != NULL && f2->alliance == ctx->f->alliance) {
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
  ctx->addresses = flist;
}

#define MAXSEEHASH 0x3000
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

void
link_seen(seen_region * seehash[], const region * first, const region * last)
{
  const region * r = first;
  seen_region * sr = NULL;

  if (first==last) return;

  do {
    sr = find_seen(seehash, r);
    r = r->next;
  } while (sr==NULL && r!=last);

  while (r!=last) {
    seen_region * sn = find_seen(seehash, r);
    if (sn!=NULL) {
      sr->next = sn;
      sr = sn;
    }
    r = r->next;
  }
  sr->next = 0;
}

seen_region *
find_seen(struct seen_region * seehash[], const region * r)
{
  unsigned int index = reg_hashkey(r) & (MAXSEEHASH-1);
  seen_region * find = seehash[index];
  while (find) {
    if (find->r==r) return find;
    find=find->nextHash;
  }
  return NULL;
}

static void
get_seen_interval(report_context * ctx)
{
  /* this is required to find the neighbour regions of the ones we are in,
   * which may well be outside of [firstregion, lastregion) */
  int i;
  for (i=0;i!=MAXSEEHASH;++i) {
    seen_region * sr = ctx->seen[i];
    while (sr!=NULL) {
      if (ctx->first==NULL || sr->r->index<ctx->first->index) {
        ctx->first = sr->r;
      }
      if (ctx->last!=NULL && sr->r->index>=ctx->last->index) {
        ctx->last = sr->r->next;
      }
      sr = sr->nextHash;
    }
  }
  link_seen(ctx->seen, ctx->first, ctx->last);
}

boolean
add_seen(struct seen_region * seehash[], struct region * r, unsigned char mode, boolean dis)
{
  seen_region * find = find_seen(seehash, r);
  if (find==NULL) {
    unsigned int index = reg_hashkey(r) & (MAXSEEHASH-1);
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

typedef struct report_type {
  struct report_type * next;
  report_fun write;
  const char * extension;
  int flag;
} report_type;

static report_type * report_types;

void 
register_reporttype(const char * extension, report_fun write, int flag)
{
	report_type * type = malloc(sizeof(report_type));
	type->extension = extension;
	type->write = write;
	type->flag = flag;
	type->next = report_types;
	report_types = type;
}

static region_list *
get_regions_distance(region * root, int radius)
{
  region_list * rptr, * rlist = NULL;
  region_list ** rp = &rlist;
  add_regionlist(rp, root);
  fset(root, FL_MARK);
  while (*rp) {
    region_list * r = *rp;
    direction_t d;
    rp = &r->next;
    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * rn = rconnect(r->data, d);
      if (rn!=NULL && !fval(rn, FL_MARK) && distance(rn, root)<=radius) {
        add_regionlist(rp, rn);
        fset(rn, FL_MARK);
      }
    }
  }
  for (rptr=rlist;rptr;rptr=rptr->next) {
    freset(rptr->data, FL_MARK);
  }
  return rlist;
}

static void
view_default(struct seen_region ** seen, region *r, faction *f)
{
  direction_t dir;
  for (dir=0;dir!=MAXDIRECTIONS;++dir) {
    region * r2 = rconnect(r, dir);
    if (r2) {
      border * b = get_borders(r, r2);
      while (b) {
        if (!b->type->transparent(b, f)) break;
        b = b->next;
      }
      if (!b) add_seen(seen, r2, see_neighbour, false);
    }
  }
}

static void
view_neighbours(struct seen_region ** seen, region * r, faction * f)
{
  direction_t dir;
  for (dir=0;dir!=MAXDIRECTIONS;++dir) {
    region * r2 = rconnect(r, dir);
    if (r2) {
      border * b = get_borders(r, r2);
      while (b) {
        if (!b->type->transparent(b, f)) break;
        b = b->next;
      }
      if (!b) {
        if (add_seen(seen, r2, see_far, false)) {
          if (!(fval(r2->terrain, FORBIDDEN_REGION))) {
            direction_t dir;
            for (dir=0;dir!=MAXDIRECTIONS;++dir) {
              region * r3 = rconnect(r2, dir);
              if (r3) {
                border * b = get_borders(r2, r3);
                while (b) {
                  if (!b->type->transparent(b, f)) break;
                  b = b->next;
                }
                if (!b) add_seen(seen, r3, see_neighbour, false);
              }
            }
          }
        }
      }
    }
  }
}

static void
recurse_regatta(struct seen_region ** seen, region *center, region *r, faction *f, int maxdist)
{
  direction_t dir;
  int dist = distance(center, r);
  for (dir=0;dir!=MAXDIRECTIONS;++dir) {
    region * r2 = rconnect(r, dir);
    if (r2) {
      int ndist = distance(center, r2);
      if (ndist>dist && fval(r2->terrain, SEA_REGION)) {
        border * b = get_borders(r, r2);
        while (b) {
          if (!b->type->transparent(b, f)) break;
          b = b->next;
        }
        if (!b) {
          if (ndist<maxdist) {
            if (add_seen(seen, r2, see_far, false)) {
              recurse_regatta(seen, center, r2, f, maxdist);
            }
          } else add_seen(seen, r2, see_neighbour, false);
        }
      }
    }
  }
}

static void
view_regatta(struct seen_region ** seen, region * r, faction * f)
{
  unit *u;
  int skill = 0;
  for (u=r->units; u; u=u->next) {
    if (u->faction==f) {
      int es = effskill(u, SK_OBSERVATION);
      if (es>skill) skill=es;
    }
  }
  recurse_regatta(seen, r, r, f, skill/2);
}

static struct seen_region **
prepare_report(faction * f)
{
  region * r;
  region * end = lastregion(f);
  struct seen_region ** seen = seen_init();

  static const struct building_type * bt_lighthouse = NULL;
  if (bt_lighthouse==NULL) bt_lighthouse = bt_find("lighthouse");

  for (r = firstregion(f); r != end; r = r->next) {
    attrib *ru;
    unit * u;
    plane * p = rplane(r);
    unsigned char mode = see_none;
    boolean dis = false;
    int light = 0;

    if (p) {
      watcher * w = p->watchers;
      while (w) {
        if (f==w->faction) {
          mode = w->mode;
          break;
        }
        w = w->next;
      }
    }

    for (u = r->units; u; u = u->next) {
      if (u->faction == f) {
        if (u->building && u->building->type==bt_lighthouse) {
          int r = lighthouse_range(u->building, f);
          if (r>light) light = r;
        }
        if (u->race != new_race[RC_SPELL] || u->number == RS_FARVISION) {
          mode = see_unit;
          if (fval(u, UFL_DISBELIEVES)) {
            dis = true;
            break;
          }
        }
      }
    }

    if (light) {
      /* we are in a lighthouse. add the others! */
      region_list * rlist = get_regions_distance(r, light);
      region_list * rp = rlist;
      while (rp) {
        region * rl = rp->data;
        if (fval(rl->terrain, SEA_REGION)) {
          direction_t d;
          add_seen(seen, rl, see_lighthouse, false);
          for (d=0;d!=MAXDIRECTIONS;++d) {
            region * rn = rconnect(rl, d);
            if (rn!=NULL) {
              add_seen(seen, rn, see_neighbour, false);
            }
          }
        }
        rp = rp->next;
      }
      free_regionlist(rlist);
    }

    if (mode<see_travel && fval(r, RF_TRAVELUNIT)) {
      for (ru = a_find(r->attribs, &at_travelunit); ru && ru->type==&at_travelunit; ru = ru->next) {
        unit * u = (unit*)ru->data.v;
        if (u->faction == f) {
          mode = see_travel;
          break;
        }
      }
    }

    if (mode == see_none)
      continue;

    add_seen(seen, r, mode, dis);
    /* nicht, wenn Verwirrung herrscht: */
    if (!is_cursed(r->attribs, C_REGCONF, 0)) {
      void (*view)(struct seen_region **, region * r, faction * f) = view_default;
      if (p && fval(p, PFL_SEESPECIAL)) {
        attrib * a = a_find(p->attribs, &at_viewrange);
        if (a) view = (void (*)(struct seen_region **, region * r, faction * f))a->data.f;
      }
      view(seen, r, f);
    }
  }
  return seen;
}

int
write_reports(faction * f, time_t ltime)
{
  boolean gotit = false;
  report_type * rtype = report_types;
  struct report_context ctx;

  ctx.f = f;
  ctx.report_time	= time(NULL);
  ctx.seen = prepare_report(f);
  ctx.first = firstregion(f);
  ctx.last = lastregion(f);
  ctx.addresses = NULL;
  ctx.userdata = NULL;
  get_seen_interval(&ctx);
  get_addresses(&ctx);

  printf("Reports für %s: ", factionname(f));
  fflush(stdout);

  for (;rtype!=NULL;rtype=rtype->next) {
    if (f->options & rtype->flag) {
      char * filename;
      sprintf(buf, "%s/%d-%s.%s", reportpath(), turn, factionid(f), rtype->extension);
      filename = strdup(buf);
      if (rtype->write(filename, &ctx)==0) {
        gotit = true;
      }
      free(filename);
    }
  }

  printf("Reports for %s: DONE\n", factionname(f));
  if (!gotit) {
    log_warning(("No report for faction %s!\n", factionid(f)));
  }

  freelist(ctx.addresses);
  seen_done(ctx.seen);
  return 0;
}

static void
nmr_warnings(void)
{
  faction *f,*fa;
#define FRIEND (HELP_GUARD|HELP_MONEY)
  for (f=factions;f;f=f->next) {
    if (f->no != MONSTER_FACTION && (turn-f->lastorders) >= 2) {
      message * msg = NULL;
      for (fa=factions;fa;fa=fa->next) {
        if (alliedfaction(NULL, f, fa, FRIEND) && alliedfaction(NULL, fa, f, FRIEND)) {
          if (msg==NULL) {
            sprintf(buf, "Achtung: %s hat einige Zeit keine "
              "Züge eingeschickt und könnte dadurch in Kürze aus dem "
              "Spiel ausscheiden.", factionname(f));
            msg = msg_message("msg_event", "string", buf);
          }
          add_message(&fa->msgs, msg);
        }
      }
      if (msg!=NULL) msg_release(msg);
    }
  }
}

static void
report_donations(void)
{
  region * r;
  for (r=regions;r;r=r->next) {
    while (r->donations) {
      donation * sp = r->donations;
      if (sp->amount > 0) {
        struct message * msg = msg_message("donation",
          "from to amount", sp->f1, sp->f2, sp->amount);
        r_addmessage(r, sp->f1, msg);
        r_addmessage(r, sp->f2, msg);
        msg_release(msg);
      }
      r->donations = sp->next;
      free(sp);
    }
  }
}

static void
write_script(FILE * F, const faction * f)
{
  report_type * rtype;

  fprintf(F, "faction=%s:email=%s", factionid(f), f->email);
  if (f->options & (1<<O_BZIP2)) fputs(":compression=bz2", F);
  else fputs(":compression=zip", F);

  fputs(":reports=", F);
  buf[0] = 0;
  for (rtype=report_types;rtype!=NULL;rtype=rtype->next) {
    if (f->options&rtype->flag) {
      if (buf[0]) strcat(buf, ",");
      strcat(buf, rtype->extension);
    }
  }
  fputs(buf, F);
  fputc('\n', F);
}

#undef GLOBAL_REPORT
#ifdef GLOBAL_REPORT
static void
global_report(const char * filename)
{
  FILE * F = fopen(filename, "w");
  region * r;
  faction * f;
  faction * monsters = findfaction(MONSTER_FACTION);
  faction_list * addresses = NULL;
  struct seen_region ** seen;

  if (!monsters) return;
  if (!F) return;

  /* list of all addresses */
  for (f=factions;f;f=f->next) {
    faction_list * flist = calloc(1, sizeof(faction_list));
    flist->data = f;
    flist->next = addresses;
    addresses = flist;
  }

  seen = seen_init();
  for (r = regions; r; r = r->next) {
    add_seen(seen, r, see_unit, true);
  }
  report_computer(F, monsters, seen, addresses, time(NULL));
  freelist(addresses);
  seen_done(seen);
  fclose(F);
}
#endif

int
reports(void)
{
  faction *f;
  FILE *mailit;
  time_t ltime = time(NULL);
  const char * str;
  int retval = 0;

  nmr_warnings();
  report_donations();
  remove_empty_units();

  sprintf(buf, "%s/reports.txt", reportpath());
  mailit = fopen(buf, "w");
  if (mailit == NULL) {
    log_error(("%s konnte nicht geöffnet werden!\n", buf));
  }

  for (f = factions; f; f = f->next) {
    int error = write_reports(f, ltime);
    if (error) retval = error;
    if (mailit) write_script(mailit, f);
  }
  if (mailit) fclose(mailit);
  free_seen();
  str = get_param(global.parameters, "globalreport"); 
#ifdef GLOBAL_REPORT
  if (str!=NULL) {
    sprintf(buf, "%s/%s.%u.cr", reportpath(), str, turn);
    global_report(buf);
  }
#endif
  return retval;
}

static variant
var_copy_string(variant x)
{
  x.v = strdup((const char*)x.v);
  return x;
}

static void
var_free_string(variant x)
{
  free(x.v);
}

static variant
var_copy_order(variant x)
{
  x.v = copy_order((order*)x.v);
  return x;
}

static void
var_free_order(variant x)
{
  free_order(x.v);
}

static variant
var_copy_items(variant x)
{
  item * isrc;
  resource * rdst = NULL, ** rptr = &rdst;

  for (isrc = (item*)x.v; isrc!=NULL; isrc=isrc->next) {
    resource * res = malloc(sizeof(resource));
    res->number = isrc->number;
    res->type = isrc->type->rtype;
    *rptr = res;
    rptr = &res->next;
  }
  *rptr = NULL;
  x.v = rdst;
  return x;
}

static void
var_free_resources(variant x)
{
  resource ** rsrc = (resource**)&x.v;
  while (*rsrc) {
    resource * res = *rsrc;
    *rsrc = res->next;
    free(res);
  }
}

void
reports_init(void)
{
  /* register datatypes for the different message objects */
  register_argtype("alliance", NULL, NULL, VAR_VOIDPTR);
  register_argtype("building", NULL, NULL, VAR_VOIDPTR);
  register_argtype("direction", NULL, NULL, VAR_INT);
  register_argtype("faction", NULL, NULL, VAR_VOIDPTR);
  register_argtype("race", NULL, NULL, VAR_VOIDPTR);
  register_argtype("region", NULL, NULL, VAR_VOIDPTR);
  register_argtype("resource", NULL, NULL, VAR_VOIDPTR);
  register_argtype("ship", NULL, NULL, VAR_VOIDPTR);
  register_argtype("skill", NULL, NULL, VAR_VOIDPTR);
  register_argtype("spell", NULL, NULL, VAR_VOIDPTR);
  register_argtype("unit", NULL, NULL, VAR_VOIDPTR);
  register_argtype("int", NULL, NULL, VAR_INT);
  register_argtype("string", var_free_string, var_copy_string, VAR_VOIDPTR);
  register_argtype("order", var_free_order, var_copy_order, VAR_VOIDPTR);
  register_argtype("resources", var_free_resources, NULL, VAR_VOIDPTR);
  register_argtype("items", var_free_resources, var_copy_items, VAR_VOIDPTR);

  /* register alternative visibility functions */
  register_function((pf_generic)view_neighbours, "view_neighbours");
  register_function((pf_generic)view_regatta, "view_regatta");
}
