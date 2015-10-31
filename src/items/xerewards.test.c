#include <platform.h>

#include "xerewards.h"

#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/item.h>
#include <kernel/pool.h>
#include <kernel/region.h>

#include <util/language.h>

#include <string.h>

#include <tests.h>
#include <CuTest.h>

static void test_manacrystal(CuTest *tc) {
    test_cleanup();
    test_create_world();
    test_cleanup();
}

static void test_skillpotion(CuTest *tc) {
    unit *u;
    faction *f;
    const struct item_type *itype;
    const char* pPotionText = NULL;
    const char* pDescription = NULL;

    test_cleanup();
    test_create_world();

    f = test_create_faction(NULL);
    u = test_create_unit(f, findregion(0, 0));
    itype = test_create_itemtype("skillpotion");
    change_resource(u, itype->rtype, 2);
    CuAssertIntEquals(tc, 1, use_skillpotion(u, itype, 1, NULL));

    pPotionText = mkname("potion", "skillpotion");
    CuAssertPtrNotNull(tc, pPotionText);

    pDescription = LOC(f->locale, pPotionText);
    CuAssertPtrNotNull(tc, pDescription);

    CuAssert(tc, "Description empty", strlen(pDescription) == 0);


    test_cleanup();
}

CuSuite *get_xerewards_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_manacrystal);
    SUITE_ADD_TEST(suite, test_skillpotion);
    return suite;
}

