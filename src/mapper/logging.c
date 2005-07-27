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

/* kernel includes */
#include <faction.h>
#include <unit.h>
#include <region.h>
#include <save.h>

/* util includes */
#include <base36.h>

/* libc includes */
#include <string.h>
#include <stdio.h>

static FILE * log;

void
log_read(const char * filename)
{
	FILE * log = fopen(filename, "r");
	faction **fp = &factions;
	char buf[64];

	while (*fp) fp=&(*fp)->next;
	fscanf(log, "LOGVERSION %u\n", &global.data_version);
	while (!feof(log)) {
		if(fscanf(log, "%s", buf) == EOF) break;
		if (strcmp(buf, "UNIT")==0) {
			short x, y;
			unit * u;
			region * r;
			fscanf(log, "%s %hd %hd", buf, &x, &y);
			u = readunit(log);
			r = findregion(x, y);
			if (r==NULL) {
				r = new_region(x, y);
				terraform(r, T_PLAIN);
			}
			if (u->region!=r) move_unit(u, r, NULL);
		} else if (strcmp(buf, "REGION")==0) {
			short x, y;
			fscanf(log, "%hd %hd", &x, &y);
			readregion(log, x, y);
		} else if (strcmp(buf, "FACTION")==0) {
			faction * f;
			fscanf(log, "%s", buf);
			f = findfaction(atoi36(buf));
			if (f) {
				readfaction(log);
			} else {
				*fp = readfaction(log);
				fp=&(*fp)->next;
			}
		} else assert(0);
	}
}

void 
log_faction(const struct faction * f)
{
	fprintf(log, "FACTION %s\n", factionid(f));
	writefaction(log, f);
}

void 
log_unit(const struct unit * u)
{
	fprintf(log, "UNIT %s %d %d\n", unitid(u), u->region->x, u->region->y);
	writeunit(log, u);
}

void 
log_region(const struct region * r)
{
	fprintf(log, "REGION %d %d\n", r->x, r->y);
	writeregion(log, r);
}

void 
log_start(const char * filename)
{
	log = fopen(filename, "w+");
	fprintf(log, "LOGVERSION %d\n", RELEASE_VERSION);
}

void 
log_stop(void)
{
	fclose(log);
	log = NULL;
}
