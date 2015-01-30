/*
Copyright (c) 1998-2015, Enno Rehling Rehling <enno@eressea.de>
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

#if ARENA_MODULE
#include "arena.h"

/* modules include */
#include "score.h"

/* items include */
#include <items/demonseye.h>

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

#include <move.h>

/* util include */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/functions.h>
#include <util/goodies.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/rng.h>

#include <storage.h>

/* libc include */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* exports: */
plane *arena = NULL;

/* local vars */
#define CENTRAL_VOLCANO 1

#ifdef ARENA_CREATION
static unsigned int arena_id = 0;
static region *arena_center = NULL;
static int newarena = 0;
#endif
static region *tower_region[6];
static region *start_region[6];

static region *arena_region(int school)
{
    return tower_region[school];
}

static building *arena_tower(int school)
{
    return arena_region(school)->buildings;
}

static int leave_fail(unit * u)
{
    ADDMSG(&u->faction->msgs, msg_message("arena_leave_fail", "unit", u));
    return 1;
}

static int
leave_arena(struct unit *u, const struct item_type *itype, int amount,
order * ord)
{
    if (!u->building && leave_fail(u)) {
        return -1;
    }
    if (u->building != arena_tower(u->faction->magiegebiet) && leave_fail(u)) {
        return -1;
    }
    unused_arg(amount);
    unused_arg(ord);
    unused_arg(itype);
    assert(!"not implemented");
    return 0;
}

static int enter_fail(unit * u)
{
    ADDMSG(&u->faction->msgs, msg_message("arena_enter_fail", "region unit",
        u->region, u));
    return 1;
}

static int
enter_arena(unit * u, const item_type * itype, int amount, order * ord)
{
    skill_t sk;
    region *r = u->region;
    unit *u2;
    int fee = u->faction->score / 5;
    unused_arg(ord);
    unused_arg(amount);
    unused_arg(itype);
    if (fee > 2000)
        fee = 2000;
    if (getplane(r) == arena)
        return -1;
    if (u->number != 1 && enter_fail(u))
        return -1;
    if (get_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT, fee) < fee
        && enter_fail(u))
        return -1;
    for (sk = 0; sk != MAXSKILLS; ++sk) {
        if (get_level(u, sk) > 1 && enter_fail(u))
            return -1;
    }
    for (u2 = r->units; u2; u2 = u2->next)
        if (u2->faction == u->faction)
            break;

    assert(!"not implemented");
    /*
            for (res=0;res!=MAXRESOURCES;++res) if (res!=R_SILVER && res!=R_ARENA_GATE && (is_item(res) || is_herb(res) || is_potion(res))) {
            int x = get_resource(u, res);
            if (x) {
            if (u2) {
            change_resource(u2, res, x);
            change_resource(u, res, -x);
            }
            else if (enter_fail(u)) return -1;
            }
            }
            */
    if (get_money(u) > fee) {
        if (u2)
            change_money(u2, get_money(u) - fee);
        else if (enter_fail(u))
            return -1;
    }
    ADDMSG(&u->faction->msgs, msg_message("arena_enter_fail", "region unit",
        u->region, u));
    use_pooled(u, itype->rtype, GET_SLACK | GET_RESERVE, 1);
    use_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT, fee);
    set_money(u, 109);
    fset(u, UFL_ANON_FACTION);
    move_unit(u, start_region[rng_int() % 6], NULL);
    return 0;
}

/***
 ** Szepter der Tränen, Demo-Item
 ***/

static int
use_wand_of_tears(unit * user, const struct item_type *itype, int amount,
order * ord)
{
    int n;
    unused_arg(ord);
    for (n = 0; n != amount; ++n) {
        unit *u;
        for (u = user->region->units; u; u = u->next) {
            if (u->faction != user->faction) {
                int i;

                for (i = 0; i != u->skill_size; ++i) {
                    if (rng_int() % 3)
                        reduce_skill(u, u->skills + i, 1);
                }
                ADDMSG(&u->faction->msgs, msg_message("wand_of_tears_effect",
                    "unit", u));
            }
        }
    }
    ADDMSG(&user->region->msgs, msg_message("wand_of_tears_usage", "unit", user));
    return 0;
}

/**
 * Tempel der Schreie, Demo-Gebäude **/

static int age_hurting(attrib * a)
{
    building *b = (building *)a->data.v;
    unit *u;
    int active = 0;
    if (b == NULL)
        return AT_AGE_REMOVE;
    for (u = b->region->units; u; u = u->next) {
        if (u->building == b) {
            if (u->faction->magiegebiet == M_DRAIG) {
                active++;
                ADDMSG(&b->region->msgs, msg_message("praytoigjarjuk", "unit", u));
            }
        }
    }
    if (active)
        for (u = b->region->units; u; u = u->next)
            if (playerrace(u->faction->race)) {
                int i;
                if (u->faction->magiegebiet != M_DRAIG) {
                    for (i = 0; i != active; ++i)
                        u->hp = (u->hp + 1) / 2;    /* make them suffer, but not die */
                    ADDMSG(&b->region->msgs, msg_message("cryinpain", "unit", u));
                }
            }
    return AT_AGE_KEEP;
}

static void
write_hurting(const attrib * a, const void *owner, struct storage *store)
{
    building *b = a->data.v;
    WRITE_INT(store, b->no);
}

static int read_hurting(attrib * a, void *owner, struct storage *store)
{
    int i;
    READ_INT(store, &i);
    a->data.v = (void *)findbuilding(i);
    if (a->data.v == NULL) {
        log_error("temple of pain is broken\n");
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

static attrib_type at_hurting = {
    "hurting", NULL, NULL, age_hurting, write_hurting, read_hurting
};

#ifdef ARENA_CREATION
static void make_temple(region * r)
{
    const building_type *btype = bt_find("temple");
    building *b;
    if (btype == NULL) {
        log_error("could not find buildingtype 'temple'\n");
        return;
    }

    b = r->buildings;
    while (b != NULL && b->type != btype)
        b = b->next;
    if (b != NULL)
        return;                     /* gibt schon einen */

    b = new_building(btype, r, NULL);
    b->size = btype->maxsize;
    b->name = _strdup("Igjarjuk's Tempel der Schreie");
    b->display =
        _strdup
        ("Ein Schrein aus spitzen Knochen und lodernden Flammen, gewidmet dem Wyrm der Wyrme");
    a_add(&b->attribs, a_new(&at_hurting))->data.v = b;
}
#endif

/**
 * Initialisierung Türme */

#ifdef ARENA_CREATION
static void tower_init(void)
{
    int i, first = newarena;
    item_type *it_demonseye = it_find("demonseye");
    item_type *it_griphonwing = it_find("griphonwing");
    assert(it_griphonwing && it_demonseye);
    for (i = 0; i != 6; ++i) {
        region *r = tower_region[i] =
            findregion(arena_center->x + delta_x[i] * 3,
            arena_center->y + delta_y[i] * 3);
        if (r) {
            start_region[i] =
                findregion(arena_center->x + delta_x[i] * 2,
                arena_center->y + delta_y[i] * 2);
            if (rterrain(r) != T_DESERT)
                terraform(r, T_DESERT);
            if (!r->buildings) {
                building *b = new_building(bt_find("castle"), r, NULL);
                b->size = 10;
                if (i != 0) {
                    sprintf(buf, "Turm des %s",
                        LOC(default_locale, mkname("school", magic_school[i])));
                } else
                    sprintf(buf, "Turm der Ahnungslosen");
                set_string(&b->name, buf);
            }
        }
    }
    if (first && !arena_center->buildings) {
        building *b = new_building(bt_find("castle"), arena_center, NULL);
        attrib *a;
        item *items;

        i_add(&items, i_new(it_griphonwing, 1));
        i_add(&items, i_new(it_demonseye, 1));
        a = a_add(&b->attribs, make_giveitem(b, items));

        b->size = 10;
        set_string(&b->name, "Höhle des Greifen");
    }
}
#endif

#ifdef ARENA_CREATION
static void guardian_faction(plane * pl, int id)
{
    region *r;
    faction *f = findfaction(id);

    if (!f) {
        f = calloc(1, sizeof(faction));
        f->banner = _strdup("Sie dienen dem großen Wyrm");
        f->passw = _strdup(itoa36(rng_int()));
        set_email(&f->email, "igjarjuk@eressea.de");
        f->name = _strdup("Igjarjuks Kundschafter");
        f->race = get_race(RC_ILLUSION);
        f->age = turn;
        f->locale = get_locale("de");
        f->options =
            want(O_COMPRESS) | want(O_REPORT) | want(O_COMPUTER) | want(O_ADRESSEN) |
            want(O_DEBUG);

        f->no = id;
        addlist(&factions, f);
        fhash(f);
    }
    if (f->race != get_race(RC_ILLUSION)) {
        assert(!"guardian id vergeben");
        exit(0);
    }
    f->lastorders = turn;
    f->alive = true;
    for (r = regions; r; r = r->next)
        if (getplane(r) == pl && rterrain(r) != T_FIREWALL) {
            unit *u;
            freset(r, RF_ENCOUNTER);
            for (u = r->units; u; u = u->next) {
                if (u->faction == f)
                    break;
            }
            if (u)
                continue;
            u = create_unit(r, f, 1, get_race(RC_GOBLIN), 0, NULL, NULL);
            set_string(&u->name, "Igjarjuks Auge");
            i_change(&u->items, it_find("roi"), 1);
            set_order(&u->thisorder, NULL);
            fset(u, UFL_ANON_FACTION);
            set_money(u, 1000);
        }
}
#endif

#define BLOCKSIZE           9

#ifdef ARENA_CREATION
static void block_create(int x1, int y1, char terrain)
{
    int x, y;
    for (x = 0; x != BLOCKSIZE; ++x) {
        for (y = 0; y != BLOCKSIZE; ++y) {
            region *r = new_region(x1 + x, y1 + y, 0);
            terraform(r, terrain);
        }
    }
}
#endif

#ifdef CENTRAL_VOLCANO

static int caldera_handle(trigger * t, void *data)
{
    /* call an event handler on caldera.
     * data.v -> ( variant event, int timer )
     */
    building *b = (building *)t->data.v;
    if (b != NULL) {
        unit **up = &b->region->units;
        while (*up) {
            unit *u = *up;
            if (u->building == b) {
                message *msg;
                if (u->items) {
                    item **ip = &u->items;
                    msg = msg_message("caldera_handle_1", "unit items", u, u->items);
                    while (*ip) {
                        item *i = *ip;
                        i_remove(ip, i);
                        if (*ip == i)
                            ip = &i->next;
                    }
                }
                else {
                    msg = msg_message("caldera_handle_0", "unit", u);
                }
                add_message(&u->region->msgs, msg);
                set_number(u, 0);
            }
            if (*up == u)
                up = &u->next;
        }
    }
    else {
        log_error("could not perform caldera::handle()\n");
    }
    unused_arg(data);
    return 0;
}

static void caldera_write(const trigger * t, struct storage *store)
{
    building *b = (building *)t->data.v;
    write_building_reference(b, store);
}

static int caldera_read(trigger * t, struct storage *store)
{
    int rb =
        read_reference(&t->data.v, store, read_building_reference,
        resolve_building);
    if (rb == 0 && !t->data.v) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

struct trigger_type tt_caldera = {
    "caldera",
    NULL,
    NULL,
    caldera_handle,
    caldera_write,
    caldera_read
};

#ifdef ARENA_CREATION
static trigger *trigger_caldera(building * b)
{
    trigger *t = t_new(&tt_caldera);
    t->data.v = b;
    return t;
}
#endif

#ifdef ARENA_CREATION
static void init_volcano(void)
{
    building *b;
    region *r = arena_center;
    assert(arena_center);
    if (rterrain(r) != T_DESERT)
        return;                     /* been done before */
    terraform(arena_center, T_VOLCANO_SMOKING);
    b = new_building(bt_find("caldera"), r, NULL);
    b->size = 1;
    b->name = _strdup("Igjarjuk's Schlund");
    b->display =
        _strdup
        ("Feurige Lava fließt aus dem Krater des großen Vulkans. Alles wird von ihr verschlungen.");
    add_trigger(&b->attribs, "timer", trigger_caldera(b));
    tt_register(&tt_caldera);
}
#endif
#endif

#ifdef ARENA_CREATION
void create_arena(void)
{
    int x;
    arena_id = hashstring("arena");
    arena = getplanebyid(arena_id);
    if (arena != NULL)
        return;
    score();                      /* ist wichtig, damit alle Parteien einen score haben, wenn sie durchs Tor wollen. */
    guardian_faction(arena, 999);
    if (arena)
        arena_center = findregion(plane_center_x(arena), plane_center_y(arena));
    if (!arena_center) {
        newarena = 1;
        arena =
            create_new_plane(arena_id, "Arena", -10000, -10000, 0, BLOCKSIZE - 1,
            PFL_LOWSTEALING | PFL_NORECRUITS | PFL_NOALLIANCES);
        block_create(arena->minx, arena->miny, T_OCEAN);
        arena_center = findregion(plane_center_x(arena), plane_center_y(arena));
        for (x = 0; x != BLOCKSIZE; ++x) {
            int y;
            for (y = 0; y != BLOCKSIZE; ++y) {
                region *r = findregion(arena->minx + x, arena->miny + y);
                freset(r, RF_ENCOUNTER);
                r->planep = arena;
                switch (distance(r, arena_center)) {
                case 4:
                    terraform(r, T_FIREWALL);
                    break;
                case 0:
                    terraform(r, T_GLACIER);
                    break;
                case 1:
                    terraform(r, T_SWAMP);
                    break;
                case 2:
                    terraform(r, T_MOUNTAIN);
                    break;
                }
            }
        }
    }
    make_temple(arena_center);
#ifdef CENTRAL_VOLCANO
    init_volcano();
#else
    if (arena_center->terrain != T_DESERT)
        terraform(arena_center, T_DESERT);
#endif
    rsetmoney(arena_center, 0);
    rsetpeasants(arena_center, 0);
    tower_init();
}
#endif
void register_arena(void)
{
    at_register(&at_hurting);
    register_item_use(use_wand_of_tears, "use_wand_of_tears");
    register_function((pf_generic)enter_arena, "enter_arena");
    register_function((pf_generic)leave_arena, "leave_arena");
    tt_register(&tt_caldera);
}

#endif /* def ARENA_MODULE */
