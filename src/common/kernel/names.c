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
#include <util/functions.h>
#include <util/rng.h>

/* Untote */


const xmlChar *
describe_braineater(unit * u, const struct locale * lang)
{
  return LOC(lang, "describe_braineater");
}

static const xmlChar *
make_names(const char * monster, int * num_postfix, int pprefix, int * num_name, int * num_prefix, int ppostfix)
{
  int uv, uu, un;
  static char name[NAMESIZE + 1];
  char zText[32];
  const xmlChar * str;

  if (*num_prefix==0) {

    for (*num_prefix=0;;++*num_prefix) {
      sprintf(zText, "%s_prefix_%d", monster, *num_prefix);
      str = locale_getstring(default_locale, zText);
      if (str==NULL) break;
    }

    for (*num_name=0;;++*num_name) {
      sprintf(zText, "%s_name_%d", monster, *num_name);
      str = locale_getstring(default_locale, zText);
      if (str==NULL) break;
    }

    for (*num_postfix=0;;++*num_postfix) {
      sprintf(zText, "%s_postfix_%d", monster, *num_postfix);
      str = locale_getstring(default_locale, zText);
      if (str==NULL) break;
    }
  }

  if (*num_name==0) {
    return NULL;
  }

  /* nur 50% aller Namen haben "Vor-Teil" */
  uv = rng_int() % (*num_prefix * pprefix);

  uu = rng_int() % *num_name;

  /* nur 50% aller Namen haben "Nach-Teil", wenn kein Vor-Teil */
  if (uv>=*num_prefix) {
    un = rng_int() % *num_postfix;
  } else {
    un = rng_int() % (*num_postfix * ppostfix);
  }

  name[0] = 0;
  if (uv < *num_prefix) {
    sprintf(zText, "%s_prefix_%d", monster, uv);
    str = locale_getstring(default_locale, zText);
    if (str) {
      strcat(name, (const char *)str);
      strcat(name, " ");
    }
  }

  sprintf(zText, "%s_name_%d", monster, uu);
  str = locale_getstring(default_locale, zText);
  if (str) strcat(name, (const char *)str);

  if (un < *num_postfix) {
    sprintf(zText, "%s_postfix_%d", monster, un);
    str = locale_getstring(default_locale, zText);
    if (str) {
      strcat(name, " ");
      strcat(name, (const char *)str);
    }
  }
  return (const xmlChar *)name;
}

const xmlChar *
undead_name(const unit * u)
{
  static int num_postfix, num_name, num_prefix;
  return make_names("undead", &num_postfix, 2, &num_name, &num_prefix, 2);
}

const xmlChar *
skeleton_name(const unit * u)
{
  static int num_postfix, num_name, num_prefix;
  return make_names("skeleton", &num_postfix, 5, &num_name, &num_prefix, 2);
}

const xmlChar *
zombie_name(const unit * u)
{
  static int num_postfix, num_name, num_prefix;
  return make_names("zombie", &num_postfix, 5, &num_name, &num_prefix, 2);
}

const xmlChar *
ghoul_name(const unit * u)
{
  static int num_postfix, num_name, num_prefix;
  return make_names("ghoul", &num_postfix, 5, &num_name, &num_prefix, 4);
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

const xmlChar *dtitel[6][DTITEL] =
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

const xmlChar *
generic_name(const unit *u)
{
  if (u->no == 1) {
    return LOC(u->faction->locale, u->race->_name[0]);
  }
  return LOC(u->faction->locale, u->race->_name[1]);
}

const xmlChar *
dragon_name(const unit *u)
{
	static char name[NAMESIZE + 1];
	int rnd = rng_int() % DTITEL;
	const xmlChar *t = dtitel[0][rnd];
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

const xmlChar *
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
	return (const xmlChar *)name;
}

const xmlChar *
abkz(const xmlChar *s, size_t max)
{
	static char buf[32];
	const xmlChar *p = s;
	unsigned int c = 0;
	size_t bpt, i;

	max = min(max, 79);

	/* Prüfen, ob Kurz genug */

	if (xstrlen(s) <= max) {
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
register_names(void)
{
  register_function((pf_generic)describe_braineater, "describe_braineater");
  /* function name 
   * generate a name for a nonplayerunit
   * race->generate_name() */
  register_function((pf_generic)undead_name, "nameundead");
  register_function((pf_generic)skeleton_name, "nameskeleton");
  register_function((pf_generic)zombie_name, "namezombie");
  register_function((pf_generic)ghoul_name, "nameghoul");
  register_function((pf_generic)dragon_name, "namedragon");
  register_function((pf_generic)dracoid_name, "namedracoid");
  register_function((pf_generic)generic_name, "namegeneric");
}


