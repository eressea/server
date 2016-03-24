#include <platform.h>
#include "move.h"

#include <kernel/build.h>
#include <kernel/building.h>
#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/attrib.h>
#include <util/base36.h>
#include <util/language.h>

#include <CuTest.h>
#include <tests.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "laws.h"

static void setup_fleet(void) {
    ship_type* ftype;
    struct locale* lang;

    lang = get_or_create_locale("de");
    locale_setstring(lang, "fleet", "Flotte");
    locale_setstring(lang, parameters[P_REMOVE], "ENTFERNE");
    locale_setstring(lang, directions[D_WEST], "WESTEN");
    locale_setstring(lang, directions[D_NORTHEAST], "NORDOSTEN");
    locale_setstring(lang, directions[D_EAST], "OSTEN");
    locale_setstring(lang, directions[D_SOUTHEAST], "SUEDOSTEN");
    locale_setstring(lang, directions[D_SOUTHWEST], "SUEDWESTEN");
    locale_setstring(lang, directions[D_NORTHWEST], "NORDWESTEN");
    init_directions(lang);
    test_create_world();
    init_locales();

    ftype = st_get_or_create("fleet");
    ftype->cptskill = 6;
    test_create_shiptype2("boat", 1, 1, 1, 2, 2000, 0, 5, 1, 1, 1);

    config_set("rules.ship.storms", "0");
}

typedef struct fleet_fixture {
    region *r, *r2;
    ship *sh1, *sh2, *sh3;
    ship *fleet;
    unit *u11, *u21;
    faction *f1, *f2;
    const ship_type *stype1;
} fleet_fixture;

static void init_fixture(fleet_fixture *ffix) {
    race *rc = test_create_race("human");
    fset(rc, RCF_CANSAIL | RCF_WALK);

    ffix->f1 = test_create_faction(rc);
    ffix->f2 = test_create_faction(rc);
    ffix->r = findregion(0, 0);
    ffix->r2 = findregion(-1, 0);

    ffix->stype1 = st_find("boat");

    ffix->sh1 = test_create_ship(ffix->r, ffix->stype1);
    ffix->sh2 = test_create_ship(ffix->r, ffix->stype1);
    ffix->sh3 = test_create_ship(ffix->r, ffix->stype1);
    ffix->u11 = test_create_unit(ffix->f1, ffix->r);

    ffix->u21 = test_create_unit(ffix->f2, ffix->r);
    ffix->fleet = NULL;
}

static void init_fleet(fleet_fixture *ffix) {
    init_fixture(ffix);

    ffix->fleet = fleet_add_ship(ffix->sh1, NULL, ffix->u11);
    fleet_add_ship(ffix->sh2, ffix->fleet, ffix->u21);
}

static ship *find_fleet(CuTest *tc, region *r) {
    ship **shp;
    ship *found = NULL;

    for (shp = &r->ships; *shp;) {
        ship *sh = *shp;
        if (ship_isfleet(sh)) {
            CuAssertPtrEquals_Msg(tc, "more than one fleet", NULL, found);
            found = sh;
            for (shp = &sh->next; *shp; shp = &sh->next) {
                sh = *shp;
                CuAssertPtrEquals_Msg(tc, "not all ships in fleet", found, (ship * ) sh->fleet);
            }
        }
        if (*shp == sh)
            shp = &sh->next;
    }
    return found;
}

static void assert_no_fleet(CuTest *tc, region *r, ship *sh, unit *cpt) {
    ship *fleet = find_fleet(tc, r);
    CuAssertPtrEquals_Msg(tc, "fleet not removed", 0, fleet);
    CuAssertPtrEquals_Msg(tc, "ship still in fleet", 0, sh->fleet);
    CuAssertPtrEquals_Msg(tc, "command not transferred", cpt, ship_owner(sh));
}

static ship *assert_fleet(CuTest *tc, region *r, ship *sh, unit *cpt) {
    ship *found = NULL;

    found = find_fleet(tc, r);
    CuAssertPtrNotNull(tc, found);

    CuAssertPtrEquals_Msg(tc, "ship not added to fleet", (void *) found, (ship *) sh->fleet);
    CuAssertPtrEquals_Msg(tc, "captain not in fleet", found, (ship *) cpt->ship);
    CuAssertTrue(tc, ship_isfleet(found));
    CuAssertPtrEquals_Msg(tc, "captain not fleet owner", cpt, ship_owner(found));
    CuAssertPtrEquals_Msg(tc, "captain not ship owner", cpt, ship_owner(sh));
    return found;
}

static void test_fleet_create(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    ship *fleet;
    unit *u3;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    u_set_ship(ffix.u11, ffix.sh1);
    u_set_ship(ffix.u21, ffix.sh1);
    u3 = test_create_unit(ffix.f1, ffix.r);

    ord = create_order(K_FLEET, ffix.f1->locale, NULL);
    unit_addorder(ffix.u11, ord);
    fleet_cmd(ffix.r);

    fleet = assert_fleet(tc, ffix.r, ffix.sh1, ffix.u11);
    CuAssertPtrEquals_Msg(tc, "units not moved", fleet, ffix.u21->ship);
    CuAssertPtrEquals_Msg(tc, "units not moved", 0, u3->ship);

    test_cleanup();
}

static void test_fleet_nonsail(CuTest *tc) {
    race *rc;
    faction *f1;
    region *r;
    ship *sh;
    unit *u1;
    order *ord;
    struct message *msg;

    test_cleanup();
    setup_fleet();
    rc = test_create_race("human");
    f1 = test_create_faction(rc);
    r = findregion(0, 0);
    sh = test_create_ship(r, st_find("boat"));
    u1 = test_create_unit(f1, r);

    ord = create_order(K_FLEET, f1->locale, "%s", itoa36(sh->no));
    unit_addorder(u1, ord);

    fleet_cmd(r);
    msg = test_get_last_message(f1->msgs);
    CuAssertStrEquals(tc, "error233", test_get_messagetype(msg));
    CuAssertPtrEquals(tc, 0, u1->ship);

    test_cleanup();
}

static void test_fleet_create_param(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh1->no));
    unit_addorder(ffix.u11, ord);
    fleet_cmd(ffix.r);

    assert_fleet(tc, ffix.r, ffix.sh1, ffix.u11);

    test_cleanup();
}

static void test_fleet_missing_param(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh1->no+1));
    unit_addorder(ffix.u11, ord);
    fleet_cmd(ffix.r);

    msg = test_get_last_message(ffix.u11->faction->msgs);
    CuAssertStrEquals(tc, "fleet_ship_invalid", test_get_messagetype(msg));
    CuAssertPtrEquals(tc, 0,  (ship *) ffix.sh1->fleet);

    test_cleanup();
}

static void test_fleet_create_external(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
//    ship **shp;
    ship *fleet = NULL;
    unit *u12;
    struct message* msg;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    u_set_ship(ffix.u21, ffix.sh1);

    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh1->no));
    unit_addorder(ffix.u11, ord);
    fleet_cmd(ffix.r);

    msg = test_get_last_message(ffix.u11->faction->msgs);
    CuAssertStrEquals(tc, "feedback_no_contact", test_get_messagetype(msg));

    usetcontact(ffix.u21, ffix.u11);

    fleet_cmd(ffix.r);

    fleet = assert_fleet(tc, ffix.r, ffix.sh1, ffix.u11);

    /* ships in fleets cannot be added */
    test_clear_messages(ffix.f1);

    u12 =  test_create_unit(ffix.f1, ffix.r);
    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh1->no));
    unit_addorder(u12, ord);
    usetcontact(ffix.u11, u12);
    fleet_cmd(ffix.r);

    msg = test_get_last_message(u12->faction->msgs);
    CuAssertStrEquals(tc, "fleet_ship_invalid", test_get_messagetype(msg));
    CuAssertPtrEquals(tc, fleet,  (ship *) ffix.sh1->fleet);

    test_cleanup();
}

static void test_fleet_create_no_add_fleet(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;
    struct ship *fleet;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    fleet = test_create_ship(ffix.r, st_find("fleet"));

    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(fleet->no));
    unit_addorder(ffix.u11, ord);

    fleet_cmd(ffix.r);

    msg = test_get_last_message(ffix.u11->faction->msgs);
    CuAssertStrEquals(tc, "fleet_ship_invalid", test_get_messagetype(msg));
    CuAssertPtrEquals(tc, 0,  (ship *) fleet->fleet);

    test_cleanup();
}

static void test_fleet_add(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;
    struct ship *fleet;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    u_set_ship(ffix.u21, ffix.sh2);

    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh2->no));
    unit_addorder(ffix.u11, ord);
    usetcontact(ffix.u21, ffix.u11);

    fleet_cmd(ffix.r);

    msg = test_get_last_message(ffix.u11->faction->msgs);
    CuAssertPtrEquals(tc, 0, msg);
    fleet = assert_fleet(tc, ffix.r, ffix.sh2, ffix.u11);
    CuAssertPtrEquals_Msg(tc, "units not moved", fleet, ffix.u21->ship);

    test_cleanup();
}

static void test_fleet_add2(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;
    struct ship *fleet;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    u_set_ship(ffix.u11, ffix.sh1);
    u_set_ship(ffix.u21, ffix.sh2);

    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh1->no));
    unit_addorder(ffix.u11, ord);
    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh2->no));
    unit_addorder(ffix.u11, ord);
    usetcontact(ffix.u21, ffix.u11);

    fleet_cmd(ffix.r);

    msg = test_get_last_message(ffix.u11->faction->msgs);
    CuAssertPtrEquals(tc, 0, msg);
    fleet = assert_fleet(tc, ffix.r, ffix.sh1, ffix.u11);
    CuAssertPtrEquals(tc, fleet, assert_fleet(tc, ffix.r, ffix.sh2, ffix.u11));
    CuAssertPtrEquals_Msg(tc, "units not moved", fleet, ffix.u21->ship);

    test_cleanup();
}

static void test_fleet_remove1(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh1->no));
    unit_addorder(ffix.u11, ord);
    ord = create_order(K_FLEET, ffix.f1->locale, "ENTFERNE %s %s", itoa36(ffix.sh1->no), itoa36(ffix.u11->no));
    unit_addorder(ffix.u11, ord);

    fleet_cmd(ffix.r);

    msg = test_get_last_message(ffix.u11->faction->msgs);
    CuAssertPtrEquals(tc, 0, msg);

    assert_no_fleet(tc, ffix.r, ffix.sh1, ffix.u11);

    test_cleanup();
}

static void test_fleet_remove2(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    u_set_ship(ffix.u21, ffix.sh2);
    usetcontact(ffix.u21, ffix.u11);

    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh1->no));
    unit_addorder(ffix.u11, ord);
    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh2->no));
    unit_addorder(ffix.u11, ord);
    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh3->no));
    unit_addorder(ffix.u11, ord);
    ord = create_order(K_FLEET, ffix.f1->locale, "ENTFERNE %s", itoa36(ffix.sh2->no));
    unit_addorder(ffix.u11, ord);
    ord = create_order(K_FLEET, ffix.f1->locale, "ENTFERNE %s %s", itoa36(ffix.sh3->no), itoa36(ffix.u21->no));
    unit_addorder(ffix.u11, ord);

    fleet_cmd(ffix.r);

    msg = test_get_last_message(ffix.u11->faction->msgs);
    CuAssertPtrEquals(tc, 0, msg);

    assert_fleet(tc, ffix.r, ffix.sh1, ffix.u11);
    CuAssertPtrEquals_Msg(tc, "ship still in fleet", 0, ffix.sh2->fleet);
    CuAssertPtrEquals_Msg(tc, "ship still in fleet", 0, ffix.sh3->fleet);
    CuAssertPtrEquals_Msg(tc, "ship not empty", 0, ship_owner(ffix.sh2));
    CuAssertPtrEquals_Msg(tc, "command not transferred", ffix.u21, ship_owner(ffix.sh3));

    test_cleanup();
}

static void test_fleet_remove_wrong_fleet(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh1->no));
    unit_addorder(ffix.u11, ord);

    ord = create_order(K_FLEET, ffix.f2->locale, "%s", itoa36(ffix.sh2->no));
    unit_addorder(ffix.u21, ord);

    ord = create_order(K_FLEET, ffix.f1->locale, "ENTFERNE %s %s", itoa36(ffix.sh1->no), itoa36(ffix.u11->no));
    unit_addorder(ffix.u21, ord);

    fleet_cmd(ffix.r);

    msg = test_get_last_message(ffix.u11->faction->msgs);
    CuAssertPtrEquals(tc, 0, msg);
    msg = test_get_last_message(ffix.u21->faction->msgs);
    CuAssertStrEquals(tc, "ship_nofleet", test_get_messagetype(msg));

    CuAssertPtrEquals(tc, ffix.u11, ship_owner(ffix.sh1));
    CuAssertPtrEquals(tc, ffix.u21, ship_owner(ffix.sh2));

    CuAssertPtrNotNull(tc, ffix.sh1->fleet);
    CuAssertPtrNotNull(tc, ffix.sh2->fleet);

    test_cleanup();
}

static void test_fleet_enter(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;
    unit *v1, *v2, *v3;

    test_cleanup();
    setup_fleet();
    init_fleet(&ffix);

    v1 = test_create_unit(ffix.f1, ffix.r);
    v2 = test_create_unit(ffix.f1, ffix.r);
    v3 = test_create_unit(ffix.f2, ffix.r);
    ord = create_order(K_ENTER, ffix.f1->locale, "SCHIFF %s", itoa36(ffix.sh1->no));
    unit_addorder(v1, ord);
    ord = create_order(K_ENTER, ffix.f1->locale, "SCHIFF %s", itoa36(ffix.fleet->no));
    unit_addorder(v2, ord);
    ord = create_order(K_ENTER, ffix.f2->locale, "SCHIFF %s", itoa36(ffix.sh1->no));
    unit_addorder(v3, ord);

    do_enter(ffix.r, 1);

    msg = test_get_last_message(v1->faction->msgs);
    CuAssertPtrEquals(tc, 0, msg);

    CuAssertPtrEquals(tc, ffix.fleet, v1->ship);
    CuAssertPtrEquals(tc, ffix.fleet, v2->ship);

    msg = test_get_last_message(v3->faction->msgs);
    CuAssertStrEquals(tc, "error34", test_get_messagetype(msg));
    CuAssertPtrEquals(tc, 0, v3->ship);

    test_cleanup();
}

static void test_fleet_remove_invalid(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    ord = create_order(K_FLEET, ffix.f1->locale, "ENTFERNE %s %s", itoa36(ffix.sh1->no), itoa36(ffix.u11->no));
    unit_addorder(ffix.u11, ord);

    fleet_cmd(ffix.r);

    msg = test_get_last_message(ffix.u11->faction->msgs);
    CuAssertStrEquals(tc, "fleet_only_captain", test_get_messagetype(msg));

    test_cleanup();
}

static void test_fleet_remove_nonsail(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;
    faction *f3;
    unit *u31;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    f3 = test_create_faction(test_create_race("blob"));
    CuAssertIntEquals(tc, 0, f3->flags);
    u31 = test_create_unit(f3, ffix.r);
    usetcontact(u31, ffix.u11);

    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh1->no));
    unit_addorder(ffix.u11, ord);
    ord = create_order(K_FLEET, ffix.f1->locale, "ENTFERNE %s %s", itoa36(ffix.sh1->no), itoa36(u31->no));
    unit_addorder(ffix.u11, ord);

    fleet_cmd(ffix.r);

    assert_no_fleet(tc, ffix.r, ffix.sh1, NULL);

    msg = test_get_last_message(ffix.u11->faction->msgs);
    CuAssertStrEquals(tc, "error233", test_get_messagetype(msg));
    msg = test_get_last_message(f3->msgs);
    CuAssertStrEquals(tc, "error233", test_get_messagetype(msg));

    test_cleanup();
}


static void test_fleet_stats(CuTest *tc) {
    ship_type *stype1, *stype2, *stype3;
    region *r;
    ship *sh1, *sh2, *sh3, *fleet;
    unit *cpt, *crew1, *crew2;
    order *move_ord;
    struct message *msg;

    test_cleanup();
    setup_fleet();

    r = test_create_region(0, 0, NULL);
    cpt = test_create_unit(test_create_faction(NULL), r);
    crew1 = test_create_unit(test_create_faction(NULL), r);
    crew2 = test_create_unit(test_create_faction(NULL), r);

    stype1 = test_create_shiptype2("boat", 1, 1, 1, 2, 1000, 2000, 5, 1, 1, 1);
    stype2 = test_create_shiptype2("boat2", 7, 14, 1, 2, 1000, 2500, 5, 1, 1, 1);
    stype3 = test_create_shiptype2("boat3", 8, 10, 2, 3, 2000, 1500, 10, 2, 1, 2);

    sh1 = test_create_ship(r, stype1);
    sh2 = test_create_ship(r, stype2);
    sh3 = test_create_ship(r, stype3);

    CuAssertIntEquals(tc, 1, ship_type_cpt_skill(sh1));
    CuAssertIntEquals(tc, 7, ship_type_cpt_skill(sh2));
    CuAssertIntEquals(tc, 14, ship_type_crew_skill(sh2));
    CuAssertIntEquals(tc, 1000, ship_capacity(sh1));

    set_level(cpt, SK_SAILING, 1);
    set_level(crew1, SK_SAILING, 2);
    set_level(crew2, SK_SAILING, 6);
    u_set_ship(cpt, sh1);
    u_set_ship(crew1, sh1);

    CuAssertIntEquals(tc, 3, ship_crew_skill(sh1));
    CuAssertTrue(tc, enoughsailors(sh1));

    u_set_ship(crew2, sh2);
    CuAssertTrue(tc, !enoughsailors(sh2));

    move_ord = create_order(K_MOVE, cpt->faction->locale, "");
    CuAssertTrue(tc, ship_ready(r, cpt, move_ord));
    CuAssertPtrEquals(tc, 0, test_get_last_message(cpt->faction->msgs));
    test_clear_messages(cpt->faction);

    fleet = fleet_add_ship(sh1, NULL, cpt);
    CuAssertIntEquals(tc, 6, ship_type_cpt_skill(fleet));
    CuAssertIntEquals(tc, 1, ship_type_crew_skill(fleet));
    CuAssertIntEquals(tc, 1, fleet->size);
    CuAssertTrue(tc, enoughsailors(fleet));

    set_level(cpt, SK_SAILING, 8);
    fleet = fleet_add_ship(sh2, NULL, cpt);
    CuAssertIntEquals(tc, 7, ship_type_cpt_skill(fleet));
    CuAssertIntEquals(tc, 14, ship_type_crew_skill(fleet));
    CuAssertIntEquals(tc, 14, ship_crew_skill(fleet));
    CuAssertIntEquals(tc, 1, fleet->size);
    CuAssertTrue(tc, enoughsailors(fleet));
    CuAssertIntEquals(tc, 1000, ship_capacity(fleet));
    CuAssertIntEquals(tc, 2500, ship_cabins(fleet));

    CuAssertTrue(tc, ship_ready(r, cpt, move_ord));
    CuAssertPtrEquals(tc, 0, test_get_last_message(cpt->faction->msgs));
    test_clear_messages(cpt->faction);

    crew1->ship = NULL;
    u_set_ship(crew1, sh3);

    fleet = fleet_add_ship(sh3, fleet, cpt);
    CuAssertIntEquals(tc, 8, ship_type_cpt_skill(fleet));
    CuAssertIntEquals(tc, 24, ship_type_crew_skill(fleet));
    CuAssertIntEquals(tc, 16, ship_crew_skill(fleet));
    CuAssertIntEquals(tc, 2, fleet->size);
    CuAssertTrue(tc, !enoughsailors(fleet));
    CuAssertIntEquals(tc, 3000, ship_capacity(fleet));
    CuAssertIntEquals(tc, 4000, ship_cabins(fleet));

    CuAssertTrue(tc, !ship_ready(r, cpt, move_ord));
    msg = test_get_last_message(cpt->faction->msgs);
    CuAssertStrEquals(tc, "error_captain_fleet_size", test_get_messagetype(msg));
    test_clear_messages(cpt->faction);

    fleet_remove_ship(sh2, cpt);
    CuAssertIntEquals(tc, 1, fleet->size);
    CuAssertPtrEquals(tc, 0, sh2->fleet);

    test_cleanup();
}

static void test_fleet_complete(CuTest *tc) {
    ship_type *stype1, *stype2, *stype3;
    region *r;
    ship *sh1, *sh2, *sh3, *fleet;
    unit *cpt;

    test_cleanup();
    setup_fleet();

    r = test_create_region(0, 0, NULL);
    cpt = test_create_unit(test_create_faction(NULL), r);

    stype1 = test_create_shiptype2("boat", 1, 1, 1, 2, 1000, 2000, 5, 1, 1, 1);
    stype1->construction = NULL;
    stype2 = test_create_shiptype2("boat2", 7, 14, 1, 2, 1000, 2000, 5, 1, 1, 1);
    stype3 = test_create_shiptype2("boat3", 8, 10, 2, 3, 2000, 1000, 10, 2, 1, 2);

    sh1 = test_create_ship(r, stype1);
    sh2 = test_create_ship(r, stype2);
    sh2->size = stype2->construction->maxsize - 1;
    sh3 = test_create_ship(r, stype3);

    CuAssertTrue(tc, ship_iscomplete(sh1));
    CuAssertTrue(tc, !ship_iscomplete(sh2));
    CuAssertTrue(tc, ship_iscomplete(sh3));

    fleet = fleet_add_ship(sh1, NULL, cpt);
    CuAssertTrue(tc, ship_iscomplete(fleet));

    fleet = fleet_add_ship(sh3, fleet, cpt);
    CuAssertTrue(tc, ship_iscomplete(fleet));

    fleet = fleet_add_ship(sh2, NULL, cpt);
    CuAssertTrue(tc, !ship_iscomplete(fleet));

    test_cleanup();
}

static ship *add_ship(ship **fleet, unit *cpt, int speed) {
    char s[255];
    ship_type *stype;
    ship *sh;
    region *r;

    if (*fleet)
        r = (*fleet)->region;
    else
        r = findregion(0,0);

    sprintf(s, "ship%d", *fleet?(*fleet)->size+1:1);
    stype = test_create_shiptype2(s, 1, 1, 1, speed, 1000, 1000, 10, 1, 1, 1);
    sh = test_create_ship(r, stype);
    *fleet = fleet_add_ship(sh, *fleet, cpt);

    return sh;
}

static void test_fleet_speed(CuTest *tc) {
    fleet_fixture ffix;
    ship *sh1, *sh2;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    sh1 = add_ship(&ffix.fleet, ffix.u11, 1);
    sh2 = add_ship(&ffix.fleet, NULL, 2);

    CuAssertIntEquals(tc, 1, ship_speed(ffix.fleet, ffix.u11));

    sh1->size = 1;
    CuAssertIntEquals(tc, 0, ship_speed(ffix.fleet, ffix.u11));
    sh1->size = sh1->type->construction->maxsize;

    config_set("movement.shipspeed.skillbonus", "1");
    set_level(ffix.u11, SK_SAILING, 3);
    CuAssertIntEquals_Msg(tc, "fleet should have minimum speed of members", 3, ship_speed(ffix.fleet, ffix.u11));

    ((ship_type *) sh1->type)->range_max = 2;
    CuAssertIntEquals_Msg(tc, "fleet speed should be bounded by max range for each ship", 2, ship_speed(ffix.fleet, ffix.u11));

    ((ship_type *) sh1->type)->range_max = 10;
    ((ship_type *) sh2->type)->range_max = 10;
    set_level(ffix.u11, SK_SAILING, 4);
    CuAssertIntEquals_Msg(tc, "ship1: 3x1 = speed 3, ship2: 1x1 = speed 2", 2, ship_speed(ffix.fleet, ffix.u11));

    set_level(ffix.u11, SK_SAILING, 5);
    CuAssertIntEquals_Msg(tc, "ship1: 3x1 = speed 3, ship2: 2x1 = speed 3", 3, ship_speed(ffix.fleet, ffix.u11));
    set_level(ffix.u11, SK_SAILING, 6);
    CuAssertIntEquals(tc, 3, ship_speed(ffix.fleet, ffix.u11));
    set_level(ffix.u11, SK_SAILING, 5);

    damage_ship(sh1, .33);
    CuAssertIntEquals_Msg(tc, "not enough damage to slow ship down", 3, ship_speed(ffix.fleet, ffix.u11));
    damage_ship(sh1, .004);
    CuAssertIntEquals_Msg(tc, "just enough to slow down", 2, ship_speed(ffix.fleet, ffix.u11));
    set_level(ffix.u11, SK_SAILING, 6);
    CuAssertIntEquals_Msg(tc, "compensate with additional crew", 3, ship_speed(ffix.fleet, ffix.u11));

    test_cleanup();
}

static void test_move_ship(CuTest *tc) {
    fleet_fixture ffix;

    test_cleanup();
    setup_fleet();
    init_fleet(&ffix);

    CuAssertPtrEquals(tc, ffix.r, ffix.fleet->region);
    CuAssertPtrEquals(tc, ffix.r, ffix.u11->region);

    move_ship(ffix.fleet, ffix.r, ffix.r2, NULL);

    CuAssertPtrEquals(tc, ffix.r2, ffix.fleet->region);
    CuAssertPtrEquals(tc, ffix.r2, ffix.sh1->region);
    CuAssertPtrEquals(tc, ffix.r2, ffix.sh2->region);
    CuAssertPtrEquals(tc, ffix.r2, ffix.u11->region);
    CuAssertPtrEquals(tc, ffix.r2, ffix.u21->region);

    test_cleanup();
}

static void test_check_ship_allowed(CuTest *tc) {
    fleet_fixture ffix;
    ship_type *stype;
    ship *sh;
    building_type* btype;
    building *b;
    unit *u1, *u2;

    test_cleanup();
    setup_fleet();
    init_fleet(&ffix);

    CuAssertIntEquals(tc, SA_COAST, check_ship_allowed(ffix.fleet, ffix.r));
    CuAssertIntEquals(tc, SA_COAST, check_ship_allowed(ffix.fleet, ffix.r2));
    stype = test_create_shiptype2("fuzzyship", 1, 1, 1, 1, 1000, 1000, 10, 1, 1, 0);
    sh = test_create_ship(ffix.r, stype);
    fleet_add_ship(sh, ffix.fleet, NULL);
    CuAssertIntEquals(tc, SA_NO_COAST, check_ship_allowed(ffix.fleet, ffix.r));

    btype = test_create_buildingtype("harbour");
    b = test_create_building(ffix.r, btype);
    b->flags |= BLD_WORKING;

    CuAssertIntEquals(tc, SA_HARBOUR, check_ship_allowed(ffix.fleet, ffix.r));

    u1 = test_create_unit(ffix.f1, ffix.r);
    u_set_building(u1, b);
    building_set_owner(u1);
    CuAssertIntEquals(tc, SA_HARBOUR, check_ship_allowed(ffix.fleet, ffix.r));

    u2 = test_create_unit(ffix.f2, ffix.r);
    u_set_building(u2, b);
    building_set_owner(u2);
    CuAssertIntEquals(tc, SA_NO_COAST, check_ship_allowed(ffix.fleet, ffix.r));

    test_cleanup();
}

static void test_fleet_coast(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;
    ship *fleet;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh1->no));
    unit_addorder(ffix.u11, ord);
    ord = create_order(K_FLEET, ffix.f1->locale, "%s", itoa36(ffix.sh2->no));
    unit_addorder(ffix.u11, ord);

    ffix.sh1->coast = D_WEST;
    ffix.sh2->coast = D_NORTHWEST;

    fleet_cmd(ffix.r);

    msg = test_get_last_message(ffix.u11->faction->msgs);
    CuAssertStrEquals(tc, "fleet_ship_invalid", test_get_messagetype(msg));

    fleet = ffix.sh1->fleet;
    CuAssertIntEquals_Msg(tc, "ship stays on coast", D_WEST, ffix.sh1->coast);
    CuAssertIntEquals_Msg(tc, "fleet should be on ship's cost", D_WEST, fleet->coast);
    CuAssertIntEquals(tc, 1 , fleet->size);

    add_ship(&ffix.sh1->fleet, NULL, 1);
    fleet_remove_ship(ffix.sh1, NULL);
    CuAssertIntEquals_Msg(tc, "fleet coast reset", NODIRECTION, fleet->coast);

    test_cleanup();
}

static void test_fleet_setcoast(CuTest *tc) {
    fleet_fixture ffix;

    test_cleanup();
    setup_fleet();
    init_fleet(&ffix);

    set_coast(ffix.sh1->fleet, ffix.r2, ffix.r);

    CuAssertIntEquals(tc, D_WEST, ffix.sh1->fleet->coast);
    CuAssertIntEquals(tc, D_WEST, ffix.sh1->coast);
    CuAssertIntEquals(tc, D_WEST, ffix.sh2->coast);

    test_cleanup();
}
static void test_fleet_move(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;
    struct message* msg;

    test_cleanup();
    setup_fleet();
    init_fleet(&ffix);

    /* ship type boat: cptskill 1, sumskill 1, minskill 1, range 2, cargo 1000,
     * construction maxsize 5, minskill 1, reqsize 1
     * coasts: plain
     */

    ord = create_order(K_MOVE, ffix.f1->locale, "W NORDOSTEN O SUEDOSTE SUEDWEST W", itoa36(ffix.sh1->no));
    unit_addorder(ffix.u11, ord);
    set_level(ffix.u11, SK_SAILING, 1);

    init_order(ord);
    move_cmd(ffix.u11, ord, false);

    msg = test_get_last_message(ffix.f1->msgs);
    CuAssertStrEquals(tc, "error_captain_skill_low", test_get_messagetype(msg));
    test_clear_messages(ffix.f1);

    set_level(ffix.u11, SK_SAILING, 6);
    set_number(ffix.u11, 2);
    init_order(ord);
    move_cmd(ffix.u11, ord, false);

    msg = test_get_last_message(ffix.f1->msgs);
    CuAssertPtrNotNull(tc, msg);
    CuAssertStrEquals(tc, "shipsail", test_get_messagetype(msg));

    CuAssertPtrEquals_Msg(tc, "fleet does not have speed 2", findregion(-1, 1), ffix.sh1->region);
    CuAssertPtrEquals_Msg(tc, "fleet does not have speed 2", findregion(-1, 1), ffix.sh2->region);
    CuAssertPtrEquals_Msg(tc, "fleet does not have speed 2", findregion(-1, 1), ffix.fleet->region);

     /* TODO maelstrom*/

    test_cleanup();
}


/*
// TODO

// NEW
// test fleet ship properties

// ADD ship:
// must be complete (?)
// TRANSFER possible?

// check coast
// check capacity (freight/cabins)
// check captain/crew skill
// LOAD: fleets of boats cannot load stone?

// KINGKILLER: check owner leaves/dies
// damage fleet
// damage fleet multiple times
// damage ship
// combat damage: if units take damage, damage at random as many ships as would fit the units?

// REMOVE:
// check kick
// who owns ship? no unintended stealing

// MOVE:
// check terrain, coast
// follow?
// piracy (active/passive)
// DRIFT
// overload: drift?, damage?
// storms
// new fleet logic compatible?

// REPORT
// ;Name
// "Flotte";Typ
// ships;Groesse
// ;Kapitaen
// ;Partei
// ;capacity
// ;cargo
// ;cabins
// ;cabins_used
// ;speed
// ; damage

// DESTROY
// build works how?
// sabotage/destroy works how?

// EFFECT
// curse fleet does not work?
// maelstrom should damage either fleet or ships
// which curses work how? (antimagic, speed, airship, whatelse?)

// scores, summary!
*/

CuSuite *get_fleets_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_fleet_create);
    SUITE_ADD_TEST(suite, test_fleet_create_param);
    SUITE_ADD_TEST(suite, test_fleet_missing_param);
    SUITE_ADD_TEST(suite, test_fleet_create_external);
    SUITE_ADD_TEST(suite, test_fleet_create_no_add_fleet);
    SUITE_ADD_TEST(suite, test_fleet_add);
    SUITE_ADD_TEST(suite, test_fleet_add2);
    SUITE_ADD_TEST(suite, test_fleet_remove1);
    SUITE_ADD_TEST(suite, test_fleet_remove2);
    SUITE_ADD_TEST(suite, test_fleet_remove_wrong_fleet);
    SUITE_ADD_TEST(suite, test_fleet_remove_invalid);
    SUITE_ADD_TEST(suite, test_fleet_remove_nonsail);
    SUITE_ADD_TEST(suite, test_fleet_nonsail);
    SUITE_ADD_TEST(suite, test_fleet_enter);

    SUITE_ADD_TEST(suite, test_fleet_stats);
    SUITE_ADD_TEST(suite, test_fleet_complete);
    SUITE_ADD_TEST(suite, test_fleet_speed);
    SUITE_ADD_TEST(suite, test_move_ship);
    SUITE_ADD_TEST(suite, test_check_ship_allowed);
    SUITE_ADD_TEST(suite, test_fleet_coast);
    SUITE_ADD_TEST(suite, test_fleet_setcoast);
    SUITE_ADD_TEST(suite, test_fleet_move);
    return suite;
}
