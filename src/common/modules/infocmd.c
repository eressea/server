/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 
 */
#include <config.h>
#include <eressea.h>

#ifdef INFOCMD_MODULE
#include "infocmd.h"

#include "command.h"

/* kernel includes */
#include <kernel/order.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/save.h>

/* util includes */
#include <base36.h>
#include <sql.h>

/* libc includes */
#include <string.h>
#include <time.h>
#include <stdlib.h>

static void
info_email(const tnode * tnext, const char * str, void * data, struct order * ord)
{
	unused(str);
	unused(data);
	unused(ord);
}

static void
info_name(const tnode * tnext, const char * str, void * data, struct order * ord)
{
	unused(tnext);
        unused(str);
        unused(data);
        unused(ord);
}

static void
info_address(const tnode * tnext, const char * str, void * data, struct order * ord)
{
}

static void
info_phone(const tnode * tnext, const char * str, void * data, struct order * ord)
{
}

static void
info_vacation(const tnode * tnext, const char * str, void * data, struct order * ord)
{

#ifdef SQLOUTPUT
	if (sqlstream!=NULL) {
		unit * u = (unit*)data;
		faction * f = u->faction;
		const char * email = sqlquote(igetstrtoken(str));
		int duration = atoi(getstrtoken());
		time_t start_time = time(NULL);
		time_t end_time = start_time + 60*60*24*duration;
		struct tm start = *localtime(&start_time);
		struct tm end = *localtime(&end_time);
		fprintf(sqlstream, "UPDATE factions SET vacation = '%s' WHERE id = '%s';\n", email, itoa36(f->no));
		fprintf(sqlstream, "UPDATE factions SET vacation_start = '%04d-%02d-%02d' WHERE id = '%s';\n", 
			start.tm_year, start.tm_mon, start.tm_mday, itoa36(f->no));
		fprintf(sqlstream, "UPDATE factions SET vacation_end = '%04d-%02d-%02d' WHERE id = '%s';\n",
			end.tm_year, end.tm_mon, end.tm_mday, itoa36(f->no));
	}
#endif
}

static tnode g_keys;
static tnode g_info;

void
infocommands(void)
{
	region ** rp = &regions;
	while (*rp) {
		region * r = *rp;
		unit **up = &r->units;
    while (*up) {
      unit * u = *up;
      order * ord;
      for (ord = u->orders; ord; ord = ord->next) {
        if (get_keyword(ord) == K_INFO) {
          do_command(&g_keys, u, ord);
        }
      }
      if (u==*up) up = &u->next;
    }
    if (*rp==r) rp = &r->next;
  }
}

void
init_info(void)
{
	add_command(&g_keys, &g_info, "info", NULL);

	add_command(&g_info, NULL, "email", &info_email);
	add_command(&g_info, NULL, "name", &info_name);
	add_command(&g_info, NULL, "adresse", &info_address);
	add_command(&g_info, NULL, "telefon", &info_phone);
	add_command(&g_info, NULL, "urlaub", &info_vacation);
}
#endif /* INFOCMD_MODULE */

