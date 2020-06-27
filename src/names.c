#include <platform.h>
#include <kernel/config.h>
#include "names.h"

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>

/* util includes */
#include <util/base36.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/macros.h>
#include <util/rng.h>
#include <util/strings.h>
#include <util/unicode.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <wctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void count_particles(const char *monster, int *num_prefix, int *num_name, int *num_postfix) 
{
    char zText[32];
    const char *str;

    for (*num_prefix = 0;; ++*num_prefix) {
        sprintf(zText, "%s_prefix_%d", monster, *num_prefix);
        str = locale_getstring(default_locale, zText);
        if (str == NULL)
            break;
    }

    for (*num_name = 0;; ++*num_name) {
        sprintf(zText, "%s_name_%d", monster, *num_name);
        str = locale_getstring(default_locale, zText);
        if (str == NULL)
            break;
    }

    for (*num_postfix = 0;; ++*num_postfix) {
        sprintf(zText, "%s_postfix_%d", monster, *num_postfix);
        str = locale_getstring(default_locale, zText);
        if (str == NULL)
            break;
    }
}

static void make_name(unit *u, const char *monster, int *num_postfix,
    int pprefix, int *num_name, int *num_prefix, int ppostfix)
{
    if (*num_name == 0) {
        count_particles(monster, num_prefix, num_name, num_postfix);
    }
    if (*num_name > 0) {
        char name[NAMESIZE + 1];
        char zText[32];
        int uv = 0, uu = 0, un = 0;
        const char *str;

        if (*num_prefix > 0) {
            uv = rng_int() % (*num_prefix * pprefix);
        }
        uu = rng_int() % *num_name;

        if (*num_postfix > 0 && uv >= *num_prefix) {
            un = rng_int() % *num_postfix;
        }
        else {
            un = rng_int() % (*num_postfix * ppostfix);
        }

        name[0] = 0;
        if (uv < *num_prefix) {
            sprintf(zText, "%s_prefix_%d", monster, uv);
            str = locale_getstring(default_locale, zText);
            if (str) {
                size_t sz = str_strlcpy(name, (const char *)str, sizeof(name));
                str_strlcpy(name + sz, " ", sizeof(name) - sz);
            }
        }

        sprintf(zText, "%s_name_%d", monster, uu);
        str = locale_getstring(default_locale, zText);
        if (str)
            str_strlcat(name, (const char *)str, sizeof(name));

        if (un < *num_postfix) {
            sprintf(zText, "%s_postfix_%d", monster, un);
            str = locale_getstring(default_locale, zText);
            if (str) {
                str_strlcat(name, " ", sizeof(name));
                str_strlcat(name, (const char *)str, sizeof(name));
            }
        }
        unit_setname(u, name);
    }
}

static void undead_name(unit * u)
{
    static int num_postfix, num_name, num_prefix;
    make_name(u, "undead", &num_postfix, 2, &num_name, &num_prefix, 2);
}

static void skeleton_name(unit * u)
{
    static int num_postfix, num_name, num_prefix;
    make_name(u, "skeleton", &num_postfix, 5, &num_name, &num_prefix, 2);
}

static void zombie_name(unit * u)
{
    static int num_postfix, num_name, num_prefix;
    make_name(u, "zombie", &num_postfix, 5, &num_name, &num_prefix, 2);
}

static void ghoul_name(unit * u)
{
    static int num_postfix, num_name, num_prefix;
    make_name(u, "ghoul", &num_postfix, 5, &num_name, &num_prefix, 4);
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

static void dragon_name(unit * u)
{
    char name[NAMESIZE + 1];
    int rnd, ter = 0;
    static int num_postfix;
    char zText[32];
    const char *str;

    assert(u);
    if (num_postfix == 0) {
        for (num_postfix = 0;; ++num_postfix) {
            sprintf(zText, "dragon_postfix_%d", num_postfix);
            str = locale_getstring(default_locale, zText);
            if (str == NULL)
                break;
        }
        if (num_postfix == 0)
            num_postfix = -1;
    }

    switch (rterrain(u->region)) {
    case T_PLAIN:
        ter = 1;
        break;
    case T_MOUNTAIN:
        ter = 2;
        break;
    case T_DESERT:
        ter = 3;
        break;
    case T_SWAMP:
        ter = 4;
        break;
    case T_GLACIER:
        ter = 5;
        break;
    default:
        ter = 0;
    }

    if (num_postfix <=0) {
        return;
    }
    else if (num_postfix < 6) {
        rnd = rng_int() % num_postfix;
    }
    else {
        rnd = num_postfix / 6;
        rnd = (rng_int() % rnd) + ter * rnd;
    }
    snprintf(zText, sizeof(zText), "dragon_postfix_%d", rnd);

    str = locale_getstring(default_locale, zText);
    assert(str != NULL);

    if (u->region->land && (u->number > 1)) {
        const char *no_article = strchr((const char *)str, ' ');
        assert(no_article);
        /* TODO: localization */
        snprintf(name, sizeof(name), "Die %sn von %s", no_article + 1, rname(u->region,
            default_locale));
    }
    else {
        char n[32];

        snprintf(n, sizeof(n), "%s%s%s", silbe1[rng_int() % SIL1], silbe2[rng_int() % SIL2], silbe3[rng_int() % SIL3]);
        if (rng_int() % 5 > 2) {
            sprintf(name, "%s, %s", n, str);  /* "Name, der Titel" */
        }
        else {
            if (u->region->land && (rng_int() % 3 == 0)) {
                /* TODO: localization */
                snprintf(name, sizeof(name), "%s %s von %s", n, str, rname(u->region, default_locale));
            }
            else {
                snprintf(name, sizeof(name), "%s %s", n, str);
            }
            name[0] = (char)toupper(name[0]); /* TODO: UNICODE - should use towupper() */
        }
    }

    unit_setname(u, name);
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

static void dracoid_name(unit * u)
{
    static char name[NAMESIZE + 1];
    int mid_syllabels;

    /* ignore u */
    UNUSED_ARG(u);
    /* Wieviele Mittelteile? */

    mid_syllabels = rng_int() % 4;

    str_strlcpy(name, drac_pre[rng_int() % DRAC_PRE], sizeof(name));
    while (mid_syllabels > 0) {
        mid_syllabels--;
        if (rng_int() % 10 < 4)
            str_strlcat(name, "'", sizeof(name));
        str_strlcat(name, drac_mid[rng_int() % DRAC_MID], sizeof(name));
    }
    str_strlcat(name, drac_suf[rng_int() % DRAC_SUF], sizeof(name));
    unit_setname(u, name);
}

void register_names(void)
{
    /* function name
     * generate a name for a nonplayerunit
     * race->name_unit() */

    register_race_function(undead_name, "name_undead");
    register_race_function(skeleton_name, "name_skeleton");
    register_race_function(zombie_name, "name_zombie");
    register_race_function(ghoul_name, "name_ghoul");
    register_race_function(dracoid_name, "name_dracoid");
    register_race_function(dragon_name, "name_dragon");
    register_race_function(dragon_name, "name_youngdragon");
    register_race_function(dragon_name, "name_wyrm");
}
