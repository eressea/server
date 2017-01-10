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
#include "curse.h"

/* kernel includes */
#include "building.h"
#include "faction.h"
#include "messages.h"
#include "objtypes.h"
#include "race.h"
#include "region.h"
#include "ship.h"
#include "skill.h"
#include "unit.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/gamedata.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/log.h>
#include <util/nrmessage.h>
#include <util/rand.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/variant.h>

#include <storage.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>

#define MAXENTITYHASH 7919
static curse *cursehash[MAXENTITYHASH];

void c_setflag(curse * c, unsigned int flags)
{
    assert(c);
    c->flags = (c->flags & ~flags) | (flags & (c->type->flags ^ flags));
}

/* -------------------------------------------------------------------------- */
void c_clearflag(curse * c, unsigned int flags)
{
    assert(c);
    c->flags = (c->flags & ~flags) | (c->type->flags & flags);
}

void chash(curse * c)
{
    int i = c->no % MAXENTITYHASH;

    c->nexthash = cursehash[i];
    cursehash[i] = c;
    assert(c->nexthash != c);
}

static void cunhash(curse * c)
{
    curse **show;
    int i = c->no % MAXENTITYHASH;

    for (show = &cursehash[i]; *show;
        show = &(*show)->nexthash) {
        if ((*show)->no == c->no)
            break;
    }
    if (*show) {
        assert(*show == c);
        *show = (*show)->nexthash;
        c->nexthash = 0;
    }
}

/* ------------------------------------------------------------- */
/* at_curse */
void curse_init(attrib * a)
{
    a->data.v = calloc(1, sizeof(curse));
}

int curse_age(attrib * a, void *owner)
{
    curse *c = (curse *)a->data.v;
    int result = 0;

    UNUSED_ARG(owner);
    c_clearflag(c, CURSE_ISNEW);

    if (c_flags(c) & CURSE_NOAGE) {
        c->duration = INT_MAX;
    }
    if (c->type->age) {
        result = c->type->age(c);
    }
    if (result != 0) {
        c->duration = 0;
    }
    else if (c->duration != INT_MAX) {
        c->duration = MAX(0, c->duration - 1);
    }
    return (c->duration > 0) ? AT_AGE_KEEP : AT_AGE_REMOVE;
}

void destroy_curse(curse * c)
{
    cunhash(c);
    free(c);
}

void curse_done(attrib * a)
{
    destroy_curse((curse *)a->data.v);
}

/** reads curses that have been removed from the code */
static int read_ccompat(const char *cursename, struct storage *store)
{
    struct compat {
        const char *name;
        const char *tokens;
    } *seek, old_curses[] = { {
            "disorientationzone", "" }, {
                "shipdisorientation", "" }, {
                    NULL, NULL } };
    for (seek = old_curses; seek->name; ++seek) {
        if (strcmp(seek->tokens, cursename) == 0) {
            const char *p;
            for (p = seek->name; p; ++p) {
                switch (*p) {
                case 'd':
                    READ_INT(store, 0);
                    break;
                case 's':
                    READ_STR(store, 0, 0);
                    break;
                case 't':
                    READ_TOK(store, 0, 0);
                    break;
                case 'i':
                    READ_INT(store, 0);
                    break;
                case 'f':
                    READ_FLT(store, 0);
                    break;
                }
            }
            return 0;
        }
    }
    return -1;
}

int curse_read(attrib * a, void *owner, gamedata *data)
{
    storage *store = data->store;
    curse *c = (curse *)a->data.v;
    int ur;
    char cursename[64];
    int n;
    int flags;
    float flt;

    assert(!c->no);
    READ_INT(store, &c->no);
    chash(c);
    READ_TOK(store, cursename, sizeof(cursename));
    READ_INT(store, &flags);
    READ_INT(store, &c->duration);
    READ_FLT(store, &flt);
    c->vigour = flt;
    if (data->version < INTPAK_VERSION) {
        ur = resolve_unit(read_int(data->store), &c->magician);
    }
    else {
        ur = read_reference(&c->magician, data, read_unit_reference, resolve_unit);
    }
    if (data->version < CURSEFLOAT_VERSION) {
        READ_INT(store, &n);
        c->effect = (float)n;
    }
    else {
        READ_FLT(store, &flt);
        c->effect = flt;
    }
    c->type = ct_find(cursename);
    if (c->type == NULL) {
        int result = read_ccompat(cursename, store);
        if (result != 0) {
            log_error("missing curse %s, no compatibility code either.\n", cursename);
        }
        assert(result == 0);
        return AT_READ_FAIL;
    }
    c->flags = flags;
    if (data->version < EXPLICIT_CURSE_ISNEW_VERSION) {
        c_clearflag(c, CURSE_ISNEW);
    }

    if (c->type->read) {
        c->type->read(data, c, owner);
    }
    else if (c->type->typ == CURSETYP_UNIT) {
        READ_INT(store, &c->data.i);
    }
    if (c->type->typ == CURSETYP_REGION) {
        int rr =
            read_reference(&c->data.v, data, read_region_reference,
            RESOLVE_REGION(data->version));
        if (ur == 0 && rr == 0 && !c->data.v) {
            return AT_READ_FAIL;
        }
    }

    return AT_READ_OK;
}

void curse_write(const attrib * a, const void *owner, struct storage *store)
{
    unsigned int flags;
    curse *c = (curse *)a->data.v;
    const curse_type *ct = c->type;
    unit *mage = (c->magician && c->magician->number) ? c->magician : NULL;

    /* copied from c_clearflag */
#if RELEASE_VERSION < EXPLICIT_CURSE_ISNEW_VERSION
    flags = (c->flags & ~CURSE_ISNEW) | (c->type->flags & CURSE_ISNEW);
#else
    flags = c->flags | c->type->flags;
#endif

    WRITE_INT(store, c->no);
    WRITE_TOK(store, ct->cname);
    WRITE_INT(store, flags);
    WRITE_INT(store, c->duration);
    WRITE_FLT(store, (float)c->vigour);
    write_unit_reference(mage, store);
    WRITE_FLT(store, (float)c->effect);

    if (c->type->write)
        c->type->write(store, c, owner);
    else if (c->type->typ == CURSETYP_UNIT) {
        WRITE_INT(store, c->data.i);
    }
    if (c->type->typ == CURSETYP_REGION) {
        write_region_reference((region *)c->data.v, store);
    }
}

attrib_type at_curse = {
    "curse",
    curse_init,
    curse_done,
    curse_age,
    curse_write,
    curse_read,
    NULL,
    ATF_CURSE
};

/* ------------------------------------------------------------- */
/* Spruch identifizieren */

#include <util/umlaut.h>
#include <quicklist.h>

#define MAXCTHASH 128
static quicklist *cursetypes[MAXCTHASH];
static int ct_changes = 1;

bool ct_changed(int *cache)
{
    assert(cache);
    if (*cache != ct_changes) {
        *cache = ct_changes;
        return true;
    }
    return false;
}

void ct_register(const curse_type * ct)
{
    unsigned int hash = tolower(ct->cname[0]) & 0xFF;
    quicklist **ctlp = cursetypes + hash;

    ql_set_insert(ctlp, (void *)ct);
    ++ct_changes;
}

void ct_remove(const char *c)
{
    unsigned int hash = tolower(c[0]);
    quicklist *ctl = cursetypes[hash];

    if (ctl) {
        int qi;

        for (qi = 0; ctl; ql_advance(&ctl, &qi, 1)) {
            curse_type *type = (curse_type *)ql_get(ctl, qi);

            if (strcmp(c, type->cname) == 0) {
                ql_delete(&ctl, qi);
                ++ct_changes;
                break;
            }
        }
    }
}

const curse_type *ct_find(const char *c)
{
    unsigned int hash = tolower(c[0]);
    quicklist *ctl = cursetypes[hash];

    if (ctl) {
        size_t c_len = strlen(c);
        int qi;

        for (qi = 0; ctl; ql_advance(&ctl, &qi, 1)) {
            curse_type *type = (curse_type *)ql_get(ctl, qi);

            if (strcmp(c, type->cname) == 0) {
                return type;
            }
            else {
                size_t k = MIN(c_len, strlen(type->cname));
                if (!memcmp(c, type->cname, k)) {
                    return type;
                }
            }
        }
    }
    return NULL;
}

void ct_checknames(void) {
    int i, qi;
    quicklist *ctl;

    for (i = 0; i < MAXCTHASH; ++i) {
        ctl = cursetypes[i];
        for (qi = 0; ctl; ql_advance(&ctl, &qi, 1)) {
            curse_type *type = (curse_type *)ql_get(ctl, qi);
            curse_name(type, default_locale);

        }
    }
}

/* ------------------------------------------------------------- */
/* get_curse identifiziert eine Verzauberung über die ID und gibt
 * einen pointer auf die struct zurück.
 */

static bool cmp_curse(const attrib * a, const void *data)
{
    const curse *c = (const curse *)data;
    if (a->type->flags & ATF_CURSE) {
        if (!data || c == (curse *)a->data.v)
            return true;
    }
    return false;
}

curse *get_curse(attrib * ap, const curse_type * ctype)
{
    attrib *a = ap;
    if (!ctype) return NULL;
    while (a) {
        if (a->type->flags & ATF_CURSE) {
            const attrib_type *at = a->type;
            while (a && a->type == at) {
                curse *c = (curse *)a->data.v;
                if (c->type == ctype)
                    return c;
                a = a->next;
            }
        }
        else {
            a = a->nexttype;
        }
    }
    return NULL;
}

/* ------------------------------------------------------------- */
/* findet einen curse global anhand seiner 'curse-Einheitnummer' */

curse *findcurse(int i)
{
    curse *old;
    for (old = cursehash[i % MAXENTITYHASH]; old; old = old->nexthash)
        if (old->no == i)
            return old;
    return NULL;
}

/* ------------------------------------------------------------- */
bool remove_curse(attrib ** ap, const curse * c)
{
    attrib *a = a_select(*ap, c, cmp_curse);
    return a && a_remove(ap, a) == 1;
}

/* gibt die allgemeine Stärke der Verzauberung zurück. id2 wird wie
 * oben benutzt. Dies ist nicht die Wirkung, sondern die Kraft und
 * damit der gegen Antimagie wirkende Widerstand einer Verzauberung */
static double get_cursevigour(const curse * c)
{
    return c ? c->vigour : 0;
}

/* setzt die Stärke der Verzauberung auf i */
static void set_cursevigour(curse * c, double vigour)
{
    assert(c && vigour > 0);
    c->vigour = vigour;
}

/* verändert die Stärke der Verzauberung um +i und gibt die neue
 * Stärke zurück. Sollte die Zauberstärke unter Null sinken, löst er
 * sich auf.
 */
double curse_changevigour(attrib ** ap, curse * c, double vigour)
{
    vigour += get_cursevigour(c);

    if (vigour <= 0) {
        remove_curse(ap, c);
        vigour = 0;
    }
    else {
        set_cursevigour(c, vigour);
    }
    return vigour;
}

/* ------------------------------------------------------------- */

double curse_geteffect(const curse * c)
{
    if (c == NULL)
        return 0;
    if (c_flags(c) & CURSE_ISNEW)
        return 0;
    return c->effect;
}

int curse_geteffect_int(const curse * c)
{
    double effect = curse_geteffect(c);
    if (effect - (int)effect != 0) {
        log_error("curse has an integer attribute with float value: '%s' = %lf",
            c->type->cname, effect);
    }
    return (int)effect;
}

/* ------------------------------------------------------------- */
static void
set_curseingmagician(struct unit *magician, struct attrib *ap_target,
const curse_type * ct)
{
    curse *c = get_curse(ap_target, ct);
    if (c) {
        c->magician = magician;
    }
}

/* ------------------------------------------------------------- */
/* gibt bei Personenbeschränkten Verzauberungen die Anzahl der
 * betroffenen Personen zurück. Ansonsten wird 0 zurückgegeben. */
int get_cursedmen(unit * u, const curse * c)
{
    int cursedmen = u->number;

    if (!c)
        return 0;

    /* je nach curse_type andere data struct */
    if (c->type->typ == CURSETYP_UNIT) {
        cursedmen = c->data.i;
    }

    return MIN(u->number, cursedmen);
}

/* setzt die Anzahl der betroffenen Personen auf cursedmen */
static void set_cursedmen(curse * c, int cursedmen)
{
    if (!c)
        return;

    /* je nach curse_type andere data struct */
    if (c->type->typ == CURSETYP_UNIT) {
        c->data.i = cursedmen;
    }
}

static int newcurseid(void) {
    int random_no;
    int start_random_no;
    random_no = 1 + (rng_int() % MAX_UNIT_NR);
    start_random_no = random_no;

    while (findcurse(random_no)) {
        random_no++;
        if (random_no == MAX_UNIT_NR + 1) {
            random_no = 1;
        }
        if (random_no == start_random_no) {
            random_no = (int)MAX_UNIT_NR + 1;
        }
    }
    return random_no;
}

/* ------------------------------------------------------------- */
/* Legt eine neue Verzauberung an. Sollte es schon einen Zauber
 * dieses Typs geben, gibt es den bestehenden zurück.
 */
static curse *make_curse(unit * mage, attrib ** ap, const curse_type * ct,
    double vigour, int duration, double effect, int men)
{
    curse *c;
    attrib *a;

    a = a_new(&at_curse);
    a_add(ap, a);
    c = (curse *)a->data.v;

    c->type = ct;
    c->flags = 0;
    c->vigour = vigour;
    c->duration = duration;
    c->effect = effect;
    c->magician = mage;

    c->no = newcurseid();
    chash(c);

    switch (c->type->typ) {
    case CURSETYP_NORM:
        break;

    case CURSETYP_UNIT:
        c->data.i = men;
        break;
    }
    return c;
}

/* Mapperfunktion für das Anlegen neuer curse. Automatisch wird zum
 * passenden Typ verzweigt und die relevanten Variablen weitergegeben.
 */
curse *create_curse(unit * magician, attrib ** ap, const curse_type * ct,
    double vigour, int duration, double effect, int men)
{
    curse *c;

    /* die Kraft eines Spruchs darf nicht 0 sein */
    assert(vigour > 0);

    c = get_curse(*ap, ct);

    if (c && (c_flags(c) & CURSE_ONLYONE)) {
        return NULL;
    }
    assert(c == NULL || ct == c->type);

    /* es gibt schon eins diese Typs */
    if (c && ct->mergeflags != NO_MERGE) {
        if (ct->mergeflags & M_DURATION) {
            c->duration = MAX(c->duration, duration);
        }
        if (ct->mergeflags & M_SUMDURATION) {
            c->duration += duration;
        }
        if (ct->mergeflags & M_SUMEFFECT) {
            c->effect += effect;
        }
        if (ct->mergeflags & M_MAXEFFECT) {
            c->effect = MAX(c->effect, effect);
        }
        if (ct->mergeflags & M_VIGOUR) {
            c->vigour = MAX(vigour, c->vigour);
        }
        if (ct->mergeflags & M_VIGOUR_ADD) {
            c->vigour = vigour + c->vigour;
        }
        if (ct->mergeflags & M_MEN) {
            switch (ct->typ) {
            case CURSETYP_UNIT:
            {
                c->data.i += men;
            }
            }
        }
        set_curseingmagician(magician, *ap, ct);
    }
    else {
        c = make_curse(magician, ap, ct, vigour, duration, effect, men);
    }
    return c;
}

/* ------------------------------------------------------------- */
/* hier müssen alle c-typen, die auf Einheiten gezaubert werden können,
 * berücksichtigt werden */

static void do_transfer_curse(curse * c, unit * u, unit * u2, int n)
{
    int cursedmen = 0;
    int men = get_cursedmen(u, c);
    bool dogive = false;
    const curse_type *ct = c->type;

    switch ((ct->flags | c->flags) & CURSE_SPREADMASK) {
    case CURSE_SPREADALWAYS:
        dogive = true;
        men = u2->number + n;
        break;

    case CURSE_SPREADMODULO:
    {
        int i;
        int u_number = u->number;
        for (i = 0; i < n + 1 && u_number > 0; i++) {
            if (rng_int() % u_number < cursedmen) {
                ++men;
                --cursedmen;
                dogive = true;
            }
            --u_number;
        }
        break;
    }
    case CURSE_SPREADCHANCE:
        if (chance(u2->number / (double)(u2->number + n))) {
            men = u2->number + n;
            dogive = true;
        }
        break;
    case CURSE_SPREADNEVER:
        break;
    }

    if (dogive) {
        curse *cnew = make_curse(c->magician, &u2->attribs, c->type, c->vigour,
            c->duration, c->effect, men);
        cnew->flags = c->flags;

        if (ct->typ == CURSETYP_UNIT)
            set_cursedmen(cnew, men);
    }
}

void transfer_curse(unit * u, unit * u2, int n)
{
    attrib *a;

    a = a_find(u->attribs, &at_curse);
    while (a && a->type == &at_curse) {
        curse *c = (curse *)a->data.v;
        do_transfer_curse(c, u, u2, n);
        a = a->next;
    }
}

/* ------------------------------------------------------------- */

int curse_cansee(const curse *c, const faction *viewer, objtype_t typ, const void *obj, int self) {
    if (self < 3 && c->magician && c->magician->faction == viewer) {
        // magicians can see their own curses better than anybody, no exceptions
        self = 3;
    }
    else if (c->type->cansee) {
        self = c->type->cansee(viewer, obj, typ, c, self);
    }
    return self;
}

bool curse_active(const curse * c)
{
    if (!c)
        return false;
    if (c_flags(c) & CURSE_ISNEW)
        return false;
    if (c->vigour <= 0)
        return false;

    return true;
}

bool is_cursed_internal(attrib * ap, const curse_type * ct)
{
    curse *c = get_curse(ap, ct);

    if (!c)
        return false;

    return true;
}

bool is_cursed_with(const attrib * ap, const curse * c)
{
    const attrib *a = ap;

    while (a) {
        if ((a->type->flags & ATF_CURSE) && (c == (const curse *)a->data.v)) {
            return true;
        }
        a = a->next;
    }

    return false;
}

/* ------------------------------------------------------------- */
/* cursedata */
/* ------------------------------------------------------------- */

static const char *oldnames[MAXCURSE] = {
    /* OBS: when removing curses, remember to update read_ccompat() */
    "fogtrap",
    "antimagiczone",
    "farvision",
    "gbdream",
    "auraboost",
    "maelstrom",
    "blessedharvest",
    "drought",
    "badlearn",
    "stormwind",
    "flyingship",
    "nodrift",
    "depression",
    "magicwalls",
    "strongwall",
    "astralblock",
    "generous",
    "peacezone",
    "magicstreet",
    "magicrunes",
    "badmagicresistancezone",
    "goodmagicresistancezone",
    "slavery",
    "calmmonster",
    "oldrace",
    "fumble",
    "riotzone",
    "godcursezone",
    "speed",
    "orcish",
    "magicboost",
    "insectfur"
};

const char *oldcursename(int id)
{
    return oldnames[id];
}

/* ------------------------------------------------------------- */
message *cinfo_simple(const void *obj, objtype_t typ, const struct curse * c,
    int self)
{
    struct message *msg;

    UNUSED_ARG(typ);
    UNUSED_ARG(self);
    UNUSED_ARG(obj);

    msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
    if (msg == NULL) {
        log_error("There is no curseinfo for %s.\n", c->type->cname);
    }
    return msg;
}

/* ------------------------------------------------------------- */
/* Antimagie - curse auflösen */
/* ------------------------------------------------------------- */

/* Wenn der Curse schwächer ist als der cast_level, dann wird er
* aufgelöst, bzw seine Kraft (vigour) auf 0 gesetzt.
* Ist der cast_level zu gering, hat die Antimagie nur mit einer Chance
* von 100-20*Stufenunterschied % eine Wirkung auf den Curse. Dann wird
* die Kraft des Curse um die halbe Stärke der Antimagie reduziert.
* Zurückgegeben wird der noch unverbrauchte Rest von force.
*/
double destr_curse(curse * c, int cast_level, double force)
{
    if (cast_level < c->vigour) { /* Zauber ist nicht stark genug */
        double probability = 0.1 + (cast_level - c->vigour) * 0.2;
        /* pro Stufe Unterschied -20% */
        if (chance(probability)) {
            force -= c->vigour;
            if (c->type->change_vigour) {
                c->type->change_vigour(c, -(cast_level + 1) / 2);
            }
            else {
                c->vigour -= (cast_level + 1) / 2.0;
            }
        }
    }
    else {                      /* Zauber ist stärker als curse */
        if (force >= c->vigour) {   /* reicht die Kraft noch aus? */
            force -= c->vigour;
            if (c->type->change_vigour) {
                c->type->change_vigour(c, -c->vigour);
            }
            else {
                c->vigour = 0;
            }

        }
    }
    return force;
}

void curses_done(void) {
    int i;
    for (i = 0; i != MAXCTHASH; ++i) {
        ql_free(cursetypes[i]);
        cursetypes[i] = 0;
    }
    ++ct_changes;
}
