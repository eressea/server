#include "give.h"

#include "contact.h"
#include "eressea.h"

#include <kernel/ally.h>
#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include "kernel/skill.h"    // for SK_MAGIC

#include <util/base36.h>
#include <util/language.h>
#include <util/message.h>
#include <util/param.h>
#include "util/keyword.h"    // for K_GIVE, K_RESERVE

#include <CuTest.h>
#include <tests.h>
#include <stdio.h>
#include <assert.h>

struct give {
    struct locale * lang;
    struct unit *src, *dst;
    struct region *r;
    struct faction *f1, *f2;
    struct item_type * itype;
};

static void setup_give(struct give *env) {
    struct terrain_type *ter = test_create_terrain("plain", LAND_REGION);
    race *rc;

    assert(env->f1);
    rc = test_create_race(env->f1->race ? env->f1->race->_name : "humon");
    rc->ec_flags |= ECF_GIVEPERSON;

    env->r = test_create_region(0, 0, ter);
    env->src = test_create_unit(env->f1, env->r);
    env->itype = it_get_or_create(rt_get_or_create("money"));
    env->itype->flags |= ITF_HERB;
    env->itype->rtype->flags |= RTF_POOLED;
    if (env->f2) {
        ally_set(&env->f2->allies, env->f1, HELP_GIVE);
        env->dst = test_create_unit(env->f2, env->r);
    }
    else {
        env->dst = NULL;
    }
    if (env->lang) {
        init_locale(env->lang);
        env->f1->locale = env->lang;
    }

    config_set("rules.give.max_men", "-1");
    /* success messages: */
    mt_create_va(mt_new("receive_person", NULL), "unit:unit", "target:unit", "amount:int", MT_NEW_END);
    mt_create_va(mt_new("give_person", NULL), "unit:unit", "target:unit", "amount:int", MT_NEW_END);
    mt_create_va(mt_new("give_person_peasants", NULL), "unit:unit", "amount:int", MT_NEW_END);
    mt_create_va(mt_new("give_person_ocean", NULL), "unit:unit", "amount:int", MT_NEW_END);
    mt_create_va(mt_new("receive", NULL), "unit:unit", "target:unit", "resource:resource", "amount:int", MT_NEW_END);
    mt_create_va(mt_new("give", NULL), "unit:unit", "target:unit", "resource:resource", "amount:int", MT_NEW_END);
    mt_create_va(mt_new("give_peasants", NULL), "unit:unit", "resource:resource", "amount:int", MT_NEW_END);
    /* error messages: */
    mt_create_error(120);
    mt_create_error(128);
    mt_create_error(129);
    mt_create_error(96);
    mt_create_error(73);
    mt_create_error(10);
    mt_create_feedback("feedback_give_forbidden");
    mt_create_feedback("peasants_give_invalid");
    mt_create_va(mt_new("too_many_units_in_faction", NULL), "unit:unit", "region:region", "command:order", "allowed:int", MT_NEW_END);
    mt_create_va(mt_new("too_many_units_in_alliance", NULL), "unit:unit", "region:region", "command:order", "allowed:int", MT_NEW_END);
    mt_create_va(mt_new("feedback_no_contact", NULL), "unit:unit", "region:region", "command:order", "target:unit", MT_NEW_END);
    mt_create_va(mt_new("feedback_unit_not_found", NULL), "unit:unit", "region:region", "command:order", MT_NEW_END);
    mt_create_va(mt_new("giverestriction", NULL), "unit:unit", "region:region", "command:order", "turns:int", MT_NEW_END);
    mt_create_va(mt_new("error_unit_size", NULL), "unit:unit", "region:region", "command:order", "maxsize:int", MT_NEW_END);
    mt_create_va(mt_new("nogive_reserved", NULL), "unit:unit", "region:region", "command:order", "resource:resource", "reservation:int", MT_NEW_END);
    mt_create_va(mt_new("race_notake", NULL), "unit:unit", "region:region", "command:order", "race:race", MT_NEW_END);
    mt_create_va(mt_new("race_noregroup", NULL), "unit:unit", "region:region", "command:order", "race:race", MT_NEW_END);
}

static void test_give_unit(CuTest * tc) {
    struct give env = { 0 };

    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = test_create_faction();
    setup_give(&env);

    CuAssertIntEquals(tc, 1, env.f1->num_units);
    CuAssertIntEquals(tc, 1, env.f2->num_units);
    join_group(env.src, "group");

    config_set("rules.give.max_men", "0");
    give_unit(env.src, env.dst, NULL);
    CuAssertPtrEquals(tc, env.f1, env.src->faction);
    CuAssertIntEquals(tc, 0, env.f2->newbies);

    config_set("rules.give.max_men", "-1");
    give_unit(env.src, env.dst, NULL);
    CuAssertPtrEquals(tc, env.f2, env.src->faction);
    CuAssertPtrEquals(tc, NULL, get_group(env.src));
    CuAssertIntEquals(tc, 1, env.f2->newbies);
    CuAssertPtrEquals(tc, NULL, env.f1->units);
    CuAssertPtrNotNull(tc, test_find_messagetype(env.f1->msgs, "give_person"));
    CuAssertPtrNotNull(tc, test_find_messagetype(env.f2->msgs, "receive_person"));

    /* must be allied to transfer a unit */
    u_setfaction(env.src, env.f1);
    ally_set(&env.f2->allies, env.f1, 0);
    give_unit(env.src, env.dst, NULL);
    CuAssertPtrEquals(tc, env.f1, env.src->faction);

    /* contact also works */
    contact_unit(env.dst, env.src);
    give_unit(env.src, env.dst, NULL);
    CuAssertPtrEquals(tc, env.f2, env.src->faction);

    test_teardown();
}

static void test_give_unit_humans(CuTest * tc) {
    struct give env = { 0 };
    race *rc;

    test_setup_ex(tc);
    env.f1 = test_create_faction_ex(test_create_race("elf"), NULL);
    env.f2 = test_create_faction_ex(rc = test_create_race("human"), env.f1->locale);
    rc->flags |= RCF_MIGRANTS;
    setup_give(&env);

    give_unit(env.src, env.dst, NULL);
    CuAssertPtrNotNull(tc, test_find_messagetype(env.f1->msgs, "error128"));
    CuAssertPtrEquals(tc, env.f1, env.src->faction);
    CuAssertIntEquals(tc, 0, env.f2->newbies);

    scale_number(env.dst, 57);
    CuAssertIntEquals(tc, 1, count_maxmigrants(env.f2));
    give_unit(env.src, env.dst, NULL);
    CuAssertPtrEquals(tc, env.f2, env.src->faction);
    CuAssertIntEquals(tc, 1, env.f2->newbies);
    test_teardown();
}

static void test_give_unit_cursed(CuTest * tc) {
    struct give env = { 0 };
    race *rc;
    item_type* itype;

    test_setup_ex(tc);
    env.f1 = test_create_faction_ex(test_create_race("elf"), NULL);
    env.f2 = test_create_faction_ex(rc = test_create_race("human"), env.f1->locale);
    rc->flags |= RCF_MIGRANTS;
    setup_give(&env);

    /* units with cursed items cannot be given */
    itype = test_create_itemtype("seashell");
    itype->flags |= ITF_CURSED;
    i_change(&env.src->items, itype, 1);

    scale_number(env.dst, 57);
    CuAssertIntEquals(tc, 1, count_maxmigrants(env.f2));
    give_unit(env.src, env.dst, NULL);
    CuAssertPtrEquals(tc, env.f1, env.src->faction);
    CuAssertPtrNotNull(tc, test_find_messagetype(env.f1->msgs, "error78"));
    CuAssertIntEquals(tc, 0, env.f2->newbies);

    test_teardown();
}

static void test_give_unit_other_race(CuTest * tc) {
    struct give env = { 0 };
    test_setup_ex(tc);
    env.f1 = test_create_faction_ex(test_create_race("elf"), NULL);
    env.f2 = test_create_faction_ex(test_create_race("orc"), env.f1->locale);
    setup_give(&env);
    scale_number(env.dst, 57);
    CuAssertIntEquals(tc, 0, count_maxmigrants(env.f2));
    give_unit(env.src, env.dst, NULL);
    CuAssertIntEquals(tc, 0, env.f2->newbies);
    CuAssertPtrNotNull(tc, test_find_messagetype(env.f1->msgs, "error120"));
    test_teardown();
}

static void test_give_unit_limits(CuTest * tc) {
    struct give env = { 0 };
    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = test_create_faction();
    setup_give(&env);
    config_set("rules.limit.faction", "1");

    give_unit(env.src, env.dst, NULL);
    CuAssertPtrEquals(tc, env.f1, env.src->faction);
    CuAssertIntEquals(tc, 0, env.f2->newbies);
    CuAssertIntEquals(tc, 1, env.f1->num_units);
    CuAssertIntEquals(tc, 1, env.f2->num_units);
    CuAssertPtrNotNull(tc, test_find_messagetype(env.f1->msgs, "too_many_units_in_faction"));
    test_teardown();
}

static void test_give_unit_to_peasants(CuTest * tc) {
    struct give env = { 0 };
    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = 0;
    setup_give(&env);
    rsetpeasants(env.r, 0);
    give_unit(env.src, NULL, NULL);
    CuAssertIntEquals(tc, 0, env.src->number);
    CuAssertIntEquals(tc, 1, rpeasants(env.r));
    test_teardown();
}

static void test_give_unit_to_ocean(CuTest * tc) {
    struct give env = { 0 };
    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = 0;
    setup_give(&env);
    env.r->terrain = test_create_terrain("ocean", SEA_REGION);
    give_unit(env.src, NULL, NULL);
    CuAssertIntEquals(tc, 0, env.src->number);
    test_teardown();
}

static void test_give_men(CuTest * tc) {
    struct give env = { 0 };
    message * msg;
    test_setup_ex(tc);
    env.f2 = env.f1 = test_create_faction();
    setup_give(&env);
    CuAssertPtrEquals(tc, NULL, msg = give_men(1, env.src, env.dst, NULL));
    assert(!msg);
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    test_teardown();
}

static void test_give_men_magicians(CuTest * tc) {
    struct give env = { 0 };
    int p;
    message * msg;

    test_setup_ex(tc);
    mt_create_error(158);
    env.f2 = env.f1 = test_create_faction();
    setup_give(&env);
    set_level(env.src, SK_MAGIC, 1);
    CuAssertPtrNotNull(tc, msg = give_men(1, env.src, env.dst, NULL));
    CuAssertStrEquals(tc, "error158", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 1, env.dst->number);
    CuAssertIntEquals(tc, 1, env.src->number);
    msg_release(msg);

    p = rpeasants(env.r);
    CuAssertPtrNotNull(tc, msg = disband_men(1, env.dst, NULL));
    CuAssertStrEquals(tc, "give_person_peasants", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 0, env.dst->number);
    CuAssertIntEquals(tc, p+1, rpeasants(env.r));
    msg_release(msg);

    test_teardown();
}

static void test_give_men_limit(CuTest * tc) {
    struct give env = { 0 };
    message *msg;

    test_setup_ex(tc);
    env.f2 = test_create_faction();
    env.f1 = test_create_faction();
    setup_give(&env);
    config_set("rules.give.max_men", "1");

    /* below the limit, give men, increase newbies counter */
    contact_unit(env.dst, env.src);
    msg = give_men(1, env.src, env.dst, NULL);
    CuAssertStrEquals(tc, "give_person", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    CuAssertIntEquals(tc, 1, env.f2->newbies);
    msg_release(msg);

    /* beyond the limit, do nothing */
    contact_unit(env.src, env.dst);
    msg = give_men(2, env.dst, env.src, NULL);
    CuAssertStrEquals(tc, "error129", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    CuAssertIntEquals(tc, 0, env.f1->newbies);
    msg_release(msg);

    test_teardown();
}

static void test_give_men_in_ocean(CuTest * tc) {
    struct give env = { 0 };
    message * msg;

    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = 0;
    setup_give(&env);
    env.r->terrain = test_create_terrain("ocean", SEA_REGION);
    msg = disband_men(1, env.src, NULL);
    CuAssertStrEquals(tc, "give_person_ocean", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 0, env.src->number);
    msg_release(msg);
    test_teardown();
}

static void test_give_men_too_many(CuTest * tc) {
    struct give env = { 0 };
    message * msg;

    test_setup_ex(tc);
    env.f2 = env.f1 = test_create_faction();
    setup_give(&env);
    CuAssertPtrEquals(tc, NULL, msg = give_men(2, env.src, env.dst, NULL));
    assert(!msg);
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    test_teardown();
}

static void test_give_cmd_limit(CuTest * tc) {
    struct give env = { 0 };
    unit *u;
    test_setup_ex(tc);
    env.f2 = env.f1 = test_create_faction();
    setup_give(&env);
    u = env.src;
    scale_number(u, 2);
    u->thisorder = create_order(K_GIVE, u->faction->locale, "%s 1 PERSON", itoa36(env.dst->no));
    give_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 1, env.src->number);
    test_teardown();
}

static void test_give_men_none(CuTest * tc) {
    struct give env = { 0 };
    message * msg;

    test_setup_ex(tc);
    env.f2 = env.f1 = test_create_faction();
    setup_give(&env);
    msg = give_men(0, env.src, env.dst, NULL);
    CuAssertStrEquals(tc, "error96", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 1, env.dst->number);
    CuAssertIntEquals(tc, 1, env.src->number);
    msg_release(msg);
    test_teardown();
}

static void test_give_men_all(CuTest* tc) {
    struct give env = { 0 };
    message* msg;

    test_setup_ex(tc);
    env.f2 = env.f1 = test_create_faction();
    setup_give(&env);
    set_level(env.src, SK_ALCHEMY, 5);
    msg = give_men(1, env.src, env.dst, NULL);
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    CuAssertPtrNotNull(tc, unit_skill(env.dst, SK_ALCHEMY));
    CuAssertPtrEquals(tc, NULL, unit_skill(env.src, SK_ALCHEMY));
    CuAssertPtrEquals(tc, NULL, msg);
    test_teardown();
}

static void test_give_men_other_faction(CuTest * tc) {
    struct give env = { 0 };
    message * msg;

    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = test_create_faction();
    setup_give(&env);
    contact_unit(env.dst, env.src);
    msg = give_men(1, env.src, env.dst, NULL);
    CuAssertStrEquals(tc, "give_person", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    msg_release(msg);
    test_teardown();
}

static void test_give_men_cursed(CuTest * tc) {
    struct give env = { 0 };
    message * msg;
    item_type* itype;

    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = test_create_faction();
    setup_give(&env);

    /* it should not matter that the unit has a seashell */
    itype = test_create_itemtype("seashell");
    itype->flags |= ITF_CURSED;
    i_change(&env.src->items, itype, 1);

    contact_unit(env.dst, env.src);
    msg = give_men(1, env.src, env.dst, NULL);
    CuAssertStrEquals(tc, "give_person", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    msg_release(msg);
    test_teardown();
}

static void test_give_men_requires_contact(CuTest * tc) {
    struct give env = { 0 };
    message * msg;
    order *ord;

    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = test_create_faction();
    setup_give(&env);
    msg = give_men(1, env.src, env.dst, NULL);
    CuAssertStrEquals(tc, "feedback_no_contact", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 1, env.dst->number);
    CuAssertIntEquals(tc, 1, env.src->number);

    ord = create_order(K_GIVE, env.f1->locale, "%s ALLES PERSONEN", itoa36(env.dst->no));
    test_clear_messages(env.f1);
    give_cmd(env.src, ord);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(env.f1->msgs, "give_person"));
    CuAssertPtrNotNull(tc, test_find_messagetype(env.f1->msgs, "feedback_no_contact"));

    msg_release(msg);
    free_order(ord);
    test_teardown();
}

static void test_give_men_not_to_self(CuTest * tc) {
    struct give env = { 0 };
    message * msg;
    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = NULL;
    setup_give(&env);
    msg = give_men(1, env.src, env.src, NULL);
    CuAssertStrEquals(tc, "error10", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 1, env.src->number);
    msg_release(msg);
    test_teardown();
}

static void test_give_men_hungry(CuTest * tc) {
    struct give env = { 0 };
    message * msg = NULL;
    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = test_create_faction();
    setup_give(&env);
    CuAssertTrue(tc, can_give_men(env.src, env.dst, NULL, &msg));
    CuAssertPtrEquals(tc, NULL, msg);
    env.src->flags |= UFL_HUNGER;
    CuAssertTrue(tc, !can_give_men(env.src, env.dst, NULL, &msg));
    CuAssertPtrNotNull(tc, msg);
    CuAssertStrEquals(tc, "error73", test_get_messagetype(msg));
    msg_release(msg);

    msg = NULL;
    CuAssertTrue(tc, can_give_men(env.src, NULL, NULL, &msg));
    CuAssertPtrEquals(tc, NULL, msg);

    test_teardown();
}

static void test_give_peasants(CuTest * tc) {
    struct give env = { 0 };
    message * msg;

    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = NULL;
    setup_give(&env);
    rsetpeasants(env.r, 0);
    msg = disband_men(1, env.src, NULL);
    CuAssertStrEquals(tc, "give_person_peasants", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 0, env.src->number);
    CuAssertIntEquals(tc, 1, rpeasants(env.r));
    msg_release(msg);
    test_teardown();
}

static void test_give(CuTest * tc) {
    struct give env = { 0 };

    test_setup_ex(tc);
    env.f2 = env.f1 = test_create_faction();
    setup_give(&env);

    i_change(&env.src->items, env.itype, 10);
    CuAssertIntEquals(tc, 0, give_item(10, env.itype, env.src, env.dst, NULL));
    CuAssertIntEquals(tc, 0, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 10, i_get(env.dst->items, env.itype));

    CuAssertIntEquals(tc, -1, give_item(10, env.itype, env.src, env.dst, NULL));
    CuAssertIntEquals(tc, 0, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 10, i_get(env.dst->items, env.itype));
    test_teardown();
}

static void test_give_cmd(CuTest * tc) {
    struct give env = { 0 };
    struct order *ord;

    test_setup_ex(tc);
    env.lang = test_create_locale();
    env.f2 = env.f1 = test_create_faction();
    setup_give(&env);

    i_change(&env.src->items, env.itype, 10);

    ord = create_order(K_GIVE, env.f1->locale, "%s 5 %s", itoa36(env.dst->no), LOC(env.f1->locale, env.itype->rtype->_name));
    assert(ord);
    give_cmd(env.src, ord);
    CuAssertIntEquals(tc, 5, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 5, i_get(env.dst->items, env.itype));

    free_order(ord);
    test_teardown();
}

static void test_give_herbs(CuTest * tc) {
    struct give env = { 0 };
    struct order *ord;

    test_setup_ex(tc);
    env.f2 = env.f1 = test_create_faction();
    setup_give(&env);
    i_change(&env.src->items, env.itype, 10);

    ord = create_order(K_GIVE, env.f1->locale, "%s %s", itoa36(env.dst->no), LOC(env.f1->locale, parameters[P_HERBS]));
    assert(ord);

    give_cmd(env.src, ord);
    CuAssertIntEquals(tc, 0, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 10, i_get(env.dst->items, env.itype));
    free_order(ord);
    test_teardown();
}

static void test_give_okay(CuTest * tc) {
    struct give env = { 0 };

    test_setup_ex(tc);
    env.f2 = env.f1 = test_create_faction();
    setup_give(&env);

    config_set("rules.give.flags", "0");
    CuAssertPtrEquals(tc, NULL, check_give(env.src, env.dst, NULL));
    test_teardown();
}

static void test_give_denied_by_rules(CuTest * tc) {
    struct give env = { 0 };
    struct message *msg;

    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = test_create_faction();
    setup_give(&env);

    config_set("rules.give.flags", "0");
    CuAssertPtrNotNull(tc, msg = check_give(env.src, env.dst, NULL));
    msg_release(msg);
    test_teardown();
}

static void test_give_dead_unit(CuTest * tc) {
    struct give env = { 0 };
    struct message *msg;

    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = test_create_faction();
    setup_give(&env);
    env.dst->number = 0;
    fset(env.dst, UFL_DEAD);
    CuAssertPtrNotNull(tc, msg = check_give(env.src, env.dst, NULL));
    msg_release(msg);
    test_teardown();
}

static void test_give_new_unit(CuTest * tc) {
    struct give env = { 0 };

    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = test_create_faction();
    setup_give(&env);
    env.dst->number = 0;
    fset(env.dst, UFL_ISNEW);
    CuAssertPtrEquals(tc, NULL, check_give(env.src, env.dst, NULL));
    test_teardown();
}

static void test_give_invalid_target(CuTest *tc) {
    /* bug https://bugs.eressea.de/view.php?id=1685 */
    struct give env = { 0 };
    order *ord;

    test_setup_ex(tc);
    env.f1 = test_create_faction();
    env.f2 = test_create_faction();
    setup_give(&env);

    i_change(&env.src->items, env.itype, 10);
    ord = create_order(K_GIVE, env.f1->locale, "## KRAUT");
    assert(ord);
    test_clear_messages(env.f1);
    give_cmd(env.src, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(env.f1->msgs, "feedback_unit_not_found"));
    CuAssertIntEquals(tc, 10, i_get(env.src->items, env.itype));
    free_order(ord);

    /* if target unit is in another region => not found */
    ord = create_order(K_GIVE, env.f1->locale, "%i KRAUT", env.dst->no);
    move_unit(env.dst, test_create_plain(env.r->x, env.r->y + 1), NULL);
    test_clear_messages(env.f1);
    give_cmd(env.src, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(env.f1->msgs, "feedback_unit_not_found"));
    CuAssertIntEquals(tc, 10, i_get(env.src->items, env.itype));
    free_order(ord);

    test_teardown();
}

static void test_give_item_cursed(CuTest* tc)
{
    struct give env = { 0 };
    order* ord;
    faction* f;

    test_setup_ex(tc);
    env.f1 = f = test_create_faction();
    env.f2 = test_create_faction();

    setup_give(&env);
    env.itype->flags |= ITF_CURSED;

    i_change(&env.src->items, env.itype, 10);
    ord = create_order(K_GIVE, f->locale, "%s 10 %s", itoa36(env.dst->no),
        LOC(f->locale, resourcename(env.itype->rtype, 0)));
    assert(ord);

    give_cmd(env.src, ord);
    CuAssertIntEquals(tc, 10, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 0, i_get(env.dst->items, env.itype));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error25"));
    free_order(ord);
    test_clear_messages(f);

    test_teardown();
}

static void test_give_item_cursed_self(CuTest* tc)
{
    struct give env = { 0 };
    order* ord;
    faction* f;

    test_setup_ex(tc);
    env.f1 = env.f2 = f = test_create_faction();

    setup_give(&env);
    env.itype->flags |= ITF_CURSED;

    i_change(&env.src->items, env.itype, 20);
    ord = create_order(K_GIVE, f->locale, "%s 10 %s", itoa36(env.dst->no),
        LOC(f->locale, resourcename(env.itype->rtype, 0)));
    assert(ord);

    give_cmd(env.src, ord);
    CuAssertIntEquals(tc, 10, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 10, i_get(env.dst->items, env.itype));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error25"));
    free_order(ord);
    test_clear_messages(f);

    ord = create_order(K_RESERVE, f->locale, "20 %s", LOC(f->locale, resourcename(env.itype->rtype, 0)));
    CuAssertIntEquals(tc, 20, reserve_cmd(env.dst, ord));
    CuAssertIntEquals(tc, 0, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 20, i_get(env.dst->items, env.itype));
    CuAssertIntEquals(tc, 20, get_reservation(env.dst, env.itype));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error25"));
    free_order(ord);
    test_clear_messages(f);

    test_teardown();
}

static void test_reserve_cmd(CuTest* tc) {
    unit* u1, * u2;
    faction* f;
    region* r;
    order* ord;
    const resource_type* rtype;

    test_setup();
    init_resources();

    rtype = get_resourcetype(R_SILVER);
    assert(rtype && rtype->itype);
    f = test_create_faction();
    r = test_create_plain(0, 0);
    assert(r && f);
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    assert(u1 && u2);
    ord = create_order(K_RESERVE, f->locale, "200 SILBER");
    assert(ord);
    i_change(&u1->items, rtype->itype, 100);
    i_change(&u2->items, rtype->itype, 100);
    CuAssertIntEquals(tc, 200, reserve_cmd(u1, ord));
    CuAssertIntEquals(tc, 200, i_get(u1->items, rtype->itype));
    CuAssertIntEquals(tc, 0, i_get(u2->items, rtype->itype));
    free_order(ord);
    test_teardown();
}

static void test_reserve_all(CuTest* tc) {
    unit* u1, * u2;
    faction* f;
    region* r;
    order* ord;
    const resource_type* rtype;
    struct locale* loc;

    test_setup();
    init_resources();
    loc = test_create_locale();
    locale_setstring(loc, parameters[P_ANY], "ALLES");
    init_parameters(loc);

    rtype = get_resourcetype(R_SILVER);
    assert(rtype && rtype->itype);
    f = test_create_faction();
    r = test_create_plain(0, 0);
    assert(r && f);
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    assert(u1 && u2);
    ord = create_order(K_RESERVE, f->locale, "ALLES SILBER");
    assert(ord);
    i_change(&u1->items, rtype->itype, 100);
    i_change(&u2->items, rtype->itype, 100);
    CuAssertIntEquals(tc, 100, reserve_cmd(u1, ord));
    CuAssertIntEquals(tc, 100, i_get(u1->items, rtype->itype));
    CuAssertIntEquals(tc, 100, i_get(u2->items, rtype->itype));
    free_order(ord);
    test_teardown();
}

static void test_reserve_self(CuTest* tc) {
    unit* u1, * u2;
    faction* f;
    region* r;
    order* ord;
    const resource_type* rtype;
    const struct locale* loc;

    test_setup();
    init_resources();

    rtype = get_resourcetype(R_SILVER);
    assert(rtype && rtype->itype);
    f = test_create_faction();
    r = test_create_plain(0, 0);
    assert(r && f);
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    assert(u1 && u2);
    loc = test_create_locale();
    assert(loc);
    ord = create_order(K_RESERVE, loc, "200 SILBER");
    assert(ord);
    i_change(&u1->items, rtype->itype, 100);
    i_change(&u2->items, rtype->itype, 100);
    CuAssertIntEquals(tc, 100, reserve_self(u1, ord));
    CuAssertIntEquals(tc, 100, i_get(u1->items, rtype->itype));
    CuAssertIntEquals(tc, 100, i_get(u2->items, rtype->itype));
    free_order(ord);
    test_teardown();
}

CuSuite *get_give_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_give);
    SUITE_ADD_TEST(suite, test_give_cmd);
    SUITE_ADD_TEST(suite, test_give_cmd_limit);
    SUITE_ADD_TEST(suite, test_give_men);
    SUITE_ADD_TEST(suite, test_give_men_magicians);
    SUITE_ADD_TEST(suite, test_give_men_limit);
    SUITE_ADD_TEST(suite, test_give_men_in_ocean);
    SUITE_ADD_TEST(suite, test_give_men_none);
    SUITE_ADD_TEST(suite, test_give_men_all);
    SUITE_ADD_TEST(suite, test_give_men_too_many);
    SUITE_ADD_TEST(suite, test_give_men_other_faction);
    SUITE_ADD_TEST(suite, test_give_men_requires_contact);
    SUITE_ADD_TEST(suite, test_give_men_not_to_self);
    SUITE_ADD_TEST(suite, test_give_men_hungry);
    SUITE_ADD_TEST(suite, test_give_men_cursed);
    SUITE_ADD_TEST(suite, test_give_unit);
    SUITE_ADD_TEST(suite, test_give_unit_humans);
    SUITE_ADD_TEST(suite, test_give_unit_cursed);
    SUITE_ADD_TEST(suite, test_give_unit_other_race);
    SUITE_ADD_TEST(suite, test_give_unit_limits);
    SUITE_ADD_TEST(suite, test_give_unit_to_ocean);
    SUITE_ADD_TEST(suite, test_give_unit_to_peasants);
    SUITE_ADD_TEST(suite, test_give_peasants);
    SUITE_ADD_TEST(suite, test_give_herbs);
    SUITE_ADD_TEST(suite, test_give_okay);
    SUITE_ADD_TEST(suite, test_give_denied_by_rules);
    SUITE_ADD_TEST(suite, test_give_invalid_target);
    SUITE_ADD_TEST(suite, test_give_new_unit);
    SUITE_ADD_TEST(suite, test_give_dead_unit);
    SUITE_ADD_TEST(suite, test_give_item_cursed);
    SUITE_ADD_TEST(suite, test_give_item_cursed_self);
    SUITE_ADD_TEST(suite, test_reserve_cmd);
    SUITE_ADD_TEST(suite, test_reserve_self);
    SUITE_ADD_TEST(suite, test_reserve_all);
    return suite;
}
