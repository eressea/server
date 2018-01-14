#include <platform.h>

#include <kernel/config.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>
#include <kernel/curse.h>

#include <util/attrib.h>

#include <spells/shipcurse.h>
#include <attributes/movement.h>

#include <CuTest.h>
#include <tests.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_register_ship(CuTest * tc)
{
    ship_type *stype;

    test_setup();

    stype = st_get_or_create("herp");
    CuAssertPtrNotNull(tc, stype);
    CuAssertPtrEquals(tc, stype, (void *)st_find("herp"));
    test_teardown();
}

static void test_ship_set_owner(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u1, *u2;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_setup();
    test_create_world();

    human = rc_find("human");
    stype = st_find("boat");
    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    u1 = test_create_unit(f, r);
    u_set_ship(u1, sh);
    CuAssertPtrEquals(tc, u1, ship_owner(sh));

    u2 = test_create_unit(f, r);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u1, ship_owner(sh));
    ship_set_owner(u2);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    test_teardown();
}

static void test_shipowner_goes_to_next_when_empty(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_setup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    u->number = 0;
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    test_teardown();
}

static void test_shipowner_goes_to_other_when_empty(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_setup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u2 = test_create_unit(f, r);
    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    u->number = 0;
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    test_teardown();
}

static void test_shipowner_goes_to_same_faction_when_empty(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2, *u3;
    struct faction *f1, *f2;
    const struct ship_type *stype;
    const struct race *human;

    test_setup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f1 = test_create_faction(human);
    f2 = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u2 = test_create_unit(f2, r);
    u3 = test_create_unit(f1, r);
    u = test_create_unit(f1, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    u_set_ship(u3, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    u->number = 0;
    CuAssertPtrEquals(tc, u3, ship_owner(sh));
    u3->number = 0;
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    test_teardown();
}

static void test_shipowner_goes_to_next_after_leave(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_setup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    leave_ship(u);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    test_teardown();
}

static void test_shipowner_goes_to_other_after_leave(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_setup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u2 = test_create_unit(f, r);
    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    leave_ship(u);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    test_teardown();
}

static void test_shipowner_goes_to_same_faction_after_leave(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2, *u3;
    struct faction *f1, *f2;
    const struct ship_type *stype;
    const struct race *human;

    test_setup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f1 = test_create_faction(human);
    f2 = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u2 = test_create_unit(f2, r);
    u3 = test_create_unit(f1, r);
    u = test_create_unit(f1, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    u_set_ship(u3, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    leave_ship(u);
    CuAssertPtrEquals(tc, u3, ship_owner(sh));
    leave_ship(u3);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    leave_ship(u2);
    CuAssertPtrEquals(tc, 0, ship_owner(sh));
    test_teardown();
}

static void test_shipowner_resets_when_empty(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_setup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    u->number = 0;
    CuAssertPtrEquals(tc, 0, ship_owner(sh));
    u->number = 1;
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    test_teardown();
}

void test_shipowner_goes_to_empty_unit_after_leave(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u1, *u2, *u3;
    struct faction *f1;
    const struct ship_type *stype;
    const struct race *human;

    test_setup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f1 = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f1, r);
    u3 = test_create_unit(f1, r);
    u_set_ship(u1, sh);
    u_set_ship(u2, sh);
    u_set_ship(u3, sh);

    CuAssertPtrEquals(tc, u1, ship_owner(sh));
    u2->number = 0;
    leave_ship(u1);
    CuAssertPtrEquals(tc, u3, ship_owner(sh));
    leave_ship(u3);
    CuAssertPtrEquals(tc, 0, ship_owner(sh));
    u2->number = 1;
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    test_teardown();
}

static void test_stype_defaults(CuTest *tc) {
    ship_type *stype;
    test_setup();
    stype = st_get_or_create("hodor");
    CuAssertPtrNotNull(tc, stype);
    CuAssertStrEquals(tc, "hodor", stype->_name);
    CuAssertPtrEquals(tc, 0, stype->construction);
    CuAssertPtrEquals(tc, 0, stype->coasts);
    CuAssertDblEquals(tc, 0.0, stype->damage, 0.0);
    CuAssertDblEquals(tc, 1.0, stype->storm, 0.0);
    CuAssertDblEquals(tc, 0.0, stype->tac_bonus, 0.0);
    CuAssertIntEquals(tc, 0, stype->cabins);
    CuAssertIntEquals(tc, 0, stype->cargo);
    CuAssertIntEquals(tc, 0, stype->combat);
    CuAssertIntEquals(tc, 0, stype->fishing);
    CuAssertIntEquals(tc, 0, stype->range);
    CuAssertIntEquals(tc, 0, stype->cptskill);
    CuAssertIntEquals(tc, 0, stype->minskill);
    CuAssertIntEquals(tc, 0, stype->sumskill);
    CuAssertIntEquals(tc, 0, stype->at_bonus);
    CuAssertIntEquals(tc, 0, stype->df_bonus);
    CuAssertIntEquals(tc, 0, stype->flags);
    test_teardown();
}

static void test_crew_skill(CuTest *tc) {
    ship *sh;
    region *r;
    struct faction *f;
    int i;

    test_setup();
    test_create_world();
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    assert(r && f);
    sh = test_create_ship(r, st_find("boat"));
    for (i = 0; i != 4; ++i) {
        unit * u = test_create_unit(f, r);
        set_level(u, SK_SAILING, 5);
        u->ship = sh;
    }
    CuAssertIntEquals(tc, 20, crew_skill(sh));
    test_teardown();
}

static ship *setup_ship(void) {
    region *r;
    ship_type *stype;

    config_set("movement.shipspeed.skillbonus", "0");
    r = test_create_ocean(0, 0);
    stype = test_create_shiptype("longboat");
    stype->cptskill = 1;
    stype->sumskill = 10;
    stype->minskill = 1;
    stype->range = 2;
    stype->range_max = 4;
    return test_create_ship(r, stype);
}

static void setup_crew(ship *sh, struct faction *f, unit **cap, unit **crew) {
    if (!f) f = test_create_faction(NULL);
    assert(cap);
    assert(crew);
    *cap = test_create_unit(f, sh->region);
    *crew = test_create_unit(f, sh->region);
    (*cap)->ship = sh;
    (*crew)->ship = sh;
    set_level(*cap, SK_SAILING, sh->type->cptskill);
    set_level(*crew, SK_SAILING, sh->type->sumskill - sh->type->cptskill);
}

static void test_shipspeed_speedy(CuTest *tc) {
    ship_type *stype;
    ship *sh;
    unit *cap, *crw;
    test_setup();
    stype = test_create_shiptype("dragonship");
    stype->range = 5;
    stype->range_max = -1;
    stype->flags |= SFL_SPEEDY;
    cap = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
    crw = test_create_unit(cap->faction, cap->region);
    sh = test_create_ship(cap->region, stype);
    cap->ship = sh;
    crw->ship = sh;
    set_level(cap, SK_SAILING, stype->cptskill);
    set_level(crw, SK_SAILING, stype->sumskill - stype->cptskill);
    CuAssertPtrEquals(tc, cap, ship_owner(sh));
    CuAssertIntEquals(tc, 5, shipspeed(sh, cap));

    set_level(cap, SK_SAILING, stype->cptskill * 3 - 1);
    CuAssertIntEquals(tc, 5, shipspeed(sh, cap));
    set_level(cap, SK_SAILING, stype->cptskill * 3);
    CuAssertIntEquals(tc, 6, shipspeed(sh, cap));

    set_level(cap, SK_SAILING, stype->cptskill * 3 * 3 - 1);
    CuAssertIntEquals(tc, 6, shipspeed(sh, cap));
    set_level(cap, SK_SAILING, stype->cptskill * 3 * 3);
    CuAssertIntEquals(tc, 7, shipspeed(sh, cap));

    test_teardown();
}

static void test_shipspeed_stormwind(CuTest *tc) {
    ship *sh;
    unit *cap, *crew;

    test_setup();
    sh = setup_ship();
    setup_crew(sh, 0, &cap, &crew);
    register_shipcurse();
    assert(sh && cap && crew);

    create_curse(0, &sh->attribs, &ct_stormwind, 1, 1, 1, 0);
    CuAssertPtrNotNull(tc, sh->attribs);
    CuAssertIntEquals_Msg(tc, "stormwind doubles ship range", sh->type->range * 2, shipspeed(sh, cap));
    a_age(&sh->attribs, sh);
    CuAssertPtrEquals(tc, NULL, sh->attribs);
    test_teardown();
}

static void test_shipspeed_nodrift(CuTest *tc) {
    ship *sh;
    unit *cap, *crew;

    test_setup();
    sh = setup_ship();
    setup_crew(sh, 0, &cap, &crew);
    register_shipcurse();
    assert(sh && cap && crew);

    create_curse(0, &sh->attribs, &ct_nodrift, 1, 1, 1, 0);
    CuAssertIntEquals_Msg(tc, "nodrift adds +1 to range", sh->type->range + 1, shipspeed(sh, cap));
    test_teardown();
}

static void test_shipspeed_shipspeedup(CuTest *tc) {
    ship *sh;
    unit *cap, *crew;

    test_setup();
    sh = setup_ship();
    setup_crew(sh, 0, &cap, &crew);
    register_shipcurse();
    assert(sh && cap && crew);

    create_curse(0, &sh->attribs, &ct_shipspeedup, 1, 1, 3, 0);
    CuAssertIntEquals_Msg(tc, "shipspeedup adds effect to range", sh->type->range + 3, shipspeed(sh, cap));
    test_teardown();
}

static void test_shipspeed_at_speedup(CuTest *tc) {
    ship *sh;
    unit *cap, *crew;
    attrib *a;

    test_setup();
    sh = setup_ship();
    setup_crew(sh, 0, &cap, &crew);
    assert(sh && cap && crew);

    a = a_new(&at_speedup);
    a->data.i = 3;
    a_add(&sh->attribs, a);
    CuAssertIntEquals_Msg(tc, "at_speedup adds value to range", sh->type->range + 3, shipspeed(sh, cap));
    test_teardown();
}

static void test_shipspeed_race_bonus(CuTest *tc) {
    ship *sh;
    unit *cap, *crew;
    race *rc;

    test_setup();
    sh = setup_ship();
    setup_crew(sh, 0, &cap, &crew);
    assert(sh && cap && crew);

    rc = rc_get_or_create(cap->_race->_name);
    rc->flags |= RCF_SHIPSPEED;
    CuAssertIntEquals_Msg(tc, "captain with RCF_SHIPSPEED adds +1 to range", sh->type->range + 1, shipspeed(sh, cap));
    test_teardown();
}

static void test_shipspeed_damage(CuTest *tc) {
    ship *sh;
    unit *cap, *crew;

    test_setup();
    sh = setup_ship();
    setup_crew(sh, 0, &cap, &crew);
    assert(sh && cap && crew);

    sh->damage = 1;
    CuAssertIntEquals_Msg(tc, "minimally damaged ships lose no range", 2, shipspeed(sh, cap));
    sh->damage = sh->size * DAMAGE_SCALE / 2;
    CuAssertIntEquals_Msg(tc, "damaged ships lose range", 1, shipspeed(sh, cap));
    sh->damage = sh->size * DAMAGE_SCALE;
    CuAssertIntEquals_Msg(tc, "fully damaged ships have no range", 0, shipspeed(sh, cap));
    test_teardown();
}

static void test_shipspeed(CuTest *tc) {
    ship *sh;
    const ship_type *stype;
    unit *cap, *crew;

    test_setup();
    sh = setup_ship();
    stype = sh->type;

    CuAssertIntEquals_Msg(tc, "ship without a captain cannot move", 0, shipspeed(sh, NULL));

    setup_crew(sh, 0, &cap, &crew);

    CuAssertPtrEquals(tc, cap, ship_owner(sh));
    CuAssertIntEquals_Msg(tc, "ship with fully skilled crew can sail at max speed", 2, shipspeed(sh, cap));
    CuAssertIntEquals_Msg(tc, "shipspeed without a hint defaults to captain", 2, shipspeed(sh, NULL));

    set_level(cap, SK_SAILING, stype->cptskill + 5);
    set_level(crew, SK_SAILING, (stype->sumskill - stype->cptskill) * 10);
    CuAssertIntEquals_Msg(tc, "higher skills should not affect top speed", 2, shipspeed(sh, cap));
    set_level(cap, SK_SAILING, stype->cptskill);
    set_level(crew, SK_SAILING, stype->sumskill - stype->cptskill);

    CuAssertIntEquals(tc, 2, shipspeed(sh, cap));

    set_level(crew, SK_SAILING, (stype->sumskill - stype->cptskill) * 11);
    set_level(cap, SK_SAILING, stype->cptskill + 10);
    CuAssertIntEquals_Msg(tc, "regular skills should not exceed sh.range", 2, shipspeed(sh, cap));
    test_teardown();
}

static void test_shipspeed_max_range(CuTest *tc) {
    ship *sh;
    ship_type *stype;
    region *r;
    struct faction *f;
    unit *cap, *crew;

    test_setup();
    sh = setup_ship();
    setup_crew(sh, 0, &cap, &crew);
    config_set("movement.shipspeed.skillbonus", "5");
    r = sh->region;
    f = test_create_faction(NULL);
    assert(r && f);
    stype = st_get_or_create(sh->type->_name);

    set_level(cap, SK_SAILING, stype->cptskill + 4);
    set_level(crew, SK_SAILING, (stype->sumskill - stype->cptskill) * 4);
    CuAssertIntEquals_Msg(tc, "skill bonus requires at least movement.shipspeed.skillbonus points", 2, shipspeed(sh, cap));

    set_level(cap, SK_SAILING, stype->cptskill + 5);
    set_level(crew, SK_SAILING, (stype->sumskill - stype->cptskill) * 5);
    CuAssertIntEquals_Msg(tc, "skill bonus from movement.shipspeed.skillbonus", 3, shipspeed(sh, cap));

    set_level(cap, SK_SAILING, stype->cptskill + 15);
    set_level(crew, SK_SAILING, (stype->sumskill - stype->cptskill) * 15);
    CuAssertIntEquals_Msg(tc, "skill-bonus cannot exceed max_range", 4, shipspeed(sh, cap));
    test_teardown();
}

CuSuite *get_ship_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_register_ship);
    SUITE_ADD_TEST(suite, test_stype_defaults);
    SUITE_ADD_TEST(suite, test_ship_set_owner);
    SUITE_ADD_TEST(suite, test_shipowner_resets_when_empty);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_next_when_empty);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_other_when_empty);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_same_faction_when_empty);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_next_after_leave);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_other_after_leave);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_same_faction_after_leave);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_empty_unit_after_leave);
    SUITE_ADD_TEST(suite, test_crew_skill);
    SUITE_ADD_TEST(suite, test_shipspeed);
    SUITE_ADD_TEST(suite, test_shipspeed_speedy);
    SUITE_ADD_TEST(suite, test_shipspeed_stormwind);
    SUITE_ADD_TEST(suite, test_shipspeed_nodrift);
    SUITE_ADD_TEST(suite, test_shipspeed_shipspeedup);
    SUITE_ADD_TEST(suite, test_shipspeed_at_speedup);
    SUITE_ADD_TEST(suite, test_shipspeed_race_bonus);
    SUITE_ADD_TEST(suite, test_shipspeed_damage);
    SUITE_ADD_TEST(suite, test_shipspeed_max_range);
    return suite;
}
