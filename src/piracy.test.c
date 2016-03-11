#include <platform.h>
#include "piracy.h"

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/race.h>

#include <util/base36.h>
#include <util/language.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>

static void setup_piracy(void) {
    struct locale *lang;
    ship_type *st_boat;

    test_cleanup();
    config_set("rules.ship.storms", "0");
    lang = get_or_create_locale("de");
    locale_setstring(lang, directions[D_EAST], "OSTEN");
    init_directions(lang);
    test_create_terrain("ocean", SAIL_INTO | SEA_REGION);
    st_boat = test_create_shiptype("boat");
    st_boat->cargo = 1000;
}

static void setup_pirate(unit **pirate, int p_r_flags, int p_rc_flags, const char *p_shiptype,
    order **ord, unit **victim, int v_r_flags, const char *v_shiptype) {
    terrain_type *vterrain;
    ship_type *st_boat = NULL;
    race *rc;
    faction *f;

    setup_piracy();
    vterrain = get_or_create_terrain("terrain1");
    fset(vterrain, v_r_flags);
    *victim = test_create_unit(test_create_faction(0), test_create_region(1, 0, vterrain));
    assert(*victim);

    if (v_shiptype) {
        st_boat = st_get_or_create(v_shiptype);
        u_set_ship(*victim, test_create_ship((*victim)->region, st_boat));
        free(st_boat->coasts);
        st_boat->coasts = (struct terrain_type **)calloc(2, sizeof(struct terrain_type *));
        st_boat->coasts[0] = vterrain;
        st_boat->coasts[1] = 0;
    }

    *pirate = create_unit(test_create_region(0, 0, get_or_create_terrain("terrain2")), f = test_create_faction(0), 1, rc = rc_get_or_create("pirate"), 0, 0, 0);
    fset(rc, p_rc_flags);
    assert(f && *pirate);

    if (p_shiptype) {
        st_boat = st_get_or_create(p_shiptype);
        u_set_ship(*pirate, test_create_ship((*pirate)->region, st_boat));
        free(st_boat->coasts);
        st_boat->coasts = (struct terrain_type **)calloc(2, sizeof(struct terrain_type *));
        st_boat->coasts[0] = vterrain;
        st_boat->coasts[1] = 0;
    }

    f->locale = get_or_create_locale("de");
    *ord = create_order(K_PIRACY, f->locale, "%s", itoa36((*victim)->faction->no));
    assert(*ord);

}

static void test_piracy_cmd(CuTest * tc) {
    faction *f;
    region *r;
    unit *u, *u2;
    order *ord;
    terrain_type *t_ocean;
    ship_type *st_boat;

    test_cleanup();

    setup_piracy();
    t_ocean = get_or_create_terrain("ocean");
    st_boat = st_get_or_create("boat");
    u2 = test_create_unit(test_create_faction(0), test_create_region(1, 0, t_ocean));
    u_set_ship(u2, test_create_ship(u2->region, st_boat));
    assert(u2);
    u = test_create_unit(f = test_create_faction(0), r = test_create_region(0, 0, t_ocean));
    set_level(u, SK_SAILING, st_boat->sumskill);
    u_set_ship(u, test_create_ship(u->region, st_boat));
    assert(f && u);
    f->locale = get_or_create_locale("de");
    ord = create_order(K_PIRACY, f->locale, "%s", itoa36(u2->faction->no));
    assert(ord);

    piracy_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertTrue(tc, u->region != r);
    CuAssertPtrEquals(tc, u2->region, u->region);
    CuAssertPtrEquals(tc, u2->region, u->ship->region);
    CuAssertPtrNotNullMsg(tc, "successful PIRACY sets attribute", r->attribs); // FIXME: this is testing implementation, not interface
    CuAssertPtrNotNullMsg(tc, "successful PIRACY message", test_find_messagetype(f->msgs, "piratesawvictim"));
    CuAssertPtrNotNullMsg(tc, "successful PIRACY movement", test_find_messagetype(f->msgs, "shipsail"));
    free_order(ord);

    test_cleanup();
}

static void test_piracy_cmd_errors(CuTest * tc) {
    race *r;
    faction *f;
    unit *u, *u2;
    order *ord;
    ship_type *st_boat;

    test_cleanup();

    setup_piracy();
    st_boat = st_get_or_create("boat");
    r = test_create_race("pirates");
    u = test_create_unit(f = test_create_faction(r), test_create_region(0, 0, get_or_create_terrain("ocean")));
    f->locale = get_or_create_locale("de");
    ord = create_order(K_PIRACY, f->locale, "");
    assert(u && ord);

    piracy_cmd(u, ord);
    CuAssertPtrNotNullMsg(tc, "must be on a ship for PIRACY", test_find_messagetype(f->msgs, "error144"));

    test_clear_messages(f);
    fset(r, RCF_SWIM);
    piracy_cmd(u, ord);
    CuAssertPtrEquals_Msg(tc, "swimmers are pirates", 0, test_find_messagetype(f->msgs, "error144"));
    CuAssertPtrEquals_Msg(tc, "swimmers are pirates", 0, test_find_messagetype(f->msgs, "error146"));
    freset(r, RCF_SWIM);
    fset(r, RCF_FLY);
    CuAssertPtrEquals_Msg(tc, "flyers are pirates", 0, test_find_messagetype(f->msgs, "error144"));
    freset(r, RCF_FLY);
    test_clear_messages(f);

    u_set_ship(u, test_create_ship(u->region, st_boat));

    u2 = test_create_unit(u->faction, u->region);
    u_set_ship(u2, u->ship);

    test_clear_messages(f);
    piracy_cmd(u2, ord);
    CuAssertPtrNotNullMsg(tc, "must be owner for PIRACY", test_find_messagetype(f->msgs, "error146"));

    test_clear_messages(f);
    piracy_cmd(u, ord);
    CuAssertPtrNotNullMsg(tc, "must specify target for PIRACY", test_find_messagetype(f->msgs, "piratenovictim"));
    free_order(ord);

    test_cleanup();
}

static void test_piracy_cmd_walking(CuTest * tc) {
    unit *pirate, *victim;
    order *ord;
    region *r;

    test_cleanup();

    setup_pirate(&pirate, 0, 0, NULL, &ord, &victim, SWIM_INTO | SEA_REGION, "boat");
    /* fset(rc, RCF_SWIM); */
    r = pirate->region;

    piracy_cmd(pirate, ord);
    CuAssertPtrEquals(tc, 0, pirate->thisorder);
    CuAssertTrue(tc, pirate->region == r);
    CuAssertPtrNotNullMsg(tc, "successful PIRACY message", test_find_messagetype(pirate->faction->msgs, "error144"));
    free_order(ord);

    test_cleanup();
}

static void test_piracy_cmd_land_to_land(CuTest * tc) {
    unit *pirate, *victim;
    order *ord;
    region *r;

    test_cleanup();

    setup_pirate(&pirate, 0, 0, "boat", &ord, &victim, SAIL_INTO, "boat");
    set_level(pirate, SK_SAILING, pirate->ship->type->sumskill);
    r = pirate->region;

    piracy_cmd(pirate, ord);
    CuAssertPtrEquals(tc, 0, pirate->thisorder);
    CuAssertTrue(tc, pirate->region == r);
    /* TODO check message
     CuAssertPtrNotNullMsg(tc, "successful PIRACY movement", test_find_messagetype(pirate->faction->msgs, "travel"));
     */
    free_order(ord);

    test_cleanup();
}

static void test_piracy_cmd_swimmer(CuTest * tc) {
    unit *pirate, *victim;
    order *ord;
    region *r;

    test_cleanup();

    setup_pirate(&pirate, 0, RCF_SWIM, NULL, &ord, &victim, SWIM_INTO | SEA_REGION, "boat");
    r = pirate->region;

    piracy_cmd(pirate, ord);
    CuAssertPtrEquals(tc, 0, pirate->thisorder);
    CuAssertTrue(tc, pirate->region != r);
    CuAssertPtrEquals(tc, victim->region, pirate->region);
    CuAssertPtrNotNullMsg(tc, "successful PIRACY message", test_find_messagetype(pirate->faction->msgs, "piratesawvictim"));
    CuAssertPtrNotNullMsg(tc, "successful PIRACY movement", test_find_messagetype(pirate->faction->msgs, "travel"));
    free_order(ord);

    test_cleanup();
}

CuSuite *get_piracy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_piracy_cmd_errors);
    SUITE_ADD_TEST(suite, test_piracy_cmd);
    SUITE_ADD_TEST(suite, test_piracy_cmd_walking);
    DISABLE_TEST(suite, test_piracy_cmd_land_to_land);
    SUITE_ADD_TEST(suite, test_piracy_cmd_swimmer);
    return suite;
}
