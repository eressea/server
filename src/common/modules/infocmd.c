/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 
 */
#include <config.h>
#include <eressea.h>

#include "infocmd.h"

#include "command.h"

/* kernel includes */
#include <faction.h>
#include <player.h>
#include <region.h>
#include <unit.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>

static void
info_email(const char * str, void * data, const char * cmd)
{
	unit * u = (unit*)data;
	const char * email = igetstrtoken(str);
	player * p = u->faction->owner;

	if (p==NULL) {
		p = u->faction->owner = make_player();
		p->faction = u->faction;
	}
	if (p->email) free(p->email);
	if (strlen(email)) p->email = strdup(email);
	else p->email = NULL;
}

static void
info_name(const char * str, void * data, const char * cmd)
{
	unit * u = (unit*)data;
	const char * name = igetstrtoken(str);
	player * p = u->faction->owner;

	if (p==NULL) {
		p = u->faction->owner = make_player();
	}
	if (p->name) free(p->name);
	if (strlen(name)) p->name = strdup(name);
	else p->name = NULL;
}

static struct command * g_cmds;
static tnode g_keys;

void
infocommands(void)
{
	region ** rp = &regions;
	while (*rp) {
		region * r = *rp;
		unit **up = &r->units;
		while (*up) {
			unit * u = *up;
			strlist * order;
			for (order = u->orders; order; order = order->next)
				if (igetkeyword(order->s, u->faction->locale) == K_INFO) {
					do_command(&g_keys, u, order->s);
				}
			if (u==*up) up = &u->next;
		}
		if (*rp==r) rp = &r->next;
	}
}

static void
info_command(const char * str, void * data, const char * cmd)
{
	do_command(&g_keys, data, str);
}

void
info_init(void)
{
	add_command(&g_keys, &g_cmds, "info", &info_command);
	add_command(&g_keys, &g_cmds, "email", &info_email);
	add_command(&g_keys, &g_cmds, "name", &info_name);
}

void
info_done(void)
{
	/* TODO */
}
