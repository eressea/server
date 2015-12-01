#include <platform.h>
#include <stdlib.h>

#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include "monster.h"
#include "guard.h"
#include "skill.h"

#include <util/language.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern void plan_monsters(struct faction *f);
extern int monster_attacks(unit * monster, bool respect_buildings, bool rich_only);

static void init_language(void)
{
    struct locale* lang;
    int i;

    lang = get_or_create_locale("de");
    locale_setstring(lang, "skill::unarmed", "Waffenloser Kampf");
    locale_setstring(lang, "keyword::attack", "ATTACKIERE");
    locale_setstring(lang, "keyword::study", "LERNE");
    locale_setstring(lang, "keyword::tax", "TREIBE");
    locale_setstring(lang, "keyword::loot", "PLUENDERE");
    locale_setstring(lang, "keyword::piracy", "PIRATERIE");
    locale_setstring(lang, "keyword::guard", "BEWACHE");
    locale_setstring(lang, "keyword::move", "NACH");
    locale_setstring(lang, "keyword::message", "BOTSCHAFT");
    locale_setstring(lang, "REGION", "REGION");
    locale_setstring(lang, "east", "O");

    for (i = 0; i < MAXKEYWORDS; ++i) {
        if (!locale_getstring(lang, mkname("keyword", keywords[i])))
            locale_setstring(lang, mkname("keyword", keywords[i]), keywords[i]);
    }
    for (i = 0; i < MAXSKILLS; ++i) {
        if (!locale_getstring(lang, mkname("skill", skillnames[i])))
            locale_setstring(lang, mkname("skill", skillnames[i]), skillnames[i]);
    }
    init_keywords(lang);
    init_skills(lang);
}

static order *find_order(const char *expected, const unit *unit)
{
    char cmd[32];
    order *order;
    for (order = unit->orders; order; order = order->next) {
        if (strcmp(expected, get_command(order, cmd, sizeof(cmd))) == 0) {
            return order;
        }
    }
    return NULL;
}

static void create_monsters(faction **player, faction **monsters, region **r, unit **u, unit **m) {
    race* rc;

    test_cleanup();

    init_language();

    test_create_world();
    *player = test_create_faction(NULL);
    *monsters = get_or_create_monsters();
    assert(rc_find((*monsters)->race->_name));
    rc = rc_get_or_create((*monsters)->race->_name);
    fset(rc, RCF_UNARMEDGUARD);
    fset(rc, RCF_NPC);
    fset(*monsters, FFL_NOIDLEOUT);
    assert(fval(*monsters, FFL_NPC) && fval((*monsters)->race, RCF_UNARMEDGUARD) && fval((*monsters)->race, RCF_NPC) && fval(*monsters, FFL_NOIDLEOUT));
    (*monsters)->locale = default_locale;

    *r = findregion(0, 0);

    *u = test_create_unit(*player, *r);
    unit_setid(*u, 1);
    *m = test_create_unit(*monsters, *r);
}

static void test_monsters_attack(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;

    create_monsters(&f, &f2, &r, &u, &m);

    guard(m, GUARD_TAX);

    config_set("rules.monsters.attack_chance", "1");

    plan_monsters(f2);

    CuAssertPtrNotNull(tc, find_order("ATTACKIERE 1", m));
    test_cleanup();
}

static void test_monsters_attack_ocean(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;

    create_monsters(&f, &f2, &r, &u, &m);
    r = findregion(-1, 0);
    u = test_create_unit(u->faction, r);
    unit_setid(u, 2);
    m = test_create_unit(m->faction, r);
    assert(!m->region->land);

    config_set("rules.monsters.attack_chance", "1");

    plan_monsters(f2);

    CuAssertPtrNotNull(tc, find_order("ATTACKIERE 2", m));
    test_cleanup();
}

static void test_monsters_waiting(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;

    create_monsters(&f, &f2, &r, &u, &m);
    guard(m, GUARD_TAX);
    fset(m, UFL_ISNEW);
    monster_attacks(m, false, false);
    CuAssertPtrEquals(tc, 0, find_order("ATTACKIERE 1", m));
    test_cleanup();
}

static void test_seaserpent_piracy(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;
    race *rc;

    create_monsters(&f, &f2, &r, &u, &m);
    r = findregion(-1, 0);
    u = test_create_unit(u->faction, r);
    unit_setid(u, 2);
    m = test_create_unit(m->faction, r);
    u_setrace(m, rc = test_create_race("seaserpent"));
    assert(!m->region->land);
    fset(m, UFL_MOVED);
    fset(rc, RCF_ATTACK_MOVED);

    config_set("rules.monsters.attack_chance", "1");

    plan_monsters(f2);
    CuAssertPtrNotNull(tc, find_order("PIRATERIE", m));
    CuAssertPtrNotNull(tc, find_order("ATTACKIERE 2", m));
    test_cleanup();
}

static void test_monsters_attack_not(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;

    create_monsters(&f, &f2, &r, &u, &m);

    guard(m, GUARD_TAX);
    guard(u, GUARD_TAX);

    config_set("rules.monsters.attack_chance", "0");

    plan_monsters(f2);

    CuAssertPtrEquals(tc, 0, find_order("ATTACKIERE 1", m));
    test_cleanup();
}

static void test_dragon_attacks_the_rich(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;
    const item_type *i_silver;

    init_language();
    create_monsters(&f, &f2, &r, &u, &m);

    guard(m, GUARD_TAX);
    set_level(m, SK_WEAPONLESS, 10);

    rsetmoney(r, 1);
    rsetmoney(findregion(1, 0), 0);
    i_silver = it_find("money");
    assert(i_silver);
    i_change(&u->items, i_silver, 5000);

    config_set("rules.monsters.attack_chance", "0.00001");

    plan_monsters(f2);

    CuAssertPtrNotNull(tc, find_order("ATTACKIERE 1", m));
    CuAssertPtrNotNull(tc, find_order("PLUENDERE", m));
    test_cleanup();
}

static void test_dragon_moves(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;

    create_monsters(&f, &f2, &r, &u, &m);
    rsetpeasants(r, 0);
    rsetmoney(r, 0);
    rsetmoney(findregion(1, 0), 1000);

    set_level(m, SK_WEAPONLESS, 10);
    config_set("rules.monsters.attack_chance", ".0");
    plan_monsters(f2);

    CuAssertPtrNotNull(tc, find_order("NACH O", m));
    test_cleanup();
}

static void test_monsters_learn_exp(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;
    skill* sk;

    create_monsters(&f, &f2, &r, &u, &m);
    config_set("study.from_use", "1");

    u_setrace(u, u_race(m));
    produceexp(u, SK_MELEE, u->number);
    sk = unit_skill(u, SK_MELEE);
    CuAssertTrue(tc, !sk);

    produceexp(m, SK_MELEE, u->number);
    sk = unit_skill(m, SK_MELEE);
    CuAssertTrue(tc, sk && (sk->level > 0 || (sk->level == 0 && sk->weeks > 0)));

    test_cleanup();
}

CuSuite *get_monsters_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_monsters_attack);
    SUITE_ADD_TEST(suite, test_monsters_attack_ocean);
    DISABLE_TEST(suite, test_seaserpent_piracy);
    SUITE_ADD_TEST(suite, test_monsters_waiting);
    SUITE_ADD_TEST(suite, test_monsters_attack_not);
    SUITE_ADD_TEST(suite, test_dragon_attacks_the_rich);
    SUITE_ADD_TEST(suite, test_dragon_moves);
    SUITE_ADD_TEST(suite, test_monsters_learn_exp);
    return suite;
}
