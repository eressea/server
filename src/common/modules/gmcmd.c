#include <config.h>
#include <eressea.h>
#include "gmcmd.h"

#include <items/demonseye.h>
#include <attributes/key.h>

/* kernel includes */
#include <unit.h>
#include <faction.h>
#include <terrain.h>
#include <region.h>
#include <item.h>

/* util includes */
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

struct attrib *
find_key(struct attrib * attribs, int key)
{
	attrib * a = a_find(attribs, &at_key);
	while (a && a->data.i!=key) a=a->nexttype;
	return a;
}

/**
 ** GM: TERRAFORM <terrain>
 **/
static void
gm_terraform(const char * str, struct unit * u)
{
	terrain_t t;
	const char * c = igetstrtoken(str);
	{
		/* checking permissions */
		attrib * permissions = a_find(u->faction->attribs, &at_permissions);
		if (!permissions || !find_key((attrib*)permissions->data.v, atoi36("gmtf"))) return;
	}
	for (t=0;t!=MAXTERRAINS;++t) {
		if (!strcasecmp(locale_string(u->faction->locale, terrain[t].name), c)) break;
	}
	if (t!=MAXTERRAINS) terraform(u->region, t);
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
	cm = (command*)findtoken(&g_keys, zText);
	if (cm && cm->perform) cm->perform(++c, u);
}

void
init_gmcmd(void)
{
	at_register(&at_gmcreate);
	at_register(&at_permissions);
	add_gmcommand(&g_cmds, "gm", &gm_command);
	add_gmcommand(&g_cmds, "terraform", &gm_terraform);
	add_gmcommand(&g_cmds, "create", &gm_create);
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
				if (igetkeyword(order->s) == K_GM) {
					gm_command(order->s, u);
				}			
			if (u==*up) up = &u->next;
		}
		if (*rp==r) rp = &r->next;
	}
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
