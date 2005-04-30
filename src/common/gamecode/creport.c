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
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <eressea.h>
#include "creport.h"

/* tweakable features */
#define ENCODE_SPECIAL 1
#define RENDER_CRMESSAGES 1

/* modules include */
#include <modules/score.h>

/* attributes include */
#include <attributes/follow.h>
#include <attributes/racename.h>
#include <attributes/orcification.h>
#include <attributes/otherfaction.h>
#include <attributes/raceprefix.h>

/* gamecode includes */
#include "laws.h"
#include "economy.h"

/* kernel includes */
#include <kernel/alchemy.h>
#include <kernel/alliance.h>
#include <kernel/border.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/karma.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/movement.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/resources.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/teleport.h>
#include <kernel/unit.h>
#include <kernel/save.h>

/* util includes */
#include <util/message.h>
#include <goodies.h>
#include <crmessage.h>
#include <nrmessage.h>
#include <base36.h>
#include <language.h>

/* libc includes */
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* imports */
extern const char *directions[];
extern const char *spelldata[];
extern int quiet;
boolean opt_cr_absolute_coords = false;

/* globals */
#define C_REPORT_VERSION 64

#define TAG_LOCALE "de"
#ifdef TAG_LOCALE
static const char *
crtag(const char * key)
{
	static const struct locale * lang = NULL;
	if (!lang) lang = find_locale(TAG_LOCALE);
	return locale_string(lang, key);
}
#else
#define crtag(x) (x)
#endif
/*
 * translation table
 */
typedef struct translation {
	struct translation * next;
	char * key;
	const char * value;
} translation;

#define TRANSMAXHASH 257
static translation * translation_table[TRANSMAXHASH];
static translation * junkyard;

static const char *
add_translation(const char * key, const char * value)
{
	int kk = ((key[0] << 5) + key[0]) % TRANSMAXHASH;
	translation * t = translation_table[kk];
	while (t && strcmp(t->key, key)!=0) t=t->next;
	if (!t) {
		if (junkyard) {
			t = junkyard;
			junkyard = junkyard->next;
		} else t = malloc(sizeof(translation));
		t->key = strdup(key);
		t->value = value;
		t->next = translation_table[kk];
		translation_table[kk] = t;
	}
	return crtag(key);
}

static void
write_translations(FILE * F)
{
	int i;
	fputs("TRANSLATION\n", F);
	for (i=0;i!=TRANSMAXHASH;++i) {
		translation * t = translation_table[i];
		while (t) {
			fprintf(F, "\"%s\";%s\n", t->value, crtag(t->key));
			t = t->next;
		}
	}
}

static void
reset_translations(void)
{
	int i;
	for (i=0;i!=TRANSMAXHASH;++i) {
		translation * t = translation_table[i];
		while (t) {
			translation * c = t->next;
			free(t->key);
			t->next = junkyard;
			junkyard = t;
			t = c;
		}
		translation_table[i] = 0;
	}
}

/* implementation */
static void
cr_output_str_list(FILE * F, const char *title, const strlist * S, const faction * f)
{
	if (!S) return;

	fprintf(F, "%s\n", title);
	while (S) {
		fprintf(F, "\"%s\"\n", S->s);
		S = S->next;
	}
}

#include "objtypes.h"

static void
print_curses(FILE * F, const faction * viewer, const void * obj, typ_t typ)
{
	boolean header = false;
	attrib *a = NULL;
	int self = 0;
	region *r;

	/* Die Sichtbarkeit eines Zaubers und die Zaubermeldung sind bei
	 * Gebäuden und Schiffen je nach, ob man Besitzer ist, verschieden.
	 * Bei Einheiten sieht man Wirkungen auf eigene Einheiten immer.
	 * Spezialfälle (besonderes Talent, verursachender Magier usw. werde
	 * bei jedem curse gesondert behandelt. */
	if (typ == TYP_SHIP){
		ship * sh = (ship*)obj;
		unit * owner = shipowner(sh);
		a = sh->attribs;
		r = sh->region;
		if(owner != NULL) {
			if (owner->faction == viewer){
				self = 2;
			} else { /* steht eine person der Partei auf dem Schiff? */
				unit *u = NULL;
				for (u = r->units; u; u = u->next) {
					if (u->ship == sh) {
						self = 1;
						break;
					}
				}
			}
		}
	} else if (typ == TYP_BUILDING) {
		building * b = (building*)obj;
		unit * owner;
		a = b->attribs;
		r = b->region;
		if((owner = buildingowner(r,b)) != NULL){
			if (owner->faction == viewer){
				self = 2;
			} else { /* steht eine Person der Partei in der Burg? */
				unit *u = NULL;
				for (u = r->units; u; u = u->next) {
					if (u->building == b) {
						self = 1;
						break;
					}
				}
			}
		}
	} else if (typ == TYP_UNIT) {
		unit *u = (unit *)obj;
		a = u->attribs;
		r = u->region;
		if (u->faction == viewer){
			self = 2;
		}
	} else if (typ == TYP_REGION) {
		r = (region *)obj;
		a = r->attribs;
	} else {
		/* fehler */
	}

	while (a) {
		int dh = 0;
		curse *c;

		if (fval(a->type, ATF_CURSE)) {

			c = (curse *)a->data.v;
			if (c->type->curseinfo) {
				if (c->type->cansee) {
					self = c->type->cansee(viewer, obj, typ, c, self);
				}
				dh = c->type->curseinfo(viewer->locale, obj, typ, c, self);
      }
      if (dh==0) {
        if (c->type->info_str!=NULL) {
          strcpy(buf, c->type->info_str);
        } else {
          sprintf(buf, "an unknown curse lies on the region. (%s)", itoa36(c->no));
        }
      }
			if (dh==1) {
				if (!header) {
					header = 1;
					fputs("EFFECTS\n", F);
				}
				fprintf(F, "\"%s\"\n", buf);
			}
		} else if (a->type==&at_effect && self) {
			effect_data * data = (effect_data *)a->data.v;
			const char * key = resourcename(data->type->itype->rtype, 0);
			if (!header) {
				header = 1;
				fputs("EFFECTS\n", F);
			}
			fprintf(F, "\"%d %s\"\n", data->value, add_translation(key, locale_string(default_locale, key)));
		}
		a = a->next;
	}
}

static int
cr_unit(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	unit * u = (unit *)v;
	sprintf(buffer, "%d", u?u->no:-1);
	unused(report);
	return 0;
}

static int
cr_ship(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	ship * u = (ship *)v;
	sprintf(buffer, "%d", u?u->no:-1);
	unused(report);
	return 0;
}

static int
cr_building(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	building * u = (building *)v;
	sprintf(buffer, "%d", u?u->no:-1);
	unused(report);
	return 0;
}

static int
cr_faction(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	faction * f = (faction *)v;
	sprintf(buffer, "%d", f?f->no:-1);
	unused(report);
	return 0;
}

static int
cr_region(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	region * r = (region *)v;
	if (r) {
		plane * p = rplane(r);
		if (!p || !(p->flags & PFL_NOCOORDS)) {
			sprintf(buffer, "%d %d %d", region_x(r, report), region_y(r, report), p?p->id:0);
			return 0;
		}
	}
	return -1;
}

static int
cr_resource(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	const resource_type * r = (const resource_type *)v;
	if (r) {
		const char * key = resourcename(r, 0);
		sprintf(buffer, "\"%s\"",
			add_translation(key, locale_string(report->locale, key)));
		return 0;
	}
	return -1;
}

static int
cr_race(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	const struct race * rc = (const race *)v;
	const char * key = rc_name(rc, 0);
	sprintf(buffer, "\"%s\"",
		add_translation(key, locale_string(report->locale, key)));
	return 0;
}

static int
cr_alliance(const void * v, char * buffer, const void * userdata)
{
	const alliance * al = (const alliance *)v;
	if (al!=NULL) {
		sprintf(buffer, "%d", al->id);
	}
	unused(userdata);
	return 0;
}

static int
cr_skill(const void * v, char * buffer, const void * userdata)
{
  const faction * report = (const faction*)userdata;
  skill_t sk = (skill_t)(int)v;
  if (sk!=NOSKILL) sprintf(buffer, "\"%s\"",
    add_translation(skillname(sk, NULL), skillname(sk, report->locale)));
  else strcpy(buffer, "\"\"");
  return 0;
}

static int
cr_order(const void * v, char * buffer, const void * userdata)
{
  order * ord = (order*)v;
  if (ord!=NULL) {
    char * wp = buffer;
    char * cmd = getcommand(ord);
    const char * rp = cmd;
    
    *wp++ = '\"';
    while (*rp) {
      switch (*rp) {
      case '\"':
      case '\\':
        *wp++ = '\\';
      default:
        *wp++ = *rp++;
      }
    }
    *wp++ = '\"';
    *wp++ = 0;
    /*	 sprintf(buffer, "\"%s\"", cmd); */
  }
  else strcpy(buffer, "\"\"");
  return 0;
}

static int
cr_spell(const void * v, char * buffer, const void * userdata)
{
  const faction * report = (const faction*)userdata;
  spell * sp = (spell*)v;
  if (sp!=NULL) sprintf(buffer, "\"%s\"", spell_name(sp, report->locale));
  else strcpy(buffer, "\"\"");
  return 0;
}

void
creport_init(void)
{
  tsf_register("report", &cr_ignore);
  tsf_register("string", &cr_string);
  tsf_register("order", &cr_order);
  tsf_register("spell", &cr_spell);
  tsf_register("int", &cr_int);
  tsf_register("unit", &cr_unit);
  tsf_register("region", &cr_region);
  tsf_register("faction", &cr_faction);
  tsf_register("ship", &cr_ship);
  tsf_register("building", &cr_building);
  tsf_register("skill", &cr_skill);
  tsf_register("resource", &cr_resource);
  tsf_register("race", &cr_race);
  tsf_register("direction", &cr_int);
  tsf_register("alliance", &cr_alliance);
}

void
creport_cleanup(void)
{
  while (junkyard) {
    translation * t = junkyard;
    junkyard = junkyard->next;
    free(t);
  }
  junkyard = 0;
}

/*static int msgno; */

#define MTMAXHASH 1021

static struct known_mtype {
  const struct message_type * mtype;
  struct known_mtype * nexthash;
} * mtypehash[MTMAXHASH];

static void
report_crtypes(FILE * F, const struct locale* lang)
{
	int i;
	for (i=0;i!=MTMAXHASH;++i) {
		struct known_mtype * kmt;
		for (kmt=mtypehash[i];kmt;kmt=kmt->nexthash) {
			const struct nrmessage_type * nrt = nrt_find(lang, kmt->mtype);
			if (nrt) {
				unsigned int hash = hashstring(mt_name(kmt->mtype));
        fprintf(F, "MESSAGETYPE %d\n", hash);
        fputc('\"', F);
				fputs(escape_string(nrt_string(nrt), NULL, 0), F);
				fputs("\";text\n", F);
				fprintf(F, "\"%s\";section\n", nrt_section(nrt));
			}
		}
		while (mtypehash[i]) {
			kmt = mtypehash[i];
			mtypehash[i] = mtypehash[i]->nexthash;
			free(kmt);
		}
	}
}

static void
render_messages(FILE * F, faction * f, message_list *msgs)
{
	struct mlist* m = msgs->begin;
	while (m) {
		char crbuffer[1024*32]; /* gross, wegen spionage-messages :-( */
		boolean printed = false;
		const struct message_type * mtype = m->msg->type;
		unsigned int hash = hashstring(mtype->name);
#if RENDER_CRMESSAGES
		char nrbuffer[1024*32];
		nrbuffer[0] = '\0';
		if (nr_render(m->msg, f->locale, nrbuffer, sizeof(nrbuffer), f)==0 && nrbuffer[0]) {
			fprintf(F, "MESSAGE %u\n", (unsigned int)m->msg);/*++msgno); */
			fprintf(F, "%d;type\n", hash);
      fwritestr(F, nrbuffer);
			fputs(";rendered\n", F);
			printed = true;
		}
#endif
		crbuffer[0] = '\0';
		if (cr_render(m->msg, crbuffer, (const void*)f)==0) {
			if (!printed) fprintf(F, "MESSAGE %u\n", (unsigned int)m->msg);/*++msgno); */
			if (crbuffer[0]) fputs(crbuffer, F);
		} else {
			log_error(("could not render cr-message %p: %s\n", m->msg, m->msg->type->name));
		}
		if (printed) {
			unsigned int ihash = hash % MTMAXHASH;
			struct known_mtype * kmt = mtypehash[ihash];
			while (kmt && kmt->mtype != mtype) kmt = kmt->nexthash;
			if (kmt==NULL) {
				kmt = (struct known_mtype*)malloc(sizeof(struct known_mtype));
				kmt->nexthash = mtypehash[ihash];
				kmt->mtype = mtype;
				mtypehash[ihash] = kmt;
			}
		}
		m = m->next;
	}
}

static void
cr_output_messages(FILE * F, message_list *msgs, faction * f)
{
	if (msgs) render_messages(F, f, msgs);
}

/* prints a building */
static void
cr_output_buildings(FILE * F, building * b, unit * u, int fno, faction *f)
{
	const building_type * type = b->type;
	const char * bname = buildingtype(b, b->size);

	fprintf(F, "BURG %d\n", b->no);
	if (!u || u->faction != f) {
		const attrib * a = a_find(b->attribs, &at_icastle);
		if (a) type = ((icastle_data*)a->data.v)->type;
	}
	fprintf(F, "\"%s\";Typ\n", add_translation(bname, LOC(f->locale, bname)));
	fprintf(F, "\"%s\";Name\n", b->name);
	if (b->display && strlen(b->display))
		fprintf(F, "\"%s\";Beschr\n", b->display);
	if (b->size)
		fprintf(F, "%d;Groesse\n", b->size);
	if (u)
		fprintf(F, "%d;Besitzer\n", u ? u->no : -1);
	if (fno >= 0)
		fprintf(F, "%d;Partei\n", fno);
#ifdef TODO
	int cost = buildingdaten[b->type].per_size * b->size + buildingdaten[b->type].unterhalt;
	if (u && u->faction == f && cost)
		fprintf(F, "%d;Unterhalt\n", cost);
#endif
	if (b->besieged)
		fprintf(F, "%d;Belagerer\n", b->besieged);
	print_curses(F, f, b, TYP_BUILDING);
}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* prints a ship */
static void
cr_output_ship(FILE * F, const ship * sh, const unit * u, int fcaptain, const faction * f, const region * r)
{
	int w = 0;
	assert(sh);
	fprintf(F, "SCHIFF %d\n", sh->no);
	fprintf(F, "\"%s\";Name\n", sh->name);
	if (sh->display && strlen(sh->display))
		fprintf(F, "\"%s\";Beschr\n", sh->display);
	fprintf(F, "\"%s\";Typ\n", add_translation(sh->type->name[0], locale_string(f->locale, sh->type->name[0])));
	fprintf(F, "%d;Groesse\n", sh->size);
	if (sh->damage) {
		int percent = sh->damage*100/(sh->size*DAMAGE_SCALE);
		fprintf(F, "%d;Schaden\n", percent);
	}
	if (u)
		fprintf(F, "%d;Kapitaen\n", u ? u->no : -1);
	if (fcaptain >= 0)
		fprintf(F, "%d;Partei\n", fcaptain);

	/* calculate cargo */
	if (u && (u->faction == f || omniscient(f))) {
    int n = 0, p = 0;
    getshipweight(sh, &n, &p);
    n = (n+99) / 100; /* 1 Silber = 1 GE */

		fprintf(F, "%d;Ladung\n", n);
		fprintf(F, "%d;MaxLadung\n", shipcapacity(sh) / 100);
	}
	/* shore */
	w = NODIRECTION;
	if (rterrain(r) != T_OCEAN) w = sh->coast;
	if (w != NODIRECTION)
		fprintf(F, "%d;Kueste\n", w);

	print_curses(F, f, sh, TYP_SHIP);
}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* prints all that belongs to a unit */
static void
cr_output_unit(FILE * F, const region * r,
	       const faction * f,	/* observers faction */
	       const unit * u, int mode)
{
	/* Race attributes are always plural and item attributes always
	 * singular */
	const item_type * lasttype;
	int pr;
	item *itm, *show;
	boolean itemcloak = is_cursed(u->attribs, C_ITEMCLOAK, 0);
	building * b;
	const char * pzTmp;
	skill * sv;
	const attrib *a_fshidden = NULL;

	assert(u);

	if(fspecial(u->faction, FS_HIDDEN))
		a_fshidden = a_find(u->attribs, &at_fshidden);

	fprintf(F, "EINHEIT %d\n", u->no);
	fprintf(F, "\"%s\";Name\n", u->name);
	if (u->display && strlen(u->display))
		fprintf(F, "\"%s\";Beschr\n", u->display);

	{
		/* print faction information */
		const faction * sf = visible_faction(f, u);
		const char * prefix = raceprefix(u);
		if (u->faction == f || omniscient(f)) {
			const attrib * a_otherfaction = a_find(u->attribs, &at_otherfaction);
			const faction * otherfaction = a_otherfaction?get_otherfaction(a_otherfaction):NULL;
			/* my own faction, full info */
			const attrib *a = a_find(u->attribs, &at_group);
			if (a!=NULL) {
				const group * g = (const group*)a->data.v;
				fprintf(F, "%d;gruppe\n", g->gid);
			} 
			fprintf(F, "%d;Partei\n", u->faction->no);
			if (sf!=u->faction) fprintf(F, "%d;Verkleidung\n", sf->no);
			if (fval(u, UFL_PARTEITARNUNG))
				fprintf(F, "%d;Parteitarnung\n", i2b(fval(u, UFL_PARTEITARNUNG)));
      if (otherfaction) {
        if (otherfaction!=u->faction) {
          fprintf(F, "%d;Anderepartei\n", otherfaction->no);
        }
      }
    } else {
			if (fval(u, UFL_PARTEITARNUNG)) {
				/* faction info is hidden */
				fprintf(F, "%d;Parteitarnung\n", i2b(fval(u, UFL_PARTEITARNUNG)));
			} else {
				const attrib * a_otherfaction = a_find(u->attribs, &at_otherfaction);
				const faction * otherfaction = a_otherfaction?get_otherfaction(a_otherfaction):NULL;
				/* other unit. show visible faction, not u->faction */
				fprintf(F, "%d;Partei\n", sf->no);
				if (sf == f) {
					fprintf(F, "1;Verraeter\n");
				}
        if (a_otherfaction) {
          if (otherfaction!=u->faction) {
            if (alliedunit(u, f, HELP_FSTEALTH)) {
              fprintf(F, "%d;Anderepartei\n", otherfaction->no);
            }
          }
				}
			}
		}
		if (prefix) {
      prefix = mkname("prefix", prefix);
			fprintf(F, "\"%s\";typprefix\n", add_translation(prefix, LOC(f->locale, prefix)));
		}
	}
	if (u->faction != f && a_fshidden
			&& a_fshidden->data.ca[0] == 1 && effskill(u, SK_STEALTH) >= 6) {
		fprintf(F, "-1;Anzahl\n");
	} else {
		fprintf(F, "%d;Anzahl\n", u->number);
	}

  pzTmp = get_racename(u->attribs);
  if (pzTmp) {
    fprintf(F, "\"%s\";Typ\n", pzTmp);
    if (u->faction==f && fval(u->race, RCF_SHAPESHIFTANY)) {
      const char * zRace = rc_name(u->race, 1);
      fprintf(F, "\"%s\";wahrerTyp\n", 
        add_translation(zRace, locale_string(f->locale, zRace)));
    }
  } else {
    const char * zRace = rc_name(u->irace, 1);
    fprintf(F, "\"%s\";Typ\n", 
      add_translation(zRace, locale_string(f->locale, zRace)));
    if (u->faction==f && u->irace!=u->race) {
      zRace = rc_name(u->race, 1);
      fprintf(F, "\"%s\";wahrerTyp\n", 
        add_translation(zRace, locale_string(f->locale, zRace)));
    }
  }

  if (u->building)
		fprintf(F, "%d;Burg\n", u->building->no);
	if (u->ship)
		fprintf(F, "%d;Schiff\n", u->ship->no);
	if (getguard(u))
		fprintf(F, "%d;bewacht\n", getguard(u)?1:0);
	if ((b=usiege(u))!=NULL)
		fprintf(F, "%d;belagert\n", b->no);

	/* additional information for own units */
  if (u->faction == f || omniscient(f)) {
    order * ord;
    const char *c;
    int i;
    const attrib * a;

    a = a_find(u->attribs, &at_follow);
    if (a) {
      unit * u = (unit*)a->data.v;
      if (u) fprintf(F, "%d;folgt\n", u->no);
    }
    i = ualias(u);
    if (i>0)
      fprintf(F, "%d;temp\n", i);
    else if (i<0)
      fprintf(F, "%d;alias\n", -i);
    i = get_money(u);
    fprintf(F, "%d;Kampfstatus\n", u->status);
    if(fval(u, UFL_NOAID)) {
      fputs("1;unaided\n", F);
    }
    i = u_geteffstealth(u);
    if (i >= 0) {
      fprintf(F, "%d;Tarnung\n", i);
    }
    c = uprivate(u);
    if (c) {
      fprintf(F, "\"%s\";privat\n", c);
    }
    c = hp_status(u);
    if (c && *c && (u->faction == f || omniscient(f))) {
      fprintf(F, "\"%s\";hp\n", add_translation(c, locale_string(u->faction->locale, c)));
    }
#ifdef HEROES
    if (fval(u, UFL_HERO)) {
      fputs("1;hero\n", F);
    }
#endif

    if (fval(u, UFL_HUNGER) && (u->faction == f)) {
      fputs("1;hunger\n", F);
    }
    if (is_mage(u)) {
      fprintf(F, "%d;Aura\n", get_spellpoints(u));
      fprintf(F, "%d;Auramax\n", max_spellpoints(u->region,u));
    }
    /* default commands */
    fprintf(F, "COMMANDS\n");
    if (u->lastorder) {
      fwriteorder(F, u->lastorder, f->locale);
      fputc('\n', F);
    }
    for (ord = u->orders; ord; ord = ord->next) {
      if (is_persistent(ord) && ord!=u->lastorder) {
        fwriteorder(F, ord, f->locale);
        fputc('\n', F);
      }
    }

    /* talents */
    pr = 0;
    for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
      if (sv->level>0) {
        skill_t sk = sv->id;
        int esk = eff_skill(u, sk, r);
        if (!pr) {
          pr = 1;
          fprintf(F, "TALENTE\n");
        }
        fprintf(F, "%d %d;%s\n", u->number*level_days(sv->level), esk,
          add_translation(skillname(sk, NULL), skillname(sk, f->locale)));
      }
    }
    /* spells */
    if (is_mage(u)) {
      sc_mage * mage = get_mage(u);
      spell_ptr *spt = mage->spellptr;
      if (spt) {
        spell *sp;
        int i;
        int t = effskill(u, SK_MAGIC);
        fprintf(F, "SPRUECHE\n");
        for (;spt; spt = spt->next) {
          sp = find_spellbyid(spt->spellid);
          if (sp) {
            const char * name = sp->sname;
            if (sp->level > t) continue;
            if (sp->info==NULL) {
              name = add_translation(mkname("spell", name), spell_name(sp, f->locale));
            }
            fprintf(F, "\"%s\"\n", name);
          }
        }
        for (i=0;i!=MAXCOMBATSPELLS;++i) {
          sp = find_spellbyid(mage->combatspell[i]);
          if (sp) {
            const char * name = sp->sname;
            if (sp->info==NULL) {
              name = add_translation(mkname("spell", name), spell_name(sp, f->locale));
            }
            fprintf(F, "KAMPFZAUBER %d\n", i);
            fprintf(F, "\"%s\";name\n", name);
            fprintf(F, "%d;level\n", mage->combatspelllevel[i]);
          }
        }
      }
    }
  }
	/* items */
	pr = 0;
	if (f == u->faction || omniscient(u->faction)) {
		show = u->items;
	} else if (itemcloak==false && mode>=see_unit && !(a_fshidden
			&& a_fshidden->data.ca[1] == 1 && effskill(u, SK_STEALTH) >= 3)) {
		show = NULL;
		for (itm=u->items;itm;itm=itm->next) {
			item * ishow;
			const char * ic;
			int in;
			report_item(u, itm, f, NULL, &ic, &in, true);
			if (in>0 && ic && *ic) {
				for (ishow = show; ishow; ishow=ishow->next) {
					const char * sc;
					int sn;
					if (ishow->type==itm->type) sc=ic;
					else report_item(u, ishow, f, NULL, &sc, &sn, true);
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
	lasttype = NULL;
	for (itm=show; itm; itm=itm->next) {
		const char * ic;
		int in;
		assert(itm->type!=lasttype || !"error: list contains two objects of the same item");
		report_item(u, itm, f, NULL, &ic, &in, true);
		if (in==0) continue;
		if (!pr) {
			pr = 1;
			fputs("GEGENSTAENDE\n", F);
		}
		fprintf(F, "%d;%s\n", in, add_translation(ic, locale_string(f->locale, ic)));
	}
  if (show!=u->items) {
    /* free the temporary items */
    while (show) {
      i_free(i_remove(&show, show));
    }
  }
    
  if ((u->faction == f || omniscient(f)) && u->botschaften)
      cr_output_str_list(F, "EINHEITSBOTSCHAFTEN", u->botschaften, f);
  
  print_curses(F, f, u, TYP_UNIT);
}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* prints allies */
static void
show_allies(FILE * F, const faction * f, const ally * sf)
{
	for (; sf; sf = sf->next) if (sf->faction) {
		int mode = alliedgroup(NULL, f, sf->faction, sf, HELP_ALL);
		if (mode!=0 && sf->status>0) {
			fprintf(F, "ALLIANZ %d\n", sf->faction->no);
			fprintf(F, "\"%s\";Parteiname\n", sf->faction->name);
			fprintf(F, "%d;Status\n", sf->status & HELP_ALL);
		}
	}
}

#ifdef REGIONOWNERS
static void
show_enemies(FILE * F, const faction_list* flist)
{
  for (;flist!=NULL;flist=flist->next) {
    if (flist->data) {
      int fno = flist->data->no;
      fprintf(F, "ENEMY %u\n%u;partei\n", fno, fno);
    }
  }
}
#endif

/* prints all visible spells in a region */
static void
show_active_spells(const region * r)
{
	char fogwall[MAXDIRECTIONS];
#ifdef TODO /* alte Regionszauberanzeigen umstellen */
	unit *u;
	int env = 0;
#endif
	memset(fogwall, 0, sizeof(char) * MAXDIRECTIONS);

}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* this is a copy of laws.c->find_address output changed. */
static void
cr_find_address(FILE * F, const faction * uf, const faction_list * addresses)
{
	const faction_list * flist = addresses;
	while (flist!=NULL) {
		const faction * f = flist->data;
		if (uf!=f && f->no != MONSTER_FACTION) {
			fprintf(F, "PARTEI %d\n", f->no);
			fprintf(F, "\"%s\";Parteiname\n", f->name);
			fprintf(F, "\"%s\";email\n", f->email);
			fprintf(F, "\"%s\";banner\n", f->banner);
			if (f->alliance!=NULL && f->alliance==uf->alliance) {
				fprintf(F, "%d;alliance\n", f->alliance->id);
				fprintf(F, "\"%s\";alliancename\n", f->alliance->name);
			}
#ifdef SHORTPWDS
      if (f->shortpwds) {
        shortpwd * spwd = f->shortpwds;
        while (spwd) {
          unsigned int vacation = 0;
          if (spwd->used) {
            fprintf(F, "VACATION %u\n", ++vacation);
            fprintf(F, "\"%s\";email\n", spwd->email);
          }
          spwd=spwd->next;
        }
      }
#endif
		}
		flist = flist->next;
	}
}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

static void
cr_reportspell(FILE * F, spellid_t id, const struct locale * lang)
{
	int k;
	spell *sp = find_spellbyid(id);
	const char * name = sp->sname;
	if (sp->info==NULL) {
		name = add_translation(mkname("spell", name), spell_name(sp, lang));
	}

	fprintf(F, "ZAUBER %d\n", hashstring(spell_name(sp, default_locale)));
	fprintf(F, "\"%s\";name\n", name);
	fprintf(F, "%d;level\n", sp->level);
	fprintf(F, "%d;rank\n", sp->rank);
	fprintf(F, "\"%s\";info\n", spell_info(sp, lang));

	if (sp->sptyp & PRECOMBATSPELL) fputs("\"precombat\";class\n", F);
	else if (sp->sptyp & COMBATSPELL) fputs("\"combat\";class\n", F);
	else if (sp->sptyp & POSTCOMBATSPELL) fputs("\"postcombat\";class\n", F);
	else fputs("\"normal\";class\n", F);

	if (sp->sptyp & FARCASTING) fputs("1;far\n", F);
	if (sp->sptyp & OCEANCASTABLE) fputs("1;ocean\n", F);
	if (sp->sptyp & ONSHIPCAST) fputs("1;ship\n", F);
	if (!(sp->sptyp & NOTFAMILIARCAST)) fputs("1;familiar\n", F);
	fputs("KOMPONENTEN\n", F);

	for (k = 0; k < MAXINGREDIENT; k++) {
		resource_t res = sp->komponenten[k][0];
		int itemanz = sp->komponenten[k][1];
		int costtyp = sp->komponenten[k][2];
		if (itemanz > 0) {
			const char * name = resname(res, 0);
			fprintf(F, "%d %d;%s\n", itemanz, costtyp == SPC_LEVEL || costtyp == SPC_LINEAR,
				add_translation(name, LOC(lang, name)));
		}
	}
}

static unsigned int
encode_region(const faction * f, const region * r) {
	unsigned int id;
	char *cp, c;
	/* obfuscation */
	assert(sizeof(int)==sizeof(char)*4);
	id = (((((r->x ^ f->no) % 1024) << 20) | ((r->y ^ f->no) % 1024)));
	cp = (char*)&id;
	c = cp[0];
	cp[0] = cp[2];
	cp[2] = cp[1];
	cp[1] = cp[3];
	cp[3] = c;
	return id;
}

static char *
report_resource(char * buf, const char * name, const struct locale * loc, int amount, int level)
{
	buf += sprintf(buf, "RESOURCE %u\n", hashstring(name));
	buf += sprintf(buf, "\"%s\";type\n", add_translation(name, LOC(loc, name)));
	if (amount>=0) {
		if (level>=0) buf += sprintf(buf, "%d;skill\n", level);
		buf += sprintf(buf, "%d;number\n", amount);
	}
	return buf;
}

static void
cr_borders(seen_region ** seen, const region * r, const faction * f, int seemode, FILE * F)
{
	direction_t d;
	int g = 0;
	for (d = 0; d != MAXDIRECTIONS; d++)
	{ /* Nachbarregionen, die gesehen werden, ermitteln */
		const region * r2 = rconnect(r, d);
		const border * b;
		if (!r2) continue;
		if (seemode==see_neighbour) {
			seen_region * sr = find_seen(seen, r2);
			if (sr==NULL || sr->mode<=see_neighbour) continue;
		}
		b = get_borders(r, r2);
		while (b) {
			boolean cs = b->type->fvisible(b, f, r);

			if (!cs) {
				cs = b->type->rvisible(b, r);
				if (!cs) {
					unit * us = r->units;
					while (us && !cs) {
						if (us->faction==f) {
							cs = b->type->uvisible(b, us);
							if (cs) break;
						}
						us=us->next;
					}
				}
			}
			if (cs) {
				fprintf(F, "GRENZE %d\n", ++g);
				fprintf(F, "\"%s\";typ\n", b->type->name(b, r, f, GF_NONE));
				fprintf(F, "%d;richtung\n", d);
				if (!b->type->transparent(b, f)) fputs("1;opaque\n", F);
				/* pfusch: */
				if (b->type==&bt_road) {
					int p = rroad(r, d)*100/terrain[rterrain(r)].roadreq;
					fprintf(F, "%d;prozent\n", p);
				}
			}
			b = b->next;
		}
	}
}

void
get_seen_interval(struct seen_region ** seen, region ** first, region ** last)
{
/* #ifdef ENUM_REGIONS : can speed stuff up */

  region * r = regions;
  while (r!=NULL) {
    if (find_seen(seen, r)!=NULL) {
      *first = r;
      break;
    }
    r = r->next;
  }
  while (r!=NULL) {
    if (find_seen(seen, r)!=NULL) {
      *last = r->next;
    }
    r = r->next;
  }
}

/* main function of the creport. creates the header and traverses all regions */
int
report_computer(FILE * F, faction * f, struct seen_region ** seen, const faction_list * addresses, 
                const time_t report_time)
{
	int i;
  item * itm;
  const char * prefix;
  region * r;
	building *b;
	ship *sh;
	unit *u;
  int score = 0, avgscore = 0;
	const char * mailto = locale_string(f->locale, "mailto");
  region * first = NULL, * last = NULL;
	const attrib * a;

  get_seen_interval(seen, &first, &last);

	/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
	/* initialisations, header and lists */

  fprintf(F, "VERSION %d\n", C_REPORT_VERSION);
  fprintf(F, "\"%s\";locale\n", locale_name(f->locale));
	fprintf(F, "%d;noskillpoints\n", 1);
	fprintf(F, "%ld;date\n", report_time);
	fprintf(F, "\"%s\";Spiel\n", global.gamename);
	fprintf(F, "\"%s\";Konfiguration\n", "Standard");
	fprintf(F, "\"%s\";Koordinaten\n", "Hex");
	fprintf(F, "%d;Basis\n", 36);
	fprintf(F, "%d;Runde\n", turn);
	fputs("2;Zeitalter\n", F);
	if (mailto!=NULL) {
		fprintf(F, "\"%s\";mailto\n", mailto);
		fprintf(F, "\"%s\";mailcmd\n", locale_string(f->locale, "mailcmd"));
	}
	fprintf(F, "PARTEI %d\n", f->no);
	fprintf(F, "\"%s\";locale\n", locale_name(f->locale));
	fprintf(F, "%d;Optionen\n", f->options);
	if (f->options & want(O_SCORE) && f->age>DISPLAYSCORE) {
    score = f->score;
    avgscore = average_score_of_age(f->age, f->age / 24 + 1);
  }
  fprintf(F, "%d;Punkte\n", score);
  fprintf(F, "%d;Punktedurchschnitt\n", avgscore);
	{
		const char * zRace = rc_name(f->race, 1);
		fprintf(F, "\"%s\";Typ\n", add_translation(zRace, LOC(f->locale, zRace)));
	}
  prefix = get_prefix(f->attribs);
	if (prefix!=NULL) {
    prefix = mkname("prefix", prefix);
		fprintf(F, "\"%s\";typprefix\n", 
      add_translation(prefix, LOC(f->locale, prefix)));
	}
	fprintf(F, "%d;Rekrutierungskosten\n", f->race->recruitcost);
	fprintf(F, "%d;Anzahl Personen\n", count_all(f));
	fprintf(F, "\"%s\";Magiegebiet\n", neue_gebiete[f->magiegebiet]);

	if (f->race == new_race[RC_HUMAN]) {
		fprintf(F, "%d;Anzahl Immigranten\n", count_migrants(f));
		fprintf(F, "%d;Max. Immigranten\n", count_maxmigrants(f));
	}

#ifdef HEROES
  i = countheroes(f);
  if (i>0) fprintf(F, "%d;heroes\n", i);
  i = maxheroes(f);
  if (i>0) fprintf(F, "%d;max_heroes\n", i);
#endif

  if (f->age > 1 && f->lastorders != turn) {
    fprintf(F, "%d;nmr\n", turn-f->lastorders);
  }

  fprintf(F, "\"%s\";Parteiname\n", f->name);
	fprintf(F, "\"%s\";email\n", f->email);
	fprintf(F, "\"%s\";banner\n", f->banner);
  for (itm=f->items; itm; itm=itm->next) {
    int in = itm->number;
    const char * ic = LOC(f->locale, resourcename(itm->type->rtype, in));
    if (itm==f->items) fputs("GEGENSTAENDE\n", F);
    fprintf(F, "%d;%s\n", in, add_translation(ic, LOC(f->locale, ic)));
  }
	fputs("OPTIONEN\n", F);
	for (i=0;i!=MAXOPTIONS;++i) {
		fprintf(F, "%d;%s\n", (f->options&want(i))?1:0, options[i]);
	}
#ifdef REGIONOWNERS
  show_enemies(F, f->enemies);
#endif
	show_allies(F, f, f->allies);
	{
		group * g;
		for (g=f->groups;g;g=g->next) {

			fprintf(F, "GRUPPE %d\n", g->gid);
			fprintf(F, "\"%s\";name\n", g->name);
      prefix = get_prefix(g->attribs);
			if (prefix!=NULL) {
        prefix = mkname("prefix", prefix);
				fprintf(F, "\"%s\";typprefix\n", 
          add_translation(prefix, LOC(f->locale, prefix)));
			}
			show_allies(F, f, g->allies);
		}
	}

	cr_output_str_list(F, "FEHLER", f->mistakes, f);
	cr_output_messages(F, f->msgs, f);
	{
		struct bmsg * bm;
		for (bm=f->battles;bm;bm=bm->next) {
			if (!rplane(bm->r)) fprintf(F, "BATTLE %d %d\n", region_x(bm->r, f), region_y(bm->r, f));
			else {
				if (rplane(bm->r)->flags & PFL_NOCOORDS) fprintf(F, "BATTLESPEC %d %d\n", encode_region(f, bm->r), rplane(bm->r)->id);
				else fprintf(F, "BATTLE %d %d %d\n", region_x(bm->r, f), region_y(bm->r, f), rplane(bm->r)->id);
			}
			cr_output_messages(F, bm->msgs, f);
		}
	}

	cr_find_address(F, f, addresses);
	a = a_find(f->attribs, &at_reportspell);
	while (a) {
		cr_reportspell(F, (spellid_t)a->data.i, f->locale);
		a = a->nexttype;
	}
	for (a=a_find(f->attribs, &at_showitem);a;a=a->nexttype) {
		const potion_type * ptype = resource2potion(((const item_type*)a->data.v)->rtype);
		requirement * m;
		const char * ch, * description;

    if (ptype==NULL) continue;
		m = ptype->itype->construction->materials;
		ch = resourcename(ptype->itype->rtype, 0);
		fprintf(F, "TRANK %d\n", hashstring(ch));
		fprintf(F, "\"%s\";Name\n", add_translation(ch, locale_string(f->locale, ch)));
		fprintf(F, "%d;Stufe\n", ptype->level);

    description = ptype->text;
    if (description==NULL || f->locale!=find_locale("de")) {
      const char * pname = resourcename(ptype->itype->rtype, 0);
      const char * potiontext = mkname("potion", pname);
      description = LOC(f->locale, potiontext);
      if (strcmp(description, potiontext)==0) {
        /* string not found */
        description = ptype->text;
      }
    }
    
    fprintf(F, "\"%s\";Beschr\n", description);
		fprintf(F, "ZUTATEN\n");

		while (m->number) {
			ch = resourcename(oldresourcetype[m->type], 0);
			fprintf(F, "\"%s\"\n", add_translation(ch, locale_string(f->locale, ch)));
			m++;
		}
	}

	/* traverse all regions */
	for (r=first;r!=last;r=r->next) {
		int modifier = 0;
		const char * tname;
    const seen_region * sd = find_seen(seen, r);

    if (sd==NULL) continue;
		
		if (!rplane(r)) {
			if (opt_cr_absolute_coords) {
				fprintf(F, "REGION %d %d\n", r->x, r->x);
			} else {
				fprintf(F, "REGION %d %d\n", region_x(r, f), region_y(r, f));
			}
		} else {
#if ENCODE_SPECIAL
			if (rplane(r)->flags & PFL_NOCOORDS) fprintf(F, "SPEZIALREGION %d %d\n", encode_region(f, r), rplane(r)->id);
#else
			if (rplane(r)->flags & PFL_NOCOORDS) continue;
#endif
			else fprintf(F, "REGION %d %d %d\n", region_x(r, f), region_y(r, f), rplane(r)->id);
		}
		if (r->land && strlen(rname(r, f->locale))) fprintf(F, "\"%s\";Name\n", rname(r, f->locale));
		if (is_cursed(r->attribs,C_MAELSTROM, 0))
			tname = "maelstrom";
		else {
			if (r_isforest(r)) tname = "forest";
			else tname = terrain[rterrain(r)].name;
		}

		fprintf(F, "\"%s\";Terrain\n", add_translation(tname, locale_string(f->locale, tname)));
		switch (sd->mode) {
		case see_far:
			fputs("\"neighbourhood\";visibility\n", F);
			break;
		case see_lighthouse:
			fputs("\"lighthouse\";visibility\n", F);
			break;
		case see_travel:
			fputs("\"travel\";visibility\n", F);
			break;
		}
    {
      unit * owner = region_owner(r);
      if (owner) {
        fprintf(F, "%d;owner\n", owner->faction->no);
      }
    }
		if (sd->mode == see_neighbour) {
			cr_borders(seen, r, f, sd->mode, F);
		} else {
#define RESOURCECOMPAT
			char cbuf[8192], *pos = cbuf;
#ifdef RESOURCECOMPAT
			if (r->display && strlen(r->display))
				fprintf(F, "\"%s\";Beschr\n", r->display);
#endif
			if (landregion(rterrain(r))) {
#if GROWING_TREES
				int trees = rtrees(r,2);
				int ytrees = rtrees(r,1);
# ifdef RESOURCECOMPAT
				if (trees > 0) fprintf(F, "%d;Baeume\n", trees);
				if (ytrees > 0) fprintf(F, "%d;Schoesslinge\n", ytrees);
				if (fval(r, RF_MALLORN) && (trees > 0 || ytrees > 0))
					fprintf(F, "1;Mallorn\n");
# endif
				if (!fval(r, RF_MALLORN)) {
					if (ytrees) pos = report_resource(pos, "rm_youngtrees", f->locale, ytrees, -1);
					if (trees) pos = report_resource(pos, "rm_trees", f->locale, trees, -1);
				} else {
					if (ytrees) pos = report_resource(pos, "rm_youngmallorn", f->locale, ytrees, -1);
					if (trees) pos = report_resource(pos, "rm_mallorn", f->locale, trees, -1);
				}
#else
				int trees = rtrees(r);
# ifdef RESOURCECOMPAT
				fprintf(F, "%d;Baeume\n", trees);
				if (fval(r, RF_MALLORN) && trees)
					fprintf(F, "1;Mallorn\n");
# endif
				if (!fval(r, RF_MALLORN)) {
					if (trees) pos = report_resource(pos, "rm_trees", f->locale, trees, -1);
				} else {
					if (trees) pos = report_resource(pos, "rm_mallorn", f->locale, trees, -1);
				}
#endif
				fprintf(F, "%d;Bauern\n", rpeasants(r));
				if(fval(r, RF_ORCIFIED)) {
					fprintf(F, "1;Verorkt\n");
				}
				fprintf(F, "%d;Pferde\n", rhorses(r));

				if (sd->mode>=see_unit) {
					struct demand * dmd = r->land->demands;
#if NEW_RESOURCEGROWTH
					struct rawmaterial * res = r->resources;
#endif
					fprintf(F, "%d;Silber\n", rmoney(r));
					fprintf(F, "%d;Unterh\n", entertainmoney(r));

					if (is_cursed(r->attribs, C_RIOT, 0)){
						fprintf(F, "0;Rekruten\n");
					} else {
						fprintf(F, "%d;Rekruten\n", rpeasants(r) / RECRUITFRACTION);
					}
					if (production(r)) {
						fprintf(F, "%d;Lohn\n", fwage(r, f, true));
					}

#if NEW_RESOURCEGROWTH
					while (res) {
						int maxskill = 0;
						int level = -1;
						int visible = -1;
						const item_type * itype = resource2item(res->type->rtype);
						if (res->type->visible==NULL) {
							visible = res->amount;
							level = res->level + itype->construction->minskill - 1;
						} else {
							const unit * u;
							for (u=r->units; visible!=res->amount && u!=NULL; u=u->next) {
								if (u->faction == f) {
									int s = eff_skill(u, itype->construction->skill, r);
									if (s>maxskill) {
										if (s>=itype->construction->minskill) {
											assert(itype->construction->minskill>0);
											level = res->level + itype->construction->minskill - 1;
										}
										maxskill = s;
										visible = res->type->visible(res, maxskill);
									}
								}
							}
						}
						if (level>=0 && visible >=0) {
							pos = report_resource(pos, res->type->name, f->locale, visible, level);
# ifdef RESOURCECOMPAT
							if (visible>=0) fprintf(F, "%d;%s\n", visible, crtag(res->type->name));
#endif
						}
						res = res->next;
					}
#else
					const unit * u;
					int maxmining = 0;
					for (u = r->units; u; u = u->next) {
						if (u->faction == f) {
							int s = eff_skill(u, SK_MINING, r);
							maxmining = max(maxmining, s);
						}
					}
					for (u = r->units; u; u = u->next) {
						if (u->faction == f) {
							int s = eff_skill(u, SK_MINING, r);
							maxmining = max(maxmining, s);
						}
					}
					if (maxmining >= 4 && riron(r) > 0)
						fprintf(F, "%d;Eisen\n", riron(r));
					if (maxmining >= 7 && rlaen(r) > 0)
						fprintf(F, "%d;Laen\n", rlaen(r));
#endif
					/* trade */
					if (!TradeDisabled() && rpeasants(r)/TRADE_FRACTION > 0) {
						fputs("PREISE\n", F);
						while (dmd) {
							const char * ch = resourcename(dmd->type->itype->rtype, 0);
							fprintf(F, "%d;%s\n", (dmd->value
									  ? dmd->value*dmd->type->price
									  : -dmd->type->price),
									  add_translation(ch, locale_string(f->locale, ch)));
							dmd=dmd->next;
						}
					}
				}
				if (pos!=cbuf) fputs(cbuf, F);
			}
			print_curses(F, f, r, TYP_REGION);
			cr_borders(seen, r, f, sd->mode, F);
			if (sd->mode==see_unit && rplane(r)==get_astralplane() && !is_cursed(r->attribs, C_ASTRALBLOCK, 0))
			{
				/* Sonderbehandlung Teleport-Ebene */
				region_list *rl = astralregions(r, inhabitable);

				if (rl) {
					region_list *rl2 = rl;
					while(rl2) {
						region * r = rl2->data;
						fprintf(F, "SCHEMEN %d %d\n", region_x(r, f), region_y(r, f));
						fprintf(F, "\"%s\";Name\n", rname(r, f->locale));
						rl2 = rl2->next;
						if(rl2) scat(", ");
					}
					free_regionlist(rl);
				}
			}

			/* describe both passed and inhabited regions */
			show_active_spells(r);
			{
				boolean seeunits = false, seeships = false;
				const attrib * ru;
				/* show units pulled through region */
				for (ru = a_find(r->attribs, &at_travelunit); ru; ru = ru->nexttype) {
					unit * u = (unit*)ru->data.v;
					if (cansee_durchgezogen(f, r, u, 0) && r!=u->region) {
						if (!u->ship || !fval(u, UFL_OWNER)) continue;
						if (!seeships) fprintf(F, "DURCHSCHIFFUNG\n");
						seeships = true;
						fprintf(F, "\"%s\"\n", shipname(u->ship));
					}
				}
				for (ru = a_find(r->attribs, &at_travelunit); ru; ru = ru->nexttype) {
					unit * u = (unit*)ru->data.v;
					if (cansee_durchgezogen(f, r, u, 0) && r!=u->region) {
						if (u->ship) continue;
						if (!seeunits) fprintf(F, "DURCHREISE\n");
						seeunits = true;
						fprintf(F, "\"%s\"\n", unitname(u));
					}
				}
			}
			cr_output_messages(F, r->msgs, f);
			{
				message_list * mlist = r_getmessages(r, f);
				if (mlist) cr_output_messages(F, mlist, f);
			}
			/* buildings */
			for (b = rbuildings(r); b; b = b->next) {
				int fno = -1;
				u = buildingowner(r, b);
				if (u && !fval(u, UFL_PARTEITARNUNG)) {
					const faction * sf = visible_faction(f,u);
					fno = sf->no;
				}
				cr_output_buildings(F, b, u, fno, f);
			}

			/* ships */
			for (sh = r->ships; sh; sh = sh->next) {
				int fno = -1;
				u = shipowner(sh);
				if (u && !fval(u, UFL_PARTEITARNUNG)) {
					const faction * sf = visible_faction(f,u);
					fno = sf->no;
				}

				cr_output_ship(F, sh, u, fno, f, r);
			}

			/* visible units */
			for (u = r->units; u; u = u->next) {
				boolean visible = true;
				switch (sd->mode) {
				case see_unit:
					modifier=0;
					break;
				case see_far:
				case see_lighthouse:
					modifier = -2;
					break;
				case see_travel:
					modifier = -1;
					break;
				default:
					visible=false;
				}
				if (u->building || u->ship || (visible && cansee(f, r, u, modifier)))
					cr_output_unit(F, r, f, u, sd->mode);
			}
		}			/* region traversal */
	}
	report_crtypes(F, f->locale);
	write_translations(F);
	reset_translations();
  if (errno) {
    log_error(("%s\n", strerror(errno)));
    errno = 0;
  }
  return 0;
}
