/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2000
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
#include "eressea.h"
#include "faction.h"

/* util includes */
#include <base36.h>

/* libc includes */
#include <stdio.h>

const char *
factionname(const faction * f)
{
	typedef char name[OBJECTIDSIZE + 1];
	static name idbuf[8];
	static int nextbuf = 0;

	char *buf = idbuf[(++nextbuf) % 8];

	if (f && f->name) {
		sprintf(buf, "%s (%s)", strcheck(f->name, NAMESIZE), itoa36(f->no));
	} else {
		sprintf(buf, "Unbekannte Partei (?)");
	}
	return buf;
}

void *
resolve_faction(void * id) {
   return findfaction((int)id);
}
