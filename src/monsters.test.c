#include <platform.h>

#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/unit.h>

#include <util/language.h>
#include <util/language_struct.h>

#include <monster.h>

#include <string.h>
#include <skill.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>

#include <stdio.h>
#include <string.h>

extern void plan_monsters(struct faction *f);

static const int REPETITIONS = 10;

static void init_language(void)
{
    locale* lang;
    lang = get_or_create_locale("de");
    locale_setstring(lang, "skill::unarmed", "Waffenloser Kampf");
    locale_setstring(lang, "keyword::attack", "ATTACKIERE");
    locale_setstring(lang, "keyword::study", "LERNE");
    locale_setstring(lang, "keyword::tax", "TREIBE");
    locale_setstring(lang, "keyword::loot", "PLUENDERE");
    locale_setstring(lang, "keyword::guard", "BEWACHE");
    locale_setstring(lang, "keyword::move", "NACH");
    locale_setstring(lang, "keyword::message", "BOTSCHAFT");
    locale_setstring(lang, "REGION", "REGION");
    locale_setstring(lang, "east", "O");
    init_skills(lang);
}

static world_fixture *create_monsters(faction **player, faction **monsters, region **r, unit **u, unit **m,
    bool two_regions) {
    world_fixture *world;

    test_cleanup();

    if (two_regions)
        world = test_create_world();
    else {
        world = test_create_essential();
        test_create_region(0, 0, world->t_plain);
    }
    init_language();

    *player = test_create_faction(NULL);
    *monsters = get_or_create_monsters();
    (*monsters)->locale = default_locale;

    *r = findregion(0, 0);

    *u = test_create_unit(*player, *r);
    unit_setid(*u, 1);
    *m = test_create_unit(*monsters, *r);

    return world;
}

static void test_monsters_attack(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;

    int i = 0;
    for (i = 0; i < REPETITIONS; ++i) {
        create_monsters(&f, &f2, &r, &u, &m, false);

        guard(m, GUARD_TAX);

        set_param(&global.parameters, "rules.monsters.attack_chance", "1");

        plan_monsters(f2);

        assert_order(tc, "ATTACKIERE 1", m, true);
    }
    test_cleanup();
}

static void test_monsters_attack_not(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;

    int i = 0;
    for (i = 0; i < REPETITIONS; ++i) {
        create_monsters(&f, &f2, &r, &u, &m, false);

        guard(m, GUARD_TAX);

        set_param(&global.parameters, "rules.monsters.attack_chance", "0");

        plan_monsters(f2);

        assert_order(tc, "ATTACKIERE 1", m, false);
    }
    test_cleanup();
}

static void test_dragon_attacks_guard(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;

    int i = 0;
    for (i = 0; i < REPETITIONS; ++i) {
        create_monsters(&f, &f2, &r, &u, &m, false);

        guard(m, GUARD_TAX);
        set_level(m, SK_WEAPONLESS, 10);

        guard(u, GUARD_TAX);
        u_setrace(u, urace(m));

        set_param(&global.parameters, "rules.monsters.attack_chance", ".0001");

        plan_monsters(f2);

        assert_order(tc, "ATTACKIERE 1", m, true);
        assert_order(tc, "PLUENDERE", m, true);
    }
    test_cleanup();
}

static void test_dragon_attacks_guard_not(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;

    int i = 0;
    for (i = 0; i < REPETITIONS; ++i) {
        create_monsters(&f, &f2, &r, &u, &m, false);

        guard(m, GUARD_TAX);
        set_level(m, SK_WEAPONLESS, 10);

        guard(u, GUARD_TAX);

        set_param(&global.parameters, "rules.monsters.attack_chance", ".0001");

        plan_monsters(f2);

        assert_order(tc, "TREIBE", m, true);
    }
    test_cleanup();
}

static void test_dragon_moves(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;

    int i = 0;
    for (i = 0; i < REPETITIONS; ++i) {
        create_monsters(&f, &f2, &r, &u, &m, true);
        rsetpeasants(r, 0);
        rsetmoney(r, 0);
        rsetmoney(findregion(1,0), 1000);

        set_level(m, SK_WEAPONLESS, 10);
        plan_monsters(f2);

        assert_order(tc, "NACH O", m, true);
    }
    test_cleanup();
}

static void test_monsters_learn_exp(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;
    skill* sk;

    float chance = global.producexpchance;
    global.producexpchance = 1.0;

    create_monsters(&f, &f2, &r, &u, &m, false);

    u_setrace(u, u_race(m));
    produceexp(u, SK_MELEE, u->number);
    sk = unit_skill(u, SK_MELEE);
    CuAssertTrue(tc, !sk);


    produceexp(m, SK_MELEE, u->number);
    sk = unit_skill(m, SK_MELEE);
    CuAssertTrue(tc, sk && (sk->level>0 || (sk->level==0 && sk->weeks > 0)));

    global.producexpchance = chance;
    test_cleanup();
}

CuSuite *get_monsters_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_monsters_attack);
    SUITE_ADD_TEST(suite, test_monsters_attack_not);
    SUITE_ADD_TEST(suite, test_dragon_attacks_guard);
    SUITE_ADD_TEST(suite, test_dragon_attacks_guard_not);
    SUITE_ADD_TEST(suite, test_dragon_moves);
    SUITE_ADD_TEST(suite, test_monsters_learn_exp);
    return suite;
}
