#include <config.h>
#include <eressea.h>
#include "gmcmd.h"

#include <items/demonseye.h>
#include <attributes/key.h>

/* kernel includes */
#include <faction.h>
#include <item.h>
#include <plane.h>
#include <region.h>
#include <terrain.h>
#include <unit.h>

/* util includes */
#include <base36.h>
#include <umlaut.h>
#include <attrib.h>

/* libc includes */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct command {
	struct command * next;
	const char * key;
	void (*perform)(const char *, unit *);
} command;

static command * g_cmds;
static tnode g_keys;

static void
add_gmcommand(command ** cmds, const char * str, void(*fun)(const char*, struct unit *))
{
	command * nc = calloc(sizeof(command), 1);
	nc->key = str;
	nc->perform = fun;
	nc->next = *cmds;
	*cmds = nc;

	addtoken(&g_keys, str, (void*)nc);
}

/**
 ** at_permissions
 **/

static void
write_permissions(const attrib * a, FILE * F)
{
	a_write(F, (attrib*)a->data.v);
}

static int
read_permissions(attrib * a, FILE * F)
{
	attrib ** p_a = (attrib**)&a->data.v;
	a_read(F, p_a);
	return 1;
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
	fprintf(F, "%s", resourcename(itype->rtype, 0));
}

static int
read_gmcreate(attrib * a, FILE * F)
{
	char zText[32];
	const item_type ** p_itype = (const item_type **)&a->data.v;
	fscanf(F, "%s", zText);
	*p_itype = it_find(zText);
	return 1;
}

/* at_gmcreate specifies that the owner can create items of a particular type */
static attrib_type at_gmcreate = {
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
gm_create(const char * str, struct unit * u)
{
	int i;
	attrib * permissions = a_find(u->faction->attribs, &at_permissions);
	if (permissions) permissions = (attrib*)permissions->data.v;
	if (!permissions) return;
	i = atoi(igetstrtoken(str));

	if (i>0) {
		char * iname = getstrtoken();
		const item_type * itype = finditemtype(iname, u->faction->locale);
		attrib * a = a_find(permissions, &at_gmcreate);

		while (a && a->data.v!=(void*)itype) a=a->nexttype;
		if (a) i_change(&u->items, itype, i);
	}
}

/**
 ** GM: TERRAFORM <terrain> <x> <y>
 ** requires: permission-key "gmterf"
 **/
static void
gm_terraform(const char * str, struct unit * u)
{
	const struct plane * p = rplane(u->region);
	int x = rel_to_abs(p, u->faction, atoi(igetstrtoken(str)), 0);
	int y = rel_to_abs(p, u->faction, atoi(getstrtoken()), 1);
	const char * c = getstrtoken();
	region * r = findregion(x, y);
	terrain_t t;
	if (r==NULL || p!=rplane(r)) {
		mistake(u, str, "Diese Regon kann die Einheit nicht umwandeln.\n", 0);
		return;
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !find_key((attrib*)permissions->data.v, atoi36("gmterf"))) return;
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
gm_teleport(const char * str, struct unit * u)
{
	const struct plane * p = rplane(u->region);
	unit * to = findunit(atoi36(igetstrtoken(str)));
	int x = rel_to_abs(p, u->faction, atoi(getstrtoken()), 0);
	int y = rel_to_abs(p, u->faction, atoi(getstrtoken()), 1);
	region * r = findregion(x, y);

	if (r==NULL || p!=rplane(r)) {
		mistake(u, str, "In diese Region kann die Einheit nicht teleportieren.\n", 0);
	} if (to==NULL || (rplane(to->region)!=rplane(r) && !ucontact(to, u))) {
		mistake(u, str, "Die Einheit wurde nicht gefunden, oder sie hat uns nicht kontaktiert.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !find_key((attrib*)permissions->data.v, atoi36("gmtele"))) {
			mistake(u, str, "Unzureichende Rechte für diesen Befehl.\n", 0);
		}
		else move_unit(to, r, NULL);
	}
}

/**
 ** GM: GIVE <unit> <int> <itemtype>
 ** requires: permission-key "gmgive"
 **/
static void
gm_give(const char * str, struct unit * u)
{
	unit * to = findunit(atoi36(igetstrtoken(str)));
	int num = atoi(getstrtoken());
	const item_type * itype = finditemtype(getstrtoken(), u->faction->locale);

	if (to==NULL || rplane(to->region) != rplane(u->region)) {
		/* unknown or in another plane */
		cmistake(u, str, 64, MSG_COMMERCE);
	} else if (itype==NULL || i_get(u->items, itype)==0) {
		/* unknown or not enough */
		mistake(u, str, "So einen Gegenstand hat die Einheit nicht.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !find_key((attrib*)permissions->data.v, atoi36("gmgive"))) {
			mistake(u, str, "Unzureichende Rechte für diesen Befehl.\n", 0);
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
gm_take(const char * str, struct unit * u)
{
	unit * to = findunit(atoi36(igetstrtoken(str)));
	int num = atoi(getstrtoken());
	const item_type * itype = finditemtype(getstrtoken(), u->faction->locale);

	if (to==NULL || rplane(to->region) != rplane(u->region)) {
		/* unknown or in another plane */
		cmistake(u, str, 64, MSG_COMMERCE);
	} else if (itype==NULL || i_get(to->items, itype)==0) {
		/* unknown or not enough */
		mistake(u, str, "So einen Gegenstand hat die Einheit nicht.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !find_key((attrib*)permissions->data.v, atoi36("gmtake"))) {
			mistake(u, str, "Unzureichende Rechte für diesen Befehl.\n", 0);
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
gm_skill(const char * str, struct unit * u)
{
	unit * to = findunit(atoi36(igetstrtoken(str)));
	skill_t skill = findskill(getstrtoken(), u->faction->locale);
	int num = atoi(getstrtoken());

	if (to==NULL || rplane(to->region) != rplane(u->region)) {
		/* unknown or in another plane */
		cmistake(u, str, 64, MSG_COMMERCE);
	} else if (skill==NOSKILL || skill==SK_MAGIC || skill==SK_ALCHEMY) {
		/* unknown or not enough */
		mistake(u, str, "Dieses Talent ist unbekannt, oder kann nicht erhöht werden.\n", 0);
	} else if (num<0 || num>5000) {
		/* sanity check failed */
		mistake(u, str, "Der gewählte Wert ist nicht zugelassen.\n", 0);
	} else {
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !find_key((attrib*)permissions->data.v, atoi36("gmskil"))) {
			mistake(u, str, "Unzureichende Rechte für diesen Befehl.\n", 0);
		}
		else {
			set_skill(to, skill, num);
		}
	}
}

static void
gm_command(const char * cmd, struct unit * u)
{
	int i;
	char zText[16];
	const char * c;
	command * cm;

	while (isspace(*cmd)) ++cmd;
	c = cmd;
	while (isalnum(*c)) ++c;
	i = min(16, c-cmd);
	strncpy(zText, cmd, i);
	zText[i]=0;
	if (findtoken(&g_keys, zText, (void**)&cm)==E_TOK_SUCCESS && cm->perform) cm->perform(++c, u);
}

void
init_gmcmd(void)
{
	at_register(&at_gmcreate);
	at_register(&at_permissions);
	add_gmcommand(&g_cmds, "gm", &gm_command);
	add_gmcommand(&g_cmds, "terraform", &gm_terraform);
	add_gmcommand(&g_cmds, "create", &gm_create);
	add_gmcommand(&g_cmds, "give", &gm_give);
	add_gmcommand(&g_cmds, "take", &gm_take);
	add_gmcommand(&g_cmds, "teleport", &gm_teleport);
	add_gmcommand(&g_cmds, "skill", &gm_skill);
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
			strlist * order;
			for (order = u->orders; order; order = order->next)
				if (igetkeyword(order->s, u->faction->locale) == K_GM) {
					gm_command(order->s, u);
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
	region * center;
	boolean invalid = false;
	int minx, miny, maxx, maxy, cx, cy;
	int x, y, i;
	faction * f = calloc(1, sizeof(faction));

	/* GM faction */
	a_add(&f->attribs, make_key(atoi36("quest")));
	f->banner = strdup("Questenpartei");
	f->passw = strdup(itoa36(rand()));
	f->email = strdup(email);
	f->name = strdup("Questenpartei");
	f->race = RC_TEMPLATE;
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

	/* generic permissions */
	a = a_add(&f->attribs, a_new(&at_permissions));

	a_add((attrib**)&a->data.v, make_key(atoi36("gmterf")));
	a_add((attrib**)&a->data.v, make_key(atoi36("gmtele")));
	a_add((attrib**)&a->data.v, make_key(atoi36("gmgive")));
	a_add((attrib**)&a->data.v, make_key(atoi36("gmskil")));
	a_add((attrib**)&a->data.v, make_key(atoi36("gmtake")));

	a_add((attrib**)&a->data.v, make_atgmcreate(resource2item(r_silver)));

	for (i=0;i<=I_INCENSE;++i) {
		a_add((attrib**)&a->data.v, make_atgmcreate(olditemtype[i]));
	}

	/* one initial unit */
	u = createunit(center, f, 1, RC_TEMPLATE);
	u->irace = RC_GNOME;
	u->number = 1;
	set_string(&u->name, "Questenmeister");
	u->irace = RC_GOBLIN;

	return f;
}

#ifdef TEST_GM_COMMANDS
void
setup_gm_faction(void)
{
	int i = atoi36("gms")-1;
	faction * f = factions;
	unit * newunit;
	region * r = regions;
	attrib * a;

	do {
		f = findfaction(++i);
	} while (f);

	f = (faction *) calloc(1, sizeof(faction));
	f->no = i;
	set_string(&f->email, "gms@eressea-pbem.de");
	set_string(&f->passw, "geheim");
	set_string(&f->name, "GMs");
	f->alive = 1;
	f->options |= (1 << O_REPORT);
	f->options |= (1 << O_COMPUTER);
	addlist(&factions, f);

	a = a_add(&f->attribs, a_new(&at_permissions));
	a_add((attrib**)&a->data.v, make_atgmcreate(&it_demonseye));
	a_add((attrib**)&a->data.v, make_key(atoi36("gmtf")));

	while (r && !r->land) r=r->next;
	newunit = createunit(r, f, 1, RC_DAEMON);
	set_string(&newunit->name, "Flamdring, Gott des Feuers");
	set_money(newunit, 100);
	fset(newunit, FL_ISNEW);
}
#endif
