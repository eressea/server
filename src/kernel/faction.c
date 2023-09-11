#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "faction.h"

#include "battle.h"
#include "calendar.h"
#include "config.h"
#include "database.h"
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

#include <kernel/attrib.h>
#include <kernel/event.h>
#include <kernel/gamedata.h>

/* util includes */
#include <util/base36.h>
#include <util/lists.h>
#include <util/goodies.h>
#include <util/lists.h>
#include <util/language.h>
#include <util/log.h>
#include <util/message.h>
#include <util/parser.h>
#include <util/password.h>
#include <util/path.h>
#include <util/rng.h>
#include <util/variant.h>

#include <selist.h>
#include <storage.h>
#include <strings.h>

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
    free(f->name);
    if (f->seen_factions) {
        selist_free(f->seen_factions);
        f->seen_factions = 0;
    }

    while (f->attribs) {
        a_remove(&f->attribs, f->attribs);
    }

    i_freeall(&f->items);

    freelist(f->origin);
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

char *faction_genpassword(faction *f, char *buffer) {
    password_generate(buffer, 8);
    faction_setpassword(f, password_hash(buffer, PASSWORD_DEFAULT));
    ADDMSG(&f->msgs, msg_message("changepasswd", "value", buffer));
    return buffer;
}

faction *addfaction(const char *email, const char *password,
    const struct race * frace, const struct locale * loc)
{
    faction *f;
    const char *fname;
    char buf[128];

    f = faction_create(unused_faction_id());
    if (!f) abort();
    if (check_email(email) == 0) {
        faction_setemail(f, email);
    } else {
        log_info("Invalid email address for faction %s: %s\n", itoa36(f->no), email?email:"");
        faction_setemail(f, NULL);
    }

    f->alliance_joindate = turn;
    f->lastorders = 0;
    f->password_id = 0;
    faction_set_age(f, 0);
    f->race = frace;
    f->magiegebiet = 0;
    f->locale = loc;
    f->uid = 0;
    f->flags = FFL_ISNEW|FFL_PWMSG;

    if (password) {
        faction_setpassword(f, password_hash(password, PASSWORD_DEFAULT));
        ADDMSG(&f->msgs, msg_message("changepasswd", "value", password));
    }

    f->options =
        WANT_OPTION(O_REPORT) | WANT_OPTION(O_ZUGVORLAGE) |
        WANT_OPTION(O_COMPUTER) | WANT_OPTION(O_COMPRESS) |
        WANT_OPTION(O_ADRESSEN) | WANT_OPTION(O_STATISTICS);

    if (rule_region_owners()) {
        alliance *al = makealliance(f->no, NULL);
        setalliance(f, al);
    }
    addlist(&factions, f);

    fname = LOC(loc, "factiondefault");
    slprintf(buf, sizeof(buf), "%s %s", fname ? fname : "faction", itoa36(f->no));
    f->name = str_strdup(buf);

    if (!f->race) {
        log_warning("creating a faction that has no race", itoa36(f->no));
    }

    return f;
}

#define PEASANT_MIN (10 * RECRUIT_FRACTION)
#define PEASANT_MAX (20 * RECRUIT_FRACTION)

unit *addplayer(region * r, faction * f)
{
    unit *u;
    const char * name;

    assert(r->land);
    assert(f->units == NULL);
    if (rpeasants(r) < PEASANT_MIN) {
        rsetpeasants(r, PEASANT_MIN + rng_int() % (PEASANT_MAX - PEASANT_MIN));
    }
    if (rtrees(r, TREE_TREE) < 50) {
        rsettrees(r, TREE_TREE, 50);
    }

    faction_setorigin(f, 0, r->x, r->y);
    u = create_unit(r, f, 1, f->race, 0, NULL, NULL);
    u->status = ST_FLEE;
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
        const race *rc;
        int urc = (race_t)(rng_int() % MAX_START_RACE);
        if (urc >= RC_DAEMON) ++urc;
        rc = get_race(urc);
        u->irace = rc;
    }
    f->lastorders = turn;
    return u;
}

bool checkpasswd(const faction * f, const char *passwd)
{
    const char *pwhash;
    if (!passwd) return false;

    pwhash = faction_getpassword(f);
    if (pwhash && password_verify(pwhash, passwd) == VERIFY_FAIL) {
        log_info("password check failed: %s", factionname(f));
        return false;
    }
    return true;
}

int read_faction_reference(gamedata * data, faction **fp)
{
    int id;
    READ_INT(data->store, &id);
    if (id > 0) {
        *fp = findfaction(id);
        if (*fp == NULL) {
            *fp = faction_create(id);
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
    *fp = NULL;
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
    unit_convert_race(u, rc_ghost, "ghost");
    give_special_items(u, &usrc->items);
}

void destroyfaction(faction ** fp)
{
    faction * f = *fp;
    unit *u = f->units;

    *fp = f->next;
    f->next = dead_factions;
    dead_factions = f;

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
        region *r = u->region;
        /* give away your stuff, to ghosts if you cannot (quest items) */
        if (u->items) {
            int result = gift_items(u, GIFT_FRIENDS | GIFT_PEASANTS);
            if (result != 0) {
                save_special_items(u);
            }
        }
        if (r->land && playerrace(u_race(u))) {
            const race *rc = u_race(u);
            /* Personen gehen nur an die Bauern, wenn sie auch von dort stammen */
            if ((rc->ec_flags & ECF_REC_ETHEREAL) == 0) {
                int p = rpeasants(u->region);
                p += (int)(u->number / rc->recruit_multi);
                rsetpeasants(r, p);
            }
        }
        set_number(u, 0);
        u = u->nextF;
    }

    handle_event(f->attribs, "destroy", f);
    if (f->alliance) {
        setalliance(f, NULL);
    }

    fset(f, FFL_QUIT);
    f->_alive = false;
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

const char *faction_getbanner(const faction * f)
{
    if (f->banner_id > 0) {
        return dbstring_load(f->banner_id, NULL);
    }
    return NULL;
}

void faction_setbanner(faction * f, const char *banner)
{
    if (banner && banner[0]) {
        f->banner_id = dbstring_save(banner);
    }
    else {
        f->banner_id = 0;
    }
}

const char *faction_getpassword(const faction *f) {
    if (f->password_id > 0) {
        return dbstring_load(f->password_id, NULL);
    }
    return NULL;
}

void faction_setpassword(faction * f, const char *pwhash)
{
    if (pwhash) {
        f->password_id = dbstring_save(pwhash);
    }
    else {
        f->password_id = 0;
    }
}

bool valid_race(const struct faction *f, const struct race *rc)
{
    if (is_monsters(f) || f->race == rc)
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
    return NULL;
}

static int allied_skillcount(const faction * f, skill_t sk)
{
    int num = 0;
    alliance *a = f_get_alliance(f);
    selist *members = a->members;
    int qi;

    for (qi = 0; members; selist_advance(&members, &qi, 1)) {
        faction *m = (faction *)selist_get(members, qi);
        num += faction_count_skill(m, sk);
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

int faction_count_skill(faction * f, skill_t sk)
{
    int n = 0;
    unit *u;

    for (u = f->units; u; u = u->nextF) {
        if (has_skill(u, sk)) {
            if (sk != SK_MAGIC || !is_familiar(u)) {
                n += u->number;
            }
        }
    }
    return n;
}

int faction_skill_limit(const faction * f, skill_t sk)
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

        if (!(f->_alive && f->units!=NULL) && !fval(f, FFL_PAUSED|FFL_NOIDLEOUT)) {
            destroyfaction(fp);
        }
        else {
            fp = &(*fp)->next;
        }
    }
}

void faction_set_age(struct faction* f, int age)
{
    f->start_turn = turn - age;
}

int faction_age(const struct faction* f)
{
    int age = turn - f->start_turn;
    return age;
}

bool faction_alive(const faction *f) {
    assert(f);
    return f->_alive || (f->flags&FFL_NPC);
}

void faction_getorigin(const faction * f, int id, int *x, int *y)
{
    origin *ur;

    assert(f && x && y);
    for (ur = f->origin; ur; ur = ur->next) {
        if (ur->id == id) {
            *x = ur->x;
            *y = ur->y;
            break;
        }
    }
}

static origin *new_origin(int id, int x, int y) {
    origin *ur = (origin *)calloc(1, sizeof(origin));
    if (!ur) abort();
    ur->id = id;
    ur->x = x;
    ur->y = y;
    return ur;
}

void faction_setorigin(faction * f, int id, int x, int y)
{
    origin **urp;
    assert(f != NULL);
    for (urp = &f->origin; *urp; urp = &(*urp)->next) {
        origin *ur = *urp;
        if (ur->id == id) {
            ur->x += x;
            ur->y += y;
            return;
        }
    }
    *urp = new_origin(id, x, y);
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
                itoa36(f->no), faction_getemail(f),
               faction_getpassword(f), f->uid);
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
                    fprintf(F, "%d\t%d\t%d\t%s\t%s\t%s\n", turn, f->lastorders, faction_age(f), itoa36(f->no), f->email, f->name);
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

faction *faction_create(int no)
{
    faction *f = (faction *)calloc(1, sizeof(faction));
    if (!f) abort();
    f->no = no;
    f->_alive = true;
    faction_set_age(f, 0);
    fhash(f);
    return f;
}

void change_locale(faction *f, const struct locale *lang, bool del ) {
    unit *ux;
    for (ux = f->units; ux; ux = ux->nextF) {
        translate_orders(ux, lang, &ux->orders, del);
        if (ux->orders) {
            translate_orders(ux, lang, &ux->orders, del);
        }
        if (ux->defaults) {
            translate_orders(ux, lang, &ux->defaults, del);
        }
        if (ux->thisorder) {
            translate_orders(ux, lang, &ux->thisorder, del);
        }
    }
    f->locale = lang;
}

bool rule_stealth_other(void)
{
    static int rule, config;
    if (config_changed(&config)) {
        rule = config_get_int("stealth.faction.other", 1);
    }
    return rule != 0;
}

bool rule_stealth_anon(void)
{
    static int rule, config;
    if (config_changed(&config)) {
        rule = config_get_int("stealth.faction.anon", 1);
    }
    return rule != 0;
}
