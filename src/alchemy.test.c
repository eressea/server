#include <platform.h>

#include "alchemy.h"
#include "move.h"

#include <kernel/config.h>
#include <kernel/messages.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/race.h>
#include <kernel/item.h>
#include <kernel/region.h>

#include <CuTest.h>
#include "tests.h"

static void test_herbsearch(CuTest * tc)
{
    faction *f;
    race *rc;
    unit *u, *u2;
    region *r;
    const item_type *itype;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    rc = rc_get_or_create("dragon");
    rc->flags |= RCF_UNARMEDGUARD;
    u2 = test_create_unit(test_create_faction(rc), r);
    guard(u2, GUARD_PRODUCE);

    f = test_create_faction(0);
    u = test_create_unit(f, r);
    itype = test_create_itemtype("rosemary");

    herbsearch(r, u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error59"));
    free_messagelist(f->msgs);
    f->msgs = 0;

    set_level(u, SK_HERBALISM, 1);
    CuAssertPtrEquals(tc, u2, is_guarded(r, u, GUARD_PRODUCE));
    herbsearch(r, u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error59"));
    free_messagelist(f->msgs);
    f->msgs = 0;

    guard(u2, GUARD_NONE);
    CuAssertPtrEquals(tc, 0, is_guarded(r, u, GUARD_PRODUCE));
    CuAssertPtrEquals(tc, 0, (void *)rherbtype(r));
    herbsearch(r, u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error108"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error59"));
    free_messagelist(f->msgs);
    f->msgs = 0;

    rsetherbtype(r, itype);
    CuAssertPtrEquals(tc, (void *)itype, (void *)rherbtype(r));
    CuAssertIntEquals(tc, 0, rherbs(r));
    herbsearch(r, u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "researchherb_none"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error108"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error59"));
    free_messagelist(f->msgs);
    f->msgs = 0;

    rsetherbs(r, 100);
    CuAssertIntEquals(tc, 100, rherbs(r));
    herbsearch(r, u, INT_MAX);
    CuAssertIntEquals(tc, 99, rherbs(r));
    CuAssertIntEquals(tc, 1, i_get(u->items, itype));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "herbfound"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "researchherb_none"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error108"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error59"));
    free_messagelist(f->msgs);
    f->msgs = 0;

    test_cleanup();
}

CuSuite *get_alchemy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_herbsearch);
    return suite;
}
