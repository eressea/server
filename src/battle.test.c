#include <platform.h>

#include "battle.h"
#include "skill.h"

#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <util/functions.h>

#include <CuTest.h>
#include "tests.h"

static void test_make_fighter(CuTest * tc)
{
    unit *au;
    region *r;
    fighter *af;
    battle *b;
    side *as;
    faction * f;
    const resource_type *rtype;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    f = test_create_faction(NULL);
    au = test_create_unit(f, r);
    enable_skill(SK_MAGIC, true);
    enable_skill(SK_RIDING, true);
    set_level(au, SK_MAGIC, 3);
    set_level(au, SK_RIDING, 3);
    au->status = ST_BEHIND;
    rtype = get_resourcetype(R_HORSE);
    i_change(&au->items, rtype->itype, 1);

    b = make_battle(r);
    as = make_side(b, au->faction, 0, 0, 0);
    af = make_fighter(b, au, as, false);

    CuAssertIntEquals(tc, 1, b->nfighters);
    CuAssertPtrEquals(tc, 0, af->building);
    CuAssertPtrEquals(tc, as, af->side);
    CuAssertIntEquals(tc, 0, af->run.hp);
    CuAssertIntEquals(tc, ST_BEHIND, af->status);
    CuAssertIntEquals(tc, 0, af->run.number);
    CuAssertIntEquals(tc, au->hp, af->person[0].hp);
    CuAssertIntEquals(tc, 1, af->person[0].speed);
    CuAssertIntEquals(tc, au->number, af->alive);
    CuAssertIntEquals(tc, 0, af->removed);
    CuAssertIntEquals(tc, 3, af->magic);
    CuAssertIntEquals(tc, 1, af->horses);
    CuAssertIntEquals(tc, 0, af->elvenhorses);
    free_battle(b);
    test_cleanup();
}

static int add_two(building * b, unit * u, building_bonus bonus) {
    return 2;
}

static void test_defenders_get_building_bonus(CuTest * tc)
{
    unit *du, *au;
    region *r;
    building * bld;
    fighter *df, *af;
    battle *b;
    side *ds, *as;
    int diff;
    troop dt, at;
    building_type * btype;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    btype = bt_get_or_create("castle");
    btype->protection = &add_two;
    bld = test_create_building(r, btype);
    bld->size = 10;

    du = test_create_unit(test_create_faction(NULL), r);
    au = test_create_unit(test_create_faction(NULL), r);
    u_set_building(du, bld);

    b = make_battle(r);
    ds = make_side(b, du->faction, 0, 0, 0);
    df = make_fighter(b, du, ds, false);
    as = make_side(b, au->faction, 0, 0, 0);
    af = make_fighter(b, au, as, true);

    CuAssertPtrEquals(tc, bld, df->building);
    CuAssertPtrEquals(tc, 0, af->building);

    dt.fighter = df;
    dt.index = 0;
    at.fighter = af;
    at.index = 0;

    diff = skilldiff(at, dt, 0);
    CuAssertIntEquals(tc, -2, diff);

    diff = skilldiff(dt, at, 0);
    CuAssertIntEquals(tc, 0, diff);
    free_battle(b);
    test_cleanup();
}

static void test_attackers_get_no_building_bonus(CuTest * tc)
{
    unit *au;
    region *r;
    building * bld;
    fighter *af;
    battle *b;
    side *as;
    building_type * btype;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    btype = bt_get_or_create("castle");
    btype->protection = &add_two;
    bld = test_create_building(r, btype);
    bld->size = 10;

    au = test_create_unit(test_create_faction(NULL), r);
    u_set_building(au, bld);

    b = make_battle(r);
    as = make_side(b, au->faction, 0, 0, 0);
    af = make_fighter(b, au, as, true);

    CuAssertPtrEquals(tc, 0, af->building);
    free_battle(b);
    test_cleanup();
}

static void test_building_bonus_respects_size(CuTest * tc)
{
    unit *au, *du;
    region *r;
    building * bld;
    fighter *af, *df;
    battle *b;
    side *as;
    building_type * btype;
    faction * f;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    btype = bt_get_or_create("castle");
    btype->protection = &add_two;
    bld = test_create_building(r, btype);
    bld->size = 10;

    f = test_create_faction(NULL);
    au = test_create_unit(f, r);
    scale_number(au, 9);
    u_set_building(au, bld);
    du = test_create_unit(f, r);
    u_set_building(du, bld);
    scale_number(du, 2);

    b = make_battle(r);
    as = make_side(b, au->faction, 0, 0, 0);
    af = make_fighter(b, au, as, false);
    df = make_fighter(b, du, as, false);

    CuAssertPtrEquals(tc, bld, af->building);
    CuAssertPtrEquals(tc, 0, df->building);
    free_battle(b);
    test_cleanup();
}

static void test_building_defence_bonus(CuTest * tc)
{
    unit *au;
    region *r;
    building * bld;
    building_type * btype;
    faction * f;
    int def;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    btype = test_create_buildingtype("castle");
    btype->protection = (int(*)(struct building *, struct unit *, building_bonus))get_function("building_protection");
    btype->construction->defense_bonus = 3;
    bld = test_create_building(r, btype);
    bld->size = 1;

    f = test_create_faction(NULL);
    au = test_create_unit(f, r);
    scale_number(au, 1);
    u_set_building(au, bld);

    def = btype->protection(bld, au, DEFENSE_BONUS);
    CuAssertIntEquals(tc, 3, def);
    test_cleanup();
}

fighter *setup_fighter(battle **bp, unit *u) {
    battle *b;

    *bp = b = make_battle(u->region);
    return make_fighter(b, u, make_side(b, u->faction, 0, 0, 0), false);
}

static void test_calculate_armor(CuTest * tc)
{
    troop dt;
    battle *b;
    region *r;
    unit *du;
    weapon_type *wtype;
    armor_type *ashield, *achain;
    item_type *ibelt, *ishield, *ichain;
    race *rc;
    double magres = 0.0;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    ibelt = it_get_or_create(rt_get_or_create("trollbelt"));
    ishield = it_get_or_create(rt_get_or_create("shield"));
    ashield = new_armortype(ishield, 0.0, 0.5, 1, ATF_SHIELD);
    ichain = it_get_or_create(rt_get_or_create("chainmail"));
    achain = new_armortype(ichain, 0.0, 0.5, 3, ATF_NONE);
    wtype = new_weapontype(it_get_or_create(rt_get_or_create("sword")), 0, 0.5, 0, 0, 0, 0, SK_MELEE, 1);
    rc = test_create_race("human");
    du = test_create_unit(test_create_faction(rc), r);
    dt.index = 0;

    dt.fighter = setup_fighter(&b, du);
    CuAssertIntEquals_Msg(tc, "default ac", 0, calculate_armor(dt, 0, 0, &magres));
    CuAssertDblEquals_Msg(tc, "magres unmodified", 0.0, magres, 0.01);
    free_battle(b);

    i_change(&du->items, ibelt, 1);
    dt.fighter = setup_fighter(&b, du);
    CuAssertIntEquals_Msg(tc, "magical armor", 1, calculate_armor(dt, 0, 0, 0));
    rc->armor = 2;
    CuAssertIntEquals_Msg(tc, "natural armor", 3, calculate_armor(dt, 0, 0, 0));
    rc->armor = 0;
    free_battle(b);

    i_change(&du->items, ishield, 1);
    i_change(&du->items, ichain, 1);
    dt.fighter = setup_fighter(&b, du);
    CuAssertIntEquals_Msg(tc, "require BF_EQUIPMENT", 1, calculate_armor(dt, 0, 0, 0));
    free_battle(b);

    rc->battle_flags |= BF_EQUIPMENT;
    dt.fighter = setup_fighter(&b, du);
    CuAssertIntEquals_Msg(tc, "stack equipment rc", 5, calculate_armor(dt, 0, 0, 0));
    rc->armor = 2;
    CuAssertIntEquals_Msg(tc, "natural armor adds 50%", 6, calculate_armor(dt, 0, 0, 0));
    wtype->flags = WTF_NONE;
    CuAssertIntEquals_Msg(tc, "regular weapon has no effect", 6, calculate_armor(dt, 0, wtype, 0));
    wtype->flags = WTF_ARMORPIERCING;
    CuAssertIntEquals_Msg(tc, "armor piercing weapon", 3, calculate_armor(dt, 0, wtype, 0));
    wtype->flags = WTF_NONE;

    CuAssertIntEquals_Msg(tc, "magical attack", 3, calculate_armor(dt, 0, 0, &magres));
    CuAssertDblEquals_Msg(tc, "magres unmodified", 0.0, magres, 0.01);

    ashield->flags |= ATF_LAEN;
    achain->flags |= ATF_LAEN;
    magres = 1.0;
    CuAssertIntEquals_Msg(tc, "laen armor", 3, calculate_armor(dt, 0, 0, &magres));
    CuAssertDblEquals_Msg(tc, "laen magres bonus", 0.25, magres, 0.01);
    free_battle(b);
    test_cleanup();
}

static void test_projectile_armor(CuTest * tc)
{
    troop dt;
    battle *b;
    region *r;
    unit *du;
    weapon_type *wtype;
    armor_type *ashield, *achain;
    item_type *ishield, *ichain;
    race *rc;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    ishield = it_get_or_create(rt_get_or_create("shield"));
    ashield = new_armortype(ishield, 0.0, 0.5, 1, ATF_SHIELD);
    ichain = it_get_or_create(rt_get_or_create("chainmail"));
    achain = new_armortype(ichain, 0.0, 0.5, 3, ATF_NONE);
    wtype = new_weapontype(it_get_or_create(rt_get_or_create("sword")), 0, 0.5, 0, 0, 0, 0, SK_MELEE, 1);
    rc = test_create_race("human");
    rc->battle_flags |= BF_EQUIPMENT;
    du = test_create_unit(test_create_faction(rc), r);
    dt.index = 0;

    i_change(&du->items, ishield, 1);
    i_change(&du->items, ichain, 1);
    dt.fighter = setup_fighter(&b, du);
    wtype->flags = WTF_MISSILE;
    achain->projectile = 1.0;
    CuAssertIntEquals_Msg(tc, "projectile armor", -1, calculate_armor(dt, 0, wtype, 0));
    achain->projectile = 0.0;
    ashield->projectile = 1.0;
    CuAssertIntEquals_Msg(tc, "projectile shield", -1, calculate_armor(dt, 0, wtype, 0));
    wtype->flags = WTF_NONE;
    CuAssertIntEquals_Msg(tc, "no projectiles", 4, calculate_armor(dt, 0, wtype, 0));
    free_battle(b);
    test_cleanup();
}

CuSuite *get_battle_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_make_fighter);
    SUITE_ADD_TEST(suite, test_defenders_get_building_bonus);
    SUITE_ADD_TEST(suite, test_attackers_get_no_building_bonus);
    SUITE_ADD_TEST(suite, test_building_bonus_respects_size);
    SUITE_ADD_TEST(suite, test_building_defence_bonus);
    SUITE_ADD_TEST(suite, test_calculate_armor);
    SUITE_ADD_TEST(suite, test_projectile_armor);
    return suite;
}
