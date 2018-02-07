#include <platform.h>

#include "alchemy.h"
#include "move.h"

#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/race.h>
#include <kernel/item.h>
#include <kernel/region.h>

#include "guard.h"

#include <limits.h>

#include <CuTest.h>
#include "tests.h"

static void test_herbsearch(CuTest * tc)
{
    faction *f;
    race *rc;
    unit *u, *u2;
    region *r;
    const item_type *itype;

    test_setup();
    test_inject_messagetypes();
    r = test_create_region(0, 0, NULL);
    rc = rc_get_or_create("dragon");
    rc->flags |= RCF_UNARMEDGUARD;
    u2 = test_create_unit(test_create_faction(rc), r);
    setguard(u2, true);

    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    itype = test_create_itemtype("rosemary");

    herbsearch(u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error59"));
    test_clear_messages(f);

    set_level(u, SK_HERBALISM, 1);
    CuAssertPtrEquals(tc, u2, is_guarded(r, u));
    herbsearch(u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error59"));
    test_clear_messages(f);

    setguard(u2, false);
    CuAssertPtrEquals(tc, 0, is_guarded(r, u));
    CuAssertPtrEquals(tc, 0, (void *)rherbtype(r));
    herbsearch(u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error108"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error59"));
    test_clear_messages(f);

    rsetherbtype(r, itype);
    CuAssertPtrEquals(tc, (void *)itype, (void *)rherbtype(r));
    CuAssertIntEquals(tc, 0, rherbs(r));
    herbsearch(u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "researchherb_none"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error108"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error59"));
    test_clear_messages(f);

    rsetherbs(r, 100);
    CuAssertIntEquals(tc, 100, rherbs(r));
    herbsearch(u, INT_MAX);
    CuAssertIntEquals(tc, 99, rherbs(r));
    CuAssertIntEquals(tc, 1, i_get(u->items, itype));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "herbfound"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "researchherb_none"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error108"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error59"));
    test_clear_messages(f);

    test_teardown();
}

CuSuite *get_alchemy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_herbsearch);
    return suite;
}
