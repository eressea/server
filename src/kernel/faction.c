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
#include "faction.h"
#include "alliance.h"
#include "ally.h"
#include "curse.h"
#include "equipment.h"
#include "group.h"
#include "item.h"
#include "messages.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "save.h"
#include "spellbook.h"
#include "terrain.h"
#include "unit.h"
#include "version.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/lists.h>
#include <util/language.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/password.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/variant.h>
#include <util/unicode.h>

#include <attributes/otherfaction.h>

#include <quicklist.h>
#include <storage.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

faction *factions;

/** remove the faction from memory.
 * this frees all memory that's only accessible through the faction,
 * but you should still call funhash and remove the faction from the
 * global list.
 */
static void free_faction(faction * f)
{
    funhash(f);
    if (f->alliance && f->alliance->_leader == f) {
        setalliance(f, 0);
    }
    if (f->msgs) {
        free_messagelist(f->msgs->begin);
        free(f->msgs);
    }
    while (f->battles) {
        struct bmsg *bm = f->battles;
        f->battles = bm->next;
        if (bm->msgs) {
            free_messagelist(bm->msgs->begin);
            free(bm->msgs);
        }
        free(bm);
    }

    if (f->spellbook) {
        free_spellbook(f->spellbook);
    }

    while (f->groups) {
        group *g = f->groups;
        f->groups = g->next;
        free_group(g);
    }
    freelist(f->allies);

    free(f->email);
    free(f->banner);
    free(f->_password);
    free(f->name);
    if (f->seen_factions) {
        ql_free(f->seen_factions);
        f->seen_factions = 0;
    }

    while (f->attribs) {
        a_remove(&f->attribs, f->attribs);
    }

    i_freeall(&f->items);

    freelist(f->ursprung);
}

#define FMAXHASH 2039
faction *factionhash[FMAXHASH];

void fhash(faction * f)
{
    int index = f->no % FMAXHASH;
    f->nexthash = factionhash[index];
    factionhash[index] = f;
}

void funhash(faction * f)
{
    int index = f->no % FMAXHASH;
    faction **fp = factionhash + index;
    while (*fp && (*fp) != f) {
        fp = &(*fp)->nexthash;
    }
    if (*fp == f) {
        *fp = f->nexthash;
    }
}

static faction *ffindhash(int no)
{
    int index = no % FMAXHASH;
    faction *f = factionhash[index];
    while (f && f->no != no)
        f = f->nexthash;
    return f;
}

faction *findfaction(int n)
{
    faction *f = ffindhash(n);
    return f;
}

faction *getfaction(void)
{
    return findfaction(getid());
}

void set_show_item(faction * f, const struct item_type *itype)
{
    attrib *a = a_add(&f->attribs, a_new(&at_showitem));
    a->data.v = (void *)itype;
}

const unit *random_unit_in_faction(const faction * f)
{
    unit *u;
    int c = 0, u_nr;

    if (!f->units) {
        return NULL;
    }
    for (u = f->units; u; u = u->next)
        c++;

    u_nr = rng_int() % c;
    c = 0;

    for (u = f->units; u; u = u->next)
        if (u_nr == c)
            return u;

    /* Hier sollte er nie ankommen */
    return NULL;
}

const char *factionname(const faction * f)
{
    typedef char name[OBJECTIDSIZE + 1];
    static name idbuf[8];
    static int nextbuf = 0;

    char *ibuf = idbuf[(++nextbuf) % 8];

    if (f && f->name) {
        slprintf(ibuf, sizeof(name), "%s (%s)", f->name, itoa36(f->no));
    }
    else {
        strcpy(ibuf, "Unbekannte Partei (?)");
    }
    return ibuf;
}

int resolve_faction(variant id, void *address)
{
    int result = 0;
    faction *f = NULL;
    if (id.i != 0) {
        f = findfaction(id.i);
        if (f == NULL) {
            result = -1;
        }
    }
    assert(address);
    *(faction **)address = f;
    return result;
}

bool faction_id_is_unused(int id)
{
    return findfaction(id) == NULL;
}

#define MAX_FACTION_ID (36*36*36*36)

static int unused_faction_id(void)
{
    int id = rng_int() % MAX_FACTION_ID;

    while (!faction_id_is_unused(id)) {
        if (++id == MAX_FACTION_ID) {
            id = 0;
        }
    }

    return id;
}

faction *addfaction(const char *email, const char *password,
    const struct race * frace, const struct locale * loc, int subscription)
{
    faction *f = calloc(sizeof(faction), 1);
    char buf[128];

    if (set_email(&f->email, email) != 0) {
        log_warning("Invalid email address for faction %s: %s\n", itoa36(f->no), email);
    }

    if (!password) password = itoa36(rng_int());
    faction_setpassword(f, password_encode(password, PASSWORD_DEFAULT));
    ADDMSG(&f->msgs, msg_message("changepasswd", "value", password));

    f->alliance_joindate = turn;
    f->lastorders = turn;
    f->_alive = true;
    f->age = 0;
    f->race = frace;
    f->magiegebiet = 0;
    f->locale = loc;
    f->subscription = subscription;

    f->options =
        want(O_REPORT) | want(O_ZUGVORLAGE) | want(O_COMPUTER) | want(O_COMPRESS) |
        want(O_ADRESSEN) | want(O_STATISTICS);

    f->no = unused_faction_id();
    if (rule_region_owners()) {
        alliance *al = makealliance(f->no, NULL);
        setalliance(f, al);
    }
    addlist(&factions, f);
    fhash(f);

    slprintf(buf, sizeof(buf), "%s %s", LOC(loc, "factiondefault"), factionid(f));
    f->name = _strdup(buf);

    if (!f->race) {
        log_warning("creating a faction that has no race", factionid(f));
    }

    return f;
}

unit *addplayer(region * r, faction * f)
{
    unit *u;
    char buffer[32];

    assert(f->units == NULL);
    faction_setorigin(f, 0, r->x, r->y);
    u = create_unit(r, f, 1, f->race, 0, NULL, NULL);
    equip_items(&u->faction->items, get_equipment("new_faction"));
    equip_unit(u, get_equipment("first_unit"));
    sprintf(buffer, "first_%s", u_race(u)->_name);
    equip_unit(u, get_equipment(buffer));
    u->hp = unit_max_hp(u) * u->number;
    fset(u, UFL_ISNEW);
    if (f->race == get_race(RC_DAEMON)) {
        race_t urc;
        race *rc;
        do {
            urc = (race_t)(rng_int() % MAXRACES);
            rc = get_race(urc);
        } while (rc == NULL || urc == RC_DAEMON || !playerrace(rc));
        u->irace = rc;
    }

    return u;
}

bool checkpasswd(const faction * f, const char *passwd)
{
    if (!passwd) return false;

    if (f->_password && password_verify(f->_password, passwd) == VERIFY_FAIL) {
        log_warning("password check failed: %s", factionname(f));
        return false;
    }
    return true;
}

variant read_faction_reference(struct storage * store)
{
    variant id;
    READ_INT(store, &id.i);
    return id;
}

void write_faction_reference(const faction * f, struct storage *store)
{
    assert(!f || f->_alive);
    WRITE_INT(store, f ? f->no : 0);
}

static faction *dead_factions;

void free_flist(faction **fp) {
    faction * flist = *fp;
    for (flist = factions; flist;) {
        faction *f = flist;
        flist = f->next;
        free_faction(f);
        free(f);
    }
    *fp = 0;
}

void free_factions(void) {
    free_flist(&factions);
    free_flist(&dead_factions);
}

void destroyfaction(faction ** fp)
{
    faction * f = *fp;
    unit *u = f->units;

    *fp = f->next;
    f->next = dead_factions;
    dead_factions = f;

    fset(f, FFL_QUIT);
    f->_alive = false;

    if (f->spellbook) {
        spellbook_clear(f->spellbook);
        free(f->spellbook);
        f->spellbook = 0;
    }

    if (f->seen_factions) {
        ql_free(f->seen_factions);
        f->seen_factions = 0;
    }

    while (u) {
        /* give away your stuff, make zombies if you cannot (quest items) */
        int result = gift_items(u, GIFT_FRIENDS | GIFT_PEASANTS);
        if (result != 0) {
            unit *zombie = u;
            u = u->nextF;
            make_zombie(zombie);
        }
        else {
            region *r = u->region;

            if (!fval(r->terrain, SEA_REGION) && !!playerrace(u_race(u))) {
                const race *rc = u_race(u);
                int m = rmoney(r);

                if ((rc->ec_flags & ECF_REC_ETHEREAL) == 0) {
                    int p = rpeasants(u->region);
                    int h = rhorses(u->region);
                    item *itm;

                    /* Personen gehen nur an die Bauern, wenn sie auch von dort
                     * stammen */
                    if (rc->ec_flags & ECF_REC_HORSES) {  /* Zentauren an die Pferde */
                        h += u->number;
                    }
                    else {              /* Orks zählen nur zur Hälfte */
                        p += (int)(u->number * rc->recruit_multi);
                    }
                    for (itm = u->items; itm; itm = itm->next) {
                        if (itm->type->flags & ITF_ANIMAL) {
                            h += itm->number;
                        }
                    }
                    rsetpeasants(r, p);
                    rsethorses(r, h);
                }
                m += get_money(u);
                rsetmoney(r, m);
            }
            set_number(u, 0);
            u = u->nextF;
        }
    }

    handle_event(f->attribs, "destroy", f);
#if 0
    faction *ff;
    for (ff = factions; ff; ff = ff->next) {
        group *g;
        ally *sf, **sfp;

        for (sfp = &ff->allies; *sfp;) {
            sf = *sfp;
            if (sf->faction == f || sf->faction == NULL) {
                *sfp = sf->next;
                free(sf);
            }
            else
                sfp = &(*sfp)->next;
        }
        for (g = ff->groups; g; g = g->next) {
            for (sfp = &g->allies; *sfp; ) {
                sf = *sfp;
                if (sf->faction == f || sf->faction == NULL) {
                    *sfp = sf->next;
                    free(sf);
                }
                else {
                    sfp = &(*sfp)->next;
                }
            }
        }
    }
#endif

    if (f->alliance && f->alliance->_leader == f) {
        setalliance(f, 0);
    }

    funhash(f);

    /* units of other factions that were disguised as this faction
     * have their disguise replaced by ordinary faction hiding. */
    if (rule_stealth_other()) {
        // TODO: f.alive should be tested for in get_otherfaction
        region *rc;
        for (rc = regions; rc; rc = rc->next) {
            for (u = rc->units; u; u = u->next) {
                attrib *a = a_find(u->attribs, &at_otherfaction);
                if (a && get_otherfaction(a) == f) {
                    a_removeall(&u->attribs, &at_otherfaction);
                    if (rule_stealth_anon()) {
                        fset(u, UFL_ANON_FACTION);
                    }
                }
            }
        }
    }
}

int get_alliance(const faction * a, const faction * b)
{
    const ally *sf = a->allies;
    for (; sf != NULL; sf = sf->next) {
        if (sf->faction == b) {
            return sf->status;
        }
    }
    return 0;
}

void set_alliance(faction * a, faction * b, int status)
{
    ally **sfp;
    sfp = &a->allies;
    while (*sfp) {
        ally *sf = *sfp;
        if (sf->faction == b)
            break;
        sfp = &sf->next;
    }
    if (*sfp == NULL) {
        ally *sf = ally_add(sfp, b);
        sf->status = status;
        return;
    }
    (*sfp)->status |= status;
}

void renumber_faction(faction * f, int no)
{
    funhash(f);
    f->no = no;
    fhash(f);
    fset(f, FFL_NEWID);
}

#ifdef SMART_INTERVALS
void update_interval(struct faction *f, struct region *r)
{
    if (r == NULL || f == NULL)
        return;
    if (f->first == NULL || f->first->index > r->index) {
        f->first = r;
    }
    if (f->last == NULL || f->last->index <= r->index) {
        f->last = r;
    }
}
#endif

const char *faction_getname(const faction * self)
{
    return self->name ? self->name : "";
}

void faction_setname(faction * self, const char *name)
{
    free(self->name);
    if (name)
        self->name = _strdup(name);
}

const char *faction_getemail(const faction * self)
{
    return self->email ? self->email : "";
}

void faction_setemail(faction * self, const char *email)
{
    free(self->email);
    if (email)
        self->email = _strdup(email);
}

const char *faction_getbanner(const faction * self)
{
    return self->banner ? self->banner : "";
}

void faction_setbanner(faction * self, const char *banner)
{
    free(self->banner);
    if (banner)
        self->banner = _strdup(banner);
}

void faction_setpassword(faction * f, const char *pwhash)
{
    assert(pwhash);
    // && pwhash[0] == '$');
    free(f->_password);
    f->_password = _strdup(pwhash);
}

bool valid_race(const struct faction *f, const struct race *rc)
{
    if (f->race == rc)
        return true;
    else {
        const char *str = get_param(f->race->parameters, "other_race");
        if (str)
            return (bool)(rc_find(str) == rc);
        return false;
    }
}

struct alliance *f_get_alliance(const struct faction *f)
{
    if (f->alliance && !(f->alliance->flags & ALF_NON_ALLIED)) {
        return f->alliance;
    }
    return NULL;
}

struct spellbook * faction_get_spellbook(struct faction *f)
{
    if (f->spellbook) {
        return f->spellbook;
    }
    if (f->magiegebiet != M_GRAY) {
        return get_spellbook(magic_school[f->magiegebiet]);
    }
    return 0;
}

static int allied_skillcount(const faction * f, skill_t sk)
{
    int num = 0;
    alliance *a = f_get_alliance(f);
    quicklist *members = a->members;
    int qi;

    for (qi = 0; members; ql_advance(&members, &qi, 1)) {
        faction *m = (faction *)ql_get(members, qi);
        num += count_skill(m, sk);
    }
    return num;
}

static int allied_skilllimit(const faction * f, skill_t sk)
{
    static int value = -1;
    if (value < 0) {
        value = config_get_int("alliance.skilllimit", 0);
    }
    return value;
}

int count_skill(faction * f, skill_t sk)
{
    int n = 0;
    unit *u;

    for (u = f->units; u; u = u->nextF) {
        if (has_skill(u, sk)) {
            if (!is_familiar(u)) {
                n += u->number;
            }
        }
    }
    return n;
}

int skill_limit(faction * f, skill_t sk)
{
    int m = INT_MAX;
    int al = allied_skilllimit(f, sk);
    if (al > 0) {
        if (sk != SK_ALCHEMY && sk != SK_MAGIC)
            return INT_MAX;
        if (f_get_alliance(f)) {
            int sc, fl, ac = listlen(f->alliance->members);   /* number of factions */

            assert(ac > 0);
            fl = (al + ac - 1) / ac;      /* faction limit, rounded up */
            /* the faction limit may not be achievable because it would break the alliance-limit */
            sc = al - allied_skillcount(f, sk);
            if (sc <= 0)
                return 0;
            return fl;
        }
    }
    if (sk == SK_MAGIC) {
        m = max_magicians(f);
    }
    else if (sk == SK_ALCHEMY) {
        m = config_get_int("rules.maxskills.alchemy", MAXALCHEMISTS);
    }
    return m;
}

void remove_empty_factions(void)
{
    faction **fp;

    for (fp = &factions; *fp;) {
        faction *f = *fp;

        if (!(f->_alive && f->units!=NULL) && !fval(f, FFL_NOIDLEOUT)) {
            log_debug("dead: %s", factionname(f));
            destroyfaction(fp);
            free_faction(f);
            free(f);
        }
        else {
            fp = &(*fp)->next;
        }
    }
}

bool faction_alive(const faction *f) {
    assert(f);
    return f->_alive || (f->flags&FFL_NPC);
}

void faction_getorigin(const faction * f, int id, int *x, int *y)
{
    ursprung *ur;

    assert(f && x && y);
    for (ur = f->ursprung; ur; ur = ur->next) {
        if (ur->id == id) {
            *x = ur->x;
            *y = ur->y;
            break;
        }
    }
}

void faction_setorigin(faction * f, int id, int x, int y)
{
    ursprung *ur;
    assert(f != NULL);
    for (ur = f->ursprung; ur; ur = ur->next) {
        if (ur->id == id) {
            ur->x = ur->x + x;
            ur->y = ur->y + y;
            return;
        }
    }

    ur = calloc(1, sizeof(ursprung));
    ur->id = id;
    ur->x = x;
    ur->y = y;

    addlist(&f->ursprung, ur);
}


int count_faction(const faction * f, int flags)
{
    unit *u;
    int n = 0;
    for (u = f->units; u; u = u->nextF) {
        const race *rc = u_race(u);
        int x = (flags&COUNT_UNITS) ? 1 : u->number;
        if (f->race != rc) {
            if (!playerrace(rc)) {
                if (flags&COUNT_MONSTERS) {
                    n += x;
                }
            }
            else if (flags&COUNT_MIGRANTS) {
                if (!is_cursed(u->attribs, C_SLAVE, 0)) {
                    n += x;
                }
            }
        }
        else if (flags&COUNT_DEFAULT) {
            n += x;
        }
    }
    return n;
}

int count_units(const faction * f)
{
    return count_faction(f, COUNT_ALL | COUNT_UNITS);
}

int count_all(const faction * f)
{
    return count_faction(f, COUNT_ALL);
}

int count_migrants(const faction * f)
{
    return count_faction(f, COUNT_MIGRANTS);
}

#define MIGRANTS_NONE 0
#define MIGRANTS_LOG10 1

int count_maxmigrants(const faction * f)
{
    int formula = get_param_int(f->race->parameters, "migrants.formula", 0);

    if (formula == MIGRANTS_LOG10) {
        int nsize = count_all(f);
        if (nsize > 0) {
            int x = (int)(log10(nsize / 50.0) * 20);
            if (x < 0) x = 0;
            return x;
        }
    }
    return 0;
}

static void init_maxmagicians(struct attrib *a)
{
    a->data.i = MAXMAGICIANS;
}

attrib_type at_maxmagicians = {
    "maxmagicians",
    init_maxmagicians,
    NULL,
    NULL,
    a_writeint,
    a_readint,
    ATF_UNIQUE
};

int max_magicians(const faction * f)
{
    int m = config_get_int("rules.maxskills.magic", MAXMAGICIANS);
    attrib *a;

    if ((a = a_find(f->attribs, &at_maxmagicians)) != NULL) {
        m = a->data.i;
    }
    if (f->race == get_race(RC_ELF))
        ++m;
    return m;
}

#define DMAXHASH 7919
typedef struct dead {
    struct dead *nexthash;
    faction *f;
    int no;
} dead;

static dead *deadhash[DMAXHASH];

void dhash(int no, faction * f)
{
    dead *hash = (dead *)calloc(1, sizeof(dead));
    dead *old = deadhash[no % DMAXHASH];
    hash->no = no;
    hash->f = f;
    deadhash[no % DMAXHASH] = hash;
    hash->nexthash = old;
}

faction *dfindhash(int no)
{
    dead *old;

    if (no < 0)
        return 0;

    for (old = deadhash[no % DMAXHASH]; old; old = old->nexthash) {
        if (old->no == no) {
            return old->f;
        }
    }
    return 0;
}

int writepasswd(void)
{
    FILE *F;
    char zText[128];

    sprintf(zText, "%s/passwd", basepath());
    F = fopen(zText, "w");
    if (!F) {
        perror(zText);
    }
    else {
        faction *f;
        log_info("writing passwords...");

        for (f = factions; f; f = f->next) {
            fprintf(F, "%s:%s:%s:%u\n",
                factionid(f), f->email, f->_password, f->subscription);
        }
        fclose(F);
        return 0;
    }
    return 1;
}

