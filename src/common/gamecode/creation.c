/* vi: set ts=2:
 *
 *	$Id: creation.c,v 1.1 2001/01/25 09:37:56 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
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
#include "creation.h"

/* kernel includes */
#include "alchemy.h"
#include "build.h"
#include "faction.h"
#include "goodies.h"
#include "item.h"
#include "magic.h"
#include "monster.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "save.h"
#include "ship.h"
#include "unit.h"

/* util includes */
#include <vmap.h>
#include <vset.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* eine insel pro 9x9 feld. das erste feld von (0,0) bis (8,8) */

/* Ozean und Grasland wird automatisch generiert (ein Ozean, darin eine Insel
 * aus Grasland). Jedes Seed generiert ein Feld des entsprechenden terrains,
 * und max. 3 angrenzende Felder. Details in der Funktion SEED und TRANSMUTE in
 * REGIONS.C */

/* ------------------------------------------------------------- */

/* OCEAN, PLAIN, FOREST, SWAMP, DESERT, HIGHLAND, MOUNTAIN, GLACIER,
 * MAXTERRAINS */

/* ------------------------------------------------------------- */

/* ------------------------------------------------------------- */

void
createmonsters(void)
{
	faction *f;

	if (findfaction(MONSTER_FACTION)) {
		puts("* Fehler! Die Monster Partei gibt es schon.");
		return;
	}
	f = (faction *) calloc(1, sizeof(faction));

	/* alles ist auf null gesetzt, ausser dem folgenden. achtung - partei
	 * no 0 muss keine orders einreichen! */

	set_string(&f->email, "keine");
	set_string(&f->name, "Monster");
	f->alive = 1;
	f->options = (char) pow(2, O_REPORT);
	addlist(&factions, f);
}

/* ------------------------------------------------------------- */



void
listnames(void)
{
	region *r;
	int i;

	puts("Die Liste der benannten Regionen ist:");

	i = 0;
	for (r = regions; r; r = r->next)
		if (rterrain(r) != T_OCEAN) {
			printf("%s,", rname(r, NULL));
			i += strlen(rname(r, NULL)) + 1;

			while (i % 15) {
				printf(" ");
				i++;
			}

			if (i > 60) {
				printf("\n");
				i = 0;
			}
		}
	printf("\n");
}

static char
factionsymbols(region * r)
{
	faction *f;
	unit *u;
	int i = 0;

	for (f = factions; f; f = f->next)
		for (u = r->units; u; u = u->next)
			if (u->faction == f) {
				i++;
				break;
			}
	assert(i);

	if (i > 9)
		return 'x';
	else
		return (char) ('1' - 1 + i);
}

static char
armedsymbols(region * r)
{
	unit *u;
	int i = 0;

	for (u = r->units; u; u = u->next)
		if (armedmen(u))
			return 'X';
		else
			i += u->number;
	assert(i);

	if (i > 9)
		return 'x';
	else
		return (char) ('1' - 1 + i);
}

void
writemap(FILE * F, int mode)
{
	int x, y, minx, miny, maxx, maxy, pos, maxpos;
	region *r;

	minx = INT_MAX;
	maxx = INT_MIN;
	miny = INT_MAX;
	maxy = INT_MIN;

	for (r = regions; r; r = r->next) if (!rplane(r)) {
		minx = min(minx, r->x);
		maxx = max(maxx, r->x);
		miny = min(miny, r->y);
		maxy = max(maxy, r->y);
	}

	fputs("\nLegende:\n\n", F);
	switch (mode) {
	case M_TERRAIN:
		for (y = 0; y != MAXTERRAINS; y++)
			fprintf(F, "%c - %s\n", terrain[y].symbol, terrain[y].name);
		break;

	case M_FACTIONS:
		for (y = 0; y != MAXTERRAINS; y++)
			fprintf(F, "%c - %s\n", terrain[y].symbol, terrain[y].name);
		fputs("x - mehr als 9 Parteien\n", F);
		fputs("1-9 - Anzahl Parteien\n", F);
		break;

	case M_UNARMED:
		for (y = 0; y != MAXTERRAINS; y++)
			fprintf(F, "%c - %s\n", terrain[y].symbol, terrain[y].name);
		fputs("X - mindestens eine bewaffnete Person\n", F);
		fputs("x - mehr als 9 Personen\n", F);
		fputs("1-9 - Anzahl unbewaffneter Personen\n", F);
		break;

	}

	fprintf(F, "\n\nKoordinaten von (%d,%d) bis (%d,%d):\n\n    ",
			minx, maxy, maxx, miny);

	for (x = minx; x <= maxx; x++) {
		if (x < 0) {
			fprintf(F, " -");
		} else {
			fprintf(F, "  ");
		}
	}
	fprintf(F, "\n    ");
	for (x = minx; x <= maxx; x++) {
		fprintf(F, "%2d", (int) (abs(x) / 10));
	}
	fprintf(F, "\n    ");
	for (x = minx; x <= maxx; x++) {
		fprintf(F, "%2d", abs(x % 10));
	}
	fprintf(F, "\n\n");

	for (y = maxy; y >= miny; y--) {
		memset(buf, ' ', sizeof buf);

		maxpos=-9999;
		for (r = regions; r; r = r->next)
			if (r->y == y) {
				pos=(r->x - minx + (y/2)) * 2;
				if (y>0) pos += (y&1);
				else pos -= (y&1);
				if (r->units && mode == M_FACTIONS)
					buf[pos] = factionsymbols(r);
				else if (r->units && mode == M_UNARMED)
					buf[pos] = armedsymbols(r);
				else	/* default: terrain */
					buf[pos] = terrain[rterrain(r)].symbol;
				maxpos=max(pos,maxpos);
			}
		buf[maxpos+1+(y&1)]=0;
		fprintf(F,"%3d %s %3d\n",y,buf,y);
	}

	fprintf(F, "\n    ");
	for (x = minx; x <= maxx; x++) {
		if (x < 0) {
			fprintf(F, " -");
		} else {
			fprintf(F, "  ");
		}
	}
	fprintf(F, "\n    ");
	for (x = minx; x <= maxx; x++) {
		fprintf(F, "%2d", (int) (abs(x) / 10));
	}
	fprintf(F, "\n    ");
	for (x = minx; x <= maxx; x++) {
		fprintf(F, "%2d", abs(x % 10));
	}
	fprintf(F, "\n\n");
}
/* ------------------------------------------------------------- */

/* ------------------------------------------------------------- */

void
changeblockterrain(void)
{
	int x1, x2, y1, y2;
	int x, y;
	int i;
	char t;
	region *r;

	printf("X1? ");
	fgets(buf, 6, stdin);
	if (buf[0] == 0)
		return;
	x1 = atoi(buf);

	printf("Y1? ");
	fgets(buf, 6, stdin);
	if (buf[0] == 0)
		return;
	y1 = atoi(buf);

	printf("X2? ");
	fgets(buf, 6, stdin);
	if (buf[0] == 0)
		return;
	x2 = atoi(buf);

	printf("Y2? ");
	fgets(buf, 6, stdin);
	if (buf[0] == 0)
		return;
	y2 = atoi(buf);

	puts("Terrain?");
	for (i = 0; i != MAXTERRAINS; i++) {
		printf("%d - %s\n", i, terrain[i].name);
	}
	fgets(buf, 6, stdin);
	if (buf[0] == 0)
		return;
	t = (char) atoi(buf);

	for (x = x1; x <= x2; x++) {
		for (y = y1; y <= y2; y++) {
			if (0 != (r = findregion(x, y))) {
				terraform(r, t);
			}
		}
	}
}

void
changeblockchaos(void)
{
	int x1, x2, y1, y2;
	int x, y;
	char chaotisch;
	region *r;

	printf("X1? ");
	fgets(buf, 6, stdin);
	if (buf[0] == 0)
		return;
	x1 = atoi(buf);

	printf("Y1? ");
	fgets(buf, 6, stdin);
	if (buf[0] == 0)
		return;
	y1 = atoi(buf);

	printf("X2? ");
	fgets(buf, 6, stdin);
	if (buf[0] == 0)
		return;
	x2 = atoi(buf);

	printf("Y2? ");
	fgets(buf, 6, stdin);
	if (buf[0] == 0)
		return;
	y2 = atoi(buf);

	puts("Chaos (0=aus, 1=an) ?");
	fgets(buf, 6, stdin);
	if (buf[0] == 0)
		return;

	chaotisch = (char) atoi(buf);

	for (x = x1; x <= x2; x++) {
		for (y = y1; y <= y2; y++) {
			if (0 != (r = findregion(x, y))) {
				if (chaotisch) fset(r, RF_CHAOTIC);
				else freset(r, RF_CHAOTIC);
			}
		}
	}
}
