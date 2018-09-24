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

#include "faction.h"

#include "calendar.h"
#include "config.h"
#include "alliance.h"
#include "ally.h"
#include "curse.h"
#include "equipment.h"
#include "group.h"
#include "item.h"
#include "messages.h"
#include "order.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "spellbook.h"
#include "terrain.h"
#include "unit.h"

#include <spells/unitcurse.h>
#include <attributes/otherfaction.h>
#include <attributes/racename.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/gamedata.h>
#include <util/goodies.h>
#include <util/lists.h>
#include <util/language.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/password.h>
#include <util/path.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/strings.h>
#include <util/variant.h>

#include <selist.h>
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
        selist_free(f->seen_factions);
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
        slprintf(ibuf, sizeof(idbuf[0]), "%s (%s)", f->name, itoa36(f->no));
    }
    else {
        strcpy(ibuf, "Unbekannte Partei (?)");
    }
    return ibuf;
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
    const struct race * frace, const struct locale * loc)
{
    faction *f = calloc(sizeof(faction), 1);
    char buf[128];

    if (check_email(email) == 0) {
        faction_setemail(f, email);
    } else {
        log_info("Invalid email address for faction %s: %s\n", itoa36(f->no), email?email:"");
        faction_setemail(f, NULL);
    }

    f->alliance_joindate = turn;
    f->lastorders = turn;
    f->_alive = true;
    f->age = 0;
    f->race = frace;
    f->magiegebiet = 0;
    f->locale = loc;
    f->uid = 0;
    f->flags = FFL_ISNEW|FFL_PWMSG;

    if (!password) password = itoa36(rng_int());
    faction_setpassword(f, password_encode(password, PASSWORD_DEFAULT));
    ADDMSG(&f->msgs, msg_message("changepasswd", "value", password));

    f->options =
        WANT_OPTION(O_REPORT) | WANT_OPTION(O_ZUGVORLAGE) |
        WANT_OPTION(O_COMPUTER) | WANT_OPTION(O_COMPRESS) |
        WANT_OPTION(O_ADRESSEN) | WANT_OPTION(O_STATISTICS);

    f->no = unused_faction_id();
    if (rule_region_owners()) {
        alliance *al = makealliance(f->no, NULL);
        setalliance(f, al);
    }
    addlist(&factions, f);
    fhash(f);

    slprintf(buf, sizeof(buf), "%s %s", LOC(loc, "factiondefault"), itoa36(f->no));
    f->name = str_strdup(buf);

    if (!f->race) {
        log_warning("creating a faction that has no race", itoa36(f->no));
    }

    return f;
}

#define PEASANT_MIN (10 * RECRUITFRACTION)
#define PEASANT_MAX (20 * RECRUITFRACTION)

unit *addplayer(region * r, faction * f)
{
    unit *u;
    const char * name;

    assert(r->land);
    if (rpeasants(r) < PEASANT_MIN) {
        rsetpeasants(r, PEASANT_MIN + rng_int() % (PEASANT_MAX - PEASANT_MIN));
    }

    assert(f->units == NULL);
    faction_setorigin(f, 0, r->x, r->y);
    u = create_unit(r, f, 1, f->race, 0, NULL, NULL);
    u->thisorder = default_order(f->locale);
    unit_addorder(u, copy_order(u->thisorder));
    name = config_get("rules.equip_first");
    if (!equip_unit(u, name ? name : "first_unit")) {
        /* give every unit enough money to survive the first turn */
        i_change(&u->items, get_resourcetype(R_SILVER)->itype, maintenance_cost(u));
    }
    u->hp = unit_max_hp(u) * u->number;
    fset(u, UFL_ISNEW);
    if (f->race == get_race(RC_DAEMON)) {
        race_t urc;
        const race *rc;
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
        log_info("password check failed: %s", factionname(f));
        return false;
    }
    return true;
}

void resolve_faction(faction *f)
{
    resolve(RESOLVE_FACTION | f->no, f);
}

int read_faction_reference(gamedata * data, faction **fp, resolve_fun fun)
{
    int id;
    READ_INT(data->store, &id);
    if (id > 0) {
        *fp = findfaction(id);
        if (*fp == NULL) {
            *fp = NULL;
            ur_add(RESOLVE_FACTION | id, (void **)fp, fun);
        }
    }
    else {
        *fp = NULL;
    }
    return id;
}

void write_faction_reference(const faction * f, struct storage *store)
{
    assert(!f || f->_alive);
    WRITE_INT(store, f ? f->no : 0);
}

void free_flist(faction **fp) {
    faction * flist = *fp;
    while (flist) {
        faction *f = flist;
        flist = f->next;
        free_faction(f);
        free(f);
    }
    *fp = 0;
}

static faction *dead_factions;

static void give_special_items(unit *u, item **items) {
    item **iter = items;
    while (*iter) {
        item *itm = *iter;
        if (itm->number > 0 && itm->type->flags & ITF_NOTLOST) {
            i_change(&u->items, itm->type, itm->number);
            *iter = itm->next;
            if (iter == items) {
                *items = *iter;
            }
            i_free(itm);
        }
        else {
            iter = &itm->next;
        }
    }
}

faction *get_or_create_monsters(void)
{
    faction *f = findfaction(MONSTER_ID);
    if (!f) {
        const race *rc = rc_get_or_create("dragon");
        const char *email = config_get("monster.email");
        f = addfaction(email, NULL, rc, default_locale);
        renumber_faction(f, MONSTER_ID);
        faction_setname(f, "Monster");
        fset(f, FFL_NPC | FFL_NOIDLEOUT);
    }
    return f;
}

faction *get_monsters(void) {
    return get_or_create_monsters();
}

void save_special_items(unit *usrc)
{
    unit *u;
    region *r = usrc->region;
    faction *fm = get_monsters();
    static const race *rc_ghost;
    static int cache;

    if (rc_changed(&cache)) {
        rc_ghost = get_race(RC_TEMPLATE);
    }
    for (u = r->units; u; u = u->next) {
        if (u->faction == fm) {
            give_special_items(u, &usrc->items);
            return;
        }
    }
    u = create_unit(r, fm, 1, rc_ghost, 0, NULL, NULL);
    unit_setname(u, unit_getname(usrc));
    if (usrc->number > 1) {
        /* some units have plural names, it would be neat if they aren't single: */
        scale_number(u, 2);
    }
    set_racename(&u->attribs, "ghost");
    give_special_items(u, &usrc->items);
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
        selist_free(f->seen_factions);
        f->seen_factions = 0;
    }

    while (u) {
        /* give away your stuff, to ghosts if you cannot (quest items) */
        if (u->items) {
            region *r = u->region;
            int result = gift_items(u, GIFT_FRIENDS | GIFT_PEASANTS);
            if (result != 0) {
                save_special_items(u);
            }
            if (r->land && !!playerrace(u_race(u))) {
                const race *rc = u_race(u);
                int m = rmoney(r);

                /* Personen gehen nur an die Bauern, wenn sie auch von dort
                    * stammen */
                if ((rc->ec_flags & ECF_REC_ETHEREAL) == 0) {
                    int p = rpeasants(u->region);
                    int h = rhorses(u->region);
                    item *itm;

                    p += (int)(u->number * rc->recruit_multi);
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
        }
        set_number(u, 0);
        u = u->nextF;
    }

    handle_event(f->attribs, "destroy", f);
    if (f->alliance) {
        setalliance(f, 0);
    }

    funhash(f);

    /* units of other factions that were disguised as this faction
     * have their disguise replaced by ordinary faction hiding. */
    if (rule_stealth_other()) {
        region *rc;
        for (rc = regions; rc; rc = rc->next) {
            for (u = rc->units; u; u = u->next) {
                if (u->attribs && get_otherfaction(u) == f) {
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
}

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

const char *faction_getname(const faction * self)
{
    return self->name ? self->name : "";
}

void faction_setname(faction * self, const char *name)
{
    free(self->name);
    if (name)
        self->name = str_strdup(name);
}

const char *faction_getemail(const faction * self)
{
    return self->email ? self->email : "";
}

void faction_setemail(faction * self, const char *email)
{
    free(self->email);
    if (email)
        self->email = str_strdup(email);
    else
        self->email = NULL;
}

const char *faction_getbanner(const faction * self)
{
    return self->banner ? self->banner : "";
}

void faction_setbanner(faction * self, const char *banner)
{
    free(self->banner);
    if (banner)
        self->banner = str_strdup(banner);
}

void faction_setpassword(faction * f, const char *pwhash)
{
    assert(pwhash);
    free(f->_password);
    f->_password = str_strdup(pwhash);
}

bool valid_race(const struct faction *f, const struct race *rc)
{
    if (f->race == rc)
        return true;
    else {
        return rc_otherrace(f->race) == rc;
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
    selist *members = a->members;
    int qi;

    for (qi = 0; members; selist_advance(&members, &qi, 1)) {
        faction *m = (faction *)selist_get(members, qi);
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
        m = config_get_int("rules.maxskills.alchemy", 3);
    }
    return m;
}

void remove_empty_factions(void)
{
    faction **fp;

    for (fp = &factions; *fp;) {
        faction *f = *fp;

        if (!(f->_alive && f->units!=NULL) && !fval(f, FFL_NOIDLEOUT)) {
            destroyfaction(fp);
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
        int x = u->number;
        if (f->race != rc) {
            if (!playerrace(rc)) {
                if (flags&COUNT_MONSTERS) {
                    n += x;
                }
            }
            else if (flags&COUNT_MIGRANTS) {
                if (!is_cursed(u->attribs, &ct_slavery)) {
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

int count_migrants(const faction * f)
{
    return count_faction(f, COUNT_MIGRANTS);
}

int count_maxmigrants(const faction * f)
{
    int formula = rc_migrants_formula(f->race);

    if (formula == MIGRANTS_LOG10) {
        int nsize = f->num_people;
        if (nsize > 0) {
            int x = (int)(log10(nsize / 50.0) * 20);
            if (x < 0) x = 0;
            return x;
        }
    }
    return 0;
}

int max_magicians(const faction * f)
{
    static int rule, config, rc_cache;
    static const race *rc_elf;
    int m;

    if (config_changed(&config)) {
        rule = config_get_int("rules.maxskills.magic", 3);
    }
    m = rule;
    if (rc_changed(&rc_cache)) {
        rc_elf = get_race(RC_ELF);
    }
    if (f->race == rc_elf) {
        ++m;
    }
    return m;
}

int writepasswd(void)
{
    FILE *F;
    char zText[128];

    path_join(basepath(), "passwd", zText, sizeof(zText));
    F = fopen(zText, "w");
    if (!F) {
        perror(zText);
    }
    else {
        faction *f;
        log_info("writing passwords...");

        for (f = factions; f; f = f->next) {
            fprintf(F, "%s:%s:%s:%d\n",
                itoa36(f->no), faction_getemail(f), f->_password, f->uid);
        }
        fclose(F);
        return 0;
    }
    return 1;
}

void log_dead_factions(void)
{
    if (dead_factions) {
        const char *logname = config_get("game.deadlog");
        if (logname) {
            FILE *F;
            char path[PATH_MAX];

            join_path(basepath(), logname, path, sizeof(path));
            F = fopen(path, "at");
            if (F) {
                faction *f;
                for (f = dead_factions; f; f = f->next) {
                    fprintf(F, "%d\t%d\t%d\t%s\t%s\t%s\n", turn, f->lastorders, f->age, itoa36(f->no), f->email, f->name);
                }
                fclose(F);
            }
        }
    }
}

void free_factions(void) {
    free_flist(&factions);
    free_flist(&dead_factions);
}
