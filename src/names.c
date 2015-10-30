/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <kernel/config.h>
#include "names.h"

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>

/* util includes */
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/rng.h>
#include <util/unicode.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <wctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *describe_braineater(unit * u, const struct locale *lang)
{
    return LOC(lang, "describe_braineater");
}

static const char *make_names(const char *monster, int *num_postfix,
    int pprefix, int *num_name, int *num_prefix, int ppostfix)
{
    int uv, uu, un;
    static char name[NAMESIZE + 1]; // FIXME: static return value
    char zText[32];
    const char *str;

    if (*num_prefix == 0) {

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

    if (*num_name == 0) {
        return NULL;
    }

    /* nur 50% aller Namen haben "Vor-Teil" */
    uv = rng_int() % (*num_prefix * pprefix);

    uu = rng_int() % *num_name;

    /* nur 50% aller Namen haben "Nach-Teil", wenn kein Vor-Teil */
    if (uv >= *num_prefix) {
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
            size_t sz = strlcpy(name, (const char *)str, sizeof(name));
            strlcpy(name + sz, " ", sizeof(name) - sz);
        }
    }

    sprintf(zText, "%s_name_%d", monster, uu);
    str = locale_getstring(default_locale, zText);
    if (str)
        strlcat(name, (const char *)str, sizeof(name));

    if (un < *num_postfix) {
        sprintf(zText, "%s_postfix_%d", monster, un);
        str = locale_getstring(default_locale, zText);
        if (str) {
            strlcat(name, " ", sizeof(name));
            strlcat(name, (const char *)str, sizeof(name));
        }
    }
    return name;
}

static const char *undead_name(const unit * u)
{
    static int num_postfix, num_name, num_prefix;
    return make_names("undead", &num_postfix, 2, &num_name, &num_prefix, 2);
}

static const char *skeleton_name(const unit * u)
{
    static int num_postfix, num_name, num_prefix;
    return make_names("skeleton", &num_postfix, 5, &num_name, &num_prefix, 2);
}

static const char *zombie_name(const unit * u)
{
    static int num_postfix, num_name, num_prefix;
    return make_names("zombie", &num_postfix, 5, &num_name, &num_prefix, 2);
}

static const char *ghoul_name(const unit * u)
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

static const char *generic_name(const unit * u)
{
    const char * name = rc_name_s(u_race(u), (u->number == 1) ? NAME_SINGULAR : NAME_PLURAL);
    return LOC(u->faction->locale, name);
}

static const char *dragon_name(const unit * u)
{
    static char name[NAMESIZE + 1]; // FIXME: static return value
    int rnd, ter = 0;
    int anzahl = 1;
    static int num_postfix;
    char zText[32];
    const char *str;

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
    if (num_postfix <= 0) {
        return NULL;
    }

    if (u) {
        region *r = u->region;
        anzahl = u->number;
        switch (rterrain(r)) {
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
        }
    }

    rnd = num_postfix / 6;
    rnd = (rng_int() % rnd) + ter * rnd;

    sprintf(zText, "dragon_postfix_%d", rnd);

    str = locale_getstring(default_locale, zText);
    assert(str != NULL);

    if (anzahl > 1) {
        const char *no_article = strchr((const char *)str, ' ');
        assert(no_article);
        // TODO: localization
        sprintf(name, "Die %sn von %s", no_article + 1, rname(u->region,
            default_locale));
    }
    else {
        char n[32];
        size_t sz;

        sz = strlcpy(n, silbe1[rng_int() % SIL1], sizeof(n));
        sz += strlcat(n, silbe2[rng_int() % SIL2], sizeof(n));
        sz += strlcat(n, silbe3[rng_int() % SIL3], sizeof(n));
        if (rng_int() % 5 > 2) {
            sprintf(name, "%s, %s", n, str);  /* "Name, der Titel" */
        }
        else {
            sz = strlcpy(name, (const char *)str, sizeof(name));  /* "Der Titel Name" */
            name[0] = (char)toupper(name[0]); /* TODO: UNICODE - should use towupper() */
            sz += strlcat(name, " ", sizeof(name));
            sz += strlcat(name, n, sizeof(name));
        }
        if (u && (rng_int() % 3 == 0)) {
            sz += strlcat(name, " von ", sizeof(name));
            sz += strlcat(name, (const char *)rname(u->region, default_locale), sizeof(name));
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

static const char *dracoid_name(const unit * u)
{
    static char name[NAMESIZE + 1]; // FIXME: static return value
    int mid_syllabels;
    size_t sz;

    /* ignore u */
    u = 0;
    /* Wieviele Mittelteile? */

    mid_syllabels = rng_int() % 4;

    sz = strlcpy(name, drac_pre[rng_int() % DRAC_PRE], sizeof(name));
    while (mid_syllabels > 0) {
        mid_syllabels--;
        if (rng_int() % 10 < 4)
            strlcat(name, "'", sizeof(name));
        sz += strlcat(name, drac_mid[rng_int() % DRAC_MID], sizeof(name));
    }
    sz += strlcat(name, drac_suf[rng_int() % DRAC_SUF], sizeof(name));
    return name;
}

/** returns an abbreviation of a string.
 * TODO: buflen is being ignored */

const char *abkz(const char *s, char *buf, size_t buflen, size_t maxchars)
{
    const char *p = s;
    char *bufp;
    unsigned int c = 0;
    size_t bpt, i;
    ucs4_t ucs;
    size_t size;
    int result;

    /* Prüfen, ob Kurz genug */

    if (strlen(s) <= maxchars) {
        return s;
    }
    /* Anzahl der Wörter feststellen */

    while (*p != 0) {

        result = unicode_utf8_to_ucs4(&ucs, p, &size);
        assert(result == 0 || "damnit, we're not handling invalid input here!");

        /* Leerzeichen überspringen */
        while (*p != 0 && !iswalnum((wint_t)ucs)) {
            p += size;
            result = unicode_utf8_to_ucs4(&ucs, p, &size);
            assert(result == 0 || "damnit, we're not handling invalid input here!");
        }

        /* Counter erhöhen */
        if (*p != 0)
            ++c;

        /* alnums überspringen */
        while (*p != 0 && iswalnum((wint_t)ucs)) {
            p += size;
            result = unicode_utf8_to_ucs4(&ucs, p, &size);
            assert(result == 0 || "damnit, we're not handling invalid input here!");
        }
    }

    /* Buchstaben pro Teilkürzel = _max(1,max/AnzWort) */

    bpt = _max(1, maxchars / c);

    /* Einzelne Wörter anspringen und jeweils die ersten BpT kopieren */

    p = s;
    c = 0;
    bufp = buf;

    result = unicode_utf8_to_ucs4(&ucs, p, &size);
    assert(result == 0 || "damnit, we're not handling invalid input here!");

    while (*p != 0 && c < maxchars) {
        /* Leerzeichen überspringen */

        while (*p != 0 && !iswalnum((wint_t)ucs)) {
            p += size;
            result = unicode_utf8_to_ucs4(&ucs, p, &size);
            assert(result == 0 || "damnit, we're not handling invalid input here!");
        }

        /* alnums übertragen */

        for (i = 0; i < bpt && *p != 0 && iswalnum((wint_t)ucs); ++i) {
            memcpy(bufp, p, size);
            p += size;
            bufp += size;
            ++c;

            result = unicode_utf8_to_ucs4(&ucs, p, &size);
            assert(result == 0 || "damnit, we're not handling invalid input here!");
        }

        /* Bis zum nächsten Leerzeichen */

        while (c < maxchars && *p != 0 && iswalnum((wint_t)ucs)) {
            p += size;
            result = unicode_utf8_to_ucs4(&ucs, p, &size);
            assert(result == 0 || "damnit, we're not handling invalid input here!");
        }
    }

    *bufp = 0;

    return buf;
}

void register_names(void)
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
