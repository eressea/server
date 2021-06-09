#include "sort.h"

#include "kernel/building.h"
#include "kernel/faction.h"
#include "kernel/unit.h"
#include "kernel/order.h"
#include "kernel/region.h"
#include "kernel/ship.h"

#include "util/base36.h"
#include "util/keyword.h"
#include "util/language.h"
#include "util/param.h"

#include "tests.h"
#include <CuTest.h>

static void test_sort_after(CuTest *tc) {
    unit *u1, *u2;
    faction *f;
    region *r;

    test_setup();
    u1 = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    u2 = test_create_unit(f, r);
    unit_addorder(u1, create_order(K_SORT, f->locale, "%s %s",
        LOC(f->locale, parameters[P_AFTER]), itoa36(u2->no)));
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);

    restack_units();
    CuAssertPtrEquals(tc, u2, r->units);
    CuAssertPtrEquals(tc, u1, u2->next);
    CuAssertPtrEquals(tc, NULL, u1->next);

    test_teardown();
}

static void test_sort_before(CuTest *tc) {
    unit *u1, *u2;
    faction *f;
    region *r;

    test_setup();
    u1 = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    u2 = test_create_unit(f, r);
    unit_addorder(u2, create_order(K_SORT, f->locale, "%s %s",
        LOC(f->locale, parameters[P_BEFORE]), itoa36(u1->no)));
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);

    restack_units();
    CuAssertPtrEquals(tc, u2, r->units);
    CuAssertPtrEquals(tc, u1, u2->next);
    CuAssertPtrEquals(tc, NULL, u1->next);

    test_teardown();
}

static void test_sort_before_owner(CuTest *tc) {
    unit *u1, *u2;
    faction *f;
    region *r;
    building *b;
    ship *sh;

    test_setup();
    u1 = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    b = test_create_building(r, NULL);
    sh = test_create_ship(r, NULL);

    u2 = test_create_unit(f, r);
    unit_addorder(u2, create_order(K_SORT, f->locale, "%s %s",
        LOC(f->locale, parameters[P_BEFORE]), itoa36(u1->no)));
    u1->building = b;
    building_update_owner(b);
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);

    /* nothing happens: */
    restack_units();
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error259"));
    test_clear_messagelist(&f->msgs);

    /* u2 must be in the same building: */
    u2->building = b;
    restack_units();
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error261"));
    test_clear_messagelist(&f->msgs);

    u1->building = NULL;
    u2->building = NULL;
    building_update_owner(b);
    u1->ship = sh;
    ship_update_owner(sh);

    /* nothing happens: */
    restack_units();
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error259"));
    test_clear_messagelist(&f->msgs);

    /* u2 must be in the same ship: */
    u2->ship = sh;
    restack_units();
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error261"));
    test_clear_messagelist(&f->msgs);

    test_teardown();
}

static void test_sort_before_paused_building_owner(CuTest *tc) {
    unit *u1, *u2;
    faction *f;
    region *r;
    building *b;

    test_setup();
    u1 = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    f->flags |= FFL_PAUSED;
    b = test_create_building(r, NULL);

    u2 = test_create_unit(f = test_create_faction(), r);
    unit_addorder(u2, create_order(K_SORT, f->locale, "%s %s",
        LOC(f->locale, parameters[P_BEFORE]), itoa36(u1->no)));
    u1->building = b;
    building_update_owner(b);
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);

    /* not in building, nothing happend: */
    restack_units();
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error259"));
    test_clear_messagelist(&f->msgs);

    /* u1 allows u2 to steal the top spot: */
    u2->building = b;
    restack_units();
    CuAssertPtrEquals(tc, u2, r->units);
    CuAssertPtrEquals(tc, u1, u2->next);
    CuAssertPtrEquals(tc, NULL, u1->next);
    CuAssertPtrEquals(tc, u2, building_owner(b));
    CuAssertPtrEquals(tc, NULL, f->msgs);
    test_teardown();
}

static void test_sort_before_paused_ship_owner(CuTest *tc) {
    unit *u1, *u2;
    faction *f;
    region *r;
    ship *sh;

    test_setup();
    u1 = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    f->flags |= FFL_PAUSED;
    sh = test_create_ship(r, NULL);

    u2 = test_create_unit(f = test_create_faction(), r);
    unit_addorder(u2, create_order(K_SORT, f->locale, "%s %s",
        LOC(f->locale, parameters[P_BEFORE]), itoa36(u1->no)));
    u1->ship = sh;
    ship_update_owner(sh);
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);

    /* not in ship, nothing happend: */
    restack_units();
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error259"));
    test_clear_messagelist(&f->msgs);

    /* u1 allows u2 to steal the top spot: */
    u2->ship = sh;
    restack_units();
    CuAssertPtrEquals(tc, u2, r->units);
    CuAssertPtrEquals(tc, u1, u2->next);
    CuAssertPtrEquals(tc, NULL, u1->next);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    CuAssertPtrEquals(tc, NULL, f->msgs);
    test_teardown();
}

static void test_sort_paused(CuTest *tc) {
    unit *u1, *u2;
    faction *f;
    region *r;

    test_setup();
    u1 = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    u2 = test_create_unit(f, r);
    unit_addorder(u2, create_order(K_SORT, f->locale, "%s %s",
        LOC(f->locale, parameters[P_BEFORE]), itoa36(u1->no)));
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);

    /* paused faction, so nothing happens: */
    f->flags |= FFL_PAUSED;
    restack_units();
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);
    CuAssertPtrEquals(tc, NULL, f->msgs);

    test_teardown();
}

CuSuite *get_sort_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_sort_before_owner);
    SUITE_ADD_TEST(suite, test_sort_before_paused_ship_owner);
    SUITE_ADD_TEST(suite, test_sort_before_paused_building_owner);
    SUITE_ADD_TEST(suite, test_sort_after);
    SUITE_ADD_TEST(suite, test_sort_before);
    SUITE_ADD_TEST(suite, test_sort_paused);
    return suite;
}
