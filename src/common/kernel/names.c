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
#include "names.h"

/* kernel includes */
#include "unit.h"
#include "region.h"
#include "faction.h"
#include "magic.h"
#include "race.h"
#include "terrain.h"
#include "terrainid.h"

/* libc includes */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* util includes */
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/rng.h>

/* Untote */

#define UNTOT_VOR 23

static const char *untot_vor[UNTOT_VOR] =
{
	"Grausige ",
	"Stöhnende ",
	"Schlurfende ",
	"Schwarzgewandete ",
	"Faulende ",
	"Angsteinflößende ",
	"Heulende ",
	"Wartende ",
	"Grauenvolle ",
	"Schwarze ",
	"Dunkle ",
	"Fürchterliche ",
	"Grauenhafte ",
	"Furchtbare ",
	"Entsetzliche ",
	"Schauderhafte ",
	"Schreckliche ",
	"Gespenstische ",
	"Ekelhafte ",
	"Düstere ",
	"Schaurige ",
	"Erbarmungslose ",
	"Hungrige "
};

#define UNTOT	13

static const char *untot[UNTOT] =
{
	"Geister",
	"Phantome",
	"Vampire",
	"Zombies",
	"Gespenster",
	"Kreaturen",
	"Gestalten",
	"Schemen",
	"Monster",
	"Krieger",
	"Ghule",
	"Kopflose",
	"Irrlichter"
};

#define UNTOT_NACH 14

static const char *untot_nach[UNTOT_NACH] =
{
	" der Nacht",
	" der Schatten",
	" der Finsternis",
	" des Bösen",
	" der Erschlagenen",
	" der Verfluchten",
	" der Gefolterten",
	" der Ruhelosen",
	" aus dem Nebel",
	" aus dem Dunkel",
	" der Tiefe",
	" in Ketten",
	" aus dem Totenreich",
	" aus der Unterwelt"
};

const char *
untoten_name(const unit * u)
{
	int uv, uu, un;
	static char name[NAMESIZE + 1];

	/* nur 50% aller Namen haben "Vor-Teil" */
	u=u;
	uv = rng_int() % (UNTOT_VOR * 2);

	uu = rng_int() % UNTOT;
	un = rng_int() % (UNTOT_NACH * (uv >= UNTOT_VOR ? 1 : 2));
	/* nur 50% aller Namen haben "Nach-Teil", wenn kein Vor-Teil */

	if (uv < UNTOT_VOR) {
		strcpy(name, untot_vor[uv]);
	} else {
		name[0] = 0;
	}

	strcat(name, untot[uu]);

	if (un < UNTOT_NACH)
		strcat(name, untot_nach[un]);

	return name;
}

/* Skelette */

#define SKEL_VOR 19

static const char *skel_vor[SKEL_VOR] =
{
	"Klapperige ",
	"Stöhnende ",
	"Schwarzknochige ",
	"Schwarzgewandete ",
	"Angsteinflößende ",
	"Heulende ",
	"Wartende ",
	"Grauenvolle ",
	"Schwarze ",
	"Dunkle ",
	"Fürchterliche ",
	"Grauenhafte ",
	"Furchtbare ",
	"Entsetzliche ",
	"Schauderhafte ",
	"Schreckliche ",
	"Düstere ",
	"Schaurige ",
	"Erbarmungslose "
};

#define SKEL	5

static const char *skel[SKEL] =
{
	"Skelette",
	"Kreaturen",
	"Krieger",
	"Kämpfer",
	"Rächer"
};

#define SKEL_NACH 14

static const char *skel_nach[SKEL_NACH] =
{
	" der Nacht",
	" der Schatten",
	" der Finsternis",
	" des Bösen",
	" der Erschlagenen",
	" der Verfluchten",
	" der Gefolterten",
	" der Ruhelosen",
	" aus dem Nebel",
	" aus dem Dunkel",
	" der Tiefe",
	" in Ketten",
	" aus dem Totenreich",
	" aus der Unterwelt"
};

const char *
skeleton_name(const unit * u)
{
	int uv, uu, un;
	static char name[NAMESIZE + 1];

	u=u;

	/* nur 20% aller Namen haben "Vor-Teil" */
	uv = rng_int() % (SKEL_VOR * 5);
	uu = rng_int() % SKEL;
	un = rng_int() % (SKEL_NACH * (uv >= SKEL_VOR ? 1 : 2));

	/* nur 50% aller Namen haben "Nach-Teil", wenn kein Vor-Teil */

	if (uv < SKEL_VOR) {
		strcpy(name, skel_vor[uv]);
	} else {
		name[0] = 0;
	}

	strcat(name, skel[uu]);

	if (un < SKEL_NACH)
		strcat(name, skel_nach[un]);

	return name;
}

/* Zombies */

#define ZOM_VOR 16

static const char *zombie_vor[ZOM_VOR] =
{
	"Faulende ",
	"Zerschlagene ",
	"Gefolterte ",
	"Angsteinflößende ",
	"Leise Schlurfende ",
	"Kinderfressende ",
	"Schwarze ",
	"Dunkle ",
	"Fürchterliche ",
	"Grauenhafte ",
	"Furchtbare ",
	"Entsetzliche ",
	"Schauderhafte ",
	"Schreckliche ",
	"Düstere ",
	"Schaurige "
};

#define ZOM	5

static const char *zombie[ZOM] =
{
	"Zombies",
	"Kreaturen",
	"Verlorene",
	"Erschlagene",
	"Verdammte"
};

#define ZOM_NACH 13

static const char *zombie_nach[ZOM_NACH] =
{
	" der Nacht",
	" der Schatten",
	" der Finsternis",
	" des Bösen",
	" der Erschlagenen",
	" der Verfluchten",
	" der Ruhelosen",
	" aus dem Nebel",
	" aus dem Dunkel",
	" der Tiefe",
	" in Ketten",
	" aus dem Totenreich",
	" aus der Unterwelt"
};

const char *
zombie_name(const unit * u)
{
	int uv, uu, un;
	static char name[NAMESIZE + 1];

	u=u;

	/* nur 20% aller Namen haben "Vor-Teil" */
	uv = rng_int() % (ZOM_VOR * 5);
	uu = rng_int() % ZOM;
	un = rng_int() % (ZOM_NACH * (uv >= ZOM_VOR ? 1 : 2));

	/* nur 50% aller Namen haben "Nach-Teil", wenn kein Vor-Teil */

	if (uv < ZOM_VOR) {
		strlcpy(name, zombie_vor[uv], sizeof(name));
	} else {
		name[0] = 0;
	}

	strlcat(name, zombie[uu], sizeof(name));

	if (un < ZOM_NACH)
		strlcat(name, zombie_nach[un], sizeof(name));

	return name;
}

/* Ghoule */

#define GHOUL_VOR 17

static const char *ghoul_vor[GHOUL_VOR] =
{
	"Faulende ",
	"Angsteinflößende ",
	"Leise ",
	"Kinderfressende ",
	"Menschenfressende ",
	"Wahnsinnige ",
	"Brutale ",
	"Schwarze ",
	"Dunkle ",
	"Fürchterliche ",
	"Grauenhafte ",
	"Furchtbare ",
	"Entsetzliche ",
	"Schauderhafte ",
	"Schreckliche ",
	"Düstere ",
	"Schaurige "
};

#define GHOUL	6

static const char *ghoul[GHOUL] =
{
	"Ghoule",
	"Kreaturen",
	"Verlorene",
	"Erschlagene",
	"Verdammte",
	"Schlurfende Ghoule",
};

#define GHOUL_NACH 13

static const char *ghoul_nach[GHOUL_NACH] =
{
	" der Nacht",
	" der Schatten",
	" der Finsternis",
	" des Bösen",
	" der Erschlagenen",
	" der Verfluchten",
	" der Ruhelosen",
	" aus dem Nebel",
	" aus dem Dunkel",
	" der Tiefe",
	" in Ketten",
	" aus dem Totenreich",
	" aus der Unterwelt"
};

const char *
ghoul_name(const unit * u)
{
	int uv, uu, un;
	static char name[NAMESIZE + 1];

	u=u;

	/* nur 20% aller Namen haben "Vor-Teil" */
	uv = rng_int() % (GHOUL_VOR * 5);
	uu = rng_int() % GHOUL;
	un = rng_int() % (GHOUL_NACH * (uv >= GHOUL_VOR ? 1 : 4));

	/* nur 25% aller Namen haben "Nach-Teil", wenn kein Vor-Teil */

	if (uv < GHOUL_VOR) {
		strcpy(name, ghoul_vor[uv]);
	} else {
		name[0] = 0;
	}

	strcat(name, ghoul[uu]);

	if (un < GHOUL_NACH)
		strcat(name, ghoul_nach[un]);

	return name;
}


/* Drachen */

#define SIL1 15

const char *silbe1[SIL1] = {
	"Tar",
	"Ter",
	"Tor",
	"Pan",
	"Par",
	"Per",
	"Nim",
	"Nan",
	"Nun",
	"Gor",
	"For",
	"Fer",
	"Kar",
	"Kur",
	"Pen",
};

#define SIL2 19

const char *silbe2[SIL2] = {
	"da",
	"do",
	"dil",
	"di",
	"dor",
	"dar",
	"ra",
	"ran",
	"ras",
	"ro",
	"rum",
	"rin",
	"ten",
	"tan",
	"ta",
	"tor",
	"gur",
	"ga",
	"gas",
};

#define SIL3 14

const char *silbe3[SIL3] = {
	"gul",
	"gol",
	"dol",
	"tan",
	"tar",
	"tur",
	"sur",
	"sin",
	"kur",
	"kor",
	"kar",
	"dul",
	"dol",
	"bus",
};

#define DTITEL 5

const char *dtitel[6][DTITEL] =
{
	{							/* Ebene, Hochland */
		"der Weise",
		"der Allwissende",
		"der Mächtige",
		"die Ehrwürdige",
		"die Listige"
	},
	{			/* Wald */
		"der Grüne",
		"die Strafende",
		"der Sehende",
		"der Reisende",
		"die Wissende"
	},
	{			/* Berge */
		"der Goldene",
		"der Graue",
		"der Steinerne",
		"die Alte",
		"die Mächtige"
	},
	{							/* Wüste */
		"die Goldene",
		"der Grausame",
		"der Sanddrache",
		"der Durstige",
		"die Verzehrende"
	},
	{			/* Sumpf */
		"die Grüne",
		"die Rote",
		"der Furchtlose",
		"der Allmächtige",
		"der Weitblickende"
	},
	{			/* Gletscher */
		"der Weiße",
		"die Glänzende",
		"der Wissende",
		"die Unbarmherzige",
		"die Schöne"
	}
};

const char *
shadow_name(const unit *u)
{
	if(u->no == 1) {
		return "Schattendämon";
	}
	return "Schattendämonen";
}

const char *
drachen_name(const unit *u)
{
	static char name[NAMESIZE + 1];
	int rnd = rng_int() % DTITEL;
	const char *t = dtitel[0][rnd];
	int anzahl = 1;

	if (u) {
		region *r = u->region;
		anzahl = u->number;
		switch (rterrain(r)) {
		case T_PLAIN:
			t = dtitel[1][rnd];
			break;
		case T_MOUNTAIN:
			t = dtitel[2][rnd];
			break;
		case T_DESERT:
			t = dtitel[3][rnd];
			break;
		case T_SWAMP:
			t = dtitel[4][rnd];
			break;
		case T_GLACIER:
			t = dtitel[5][rnd];
			break;
		}
	}

	if (anzahl > 1) {
		sprintf(name, "Die %sn von %s", t+4, rname(u->region, NULL));
	} else {
		char n[32];

		strcpy(n, silbe1[rng_int() % SIL1]);
		strcat(n, silbe2[rng_int() % SIL2]);
		strcat(n, silbe3[rng_int() % SIL3]);
		if (rng_int() % 5 > 2) {
			sprintf(name, "%s, %s", n, t);	/* "Name, der Titel" */
		} else {
			strcpy(name, t);	/* "Der Titel Name" */
			name[0] = (char) toupper(name[0]);
			strcat(name, " ");
			strcat(name, n);
		}
		if (u && (rng_int() % 3 == 0)) {
			strcat(name, " von ");
			strcat(name, rname(u->region, NULL));
		}
	}

	return name;
}

/* Dracoide */

#define DRAC_PRE 13
static const char *drac_pre[DRAC_PRE] = {
		"Siss",
		"Xxaa",
		"Shht",
		"X'xixi",
		"Xar",
		"X'zish",
		"X",
		"Sh",
		"R",
		"Z",
		"Y",
		"L",
		"Ck",
};

#define DRAC_MID 12
static const char *drac_mid[DRAC_MID] = {
		"siss",
		"xxaa",
		"shht",
		"xxi",
		"xar",
		"x'zish",
		"x",
		"sh",
		"r",
		"z'ck",
		"y",
		"rl"
};

#define DRAC_SUF 10
static const char *drac_suf[DRAC_SUF] = {
		"xil",
		"shh",
		"s",
		"x",
		"arr",
		"lll",
		"lll",
		"shack",
		"ck",
		"k"
};

const char *
dracoid_name(const unit *u)
{
	static char name[NAMESIZE + 1];
	int         mid_syllabels;

	u=u;
	/* Wieviele Mittelteile? */

	mid_syllabels = rng_int()%4;

	strcpy(name, drac_pre[rng_int()%DRAC_PRE]);
	while(mid_syllabels > 0) {
		mid_syllabels--;
		if(rng_int()%10 < 4) strcat(name,"'");
		strcat(name, drac_mid[rng_int()%DRAC_MID]);
	}
	strcat(name, drac_suf[rng_int()%DRAC_SUF]);
	return name;
}

const char *
abkz(const char *s, size_t max)
{
	static char buf[32];
	const char *p = s;
	unsigned int c = 0;
	size_t bpt, i;

	max = min(max, 79);

	/* Prüfen, ob Kurz genug */

	if (strlen(s) <= max) {
		return s;
	}
	/* Anzahl der Wörter feststellen */

	while (*p != 0) {
		/* Leerzeichen überspringen */
		while (*p != 0 && !isalnum(*(unsigned char*)p))
			p++;

		/* Counter erhöhen */
		if (*p != 0)
			c++;

		/* alnums überspringen */
		while(*p != 0 && isalnum(*(unsigned char*)p))
			p++;
	}

	/* Buchstaben pro Teilkürzel = max(1,max/AnzWort) */

	bpt = max(1, max / c);

	/* Einzelne Wörter anspringen und jeweils die ersten BpT kopieren */

	p = s;
	c = 0;

	while (*p != 0 && c < max) {
		/* Leerzeichen überspringen */

		while (*p != 0 && !isalnum(*(unsigned char*)p))
			p++;

		/* alnums übertragen */

		for (i = 0; i < bpt && *p != 0 && isalnum(*(unsigned char*)p); i++) {
			buf[c] = *p;
			c++;
			p++;
		}

		/* Bis zum nächsten Leerzeichen */

		while (c < max && *p != 0 && isalnum(*(unsigned char*)p))
			p++;
	}

	buf[c] = 0;

	return buf;
}

void
name_unit(unit *u)
{
	char name[16];

	if (u->race->generate_name) {
		set_string(&u->name, (u->race->generate_name(u)));
	} else {
		sprintf(name, "%s %s", LOC(u->faction->locale, "unitdefault"), itoa36(u->no));
		set_string(&u->name, name);
	}
}
