#include <platform.h>
#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/unit.h>
#include <util/language.h>
#include <util/attrib.h>
#include <spells/regioncurse.h>
#include "spells.h"

#include <CuTest.h>
#include <tests.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


static struct castorder *test_create_castorder(castorder *order, unit *u, const char *name, int level, float force, int range) {
    struct locale * lang;
    spell *sp;

    lang = get_or_create_locale("en");
    sp = create_spell(name, 0);
    return order = create_castorder(order, u, NULL, sp, u->region, level, force, range, create_order(K_CAST, lang, ""), NULL);
}

static void test_magicresistance(CuTest *tc) {
    struct region *r;
    struct faction *f1, *f2;
    unit *u1, *u2;
    int level;
    curse *c;

    test_cleanup();
    test_create_world();
    r=findregion(0, 0);
    f1 = test_create_faction(test_create_race("human"));
    u1 = test_create_unit(f1, r);

    f2 = test_create_faction(test_create_race("human"));
    u2 = test_create_unit(f2, r);

    test_cleanup();
}

CuSuite *get_magicresistance_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_magicresistance);
    return suite;
}
