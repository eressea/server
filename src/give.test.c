#include <platform.h>

#include "give.h"
#include "economy.h"

#include <kernel/config.h>
#include <kernel/item.h>
#include <kernel/terrain.h>
#include <kernel/order.h>
#include <kernel/unit.h>
#include <kernel/faction.h>

#include <util/base36.h>
#include <util/language.h>
#include <util/message.h>

#include <CuTest.h>
#include <tests.h>
#include <stdio.h>
#include <assert.h>

struct give {
    struct unit *src, *dst;
    struct region *r;
    struct faction *f1, *f2;
    struct item_type * itype;
};

static void setup_give(struct give *env) {
    struct terrain_type *ter = test_create_terrain("plain", LAND_REGION);
    env->r = test_create_region(0, 0, ter);
    env->src = test_create_unit(env->f1, env->r);
    env->dst = test_create_unit(env->f2, env->r);
    env->itype = it_get_or_create(rt_get_or_create("money"));
    env->itype->flags |= ITF_HERB;
}

static void test_give(CuTest * tc) {
    struct give env;

    test_cleanup();
    env.f2 = env.f1 = test_create_faction(0);
    setup_give(&env);

    i_change(&env.src->items, env.itype, 10);
    CuAssertIntEquals(tc, 0, give_item(10, env.itype, env.src, env.dst, 0));
    CuAssertIntEquals(tc, 0, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 10, i_get(env.dst->items, env.itype));

    CuAssertIntEquals(tc, -1, give_item(10, env.itype, env.src, env.dst, 0));
    CuAssertIntEquals(tc, 0, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 10, i_get(env.dst->items, env.itype));
    test_cleanup();
}

static void test_give_herbs(CuTest * tc) {
    struct give env;
    struct order *ord;
    struct locale * lang;
    char cmd[32];

    test_cleanup();
    test_create_world();
    env.f2 = env.f1 = test_create_faction(0);
    setup_give(&env);
    i_change(&env.src->items, env.itype, 10);

    lang = get_or_create_locale("test");
    env.f1->locale = lang;
    locale_setstring(lang, "KRAEUTER", "HERBS");
    init_locale(lang);
    _snprintf(cmd, sizeof(cmd), "%s HERBS", itoa36(env.dst->no));
    ord = create_order(K_GIVE, lang, cmd);
    assert(ord);

    give_cmd(env.src, ord);
    CuAssertIntEquals(tc, 0, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 10, i_get(env.dst->items, env.itype));
    test_cleanup();
}

static void test_give_okay(CuTest * tc) {
    struct give env;

    test_cleanup();
    env.f2 = env.f1 = test_create_faction(0);
    setup_give(&env);

    set_param(&global.parameters, "rules.give", "0");
    CuAssertPtrEquals(tc, 0, check_give(env.src, env.dst, 0));
    test_cleanup();
}

static void test_give_denied_by_rules(CuTest * tc) {
    struct give env;
    struct message *msg;

    test_cleanup();
    env.f1 = test_create_faction(0);
    env.f2 = test_create_faction(0);
    setup_give(&env);

    set_param(&global.parameters, "rules.give", "0");
    CuAssertPtrNotNull(tc, msg = check_give(env.src, env.dst, 0));
    msg_release(msg);
    test_cleanup();
}

CuSuite *get_give_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_give);
    SUITE_ADD_TEST(suite, test_give_herbs);
    SUITE_ADD_TEST(suite, test_give_okay);
    SUITE_ADD_TEST(suite, test_give_denied_by_rules);
    return suite;
}
