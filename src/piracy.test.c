#include "piracy.h"

#include <kernel/config.h>
#include "kernel/direction.h" // for init_directions, D_EAST, directions
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/ship.h>
#include "kernel/skill.h"    // for SK_SAILING
#include <kernel/terrain.h>

#include <util/base36.h>
#include "util/keyword.h"    // for K_PIRACY
#include <util/language.h>
#include <util/message.h>

#include <stb_ds.h>
#include <CuTest.h>

#include <tests.h>
#include <assert.h>
#include <stddef.h>          // for NULL

static void setup_piracy(void) {
    struct locale *lang;
    ship_type *st_boat;

    config_set_int("rules.ship.storms", 0);
    lang = get_or_create_locale("de");
    locale_setstring(lang, directions[D_EAST], "OSTEN");
    init_directions(lang);
    test_create_terrain("ocean", SEA_REGION);
    st_boat = test_create_shiptype("boat");
    st_boat->cargo = 1000;

    mt_create_error(144);
    mt_create_error(146);
    mt_create_va(mt_new("piratenovictim", NULL),
        "ship:ship", "unit:unit", "region:region", MT_NEW_END);
    mt_create_va(mt_new("piratesawvictim", NULL),
        "ship:ship", "unit:unit", "region:region", "dir:int", MT_NEW_END);
    mt_create_va(mt_new("shipsail", NULL),
        "ship:ship", "from:region", "to:region", MT_NEW_END);
    mt_create_va(mt_new("shipfly", NULL),
        "ship:ship", "from:region", "to:region", MT_NEW_END);
    mt_create_va(mt_new("shipnoshore", NULL),
        "ship:ship", "region:region", MT_NEW_END);
    mt_create_va(mt_new("travel", NULL),
        "unit:unit", "start:region", "end:region", "mode:int", "regions:regions", MT_NEW_END);
}

static void setup_pirate(unit **pirate, int p_r_flags, int p_rc_flags, const char *p_shiptype,
    unit **victim, int v_r_flags, const char *v_shiptype) {
    terrain_type *vterrain;
    ship_type *st_boat = NULL;
    race *rc;
    faction *f;

    setup_piracy();
    vterrain = get_or_create_terrain("terrain1");
    fset(vterrain, v_r_flags);
    *victim = test_create_unit(test_create_faction(), test_create_region(1, 0, vterrain));
    assert(*victim);

    if (v_shiptype) {
        st_boat = st_get_or_create(v_shiptype);
        u_set_ship(*victim, test_create_ship((*victim)->region, st_boat));
        arrsetlen(st_boat->coasts, 1);
        st_boat->coasts[0] = vterrain;
    }

    *pirate = create_unit(test_create_region(0, 0, get_or_create_terrain("terrain2")), f = test_create_faction(), 1, rc = rc_get_or_create("pirate"), 0, 0, 0);
    fset(rc, p_rc_flags);
    assert(f && *pirate);

    if (p_shiptype) {
        st_boat = st_get_or_create(p_shiptype);
        u_set_ship(*pirate, test_create_ship((*pirate)->region, st_boat));
        arrsetlen(st_boat->coasts, 1);
        st_boat->coasts[0] = vterrain;
    }

    f->locale = get_or_create_locale("de");
    (*pirate)->thisorder = create_order(K_PIRACY, f->locale, "%s", itoa36((*victim)->faction->no));
}

static void test_piracy_cmd(CuTest * tc) {
    faction *f;
    region *r;
    unit *u, *u2;
    terrain_type *t_ocean;
    ship_type *st_boat;

    test_setup();
    setup_piracy();

    t_ocean = get_or_create_terrain("ocean");
    st_boat = st_get_or_create("boat");
    u2 = test_create_unit(test_create_faction(), test_create_region(1, 0, t_ocean));
    assert(u2);
    u_set_ship(u2, test_create_ship(u2->region, st_boat));
    u = test_create_unit(f = test_create_faction(), r = test_create_region(0, 0, t_ocean));
    assert(f && u);
    set_level(u, SK_SAILING, st_boat->sumskill);
    u_set_ship(u, test_create_ship(u->region, st_boat));
    f->locale = get_or_create_locale("de");
    u->thisorder = create_order(K_PIRACY, f->locale, "%s", itoa36(u2->faction->no));

    piracy_cmd(u);
    CuAssertPtrNotNull(tc, u->thisorder);
    CuAssertTrue(tc, u->region != r);
    CuAssertPtrEquals(tc, u2->region, u->region);
    CuAssertPtrEquals(tc, u2->region, u->ship->region);
    CuAssertPtrNotNullMsg(tc, "successful PIRACY sets attribute", r->attribs); /* FIXME: this is testing implementation, not interface */
    CuAssertPtrNotNullMsg(tc, "successful PIRACY message", test_find_messagetype(f->msgs, "piratesawvictim"));
    CuAssertPtrNotNullMsg(tc, "successful PIRACY movement", test_find_messagetype(f->msgs, "shipsail"));

    test_teardown();
}

static void test_piracy_cmd_errors(CuTest * tc) {
    race *r;
    faction *f;
    unit *u, *u2;
    ship_type *st_boat;

    test_setup();
    setup_piracy();

    st_boat = st_get_or_create("boat");
    r = test_create_race("pirates");
    u = test_create_unit(f = test_create_faction_ex(r, NULL), test_create_region(0, 0, get_or_create_terrain("ocean")));
    u->thisorder = create_order(K_PIRACY, f->locale, "");
    assert(u && u->thisorder);

    piracy_cmd(u);
    CuAssertPtrNotNullMsg(tc, "must be on a ship for PIRACY", test_find_messagetype(f->msgs, "error144"));

    test_clear_messages(f);
    fset(r, RCF_SWIM);
    piracy_cmd(u);
    CuAssertPtrEquals_Msg(tc, "swimmers are pirates", 0, test_find_messagetype(f->msgs, "error144"));
    CuAssertPtrEquals_Msg(tc, "swimmers are pirates", 0, test_find_messagetype(f->msgs, "error146"));
    freset(r, RCF_SWIM);
    fset(r, RCF_FLY);
    CuAssertPtrEquals_Msg(tc, "flyers are pirates", 0, test_find_messagetype(f->msgs, "error144"));
    freset(r, RCF_FLY);
    test_clear_messages(f);

    u_set_ship(u, test_create_ship(u->region, st_boat));

    u2 = test_create_unit(u->faction, u->region);
    u2->thisorder = create_order(K_PIRACY, f->locale, "");
    u_set_ship(u2, u->ship);

    test_clear_messages(f);
    piracy_cmd(u2);
    CuAssertPtrNotNullMsg(tc, "must be owner for PIRACY", test_find_messagetype(f->msgs, "error146"));

    test_clear_messages(f);
    piracy_cmd(u);
    CuAssertPtrNotNullMsg(tc, "must specify target for PIRACY", test_find_messagetype(f->msgs, "piratenovictim"));
    CuAssertPtrNotNull(tc, u->thisorder);

    test_teardown();
}

static void test_piracy_cmd_walking(CuTest * tc) {
    unit *pirate, *victim;
    region *r;

    test_setup();
    setup_pirate(&pirate, 0, 0, NULL, &victim, SWIM_INTO | SEA_REGION, "boat");
    /* fset(rc, RCF_SWIM); */
    r = pirate->region;

    piracy_cmd(pirate);
    CuAssertPtrNotNull(tc, pirate->thisorder);
    CuAssertTrue(tc, pirate->region == r);
    CuAssertPtrNotNullMsg(tc, "successful PIRACY message", test_find_messagetype(pirate->faction->msgs, "error144"));

    test_teardown();
}

static void test_piracy_cmd_land_to_land(CuTest * tc) {
    unit *u;
    region *r;
    faction *f;
    int target;
    const terrain_type *t_plain;
    const ship_type *stype;

    test_setup();
    setup_piracy();
    t_plain = get_or_create_terrain("plain");
    stype = test_create_shiptype("boat");

    /* create a target: */
    r = test_create_region(0, 0, t_plain);
    f = test_create_faction();
    u = test_create_unit(f, r);
    u->ship = test_create_ship(r, stype);
    target = f->no;

    /* create a pirate: */
    r = test_create_region(1, 0, t_plain);
    f = test_create_faction();
    u = test_create_unit(f, r);
    u->ship = test_create_ship(r, stype);
    set_level(u, SK_SAILING, u->ship->type->sumskill);
    u->thisorder = create_order(K_PIRACY, f->locale, "%s", itoa36(target));

    piracy_cmd(u);
    CuAssertPtrNotNull(tc, u->thisorder);
    CuAssertPtrEquals(tc, r, u->region);

    test_teardown();
}

static void test_piracy_cmd_swimmer(CuTest * tc) {
    unit *pirate, *victim;
    region *r;

    test_setup();
    setup_pirate(&pirate, 0, RCF_SWIM, NULL, &victim, SWIM_INTO | SEA_REGION, "boat");
    r = pirate->region;

    piracy_cmd(pirate);
    CuAssertPtrNotNull(tc, pirate->thisorder);
    CuAssertTrue(tc, pirate->region != r);
    CuAssertPtrEquals(tc, victim->region, pirate->region);
    CuAssertPtrNotNullMsg(tc, "successful PIRACY message", test_find_messagetype(pirate->faction->msgs, "piratesawvictim"));
    CuAssertPtrNotNullMsg(tc, "successful PIRACY movement", test_find_messagetype(pirate->faction->msgs, "travel"));

    test_teardown();
}

CuSuite *get_piracy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_piracy_cmd_errors);
    SUITE_ADD_TEST(suite, test_piracy_cmd);
    SUITE_ADD_TEST(suite, test_piracy_cmd_walking);
    SUITE_ADD_TEST(suite, test_piracy_cmd_land_to_land);
    SUITE_ADD_TEST(suite, test_piracy_cmd_swimmer);
    return suite;
}
