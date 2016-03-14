#include <platform.h>
#include "move.h"

#include <kernel/build.h>
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

#include <spells/shipcurse.h>
#include <attributes/movement.h>

#include <CuTest.h>
#include <tests.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void setup_fleet(void) {
    ship_type* ftype;
    struct locale* lang;

    lang = get_or_create_locale("de");
    locale_setstring(lang, "fleet", "Flotte");
    locale_setstring(lang, directions[D_WEST], "WESTEN");
    test_create_world();
    init_locales();

    ftype = st_get_or_create("fleet");
    ftype->cptskill = 6;
}

typedef struct fleet_fixture {
    region *r, *r2;
    ship *sh1;
    unit *u11;
    faction *f1;
    const ship_type *stype1;
} fleet_fixture;

static void init_fixture(fleet_fixture *ffix) {
    ffix->f1 = test_create_faction(NULL);
    ffix->r = findregion(0, 0);
    ffix->r2 = findregion(-1, 0);

    ffix->stype1 = st_find("boat");

    ffix->sh1 = test_create_ship(ffix->r, ffix->stype1);
    ffix->u11 = test_create_unit(ffix->f1, ffix->r);
}

static void assert_fleet(CuTest *tc, region *r, ship *sh, unit *cpt) {
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
    CuAssertPtrNotNull(tc, found);

    CuAssertPtrEquals_Msg(tc, "ship not added to fleet", (void *) found, (ship *) sh->fleet);
    CuAssertPtrEquals_Msg(tc, "captain not in fleet", found, (ship *) cpt->ship);
    CuAssertTrue(tc, ship_isfleet(found));
    CuAssertPtrEquals_Msg(tc, "captain not fleet owner", cpt, ship_owner(found));
    CuAssertPtrEquals_Msg(tc, "captain not ship owner", cpt, ship_owner(sh));
}

static void test_fleet_create(CuTest *tc) {
    fleet_fixture ffix;
    order *ord;

    test_cleanup();
    setup_fleet();
    init_fixture(&ffix);

    u_set_ship(ffix.u11, ffix.sh1);

    ord = create_order(K_FLEET, ffix.f1->locale, NULL);
    unit_addorder(ffix.u11, ord);
    fleet_cmd(ffix.r);

    assert_fleet(tc, ffix.r, ffix.sh1, ffix.u11);

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
}

/*
// TODO

// NEW
// test command variants
// test unhappy paths (captain valid, ship owner contacts...)
// test fleet ship properties

// ADD ship:
// must be empty or own or owner must contact
// must be complete (?)
// units where?
// TRANSFER possible?

// check range
// check coast
// check capacity (freight/cabins)
// check captain/crew skill
// LOAD: fleets of boats cannot load stone?

// ENTER
// check enter/leave
// KINGKILLER: check owner leaves/dies
// damage fleet
// damage fleet multiple times
// damage ship
// combat damage: if units take damage, damage at random as many ships as would fit the units?

// REMOVE:
// check kick
// who owns ship? no unintended stealing

// MOVE:
// check skill
// check terrain, coast, range
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
    return suite;
}
