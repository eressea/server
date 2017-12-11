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
#include <kernel/version.h>
#include "save.h"

#include "alchemy.h"
#include "alliance.h"
#include "ally.h"
#include "building.h"
#include "connection.h"
#include "equipment.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "magic.h"
#include "messages.h"
#include "move.h"
#include "objtypes.h"
#include "order.h"
#include "pathfinder.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "resources.h"
#include "ship.h"
#include "skill.h"
#include "spell.h"
#include "spellbook.h"
#include "terrain.h"
#include "terrainid.h"          /* only for conversion code */
#include "unit.h"
#include "lighthouse.h"

/* attributes includes */
#include <attributes/attributes.h>
#include <attributes/key.h>
#include <triggers/timeout.h>
#include <triggers/shock.h>

/* util includes */
#include <util/assert.h>
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/filereader.h>
#include <util/gamedata.h>
#include <util/goodies.h>
#include <util/gamedata.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/password.h>
#include <util/rand.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/umlaut.h>
#include <util/unicode.h>

#include <selist.h>
#include <stream.h>
#include <filestream.h>
#include <storage.h>
#include <binarystore.h>

/* libc includes */
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#define xisdigit(c)     (((c) >= '0' && (c) <= '9') || (c) == '-')

#define ESCAPE_FIX
#define MAXORDERS 256
#define MAXPERSISTENT 128

/* exported symbols symbols */
int firstx = 0, firsty = 0;

/* TODO: is this still important? */
int enc_gamedata = ENCODING_UTF8;

static void read_alliances(gamedata *data)
{
    storage *store = data->store;
    char pbuf[8];
    int id, terminator = 0;
    if (data->version < ALLIANCELEADER_VERSION) {
        terminator = atoi36("end");
        READ_STR(store, pbuf, sizeof(pbuf));
        id = atoi36(pbuf);
    }
    else {
        READ_INT(store, &id);
    }
    while (id != terminator) {
        char aname[128];
        alliance *al;
        READ_STR(store, aname, sizeof(aname));
        al = new_alliance(id, aname);
        if (data->version >= OWNER_2_VERSION) {
            READ_INT(store, &al->flags);
        }
        if (data->version >= ALLIANCELEADER_VERSION) {
            read_faction_reference(data, &al->_leader, NULL);
            READ_INT(store, &id);
        }
        else {
            READ_STR(store, pbuf, sizeof(pbuf));
            id = atoi36(pbuf);
        }
    }
}

void read_planes(gamedata *data) {
    struct storage *store = data->store;
    int nread;
    char name[32];

    /* Planes */
    planes = NULL;
    READ_INT(store, &nread);
    while (--nread >= 0) {
        int id;
        plane *pl;

        READ_INT(store, &id);
        pl = getplanebyid(id);

        if (pl == NULL) {
            pl = calloc(1, sizeof(plane));
        }
        else {
            log_warning("the plane with id=%d already exists.", id);
        }
        pl->id = id;
        READ_STR(store, name, sizeof(name));
        pl->name = strdup(name);
        READ_INT(store, &pl->minx);
        READ_INT(store, &pl->maxx);
        READ_INT(store, &pl->miny);
        READ_INT(store, &pl->maxy);
        READ_INT(store, &pl->flags);

        /* read watchers */
        if (data->version < FIX_WATCHERS_VERSION) {
            char rname[64];
            /* before this version, watcher storage was pretty broken. we are incompatible and don't read them */
            for (;;) {
                READ_TOK(store, rname, sizeof(rname));
                if (strcmp(rname, "end") == 0) {
                    break;                /* this is most likely the end of the list */
                }
                else {
                    log_error(
                        ("This datafile contains watchers, but we are unable to read them."));
                }
            }
        }
        else {
            /* WATCHERS - eliminated in February 2016, ca. turn 966 */
            if (data->version < NOWATCH_VERSION) {
                int fno;
                READ_INT(data->store, &fno);
                while (fno) {
                    READ_INT(data->store, &fno);
                }
            }
        }
        read_attribs(data, &pl->attribs, pl);
        if (pl->id != 1094969858) { /* Regatta */
            addlist(&planes, pl);
        }
    }
}

void write_planes(storage *store) {
    plane *pl;
    /* Write planes */
    WRITE_INT(store, listlen(planes));
    for (pl = planes; pl; pl = pl->next) {
        WRITE_INT(store, pl->id);
        WRITE_STR(store, pl->name);
        WRITE_INT(store, pl->minx);
        WRITE_INT(store, pl->maxx);
        WRITE_INT(store, pl->miny);
        WRITE_INT(store, pl->maxy);
        WRITE_INT(store, pl->flags);
#if RELEASE_VERSION < NOWATCH_VERSION
        write_faction_reference(NULL, store);  /* mark the end of pl->watchers (gone since T966)  */
#endif
        a_write(store, pl->attribs, pl);
        WRITE_SECTION(store);
    }
}

void write_alliances(gamedata *data)
{
    alliance *al = alliances;
    while (al) {
        if (al->_leader) {
            WRITE_INT(data->store, al->id);
            WRITE_STR(data->store, al->name);
            WRITE_INT(data->store, (int)al->flags);
            write_faction_reference(al->_leader, data->store);
            WRITE_SECTION(data->store);
        }
        al = al->next;
    }
    WRITE_INT(data->store, 0);
    WRITE_SECTION(data->store);
}

static void read_owner(gamedata *data, region_owner ** powner)
{
    int since_turn;

    READ_INT(data->store, &since_turn);
    if (since_turn >= 0) {
        region_owner *owner = malloc(sizeof(region_owner));
        owner->since_turn = since_turn;
        READ_INT(data->store, &owner->morale_turn);
        if (data->version >= MOURNING_VERSION) {
            READ_INT(data->store, &owner->flags);
        }
        else {
            owner->flags = 0;
        }
        if (data->version >= OWNER_3_VERSION) {
            read_faction_reference(data, &owner->last_owner, NULL);
        }
        else if (data->version >= OWNER_2_VERSION) {
            int id;
            alliance *a;
            READ_INT(data->store, &id);
            a = id ? findalliance(id) : NULL;
            /* don't know which faction, take the leader */
            owner->last_owner = a ? a->_leader : NULL;
        }
        else {
            owner->last_owner = NULL;
        }
        read_faction_reference(data, &owner->owner, NULL);
        *powner = owner;
    }
    else {
        *powner = 0;
    }
}

static void write_owner(gamedata *data, region_owner * owner)
{
    if (owner) {
        faction *f;
        WRITE_INT(data->store, owner->since_turn);
        if (owner->since_turn >= 0) {
            WRITE_INT(data->store, owner->morale_turn);
            WRITE_INT(data->store, owner->flags);
            f = owner->last_owner;
            write_faction_reference((f && f->_alive) ? f : NULL, data->store);
            f = owner->owner;
            write_faction_reference((f && f->_alive) ? f : NULL, data->store);
        }
    }
    else {
        WRITE_INT(data->store, -1);
    }
}

int current_turn(void)
{
    char zText[MAX_PATH];
    int cturn = 0;
    FILE *F;

    join_path(basepath(), "turn", zText, sizeof(zText));
    F = fopen(zText, "r");
    if (!F) {
        perror(zText);
    }
    else {
        int c = fscanf(F, "%4d\n", &cturn);
        fclose(F);
        if (c != 1) {
            return -1;
        }
    }
    return cturn;
}

static void writeorder(gamedata *data, const struct order *ord,
    const struct locale *lang)
{
    char obuf[1024];
    write_order(ord, lang, obuf, sizeof(obuf));
    if (obuf[0])
        WRITE_STR(data->store, obuf);
}

static void read_skills(gamedata *data, unit *u)
{
    if (data->version < SKILLSORT_VERSION) {
        for (;;) {
            int n = NOSKILL, level, weeks;
            skill_t sk;
            READ_INT(data->store, &n);
            sk = (skill_t)n;
            if (sk == NOSKILL) break;
            READ_INT(data->store, &level);
            READ_INT(data->store, &weeks);
            if (level) {
                skill *sv = add_skill(u, sk);
                sv->level = sv->old = (unsigned char)level;
                sv->weeks = (unsigned char)weeks;
            }
        }
    }
    else {
        int i;
        READ_INT(data->store, &u->skill_size);
        u->skills = malloc(sizeof(skill)*u->skill_size);
        for (i = 0; i != u->skill_size; ++i) {
            skill *sv = u->skills + i;
            int val;
            READ_INT(data->store, &val);
            sv->id = (skill_t)val;
            READ_INT(data->store, &sv->level);
            sv->old = sv->level;
            READ_INT(data->store, &sv->weeks);
        }
    }
}

static void write_skills(gamedata *data, const unit *u) {
    int i;
    skill_t sk = NOSKILL;
    WRITE_INT(data->store, u->skill_size);
    for (i = 0; i != u->skill_size; ++i) {
        skill *sv = u->skills + i;
#ifndef NDEBUG
        assert(sv->id > sk);
        sk = sv->id;
        assert(sv->weeks <= sv->level * 2 + 1);
#endif
        WRITE_INT(data->store, sv->id);
        WRITE_INT(data->store, sv->level);
        WRITE_INT(data->store, sv->weeks);
    }
}

unit *read_unit(gamedata *data)
{
    unit *u;
    const race *rc;
    int number, n, p;
    order **orderp;
    char obuf[DISPLAYSIZE];
    faction *f;
    char rname[32];

    READ_INT(data->store, &n);
    if (n <= 0) {
        log_error("data contains invalid unit %d.", n);
        assert(n > 0);
        return 0;
    }
    u = findunit(n);
    if (u) {
        log_error("reading unit %s that already exists.", unitname(u));
        while (u->attribs) {
            a_remove(&u->attribs, u->attribs);
        }
        while (u->items) {
            i_free(i_remove(&u->items, u->items));
        }
        free(u->skills);
        u->skills = 0;
        u->skill_size = 0;
        u_setfaction(u, NULL);
    }
    else {
        u = calloc(sizeof(unit), 1);
        assert_alloc(u);
        u->no = n;
        uhash(u);
    }

    READ_INT(data->store, &n);
    f = findfaction(n);
    if (f != u->faction) {
        u_setfaction(u, f);
    }
    if (!u->faction) {
        log_error("unit %s has faction == NULL", itoa36(u->no));
        return 0;
    }

    READ_STR(data->store, obuf, sizeof(obuf));
    if (unicode_utf8_trim(obuf)!=0) {
		log_warning("trim unit %s name to '%s'", itoa36(u->no), obuf);
	}
    u->_name = obuf[0] ? strdup(obuf) : 0;
    READ_STR(data->store, obuf, sizeof(obuf));
    if (unicode_utf8_trim(obuf)!=0) {
        log_warning("trim unit %s info to '%s'", itoa36(u->no), obuf);
    }
    u->display = obuf[0] ? strdup(obuf) : 0;
    READ_INT(data->store, &number);
    set_number(u, number);

    READ_INT(data->store, &n);
    u->age = (short)n;

    READ_TOK(data->store, rname, sizeof(rname));
    rc = rc_find(rname);
    assert(rc);
    u_setrace(u, rc);

    READ_TOK(data->store, rname, sizeof(rname));
    if (rname[0])
        u->irace = rc_find(rname);
    else
        u->irace = NULL;

    READ_INT(data->store, &n);
    if (n > 0) {
        building * b = findbuilding(n);
        if (b) {
            u_set_building(u, b);
            if (fval(u, UFL_OWNER)) {
                building_set_owner(u);
            }
        }
        else {
            log_error("read_unit: unit in unkown building '%s'", itoa36(n));
        }
    }

    READ_INT(data->store, &n);
    if (n > 0) {
        ship * sh = findship(n);
        if (sh) {
            u_set_ship(u, sh);
            if (fval(u, UFL_OWNER)) {
                ship_set_owner(u);
            }
        }
        else {
            log_error("read_unit: unit in unkown ship '%s'", itoa36(n));
        }
    }

    READ_INT(data->store, &n);
    setstatus(u, n);
    READ_INT(data->store, &u->flags);
    u->flags &= UFL_SAVEMASK;
    if ((u->flags & UFL_ANON_FACTION) && !rule_stealth_anon()) {
        /* if this rule is broken, then fix broken units */
        u->flags -= UFL_ANON_FACTION;
        log_warning("%s was anonymous.", unitname(u));
    }
    /* Persistente Befehle einlesen */
    free_orders(&u->orders);
    READ_STR(data->store, obuf, sizeof(obuf));
    p = n = 0;
    orderp = &u->orders;
    while (obuf[0]) {
        order *ord = parse_order(obuf, u->faction->locale);
        if (ord != NULL) {
            if (++n < MAXORDERS) {
                if (!is_persistent(ord) || ++p < MAXPERSISTENT) {
                    *orderp = ord;
                    orderp = &ord->next;
                    ord = NULL;
                }
                else if (p == MAXPERSISTENT) {
                    log_info("%s had %d or more persistent orders", unitname(u), MAXPERSISTENT);
                }
            }
            else if (n == MAXORDERS) {
                log_info("%s had %d or more orders", unitname(u), MAXORDERS);
            }
            if (ord != NULL) {
                free_order(ord);
            }
        }
        READ_STR(data->store, obuf, sizeof(obuf));
    }
    set_order(&u->thisorder, NULL);

    assert(u_race(u));
    read_skills(data, u);
    read_items(data->store, &u->items);
    READ_INT(data->store, &u->hp);
    if (u->hp < u->number) {
        log_error("Einheit %s hat %u Personen, und %u Trefferpunkte", itoa36(u->no), u->number, u->hp);
        u->hp = u->number;
    }
    read_attribs(data, &u->attribs, u);
    resolve_unit(u);
    return u;
}

void write_unit(gamedata *data, const unit * u)
{
    order *ord;
    int p = 0;
    unsigned int flags = u->flags & UFL_SAVEMASK;
    const race *irace = u_irace(u);

    write_unit_reference(u, data->store);
    assert(u->faction->_alive);
    write_faction_reference(u->faction, data->store);
    WRITE_STR(data->store, u->_name);
    WRITE_STR(data->store, u->display ? u->display : "");
    WRITE_INT(data->store, u->number);
    WRITE_INT(data->store, u->age);
    WRITE_TOK(data->store, u_race(u)->_name);
    WRITE_TOK(data->store, (irace && irace != u_race(u)) ? irace->_name : "");
    write_building_reference(u->building, data->store);
    write_ship_reference(u->ship, data->store);
    WRITE_INT(data->store, u->status);
    if (u->building && u == building_owner(u->building)) flags |= UFL_OWNER;
    if (u->ship && u == ship_owner(u->ship)) flags |= UFL_OWNER;
    WRITE_INT(data->store, flags);
    WRITE_SECTION(data->store);
    for (ord = u->old_orders; ord; ord = ord->next) {
        if (++p < MAXPERSISTENT) {
            writeorder(data, ord, u->faction->locale);
        }
        else {
            log_info("%s had %d or more persistent orders", unitname(u), MAXPERSISTENT);
            break;
        }
    }
    for (ord = u->orders; ord; ord = ord->next) {
        keyword_t kwd = getkeyword(ord);
        if (u->old_orders && is_repeated(kwd))
            continue;                 /* has new defaults */
        if (is_persistent(ord)) {
            if (++p < MAXPERSISTENT) {
                writeorder(data, ord, u->faction->locale);
            }
            else {
                log_info("%s had %d or more persistent orders", unitname(u), MAXPERSISTENT);
                break;
            }
        }
    }
    /* write an empty string to terminate the list */
    WRITE_STR(data->store, "");
    WRITE_SECTION(data->store);

    assert(u_race(u));
    write_skills(data, u);
    write_items(data->store, u->items);
    WRITE_SECTION(data->store);
    if (u->hp == 0 && data->version < NORCSPELL_VERSION) {
        log_error("unit %s has 0 hitpoints, adjusting.", itoa36(u->no));
        ((unit *)u)->hp = u->number;
    }
    WRITE_INT(data->store, u->hp);
    WRITE_SECTION(data->store);
    write_attribs(data->store, u->attribs, u);
    WRITE_SECTION(data->store);
}

static void read_regioninfo(gamedata *data, const region *r, char *info, size_t len) {
    READ_STR(data->store, info, len);
    if (unicode_utf8_trim(info) != 0) {
        log_warning("trim region %d info to '%s'", r->uid, info);
    }
}

static region *readregion(gamedata *data, int x, int y)
{
    region *r = findregion(x, y);
    const terrain_type *terrain;
    char name[NAMESIZE];
    char info[DISPLAYSIZE];
    int uid = 0;
    int n;

    READ_INT(data->store, &uid);

    if (r == NULL) {
        plane *pl = findplane(x, y);
        r = new_region(x, y, pl, uid);
    }
    else {
        assert(uid == 0 || r->uid == uid);
        while (r->attribs)
            a_remove(&r->attribs, r->attribs);
        if (r->land) {
            free_land(r->land);
            r->land = 0;
        }
        while (r->resources) {
            rawmaterial *rm = r->resources;
            r->resources = rm->next;
            free(rm);
        }
        r->land = 0;
    }
    if (data->version < LANDDISPLAY_VERSION) {
        read_regioninfo(data, r, info, sizeof(info));
    }
    else {
        info[0] = '\0';
    }

    READ_STR(data->store, name, sizeof(name));
    terrain = get_terrain(name);
    if (terrain == NULL) {
        log_error("Unknown terrain '%s'", name);
        assert(!"unknown terrain");
    }
    r->terrain = terrain;
    READ_INT(data->store, &r->flags);
    READ_INT(data->store, &n);
    r->age = (unsigned short)n;

    if (fval(r->terrain, LAND_REGION)) {
        r->land = calloc(1, sizeof(land_region));
        READ_STR(data->store, name, sizeof(name));
        if (unicode_utf8_trim(name) != 0) {
            log_warning("trim region %d name to '%s'", uid, name);
        };
        r->land->name = strdup(name);
    }
    if (r->land) {
        int i;
        rawmaterial **pres = &r->resources;

        if (data->version >= LANDDISPLAY_VERSION) {
            read_regioninfo(data, r, info, sizeof(info));
        }
        region_setinfo(r, info);
        READ_INT(data->store, &i);
        if (i < 0) {
            log_error("number of trees in %s is %d.", regionname(r, NULL), i);
            i = 0;
        }
        rsettrees(r, 0, i);
        READ_INT(data->store, &i);
        if (i < 0) {
            log_error("number of young trees in %s is %d.", regionname(r, NULL), i);
            i = 0;
        }
        rsettrees(r, 1, i);
        READ_INT(data->store, &i);
        if (i < 0) {
            log_error("number of seeds in %s is %d.", regionname(r, NULL), i);
            i = 0;
        }
        rsettrees(r, 2, i);

        READ_INT(data->store, &i);
        rsethorses(r, i);
        assert(*pres == NULL);
        for (;;) {
            rawmaterial *res;
            READ_STR(data->store, name, sizeof(name));
            if (strcmp(name, "end") == 0)
                break;
            res = malloc(sizeof(rawmaterial));
            res->rtype = rt_find(name);
            if (!res->rtype && strncmp("rm_", name, 3) == 0) {
                res->rtype = rt_find(name + 3);
            }
            if (!res->rtype || !res->rtype->raw) {
                log_error("invalid resourcetype %s in data.", name);
            }
            assert(res->rtype);
            READ_INT(data->store, &n);
            res->level = n;
            READ_INT(data->store, &n);
            res->amount = n;
            res->flags = 0;

            READ_INT(data->store, &n);
            res->startlevel = n;
            READ_INT(data->store, &n);
            res->base = n;
            READ_INT(data->store, &n);
            res->divisor = n;

            *pres = res;
            pres = &res->next;
        }
        *pres = NULL;

        READ_STR(data->store, name, sizeof(name));
        if (strcmp(name, "noherb") != 0) {
            const resource_type *rtype = rt_find(name);
            assert(rtype && rtype->itype && fval(rtype->itype, ITF_HERB));
            rsetherbtype(r, rtype->itype);
        }
        else {
            rsetherbtype(r, NULL);
        }
        READ_INT(data->store, &n);
        rsetherbs(r, n);
        READ_INT(data->store, &n);
        if (n < 0) {
            /* bug 2182 */
            log_error("data has negative peasants: %d in %s", n, regionname(r, 0));
            rsetpeasants(r, 0);
        }
        else {
            rsetpeasants(r, n);
        }
        READ_INT(data->store, &n);
        rsetmoney(r, n);
    }
    else {
        if (info[0]) {
            log_error("%s %d has a description: %s", r->terrain->_name, r->uid, info);
        }
    }
    assert(r->terrain != NULL);

    if (r->land) {
        int n;
        for (;;) {
            const struct resource_type *rtype;
            READ_STR(data->store, name, sizeof(name));
            if (!strcmp(name, "end"))
                break;
            rtype = rt_find(name);
            assert(rtype && rtype->ltype);
            READ_INT(data->store, &n);
            r_setdemand(r, rtype->ltype, n);
        }
        if (!r->land->demands) {
            fix_demand(r);
        }
        if (data->version < NOLANDITEM_VERSION) {
            read_items(data->store, NULL);
        }
        if (data->version >= REGIONOWNER_VERSION) {
            READ_INT(data->store, &n);
            region_set_morale(r, MAX(0, (short)n), -1);
            read_owner(data, &r->land->ownership);
        }
    }
    read_attribs(data, &r->attribs, r);
    return r;
}

region *read_region(gamedata *data)
{
    storage *store = data->store;
    region *r;
    int x, y;
    READ_INT(store, &x);
    READ_INT(store, &y);
    r = readregion(data, x, y);
    resolve_region(r);
    return r;
}

void writeregion(gamedata *data, const region * r)
{
    assert(r);
    assert(data);

    WRITE_INT(data->store, r->uid);
    WRITE_TOK(data->store, r->terrain->_name);
    WRITE_INT(data->store, r->flags & RF_SAVEMASK);
    WRITE_INT(data->store, r->age);
    WRITE_SECTION(data->store);
    if (r->land) {
        const item_type *rht;
        struct demand *demand;
        rawmaterial *res = r->resources;

        WRITE_STR(data->store, (const char *)r->land->name);
        WRITE_STR(data->store, region_getinfo(r));
        assert(rtrees(r, 0) >= 0);
        assert(rtrees(r, 1) >= 0);
        assert(rtrees(r, 2) >= 0);
        WRITE_INT(data->store, rtrees(r, 0));
        WRITE_INT(data->store, rtrees(r, 1));
        WRITE_INT(data->store, rtrees(r, 2));
        WRITE_INT(data->store, rhorses(r));

        while (res) {
            WRITE_TOK(data->store, res->rtype->_name);
            WRITE_INT(data->store, res->level);
            WRITE_INT(data->store, res->amount);
            WRITE_INT(data->store, res->startlevel);
            WRITE_INT(data->store, res->base);
            WRITE_INT(data->store, res->divisor);
            res = res->next;
        }
        WRITE_TOK(data->store, "end");

        rht = rherbtype(r);
        if (rht) {
            WRITE_TOK(data->store, resourcename(rht->rtype, 0));
        }
        else {
            WRITE_TOK(data->store, "noherb");
        }
        WRITE_INT(data->store, rherbs(r));
        WRITE_INT(data->store, rpeasants(r));
        WRITE_INT(data->store, rmoney(r));
        for (demand = r->land->demands; demand; demand = demand->next) {
            WRITE_TOK(data->store, resourcename(demand->type->itype->rtype, 0));
            WRITE_INT(data->store, demand->value);
        }
        WRITE_TOK(data->store, "end");
        WRITE_SECTION(data->store);
        WRITE_INT(data->store, region_get_morale(r));
        write_owner(data, r->land->ownership);
        WRITE_SECTION(data->store);
    }
    write_attribs(data->store, r->attribs, r);
    WRITE_SECTION(data->store);
}

void write_region(gamedata *data, const region *r)
{
    storage *store = data->store;
    WRITE_INT(store, r->x);
    WRITE_INT(store, r->y);
    writeregion(data, r);
}

int get_spell_level_faction(const spell * sp, void * cbdata)
{
    static spellbook * common = 0;
    spellbook * book;
    faction * f = (faction *)cbdata;
    spellbook_entry * sbe;

    book = get_spellbook(magic_school[f->magiegebiet]);
    if (book) {
        sbe = spellbook_get(book, sp);
        if (sbe) return sbe->level;
    }
    if (!common) {
        common = get_spellbook(magic_school[M_COMMON]);
    }
    sbe = spellbook_get(common, sp);
    if (sbe) {
        return sbe->level;
    }
    else {
        log_error("read_spellbook: faction '%s' has a spell with unknown level: '%s'", factionname(f), sp->sname);
    }
    return 0;
}

static char * getpasswd(int fno) {
    const char *prefix = itoa36(fno);
    size_t len = strlen(prefix);
    FILE * F = fopen("passwords.txt", "r");
    char line[80];
    if (F) {
        while (!feof(F)) {
            fgets(line, sizeof(line), F);
            if (line[len] == ':' && strncmp(prefix, line, len) == 0) {
                size_t slen = strlen(line) - 1;
                assert(line[slen] == '\n');
                line[slen] = 0;
                fclose(F);
                return strdup(line + len + 1);
            }
        }
        fclose(F);
    }
    return NULL;
}

static void read_password(gamedata *data, faction *f) {
    char name[128];
    READ_STR(data->store, name, sizeof(name));
    if (name[0] == '$' && data->version == BADCRYPT_VERSION) {
        char * pass = getpasswd(f->no);
        if (pass) {
            faction_setpassword(f, password_encode(pass, PASSWORD_DEFAULT));
            free(pass); /* TODO: remove this allocation! */
        }
        else {
            log_error("data version is BADCRYPT but %s not in password.txt", itoa36(f->no));
        }
    }
    else {
        faction_setpassword(f, (data->version >= CRYPT_VERSION) ? name : password_encode(name, PASSWORD_DEFAULT));
    }
}

void _test_read_password(gamedata *data, faction *f) {
    read_password(data, f);
}

static void write_password(gamedata *data, const faction *f) {
    WRITE_TOK(data->store, (const char *)f->_password);
}

void _test_write_password(gamedata *data, const faction *f) {
    write_password(data, f);
}

faction *read_faction(gamedata * data)
{
    int planes, n;
    faction *f;
    char name[DISPLAYSIZE];

    READ_INT(data->store, &n);
    assert(n > 0);
    f = findfaction(n);
    if (f == NULL) {
        f = (faction *)calloc(1, sizeof(faction));
        f->no = n;
    }
    else {
        f->allies = NULL;           /* FIXME: mem leak */
        while (f->attribs) {
            a_remove(&f->attribs, f->attribs);
        }
    }
    READ_INT(data->store, &f->subscription);

    if (data->version >= SPELL_LEVEL_VERSION) {
        READ_INT(data->store, &f->max_spelllevel);
    }
    if (alliances || data->version >= OWNER_2_VERSION) {
        int allianceid;
        READ_INT(data->store, &allianceid);
        if (allianceid > 0)
            f->alliance = findalliance(allianceid);
        if (f->alliance) {
            alliance *al = f->alliance;
            if (al->flags & ALF_NON_ALLIED) {
                assert(!al->members
                    || !"non-allied dummy-alliance has more than one member");
            }
            selist_push(&al->members, f);
        }
        else if (rule_region_owners()) {
            /* compat fix for non-allied factions */
            alliance *al = makealliance(0, NULL);
            setalliance(f, al);
        }
        if (data->version >= OWNER_2_VERSION) {
            READ_INT(data->store, &f->alliance_joindate);
        }
        else {
            f->alliance_joindate = turn - 10; /* we're guessing something safe here */
        }
    }

    READ_STR(data->store, name, sizeof(name));
	if (unicode_utf8_trim(name)!=0) {
		log_warning("trim faction %s name to '%s'", itoa36(f->no), name);
	};
    f->name = strdup(name);
    READ_STR(data->store, name, sizeof(name));
	if (unicode_utf8_trim(name)!=0) {
		log_warning("trim faction %s banner to '%s'", itoa36(f->no), name);
	};
    f->banner = strdup(name);

    log_debug("   - Lese Partei %s (%s)", f->name, itoa36(f->no));

    READ_STR(data->store, name, sizeof(name));
    if (check_email(name) == 0) {
      faction_setemail(f, name);
    } else {
      log_warning("Invalid email address for faction %s: %s", itoa36(f->no), name);
      faction_setemail(f, NULL);
    }

    read_password(data, f);
    if (data->version < NOOVERRIDE_VERSION) {
        READ_STR(data->store, 0, 0);
    }

    READ_STR(data->store, name, sizeof(name));
    f->locale = get_locale(name);
    if (!f->locale) f->locale = default_locale;
    READ_INT(data->store, &f->lastorders);
    READ_INT(data->store, &f->age);
    READ_STR(data->store, name, sizeof(name));
    f->race = rc_find(name);
    if (!f->race) {
        log_error("unknown race in data: %s", name);
    }
    assert(f->race);
    READ_INT(data->store, &n);
    f->magiegebiet = (magic_t)n;

    if (data->version < FOSS_VERSION) {
        /* ignore karma */
        READ_INT(data->store, &n);
    }

    READ_INT(data->store, &f->flags);
    if (data->version < INTFLAGS_VERSION) {
        if (f->no == 0 || f->no == 666) {
            f->flags = FFL_NPC | FFL_NOIDLEOUT;
        }
    }

    read_attribs(data, &f->attribs, f);
    read_items(data->store, &f->items);
    for (;;) {
        READ_TOK(data->store, name, sizeof(name));
        if (strcmp("end", name) == 0)
            break;
        READ_INT(data->store, &n); /* there used to be a level here, which is now ignored */
    }
    READ_INT(data->store, &planes);
    while (--planes >= 0) {
        int id, ux, uy;
        READ_INT(data->store, &id);
        READ_INT(data->store, &ux);
        READ_INT(data->store, &uy);
        faction_setorigin(f, id, ux, uy);
    }
    f->newbies = 0;

    READ_INT(data->store, &n);
    f->options = n;

    n = want(O_REPORT) | want(O_COMPUTER);
    if ((f->options & n) == 0) {
        /* Kein Report eingestellt, Fehler */
        f->options |= n;
    }
    if (data->version < JSON_REPORT_VERSION) {
        /* mistakes were made in the past*/
        f->options &= ~want(O_JSON);
    }
    read_allies(data, f);
    read_groups(data, f);
    f->spellbook = 0;
    if (data->version >= REGIONOWNER_VERSION) {
        read_spellbook(FactionSpells() ? &f->spellbook : 0, data, get_spell_level_faction, (void *)f);
    }
    resolve_faction(f);
    return f;
}

void write_faction(gamedata *data, const faction * f)
{
    ally *sf;
    ursprung *ur;

    assert(f->_alive);
    write_faction_reference(f, data->store);
    WRITE_INT(data->store, f->subscription);
#if RELEASE_VERSION >= SPELL_LEVEL_VERSION
    WRITE_INT(data->store, f->max_spelllevel);
#endif
    if (f->alliance) {
        WRITE_INT(data->store, f->alliance->id);
        if (f->alliance->flags & ALF_NON_ALLIED) {
            assert(f == f->alliance->_leader
                || !"non-allied faction is not leader of its own dummy-alliance.");
        }
    }
    else {
        WRITE_INT(data->store, 0);
    }
    WRITE_INT(data->store, f->alliance_joindate);

    WRITE_STR(data->store, f->name);
    WRITE_STR(data->store, f->banner);
    WRITE_STR(data->store, f->email?f->email:"");
    write_password(data, f);
    WRITE_TOK(data->store, locale_name(f->locale));
    WRITE_INT(data->store, f->lastorders);
    WRITE_INT(data->store, f->age);
    WRITE_TOK(data->store, f->race->_name);
    WRITE_SECTION(data->store);
    WRITE_INT(data->store, f->magiegebiet);

    WRITE_INT(data->store, f->flags & FFL_SAVEMASK);
    write_attribs(data->store, f->attribs, f);
    WRITE_SECTION(data->store);
    write_items(data->store, f->items);
    WRITE_SECTION(data->store);
    WRITE_TOK(data->store, "end");
    WRITE_SECTION(data->store);
    WRITE_INT(data->store, listlen(f->ursprung));
    for (ur = f->ursprung; ur; ur = ur->next) {
        WRITE_INT(data->store, ur->id);
        WRITE_INT(data->store, ur->x);
        WRITE_INT(data->store, ur->y);
    }
    WRITE_SECTION(data->store);
    WRITE_INT(data->store, f->options & ~want(O_DEBUG));
    WRITE_SECTION(data->store);

    for (sf = f->allies; sf; sf = sf->next) {
        int no;
        int status;

        assert(sf->faction);

        no = sf->faction->no;
        status = alliedfaction(NULL, f, sf->faction, HELP_ALL);

        if (status != 0) {
            WRITE_INT(data->store, no);
            WRITE_INT(data->store, sf->status);
        }
    }
    WRITE_INT(data->store, 0);
    WRITE_SECTION(data->store);
    write_groups(data->store, f);
    write_spellbook(f->spellbook, data->store);
}

static int cb_sb_maxlevel(spellbook_entry *sbe, void *cbdata) {
    faction *f = (faction *)cbdata;
    if (sbe->level > f->max_spelllevel) {
        f->max_spelllevel = sbe->level;
    }
    return 0;
}

int readgame(const char *filename)
{
    int n, stream_version;
    char path[MAX_PATH];
    gamedata gdata = { 0 };
    storage store;
    stream strm;
    FILE *F;
    size_t sz;

    log_debug("- reading game data from %s", filename);
    join_path(datapath(), filename, path, sizeof(path));

    F = fopen(path, "rb");
    if (!F) {
        perror(path);
        return -1;
    }
    sz = fread(&gdata.version, sizeof(int), 1, F);
    sz = fread(&stream_version, sizeof(int), 1, F);
    assert((sz == 1 && stream_version == STREAM_VERSION) || !"unsupported data format");
    assert(gdata.version >= MIN_VERSION || !"unsupported data format");
    assert(gdata.version <= MAX_VERSION || !"unsupported data format");

    fstream_init(&strm, F);
    binstore_init(&store, &strm);
    gdata.store = &store;

    if (gdata.version >= BUILDNO_VERSION) {
        int build;
        READ_INT(&store, &build);
        log_debug("data in %s created with build %d.", filename, build);
    }
    n = read_game(&gdata);
    binstore_done(&store);
    fstream_done(&strm);
    return n;
}

void write_building(gamedata *data, const building *b)
{
    storage *store = data->store;

    write_building_reference(b, store);
    WRITE_STR(store, b->name);
    WRITE_STR(store, b->display ? b->display : "");
    WRITE_INT(store, b->size);
    WRITE_TOK(store, b->type->_name);
    write_attribs(store, b->attribs, b);
}

struct building *read_building(gamedata *data) {
    char name[DISPLAYSIZE];
	building *b;
    storage * store = data->store;

    b = (building *)calloc(1, sizeof(building));
    READ_INT(store, &b->no);
    bhash(b);
    READ_STR(store, name, sizeof(name));
    if (unicode_utf8_trim(name)!=0) {
		log_warning("trim building %s name to '%s'", itoa36(b->no), name);
	}
    b->name = strdup(name);
    READ_STR(store, name, sizeof(name));
    if (unicode_utf8_trim(name)!=0) {
        log_warning("trim building %s info to '%s'", itoa36(b->no), name);
    }
    b->display = strdup(name);
    READ_INT(store, &b->size);
    READ_STR(store, name, sizeof(name));
    b->type = bt_find(name);
    if (!b->type) {
        log_error("building %d has unknown type %s", b->no, name);
        b->type = bt_find("building");
        assert(b->type);
    }
    read_attribs(data, &b->attribs, b);

    /* repairs, bug 2221: */
    if (b->type->maxsize>0 && b->size>b->type->maxsize) {
        log_error("building too big: %s (%s size %d of %d), fixing.", buildingname(b), b->type->_name, b->size, b->type->maxsize);
        b->size = b->type->maxsize;
    }
    resolve_building(b);
    return b;
}

void write_ship(gamedata *data, const ship *sh)
{
    storage *store = data->store;
    write_ship_reference(sh, store);
    WRITE_STR(store, (const char *)sh->name);
    WRITE_STR(store, sh->display ? (const char *)sh->display : "");
    WRITE_TOK(store, sh->type->_name);
    WRITE_INT(store, sh->size);
    WRITE_INT(store, sh->damage);
    WRITE_INT(store, sh->flags & SFL_SAVEMASK);
    assert((sh->type->flags & SFL_NOCOAST) == 0 || sh->coast == NODIRECTION);
    WRITE_INT(store, sh->coast);
    write_attribs(store, sh->attribs, sh);
}

ship *read_ship(gamedata *data)
{
    char name[DISPLAYSIZE];
    ship *sh;
    int n;
    storage *store = data->store;

    sh = (ship *)calloc(1, sizeof(ship));
    READ_INT(store, &sh->no);
    shash(sh);
    READ_STR(store, name, sizeof(name));
    if (unicode_utf8_trim(name)!=0) {
		log_warning("trim ship %s name to '%s'", itoa36(sh->no), name);
	}
    sh->name = strdup(name);
    READ_STR(store, name, sizeof(name));
    if (unicode_utf8_trim(name)!=0) {
        log_warning("trim ship %s info to '%s'", itoa36(sh->no), name);
    }
    sh->display = strdup(name);
    READ_STR(store, name, sizeof(name));
    sh->type = st_find(name);
    if (sh->type == NULL) {
        /* old datafiles */
        sh->type = st_find((const char *)LOC(default_locale, name));
    }
    assert(sh->type || !"ship_type not registered!");

    READ_INT(store, &sh->size);
    READ_INT(store, &sh->damage);
    if (data->version >= FOSS_VERSION) {
        READ_INT(store, &sh->flags);
    }

    /* Attribute rekursiv einlesen */

    READ_INT(store, &n);
    sh->coast = (direction_t)n;
    if (sh->type->flags & SFL_NOCOAST) {
        sh->coast = NODIRECTION;
    }
    read_attribs(data, &sh->attribs, sh);
    return sh;
}


static void fix_familiars(void) {
    region *r;
    for (r = regions; r; r = r->next) {
        unit * u;
        for (u = r->units; u; u = u->next) {
            if (u->_race != u->faction->race && (u->_race->flags & RCF_FAMILIAR)) {
                /* unit is potentially a familiar */
                attrib * a = a_find(u->attribs, &at_mage);
                attrib * am = a_find(u->attribs, &at_familiarmage);
                if (am) {
                    sc_mage *mage = a ? (sc_mage *)a->data.v : NULL;
                    /* a familiar */
                    if (!mage) {
                        log_error("%s seems to be a familiar with no magic.",
                            unitname(u));
                        mage = create_mage(u, M_GRAY);
                    }
                    if (!mage->spellbook) {
                        char eqname[32];
                        equipment *eq;
                        
                        snprintf(eqname, sizeof(eqname), "fam_%s", u->_race->_name);
                        eq = get_equipment(eqname);
                        if (eq && eq->spells) {
                            log_error("%s seems to be a familiar with no spells.",
                                unitname(u));
                            /* magical familiar, no spells */
                            equip_unit_mask(u, eq, EQUIP_SPELLS);
                        }
                    }
                }
                else if (a) {
                    /* not a familiar, but magical */
                    attrib * ae = a_find(u->attribs, &at_eventhandler);
                    if (ae) {
                        trigger **tlist;
                        tlist = get_triggers(ae, "destroy");
                        if (tlist) {
                            trigger *t;
                            unit *um = NULL;
                            for (t = *tlist; t; t = t->next) {
                                if (t->type == &tt_shock) {
                                    um = (unit *)t->data.v;
                                    break;
                                }
                            }
                            if (um) {
                                attrib *af = a_find(um->attribs, &at_familiar);
                                log_error("%s seems to be a broken familiar of %s.",
                                    unitname(u), unitname(um));
                                if (af) {
                                    unit * uf = (unit *)af->data.v;
                                    log_error("%s already has a familiar: %s.",
                                        unitname(um), unitname(uf));
                                }
                                else {
                                    set_familiar(um, u);
                                }
                            }
                            else {
                                log_error("%s seems to be a broken familiar with no trigger.", unitname(u));
                            }
                        }
                    }
                }
            }
        }
    }
}

int read_game(gamedata *data)
{
    int p, nread;
    faction *f, **fp;
    region *r;
    building **bp;
    ship **shp;
    unit *u;
    storage * store = data->store;
    const struct building_type *bt_lighthouse = bt_find("lighthouse");
    const struct race *rc_spell = rc_find("spell");

    if (data->version >= SAVEGAMEID_VERSION) {
        int gameid;

        READ_INT(store, &gameid);
        if (gameid != game_id()) {
            log_warning("game mismatch: datafile contains game %d, but config is for %d", gameid, game_id());
        }
    }
    else {
        READ_STR(store, NULL, 0);
    }
    read_attribs(data, &global.attribs, NULL);
    READ_INT(store, &turn);
    log_debug(" - reading turn %d", turn);
    rng_init(turn);
    READ_INT(store, NULL);          /* max_unique_id = ignore */
    READ_INT(store, &nextborder);

    read_planes(data);
    read_alliances(data);
    READ_INT(store, &nread);
    log_debug(" - Einzulesende Parteien: %d\n", nread);
    fp = &factions;
    while (*fp) {
        fp = &(*fp)->next;
    }

    while (--nread >= 0) {
        faction *f = read_faction(data);

        *fp = f;
        fp = &f->next;
        fhash(f);
    }
    *fp = 0;

    /* Regionen */

    READ_INT(store, &nread);
    assert(nread < MAXREGIONS && nread >=0);

    log_debug(" - Einzulesende Regionen: %d", nread);

    while (--nread >= 0) {
        unit **up;

        r = read_region(data);

        /* Burgen */
        READ_INT(store, &p);
        if (p > 0 && !r->land) {
            log_error("%s, uid=%d has %d %s", regionname(r, NULL), r->uid, p, (p==1) ? "building" : "buildings");
        }
        bp = &r->buildings;

        while (--p >= 0) {
            building *b = *bp = read_building(data);
            if (b->type == bt_lighthouse) {
                r->flags |= RF_LIGHTHOUSE;
            }
            b->region = r;
            bp = &b->next;
        }
        /* Schiffe */

        READ_INT(store, &p);
        shp = &r->ships;

        while (--p >= 0) {
            ship *sh = *shp = read_ship(data);
            sh->region = r;
            shp = &sh->next;
        }

        *shp = 0;

        /* Einheiten */

        READ_INT(store, &p);
        up = &r->units;

        while (--p >= 0) {
            unit *u = read_unit(data);

            if (data->version < NORCSPELL_VERSION && u_race(u) == rc_spell) {
                set_observer(r, u->faction, get_level(u, SK_PERCEPTION), u->age);
                u_setfaction(u, NULL);
                free_unit(u);
            }
            else {
                if (data->version < JSON_REPORT_VERSION) {
                    if (u->_name && fval(u->faction, FFL_NPC)) {
                        if (!u->_name[0] || unit_name_equals_race(u)) {
                            unit_setname(u, NULL);
                        }
                    }
                }
                assert(u->region == NULL);
                u->region = r;
                *up = u;
                up = &u->next;
                update_interval(u->faction, r);
            }
        }
    }
    read_borders(data);

    log_debug("updating area information for lighthouses.");
    for (r = regions; r; r = r->next) {
        if (r->flags & RF_LIGHTHOUSE) {
            building *b;
            for (b = r->buildings; b; b = b->next) {
                update_lighthouse(b);
            }
        }
    }
    log_debug("marking factions as alive.");
    for (f = factions; f; f = f->next) {
        if (f->flags & FFL_NPC) {
            f->_alive = true;
            f->magiegebiet = M_GRAY;
            if (f->no == 0) {
                int no = 666;
                while (findfaction(no))
                    ++no;
                log_warning("renum(monsters, %d)", no);
                renumber_faction(f, no);
            }
        }
        else {
            for (u = f->units; u; u = u->nextF) {
                if (data->version < SPELL_LEVEL_VERSION) {
                    sc_mage *mage = get_mage_depr(u);
                    if (mage) {
                        faction *f = u->faction;
                        int skl = effskill(u, SK_MAGIC, 0);
                        if (f->magiegebiet == M_GRAY) {
                            log_error("faction %s had magic=gray, fixing (%s)", factionname(f), magic_school[mage->magietyp]);
                            f->magiegebiet = mage->magietyp;
                        }
                        if (f->max_spelllevel < skl) {
                            f->max_spelllevel = skl;
                        }
                        if (mage->spellcount < 0) {
                            mage->spellcount = 0;
                        }
                    }
                }
                if (u->number > 0) {
                    f->_alive = true;
                    if (data->version >= SPELL_LEVEL_VERSION) {
                        break;
                    }
                }
            }
            if (data->version < SPELL_LEVEL_VERSION && f->spellbook) {
                spellbook_foreach(f->spellbook, cb_sb_maxlevel, f);
            }
        }
    }

    if (data->version < FAMILIAR_FIX_VERSION) {
        fix_familiars();
    }

    log_debug("Done loading turn %d.", turn);

    return 0;
}

static void clear_npc_orders(faction *f)
{
    if (f) {
        unit *u;
        for (u = f->units; u; u = u->nextF) {
            free_orders(&u->orders);
        }
    }
}

int writegame(const char *filename)
{
    int n;
    char path[MAX_PATH];
    gamedata gdata;
    storage store;
    stream strm;
    FILE *F;

    create_directories();
    join_path(datapath(), filename, path, sizeof(path));
#ifdef HAVE_UNISTD_H
    /* make sure we don't overwrite an existing file (hard links) */
    if (remove(path)!=0) {
        if (errno==ENOENT) {
            errno = 0;
        }
    }
#endif
    F = fopen(path, "wb");
    if (!F) {
        perror(path);
        return -1;
    }

    gdata.store = &store;
    gdata.version = RELEASE_VERSION;
    fwrite(&gdata.version, sizeof(int), 1, F);
    n = STREAM_VERSION;
    fwrite(&n, sizeof(int), 1, F);

    fstream_init(&strm, F);
    binstore_init(&store, &strm);

    WRITE_INT(&store, version_no(eressea_version()));
    n = write_game(&gdata);
    binstore_done(&store);
    fstream_done(&strm);
    return n;
}

int write_game(gamedata *data) {
    storage * store = data->store;
    region *r;
    faction *f;
    int n;

    /* globale Variablen */
    assert(data->version <= MAX_VERSION && data->version >= MIN_VERSION);

    WRITE_INT(store, game_id());
    WRITE_SECTION(store);

    write_attribs(store, global.attribs, NULL);
    WRITE_SECTION(store);

    WRITE_INT(store, turn);
    WRITE_INT(store, 0 /* max_unique_id */);
    WRITE_INT(store, nextborder);

    write_planes(store);
    write_alliances(data);
    n = listlen(factions);
    WRITE_INT(store, n);
    WRITE_SECTION(store);

    log_debug(" - Schreibe %d Parteien...", n);
    for (f = factions; f; f = f->next) {
        if (fval(f, FFL_NPC)) {
            clear_npc_orders(f);
        }
        write_faction(data, f);
        WRITE_SECTION(store);
    }

    /* Write regions */

    n = listlen(regions);
    WRITE_INT(store, n);
    WRITE_SECTION(store);
    log_debug(" - Schreibe Regionen: %d", n);

    for (r = regions; r; r = r->next, --n) {
        ship *sh;
        building *b;
        unit *u;
        /* plus leerzeile */
        if ((n % 1024) == 0) {      /* das spart extrem Zeit */
            log_debug(" - Schreibe Regionen: %d", n);
        }
        WRITE_SECTION(store);
        write_region(data, r);

        WRITE_INT(store, listlen(r->buildings));
        WRITE_SECTION(store);
        for (b = r->buildings; b; b = b->next) {
            assert(b->region == r);
            write_building(data, b);
        }

        WRITE_INT(store, listlen(r->ships));
        WRITE_SECTION(store);
        for (sh = r->ships; sh; sh = sh->next) {
            assert(sh->region == r);
            write_ship(data, sh);
        }

        WRITE_INT(store, listlen(r->units));
        WRITE_SECTION(store);
        for (u = r->units; u; u = u->next) {
            assert(u->region == r);
            write_unit(data, u);
        }
    }
    WRITE_SECTION(store);
    write_borders(store);
    WRITE_SECTION(store);

    return 0;
}
