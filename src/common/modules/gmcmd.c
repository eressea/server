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
#include <eressea.h>
#include "gmcmd.h"
#include "command.h"

/* misc includes */
#include <items/demonseye.h>
#include <attributes/key.h>
#include <triggers/gate.h>
#include <triggers/unguard.h>

/* kernel includes */
#include <building.h>
#include <reports.h>
#include <faction.h>
#include <item.h>
#include <message.h>
#include <order.h>
#include <plane.h>
#include <region.h>
#include <terrain.h>
#include <unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/umlaut.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/**
 ** at_permissions
 **/

static void
write_permissions(const attrib * a, FILE * F)
{
	a_write(F, (attrib*)a->data.v);
}

static int
read_permissions(attrib * at, FILE * F)
{
	attrib ** p_a = (attrib**)&at->data.v;
	a_read(F, p_a);
	/* eliminate duplicates: */
	while (*p_a) {
		attrib * a = (*p_a)->next;
		while (a) {
			attrib * nexta = a->next;
			if (a->type == (*p_a)->type && a->data.v==(*p_a)->data.v) {
				static int print = 0;
				a_remove((attrib**)&at->data.v, a);
				if (!print) {
					log_error(("duplicated entries in permission structure\n"));
					print = 1;
				}
			}
			a = nexta;
		}
		p_a = &(*p_a)->next;
	}
	return AT_READ_OK;
}

struct attrib_type at_permissions = {
	"GM:permissions",
	NULL, NULL, NULL,
	write_permissions, read_permissions,
	ATF_UNIQUE
};

attrib *
make_atpermissions(void)
{
	return a_new(&at_permissions);
}

/**
 ** GM: CREATE <number> <itemtype>
 **/

static void
write_gmcreate(const attrib * a, FILE * F)
{
  const item_type * itype = (const item_type *)a->data.v;
  assert(itype);
  fprintf(F, "%s ", resourcename(itype->rtype, 0));
}

static int
read_gmcreate(attrib * a, FILE * F)
{
  char zText[32];
  const item_type ** p_itype = (const item_type **)&a->data.v;
  fscanf(F, "%s", zText);
  *p_itype = it_find(zText);
  if (a->data.v==NULL) {
    log_error(("unknown itemtype %s in gmcreate attribute\n", zText));
    return AT_READ_FAIL;
  }
  return AT_READ_OK;
}

/* at_gmcreate specifies that the owner can create items of a particular type */
attrib_type at_gmcreate = {
	"GM:create",
	NULL, NULL, NULL,
	write_gmcreate, read_gmcreate
};

attrib *
make_atgmcreate(const struct item_type * itype)
{
	attrib * a = a_new(&at_gmcreate);
	a->data.v = (void*)itype;
	return a;
}

static void
gm_create(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  int i;
	attrib * permissions = a_find(u->faction->attribs, &at_permissions);
	if (permissions) permissions = (attrib*)permissions->data.v;
	if (!permissions) return;
	i = atoi(igetstrtoken(str));

	if (i>0) {
		const char * iname = getstrtoken();
		const item_type * itype = finditemtype(iname, u->faction->locale);
		if (itype==NULL) {
			mistake(u, ord, "Unbekannter Gegenstand.\n", 0);
		} else {
			attrib * a = a_find(permissions, &at_gmcreate);

			while (a && a->data.v!=(void*)itype) a=a->nexttype;
			if (a) i_change(&u->items, itype, i);
			else mistake(u, ord, "Diesen Gegenstand darf deine Partei nicht machen.\n", 0);
		}
	}
}

static boolean
has_permission(const attrib * permissions, unsigned int key)
{
	return (find_key((attrib*)permissions->data.v, key) ||
		find_key((attrib*)permissions->data.v, atoi36("master")));
}

/**
 ** GM: GATE <id> <x> <y>
 ** requires: permission-key "gmgate"
 **/
static void
gm_gate(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  const struct plane * p = rplane(u->region);
  int id = atoi36(igetstrtoken(str));
	int x = rel_to_abs(p, u->faction, atoi(getstrtoken()), 0);
	int y = rel_to_abs(p, u->faction, atoi(getstrtoken()), 1);
	region * r = findregion(x, y);
	building * b = findbuilding(id);
	if (b==NULL || r==NULL || p!=rplane(b->region) || p!=rplane(r)) {
		mistake(u, ord, "Dieses Gebäude kann die Einheit nicht umwandeln.\n", 0);
		return;
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (permissions && has_permission(permissions, atoi36("gmgate"))) {
			remove_triggers(&b->attribs, "timer", &tt_gate);
			remove_triggers(&b->attribs, "create", &tt_unguard);
			if (r!=b->region) {
				add_trigger(&b->attribs, "timer", trigger_gate(b, r));
				add_trigger(&b->attribs, "create", trigger_unguard(b));
				fset(b, BLD_UNGUARDED);
			}
		}
	}
}


/**
 ** GM: TERRAFORM <x> <y> <terrain>
 ** requires: permission-key "gmterf"
 **/
static void
gm_terraform(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
	const struct plane * p = rplane(u->region);
	int x = rel_to_abs(p, u->faction, atoi(igetstrtoken(str)), 0);
	int y = rel_to_abs(p, u->faction, atoi(getstrtoken()), 1);
	const char * c = getstrtoken();
	region * r = findregion(x, y);
	terrain_t t;
	if (r==NULL || p!=rplane(r)) {
		mistake(u, ord, "Diese Region kann die Einheit nicht umwandeln.\n", 0);
		return;
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !has_permission(permissions, atoi36("gmterf"))) return;
	}
	for (t=0;t!=MAXTERRAINS;++t) {
		if (!strcasecmp(locale_string(u->faction->locale, terrain[t].name), c)) break;
	}
	if (t!=MAXTERRAINS) terraform(r, t);
}

/**
 ** GM: TELEPORT <unit> <x> <y>
 ** requires: permission-key "gmtele"
 **/
static void
gm_teleport(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
	const struct plane * p = rplane(u->region);
	unit * to = findunit(atoi36(igetstrtoken(str)));
	int x = rel_to_abs(p, u->faction, atoi(getstrtoken()), 0);
	int y = rel_to_abs(p, u->faction, atoi(getstrtoken()), 1);
	region * r = findregion(x, y);

	if (r==NULL || p!=rplane(r)) {
		mistake(u, ord, "In diese Region kann die Einheit nicht teleportieren.\n", 0);
	} else if (to==NULL) {
		mistake(u, ord, "Die Einheit wurde nicht gefunden.\n", 0);
	} else if (rplane(to->region)!=rplane(r) && !ucontact(to, u)) {
		mistake(u, ord, "Die Einheit hat uns nicht kontaktiert.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !has_permission(permissions, atoi36("gmtele"))) {
			mistake(u, ord, "Unzureichende Rechte für diesen Befehl.\n", 0);
		}
		else move_unit(to, r, NULL);
	}
}

/**
 ** GM: TELL PLANE <string>
 ** requires: permission-key "gmmsgr"
 **/
static void
gm_messageplane(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
	const struct plane * p = rplane(u->region);
	const char * zmsg = igetstrtoken(str);
	if (p==NULL) {
		mistake(u, ord, "In diese Ebene kann keine Nachricht gesandt werden.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !has_permission(permissions, atoi36("gmmsgr"))) {
			mistake(u, ord, "Unzureichende Rechte für diesen Befehl.\n", 0);
		}
		else {
			message * msg = msg_message("msg_event", "string", strdup(zmsg));
			faction * f;
			region * r;
			for (f=factions;f;f=f->next) {
				freset(f, FL_DH);
			}
			for (r=regions;r;r=r->next) {
				unit * u;
				if (rplane(r)!=p) continue;
				for (u=r->units;u;u=u->next) if (!fval(u->faction, FL_DH)) {
					f = u->faction;
					fset(f, FL_DH);
					add_message(&f->msgs, msg);
				}
			}
			msg_release(msg);
		}
	}
}

static void
gm_messagefaction(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  int n = atoi36(igetstrtoken(str));
  faction * f = findfaction(n);
	const char * msg = getstrtoken();
	plane * p = rplane(u->region);
	attrib * permissions = a_find(u->faction->attribs, &at_permissions);
	if (!permissions || !has_permission(permissions, atoi36("gmmsgr"))) {
		mistake(u, ord, "Unzureichende Rechte für diesen Befehl.\n", 0);
		return;
	}
	if (f!=NULL) {
		region * r;
		for (r=regions;r;r=r->next) if (rplane(r)==p) {
			unit * u;
			for (u=r->units;u;u=u->next) if (u->faction==f) {
				add_message(&f->msgs, msg_message("msg_event", "string", msg));
				return;
			}
		}
	}
	mistake(u, ord, "An diese Partei kann keine Nachricht gesandt werden.\n", 0);
}

/**
 ** GM: TELL REGION <x> <y> <string>
 ** requires: permission-key "gmmsgr"
 **/
static void
gm_messageregion(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  const struct plane * p = rplane(u->region);
  int x = rel_to_abs(p, u->faction, atoi(igetstrtoken(str)), 0);
	int y = rel_to_abs(p, u->faction, atoi(getstrtoken()), 1);
	const char * msg = getstrtoken();
	region * r = findregion(x, y);

	if (r==NULL || p!=rplane(r)) {
		mistake(u, ord, "In diese Region kann keine Nachricht gesandt werden.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !has_permission(permissions, atoi36("gmmsgr"))) {
			mistake(u, ord, "Unzureichende Rechte für diesen Befehl.\n", 0);
		}
		else {
			add_message(&r->msgs, msg_message("msg_event", "string", msg));
		}
	}
}

/**
 ** GM: KILL UNIT <id> <string>
 ** requires: permission-key "gmkill"
 **/
static void
gm_killunit(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  const struct plane * p = rplane(u->region);
  unit * target = findunit(atoi36(igetstrtoken(str)));
	const char * msg = getstrtoken();
	region * r = target->region;

	if (r==NULL || p!=rplane(r)) {
		mistake(u, ord, "In dieser Region kann niemand getötet werden.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !has_permission(permissions, atoi36("gmkill"))) {
			mistake(u, ord, "Unzureichende Rechte für diesen Befehl.\n", 0);
		}
		else {
			char * zmsg = (char*)gc_add(strdup(msg));
			scale_number(target, 0);
			ADDMSG(&target->faction->msgs, msg_message("killedbygm", 
				"region unit string", r, target, zmsg));
		}
	}
}


/**
 ** GM: KILL FACTION <id> <string>
 ** requires: permission-key "gmmsgr"
 **/
static void
gm_killfaction(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  int n = atoi36(igetstrtoken(str));
  faction * f = findfaction(n);
	const char * msg = getstrtoken();
	plane * p = rplane(u->region);
	attrib * permissions = a_find(u->faction->attribs, &at_permissions);
	if (!permissions || !has_permission(permissions, atoi36("gmkill"))) {
		mistake(u, ord, "Unzureichende Rechte für diesen Befehl.\n", 0);
		return;
	}
	if (f!=NULL) {
		region * r;
		for (r=regions;r;r=r->next) if (rplane(r)==p) {
			unit * target;
			for (target=r->units;target;target=target->next) {
				if (target->faction==f) {
					char * zmsg = (char*)gc_add(strdup(msg));
					scale_number(target, 0);
					ADDMSG(&target->faction->msgs, msg_message("killedbygm", 
						"region unit string", r, target, zmsg));
					return;
				}
			}
		}
	}
	mistake(u, ord, "Aus dieser Partei kann niemand gelöscht werden.\n", 0);
}

/**
 ** GM: TELL <unit> <string>
 ** requires: permission-key "gmmsgr"
 **/
static void
gm_messageunit(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  const struct plane * p = rplane(u->region);
  unit * target = findunit(atoi36(igetstrtoken(str)));
	const char * msg = getstrtoken();
	region * r;

	if (target == NULL) {
		cmistake(u, ord, 63, MSG_EVENT);
		return;
	}

	r = target->region;

	if (r==NULL || p!=rplane(r)) {
		mistake(u, ord, "In diese Region kann keine Nachricht gesandt werden.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !has_permission(permissions, atoi36("gmmsgu"))) {
			mistake(u, ord, "Unzureichende Rechte für diesen Befehl.\n", 0);
		}
		else {
			char * m = (char*)gc_add(strdup(msg));
			add_message(&target->faction->msgs,
				msg_message("unitmessage", "region unit string", r, u, m));
		}
	}
}

/**
 ** GM: GIVE <unit> <int> <itemtype>
 ** requires: permission-key "gmgive"
 **/
static void
gm_give(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  unit * to = findunit(atoi36(igetstrtoken(str)));
  int num = atoi(getstrtoken());
	const item_type * itype = finditemtype(getstrtoken(), u->faction->locale);

	if (to==NULL || rplane(to->region) != rplane(u->region)) {
		/* unknown or in another plane */
		mistake(u, ord, "Die Einheit wurde nicht gefunden.\n", 0);
	} else if (itype==NULL || i_get(u->items, itype)==0) {
		/* unknown or not enough */
		mistake(u, ord, "So einen Gegenstand hat die Einheit nicht.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !has_permission(permissions, atoi36("gmgive"))) {
			mistake(u, ord, "Unzureichende Rechte für diesen Befehl.\n", 0);
		}
		else {
			int i = i_get(u->items, itype);
			if (i<num) num=i;
			if (num) {
				i_change(&u->items, itype, -num);
				i_change(&to->items, itype, num);
			}
		}
	}
}

/**
 ** GM: TAKE <unit> <int> <itemtype>
 ** requires: permission-key "gmtake"
 **/
static void
gm_take(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  unit * to = findunit(atoi36(igetstrtoken(str)));
  int num = atoi(getstrtoken());
	const item_type * itype = finditemtype(getstrtoken(), u->faction->locale);

	if (to==NULL || rplane(to->region) != rplane(u->region)) {
		/* unknown or in another plane */
		mistake(u, ord, "Die Einheit wurde nicht gefunden.\n", 0);
	} else if (itype==NULL || i_get(to->items, itype)==0) {
		/* unknown or not enough */
		mistake(u, ord, "So einen Gegenstand hat die Einheit nicht.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !has_permission(permissions, atoi36("gmtake"))) {
			mistake(u, ord, "Unzureichende Rechte für diesen Befehl.\n", 0);
		}
		else {
			int i = i_get(to->items, itype);
			if (i<num) num=i;
			if (num) {
				i_change(&to->items, itype, -num);
				i_change(&u->items, itype, num);
			}
		}
	}
}

/**
 ** GM: SKILL <unit> <skill> <tage>
 ** requires: permission-key "gmskil"
 **/
static void
gm_skill(const tnode * tnext, const char * str, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  unit * to = findunit(atoi36(igetstrtoken(str)));
  skill_t skill = findskill(getstrtoken(), u->faction->locale);
	int num = atoi(getstrtoken());

	if (to==NULL || rplane(to->region) != rplane(u->region)) {
		/* unknown or in another plane */
		mistake(u, ord, "Die Einheit wurde nicht gefunden.\n", 0);
	} else if (skill==NOSKILL || skill==SK_MAGIC || skill==SK_ALCHEMY) {
		/* unknown or not enough */
		mistake(u, ord, "Dieses Talent ist unbekannt, oder kann nicht erhöht werden.\n", 0);
	} else if (num<0 || num>30) {
		/* sanity check failed */
		mistake(u, ord, "Der gewählte Wert ist nicht zugelassen.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !has_permission(permissions, atoi36("gmskil"))) {
			mistake(u, ord, "Unzureichende Rechte für diesen Befehl.\n", 0);
		}
		else {
			set_level(to, skill, num);
		}
	}
}

static tnode g_keys;
static tnode g_root;
static tnode g_tell;
static tnode g_kill;

void
init_gmcmd(void)
{
	at_register(&at_gmcreate);
	at_register(&at_permissions);
	add_command(&g_root, &g_keys, "gm", NULL);
	add_command(&g_keys, NULL, "terraform", &gm_terraform);
	add_command(&g_keys, NULL, "create", &gm_create);
	add_command(&g_keys, NULL, "gate", &gm_gate);
	add_command(&g_keys, NULL, "give", &gm_give);
	add_command(&g_keys, NULL, "take", &gm_take);
	add_command(&g_keys, NULL, "teleport", &gm_teleport);
	add_command(&g_keys, NULL, "skill", &gm_skill);
	add_command(&g_keys, &g_tell, "tell", NULL);
	add_command(&g_tell, NULL, "region", &gm_messageregion);
	add_command(&g_tell, NULL, "unit", &gm_messageunit);
	add_command(&g_tell, NULL, "plane", &gm_messageplane);
	add_command(&g_tell, NULL, "faction", &gm_messagefaction);
	add_command(&g_keys, &g_kill, "kill", NULL);
	add_command(&g_kill, NULL, "unit", &gm_killunit);
	add_command(&g_kill, NULL, "faction", &gm_killfaction);
}

/*
 * execute gm-commands for all units in the game
 */

void
gmcommands(void)
{
	region ** rp = &regions;
  while (*rp) {
    region * r = *rp;
    unit **up = &r->units;
    while (*up) {
      unit * u = *up;
      struct order * ord;
      for (ord = u->orders; ord; ord = ord->next) {
        if (get_keyword(ord) == K_GM) {
          do_command(&g_root, u, ord);
        }
      }
      if (u==*up) up = &u->next;
    }
    if (*rp==r) rp = &r->next;
  }
}

#define EXTENSION 10000

faction *
gm_addquest(const char * email, const char * name, int radius, unsigned int flags)
{
	plane * p;
	attrib * a;
	unit * u;
	watcher * w = calloc(sizeof(watcher), 1);
	region * center;
	boolean invalid = false;
	int minx, miny, maxx, maxy, cx, cy;
	int x, y, i;
	faction * f = calloc(1, sizeof(faction));

	/* GM faction */
	a_add(&f->attribs, make_key(atoi36("quest")));
	f->banner = strdup("Questenpartei");
	f->passw = strdup(itoa36(rand()));
	f->override = strdup(itoa36(rand()));
	f->override = strdup(itoa36(rand()));
	f->email = strdup(email);
	f->name = strdup("Questenpartei");
	f->race = new_race[RC_TEMPLATE];
	f->age = 0;
	f->lastorders = turn;
	f->alive = true;
	f->locale = find_locale("de");
	f->options = want(O_COMPRESS) | want(O_REPORT) | want(O_COMPUTER) | want(O_ADRESSEN);
	{
		faction * xist;
		int i = atoi36("gm00")-1;
		do {
			xist = findfaction(++i);
		} while (xist);

		f->no = i;
		addlist(&factions, f);
	}

	/* GM playfield */
	do {
		minx = (rand() % (2*EXTENSION)) - EXTENSION;
		miny = (rand() % (2*EXTENSION)) - EXTENSION;
		for (x=0;!invalid && x<=radius*2;++x) {
			for (y=0;!invalid && y<=radius*2;++y) {
				region * r = findregion(minx+x, miny+y);
				if (r) invalid = true;
			}
		}
	} while (invalid);
	maxx = minx+2*radius; cx = minx+radius;
	maxy = miny+2*radius; cy = miny+radius;
	p = create_new_plane(rand(), name, minx, maxx, miny, maxy, flags);
	center = new_region(cx, cy);
	for (x=0;x<=2*radius;++x) {
		int y;
		for (y=0;y<=2*radius;++y) {
			region * r = findregion(minx+x, miny+y);
			if (!r) r = new_region(minx+x, miny+y);
			freset(r, RF_ENCOUNTER);
			r->planep = p;
			if (distance(r, center)==radius) {
				terraform(r, T_FIREWALL);
			} else if (r==center) {
				terraform(r, T_PLAIN);
			} else {
				terraform(r, T_OCEAN);
			}
		}
	}

	/* watcher: */
	w->faction = f;
	w->mode = see_unit;
	w->next = p->watchers;
	p->watchers	= w;

	/* generic permissions */
	a = a_add(&f->attribs, a_new(&at_permissions));

	add_key((attrib**)&a->data.v, atoi36("gmterf"));
	add_key((attrib**)&a->data.v, atoi36("gmtele"));
	add_key((attrib**)&a->data.v, atoi36("gmgive"));
	add_key((attrib**)&a->data.v, atoi36("gmskil"));
	add_key((attrib**)&a->data.v, atoi36("gmtake"));
	add_key((attrib**)&a->data.v, atoi36("gmmsgr"));
	add_key((attrib**)&a->data.v, atoi36("gmmsgu"));
	add_key((attrib**)&a->data.v, atoi36("gmgate"));

	a_add((attrib**)&a->data.v, make_atgmcreate(resource2item(r_silver)));

	for (i=0;i<=I_INCENSE;++i) {
		a_add((attrib**)&a->data.v, make_atgmcreate(olditemtype[i]));
	}
	for (i=I_GREATSWORD;i!=I_KEKS;++i) {
		a_add((attrib**)&a->data.v, make_atgmcreate(olditemtype[i]));
	}

	/* one initial unit */
	u = createunit(center, f, 1, new_race[RC_TEMPLATE]);
	u->irace = new_race[RC_GNOME];
	u->number = 1;
	set_string(&u->name, "Questenmeister");

	return f;
}

faction *
gm_addfaction(const char * email, plane * p, region * r)
{
	attrib * a;
	unit * u;
	int i;
	faction * f = calloc(1, sizeof(faction));

	assert(p!=NULL);

	/* GM faction */
	add_key(&f->attribs, atoi36("quest"));
	f->banner = strdup("Questenpartei");
	f->passw = strdup(itoa36(rand()));
	f->email = strdup(email);
	f->name = strdup("Questenpartei");
	f->race = new_race[RC_TEMPLATE];
	f->age = 0;
	f->lastorders = turn;
	f->alive = true;
	f->locale = find_locale("de");
	f->options = want(O_COMPRESS) | want(O_REPORT) | want(O_COMPUTER) | want(O_ADRESSEN);
	{
		faction * xist;
		int i = atoi36("gm00")-1;
		do {
			xist = findfaction(++i);
		} while (xist);

		f->no = i;
		addlist(&factions, f);
	}
	/* generic permissions */
	a = a_add(&f->attribs, a_new(&at_permissions));

	add_key((attrib**)&a->data.v, atoi36("gmterf"));
	add_key((attrib**)&a->data.v, atoi36("gmtele"));
	add_key((attrib**)&a->data.v, atoi36("gmgive"));
	add_key((attrib**)&a->data.v, atoi36("gmskil"));
	add_key((attrib**)&a->data.v, atoi36("gmtake"));
	add_key((attrib**)&a->data.v, atoi36("gmmsgr"));
	add_key((attrib**)&a->data.v, atoi36("gmmsgu"));
	add_key((attrib**)&a->data.v, atoi36("gmgate"));

	a_add((attrib**)&a->data.v, make_atgmcreate(resource2item(r_silver)));

	for (i=0;i<=I_INCENSE;++i) {
		a_add((attrib**)&a->data.v, make_atgmcreate(olditemtype[i]));
	}
	for (i=I_GREATSWORD;i!=I_KEKS;++i) {
		a_add((attrib**)&a->data.v, make_atgmcreate(olditemtype[i]));
	}

	/* one initial unit */
	u = createunit(r, f, 1, new_race[RC_TEMPLATE]);
	u->irace = new_race[RC_GNOME];
	u->number = 1;
	set_string(&u->name, "Questenmeister");

	return f;
}

plane *
gm_addplane(int radius, unsigned int flags, const char * name)
{
	region * center;
	plane * p;
	boolean invalid = false;
	int minx, miny, maxx, maxy, cx, cy;
	int x, y;

	/* GM playfield */
	do {
		minx = (rand() % (2*EXTENSION)) - EXTENSION;
		miny = (rand() % (2*EXTENSION)) - EXTENSION;
		for (x=0;!invalid && x<=radius*2;++x) {
			for (y=0;!invalid && y<=radius*2;++y) {
				region * r = findregion(minx+x, miny+y);
				if (r) invalid = true;
			}
		}
	} while (invalid);
	maxx = minx+2*radius; cx = minx+radius;
	maxy = miny+2*radius; cy = miny+radius;
	p = create_new_plane(rand(), name, minx, maxx, miny, maxy, flags);
	center = new_region(cx, cy);
	for (x=0;x<=2*radius;++x) {
		int y;
		for (y=0;y<=2*radius;++y) {
			region * r = findregion(minx+x, miny+y);
			if (!r) r = new_region(minx+x, miny+y);
			freset(r, RF_ENCOUNTER);
			r->planep = p;
			if (distance(r, center)==radius) {
				terraform(r, T_FIREWALL);
			} else if (r==center) {
				terraform(r, T_PLAIN);
			} else {
				terraform(r, T_OCEAN);
			}
		}
	}
	return p;
}
