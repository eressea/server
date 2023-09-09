#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "battle.h"

#include "magic.h"
#include "reports.h"

#include "spells/buildingcurse.h"

#include "kernel/config.h"
#include "kernel/building.h"
#include "kernel/faction.h"
#include "kernel/curse.h"
#include "kernel/item.h"
#include "kernel/order.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/ship.h"
#include "kernel/skill.h"
#include "kernel/skills.h"
#include "kernel/status.h"
#include "kernel/unit.h"

#include "util/base36.h"
#include "util/keyword.h"
#include "util/language.h"
#include "util/message.h"
#include "util/rand.h"
#include "util/variant.h"

#include <strings.h>

#include <CuTest.h>

#include <stb_ds.h>

#include <stdbool.h>
#include <stdio.h>

#include "tests.h"

static void setup_messages(void) {
    mt_create_va(mt_new("start_battle", NULL), "factions:string", MT_NEW_END);
    mt_create_va(mt_new("para_army_index", NULL), "index:int", "name:string", MT_NEW_END);
    mt_create_va(mt_new("battle_msg", NULL), "string:string", MT_NEW_END);
    mt_create_va(mt_new("battle_row", NULL), "row:int", MT_NEW_END);
    mt_create_va(mt_new("para_lineup_battle", NULL), "turn:int", MT_NEW_END);
    mt_create_va(mt_new("para_after_battle", NULL), MT_NEW_END);
    mt_create_va(mt_new("army_report", NULL), 
        "index:int",  "abbrev:string", "dead:int", "fled:int", "survived:int",
        MT_NEW_END);
    mt_create_va(mt_new("casualties", NULL), 
        "unit:unit", "runto:region", "run:int", "alive:int", "fallen:int",
        MT_NEW_END);
}

static void test_make_fighter(CuTest * tc)
{
    unit *au;
    region *r;
    fighter *af;
    battle *b;
    side *as;
    faction * f;
    const resource_type *rtype;

    test_setup();
    test_create_horse();
    r = test_create_plain(0, 0);
    f = test_create_faction();
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
    CuAssertPtrEquals(tc, NULL, af->building);
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
    test_teardown();
}

static void test_select_weapon(CuTest *tc) {
    item_type *it_missile, *it_axe, *it_sword;
    unit *au;
    fighter *af;
    battle *b;

    test_setup();
    au = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_number(au, 3);
    set_level(au, SK_MELEE, 1);
    it_axe = test_create_itemtype("axe");
    new_weapontype(it_axe, 0, frac_zero, NULL, 1, 0, 0, SK_MELEE);
    i_change(&au->items, it_axe, 1);
    it_sword = test_create_itemtype("sword");
    new_weapontype(it_sword, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE);
    i_change(&au->items, it_sword, 1);
    it_missile = test_create_itemtype("crossbow");
    new_weapontype(it_missile, WTF_MISSILE, frac_zero, NULL, 0, 0, 0, SK_CROSSBOW);
    i_change(&au->items, it_missile, 2);

    b = make_battle(au->region);
    af = make_fighter(b, au, make_side(b, au->faction, 0, 0, 0), false);
    CuAssertIntEquals(tc, 3, (int)arrlen(af->weapons));
    CuAssertPtrEquals(tc, it_axe, (item_type *)af->person[0].melee->item.type);
    CuAssertPtrEquals(tc, NULL, (weapon *)af->person[0].missile);
    CuAssertPtrEquals(tc, it_sword, (item_type *)af->person[1].melee->item.type);
    CuAssertPtrEquals(tc, it_missile, (item_type *)af->person[1].missile->item.type);
    CuAssertPtrEquals(tc, NULL, (weapon *)af->person[2].melee);
    CuAssertPtrEquals(tc, it_missile, (item_type *)af->person[2].missile->item.type);
    free_battle(b);

    test_teardown();
}

static void test_select_weapon(CuTest *tc) {
    item_type *it_missile, *it_axe, *it_sword;
    item *i_missile, *i_sword, *i_axe;
    unit *au;
    fighter *af;
    battle *b;

    test_setup();
    au = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_number(au, 3);
    set_level(au, SK_MELEE, 1);
    it_axe = test_create_itemtype("axe");
    new_weapontype(it_axe, 0, frac_zero, NULL, 1, 0, 0, SK_MELEE);
    i_axe = i_change(&au->items, it_axe, 1);
    it_sword = test_create_itemtype("sword");
    new_weapontype(it_sword, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE);
    i_sword = i_change(&au->items, it_sword, 1);
    it_missile = test_create_itemtype("crossbow");
    new_weapontype(it_missile, WTF_MISSILE, frac_zero, NULL, 0, 0, 0, SK_CROSSBOW);
    i_missile = i_change(&au->items, it_missile, 2);

    b = make_battle(au->region);
    af = make_fighter(b, au, make_side(b, au->faction, 0, 0, 0), false);
    CuAssertIntEquals(tc, 3, (int)arrlen(af->weapons));
    CuAssertPtrEquals(tc, it_axe, (item_type *)af->person[0].melee->item.type);
    CuAssertPtrEquals(tc, NULL, (weapon *)af->person[0].missile);
    CuAssertPtrEquals(tc, it_sword, (item_type *)af->person[1].melee->item.type);
    CuAssertPtrEquals(tc, it_missile, (item_type *)af->person[1].missile->item.type);
    CuAssertPtrEquals(tc, NULL, (weapon *)af->person[2].melee);
    CuAssertPtrEquals(tc, it_missile, (item_type *)af->person[2].missile->item.type);
    free_battle(b);

    test_teardown();
}

static void test_select_weapon_restricted(CuTest *tc) {
    item_type *itype;
    unit *au;
    fighter *af;
    battle *b;
    race * rc;

    test_setup();
    au = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    itype = test_create_itemtype("halberd");
    new_weapontype(itype, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE);
    i_change(&au->items, itype, 1);
    rc = test_create_race("smurf");
    CuAssertIntEquals(tc, 0, rc->mask_item & au->_race->mask_item);

    /* melee weapon, can be used by any race: */
    b = make_battle(au->region);
    af = make_fighter(b, au, make_side(b, au->faction, 0, 0, 0), false);
    CuAssertIntEquals(tc, 1, (int)arrlen(af->weapons));
    CuAssertPtrEquals(tc, (item_type *)au->items->type, (item_type *)af->weapons[0].item.type);
    CuAssertPtrEquals(tc, af->weapons, (void *)af->person[0].melee);
    free_battle(b);

    /* weapon is for denied to our race: */
    itype->mask_deny = rc_mask(au->_race);
    b = make_battle(au->region);
    af = make_fighter(b, au, make_side(b, au->faction, 0, 0, 0), false);
    CuAssertIntEquals(tc, 1, (int)arrlen(af->weapons));
    CuAssertPtrEquals(tc, (item_type *)au->items->type, (item_type *)af->weapons[0].item.type);
    CuAssertPtrNotNull(tc, af->person);
    CuAssertPtrEquals(tc, NULL, (void *)af->person[0].melee);
    free_battle(b);

    /* weapon is for exclusive use by our race: */
    itype->mask_deny = 0;
    itype->mask_allow = rc_mask(au->_race);
    b = make_battle(au->region);
    af = make_fighter(b, au, make_side(b, au->faction, 0, 0, 0), false);
    CuAssertPtrNotNull(tc, af->weapons);
    CuAssertIntEquals(tc, 1, (int)arrlen(af->weapons));
    CuAssertPtrEquals(tc, (item_type *)au->items->type, (item_type *)af->weapons[0].item.type);
    CuAssertPtrEquals(tc, af->weapons, (void *)af->person[0].melee);
    free_battle(b);

    /* weapon is for exclusive use by another race: */
    itype->mask_deny = 0;
    itype->mask_allow = rc_mask(rc);
    b = make_battle(au->region);
    af = make_fighter(b, au, make_side(b, au->faction, 0, 0, 0), false);
    CuAssertIntEquals(tc, 1, (int)arrlen(af->weapons));
    CuAssertPtrEquals(tc, (item_type *)au->items->type, (void *)af->weapons[0].item.type);
    CuAssertPtrEquals(tc, NULL, (void *)af->person[0].melee);
    free_battle(b);

    test_teardown();
}

static void test_select_armor(CuTest *tc) {
    item_type *itype, *iscale;
    unit *au;
    fighter *af;
    battle *b;

    test_setup();
    au = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    itype = test_create_itemtype("plate");
    new_armortype(itype, 0.0, frac_zero, 1, 0);
    i_change(&au->items, itype, 2);
    iscale = test_create_itemtype("scale");
    new_armortype(iscale, 0.0, frac_zero, 2, 0);
    i_change(&au->items, iscale, 1);

    b = make_battle(au->region);
    af = make_fighter(b, au, make_side(b, au->faction, 0, 0, 0), false);
    CuAssertPtrNotNull(tc, af->armors);
    CuAssertIntEquals(tc, 1, af->armors->count);
    CuAssertPtrEquals(tc, iscale->rtype->atype, (armor_type *)af->armors->atype);
    CuAssertIntEquals(tc, 2, af->armors->next->count);
    CuAssertPtrEquals(tc, itype->rtype->atype, (armor_type *)af->armors->next->atype);
    CuAssertPtrEquals(tc, NULL, af->armors->next->next);
    free_battle(b);

    test_teardown();
}

static building_type * setup_castle(void) {
    return test_create_castle();
}

static void test_defenders_get_building_bonus(CuTest * tc)
{
    unit *du, *au;
    region *r;
    building * bld;
    fighter *df, *af;
    battle *b;
    side *ds, *as;
    troop dt, at;
    building_type * btype;

    test_setup();
    btype = setup_castle();
    r = test_create_plain(0, 0);
    bld = test_create_building(r, btype);

    du = test_create_unit(test_create_faction(), r);
    au = test_create_unit(test_create_faction(), r);
    u_set_building(du, bld);

    b = make_battle(r);
    ds = make_side(b, du->faction, 0, 0, 0);
    df = make_fighter(b, du, ds, false);
    as = make_side(b, au->faction, 0, 0, 0);
    af = make_fighter(b, au, as, true);

    CuAssertPtrEquals(tc, bld, df->building);
    CuAssertPtrEquals(tc, NULL, af->building);

    dt.fighter = df;
    dt.index = 0;
    at.fighter = af;
    at.index = 0;

    bld->size = 10; /* stage 2 building */
    CuAssertIntEquals(tc, 2, buildingeffsize(bld, false));
    CuAssertIntEquals(tc, -1, skilldiff(at, dt, 0));
    CuAssertIntEquals(tc, 0, skilldiff(dt, at, 0));

    bld->size = 9; /* stage 1 building */
    CuAssertIntEquals(tc, 1, buildingeffsize(bld, false));
    CuAssertIntEquals(tc, 0, skilldiff(at, dt, 0));
    CuAssertIntEquals(tc, 0, skilldiff(dt, at, 0));

    bld->size = 1; /* stage 0 building */
    CuAssertIntEquals(tc, 0, buildingeffsize(bld, false));
    CuAssertIntEquals(tc, 0, skilldiff(at, dt, 0));
    CuAssertIntEquals(tc, 0, skilldiff(dt, at, 0));

    free_battle(b);
    test_teardown();
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

    test_setup();
    r = test_create_plain(0, 0);
    btype = setup_castle();
    btype->flags |= BTF_FORTIFICATION;
    bld = test_create_building(r, btype);
    bld->size = 10;

    au = test_create_unit(test_create_faction(), r);
    u_set_building(au, bld);

    b = make_battle(r);
    as = make_side(b, au->faction, 0, 0, 0);
    af = make_fighter(b, au, as, true);

    CuAssertPtrEquals(tc, NULL, af->building);
    free_battle(b);
    test_teardown();
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

    test_setup();
    btype = setup_castle();
    r = test_create_plain(0, 0);
    btype->flags |= BTF_FORTIFICATION;
    bld = test_create_building(r, btype);
    bld->size = 10;

    f = test_create_faction();
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
    CuAssertPtrEquals(tc, NULL, df->building);
    free_battle(b);
    test_teardown();
}

static void test_building_defense_bonus(CuTest * tc)
{
    building_type * btype;

    test_setup();
    btype = setup_castle();

    btype->maxsize = -1; /* unlimited buildings get the castle bonus */
    CuAssertIntEquals(tc, 0, bt_protection(btype, 0));
    CuAssertIntEquals(tc, 0, bt_protection(btype, 1));
    CuAssertIntEquals(tc, 1, bt_protection(btype, 2));
    CuAssertIntEquals(tc, 2, bt_protection(btype, 3));
    CuAssertIntEquals(tc, 3, bt_protection(btype, 4));
    CuAssertIntEquals(tc, 4, bt_protection(btype, 5));
    CuAssertIntEquals(tc, 5, bt_protection(btype, 6));
    CuAssertIntEquals(tc, 5, bt_protection(btype, 7)); /* illegal castle size */

    btype->maxsize = 10; /* limited-size buildings are treated like an E3 watchtower */
    CuAssertIntEquals(tc, 0, bt_protection(btype, 0));
    CuAssertIntEquals(tc, 1, bt_protection(btype, 1));
    CuAssertIntEquals(tc, 2, bt_protection(btype, 2));
    CuAssertIntEquals(tc, 2, bt_protection(btype, 3));
    test_teardown();
}

static fighter *setup_fighter(battle **bp, unit *u) {
    battle *b = *bp;
    side *s;

    if (!b) {
        *bp = b = make_battle(u->region);
    }
    s = make_side(b, u->faction, 0, 0, 0);
    return make_fighter(b, u, s, false);
}

static void test_natural_armor(CuTest * tc)
{
    race *rc;
    unit *u;

    test_setup();
    rc = test_create_race("human");
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));
    set_level(u, SK_STAMINA, 2);
    CuAssertIntEquals(tc, 0, rc_armor_bonus(rc));
    CuAssertIntEquals(tc, 0, natural_armor(u));
    rc_set_param(rc, "armor.stamina", "1");
    CuAssertIntEquals(tc, 1, rc_armor_bonus(rc));
    CuAssertIntEquals(tc, 2, natural_armor(u));
    rc_set_param(rc, "armor.stamina", "2");
    CuAssertIntEquals(tc, 2, rc_armor_bonus(rc));
    CuAssertIntEquals(tc, 1, natural_armor(u));
    test_teardown();
}

static int test_armor(troop dt, weapon_type *awtype, bool magic) {
  return calculate_armor(dt, 0, awtype, select_armor(dt, false), select_armor(dt, true), magic);
}

static int test_resistance(troop dt, bool magic) {
    const weapon *dw = select_weapon(dt, false, true);
    return apply_resistance(1000, dt,
        dw ? WEAPON_TYPE(dw) : NULL,
        select_armor(dt, false),
        select_armor(dt, true), magic);
}

static void test_calculate_armor(CuTest * tc)
{
    troop dt;
    battle *b = NULL;
    region *r;
    unit *du;
    weapon_type *wtype;
    armor_type *ashield, *achain;
    item_type *ibelt, *ishield, *ichain;
    race *rc;
    variant v50p = frac_make(1, 2);

    test_setup();
    r = test_create_plain(0, 0);
    ibelt = it_get_or_create(rt_get_or_create("trollbelt"));
    ishield = it_get_or_create(rt_get_or_create("shield"));
    ashield = new_armortype(ishield, 0.0, v50p, 1, ATF_SHIELD);
    ichain = it_get_or_create(rt_get_or_create("chainmail"));
    achain = new_armortype(ichain, 0.0, v50p, 3, ATF_NONE);
    wtype = new_weapontype(it_get_or_create(rt_get_or_create("sword")), 0, v50p, 0, 0, 0, 0, SK_MELEE);
    rc = test_create_race("human");
    du = test_create_unit(test_create_faction_ex(rc, NULL), r);
    dt.index = 0;

    dt.fighter = setup_fighter(&b, du);
    CuAssertIntEquals_Msg(tc, "default ac", 0, test_armor(dt, 0, false));

    CuAssertIntEquals_Msg(tc, "magres unmodified", 1000, test_resistance(dt, true));
    free_battle(b);

    b = NULL;
    i_change(&du->items, ibelt, 1);
    dt.fighter = setup_fighter(&b, du);
    CuAssertIntEquals_Msg(tc, "without natural armor", 0, natural_armor(du));
    CuAssertIntEquals_Msg(tc, "magical armor", 1, test_armor(dt, 0, false));
    rc->armor = 2;
    CuAssertIntEquals_Msg(tc, "with natural armor", 2, natural_armor(du));
    CuAssertIntEquals_Msg(tc, "natural armor", 3, test_armor(dt, 0, false));
    rc->armor = 0;
    free_battle(b);

    b = NULL;
    i_change(&du->items, ishield, 1);
    i_change(&du->items, ichain, 1);
    dt.fighter = setup_fighter(&b, du);
    rc->battle_flags &= ~BF_EQUIPMENT;
    CuAssertIntEquals_Msg(tc, "require BF_EQUIPMENT", 1, test_armor(dt, 0, false));
    free_battle(b);

    b = NULL;
    rc->battle_flags |= BF_EQUIPMENT;
    dt.fighter = setup_fighter(&b, du);
    CuAssertIntEquals_Msg(tc, "stack equipment rc", 5, test_armor(dt, 0, false));
    rc->armor = 2;
    CuAssertIntEquals_Msg(tc, "natural armor adds 50%", 6, test_armor(dt, 0, false));
    wtype->flags = WTF_NONE;
    CuAssertIntEquals_Msg(tc, "regular weapon has no effect", 6, test_armor(dt, wtype, false));
    wtype->flags = WTF_ARMORPIERCING;
    CuAssertIntEquals_Msg(tc, "armor piercing weapon", 3, test_armor(dt, wtype, false));
    wtype->flags = WTF_NONE;

    CuAssertIntEquals_Msg(tc, "magical attack", 3, test_armor(dt, wtype, true));
    CuAssertIntEquals_Msg(tc, "magres unmodified", 1000,
        test_resistance(dt, true));

    ashield->flags |= ATF_LAEN;
    achain->flags |= ATF_LAEN;

    CuAssertIntEquals_Msg(tc, "laen armor", 3, test_armor(dt, wtype, true));
    CuAssertIntEquals_Msg(tc, "laen magres bonus", 250, test_resistance(dt, true));
    free_battle(b);
    test_teardown();
}

static void test_spells_reduce_damage(CuTest *tc)
{
    struct meffect me;

    me.typ = SHIELD_ARMOR;
    me.duration = 10;
    me.effect = 5;
    CuAssertIntEquals(tc, 5, meffect_apply(&me, 10));
    CuAssertIntEquals(tc, 9, me.duration);
    CuAssertIntEquals(tc, 5, me.effect);
    CuAssertIntEquals(tc, 0, meffect_apply(&me, 1));
    CuAssertIntEquals(tc, 8, me.duration);
    CuAssertIntEquals(tc, 5, me.effect);

    me.typ = SHIELD_REDUCE;
    me.duration = 10;
    me.effect = 50;
    CuAssertIntEquals(tc, 5, meffect_apply(&me, 10));
    CuAssertIntEquals(tc, 5, me.duration);
    CuAssertIntEquals(tc, 50, me.effect);
    CuAssertIntEquals(tc, 7, meffect_apply(&me, 12));
    CuAssertIntEquals(tc, 0, me.duration);
}

static void test_magic_resistance(CuTest *tc)
{
    troop dt;
    battle *b = NULL;
    region *r;
    unit *du;
    armor_type *ashield, *achain;
    item_type *ishield, *ichain;
    race *rc;
    variant magres;
    variant v50p = frac_make(1, 2);
    variant v10p = frac_make(1, 10);

    test_setup();
    r = test_create_plain(0, 0);
    ishield = it_get_or_create(rt_get_or_create("shield"));
    ashield = new_armortype(ishield, 0.0, v50p, 1, ATF_SHIELD);
    ichain = it_get_or_create(rt_get_or_create("chainmail"));
    achain = new_armortype(ichain, 0.0, v50p, 3, ATF_NONE);
    rc = test_create_race("human");
    du = test_create_unit(test_create_faction_ex(rc, NULL), r);
    dt.index = 0;

    i_change(&du->items, ishield, 1);
    dt.fighter = setup_fighter(&b, du);
    CuAssertIntEquals_Msg(tc, "no magres reduction", 1000, test_resistance(dt, true));
    magres = magic_resistance(du);
    CuAssertIntEquals_Msg(tc, "no magres reduction", 0, magres.sa[0]);

    ashield->flags |= ATF_LAEN;
    ashield->magres = v10p;
    CuAssertIntEquals_Msg(tc, "laen reduction => 10%%", 900, test_resistance(dt, true));
    CuAssertIntEquals_Msg(tc, "no magic, no resistance", 1000,
        test_resistance(dt, false));
    free_battle(b);

    b = NULL;
    i_change(&du->items, ichain, 1);
    achain->flags |= ATF_LAEN;
    achain->magres = v10p;
    ashield->flags |= ATF_LAEN;
    ashield->magres = v10p;
    dt.fighter = setup_fighter(&b, du);
    CuAssertIntEquals_Msg(tc, "2x laen reduction => 81%%", 810, test_resistance(dt, true));
    free_battle(b);

    b = NULL;
    i_change(&du->items, ishield, -1);
    i_change(&du->items, ichain, -1);
    set_level(du, SK_MAGIC, 2);
    dt.fighter = setup_fighter(&b, du);
    CuAssertIntEquals_Msg(tc, "skill reduction => 90%%", 900, test_resistance(dt, true));
    magres = magic_resistance(du);
    CuAssert(tc, "skill reduction", frac_equal(magres, v10p));
    rc->magres = v50p; /* percentage, gets added to skill bonus */
    CuAssertIntEquals_Msg(tc, "race reduction => 40%%", 400, test_resistance(dt, true));
    magres = magic_resistance(du);
    CuAssert(tc, "race bonus => 60%%", frac_equal(magres, frac_make(60, 100)));

    rc->magres = frac_make(15, 10); /* 150% resistance should not cause negative damage multiplier */
    magres = magic_resistance(du);
    CuAssert(tc, "magic resistance is never > 0.9", frac_equal(magres, frac_make(9, 10)));
    CuAssertIntEquals_Msg(tc, "damage reduction is never < 0.1", 100, test_resistance(dt, true));

    free_battle(b);
    test_teardown();
}

static void test_projectile_armor(CuTest * tc)
{
    troop dt;
    battle *b = NULL;
    region *r;
    unit *du;
    weapon_type *wtype;
    armor_type *ashield, *achain;
    item_type *ishield, *ichain;
    race *rc;
    variant v50p = frac_make(1, 2);

    test_setup();
    r = test_create_plain(0, 0);
    ishield = it_get_or_create(rt_get_or_create("shield"));
    ashield = new_armortype(ishield, 0.0, v50p, 1, ATF_SHIELD);
    ichain = it_get_or_create(rt_get_or_create("chainmail"));
    achain = new_armortype(ichain, 0.0, v50p, 3, ATF_NONE);
    wtype = new_weapontype(it_get_or_create(rt_get_or_create("sword")), 0, v50p, 0, 0, 0, 0, SK_MELEE);
    rc = test_create_race("human");
    rc->battle_flags |= BF_EQUIPMENT;
    du = test_create_unit(test_create_faction_ex(rc, NULL), r);
    dt.index = 0;

    i_change(&du->items, ishield, 1);
    i_change(&du->items, ichain, 1);
    dt.fighter = setup_fighter(&b, du);
    wtype->flags = WTF_MISSILE;
    achain->projectile = 1.0;
    CuAssertIntEquals_Msg(tc, "projectile armor", -1, test_armor(dt, wtype, false));
    achain->projectile = 0.0;
    ashield->projectile = 1.0;
    CuAssertIntEquals_Msg(tc, "projectile shield", -1, test_armor(dt, wtype, false));
    wtype->flags = WTF_NONE;
    CuAssertIntEquals_Msg(tc, "no projectiles", 4, test_armor(dt, wtype, false));
    free_battle(b);
    test_teardown();
}

static void test_battle_skilldiff(CuTest *tc)
{
    troop ta, td;
    region *r;
    unit *ua, *ud;
    battle *b = NULL;

    test_setup();

    r = test_create_plain(0, 0);
    ud = test_create_unit(test_create_faction(), r);
    ua = test_create_unit(test_create_faction(), r);
    td.fighter = setup_fighter(&b, ud);
    td.index = 0;
    ta.fighter = setup_fighter(&b, ua);
    ta.index = 0;
    CuAssertIntEquals(tc, 0, skilldiff(ta, td, 0));

    ta.fighter->person[0].attack = 2;
    td.fighter->person[0].defense = 1;
    CuAssertIntEquals(tc, 1, skilldiff(ta, td, 0));

    td.fighter->person[0].flags |= FL_SLEEPING;
    CuAssertIntEquals(tc, 3, skilldiff(ta, td, 0));

    /* TODO: unarmed halfling vs. dragon: +5 */
    /* TODO: rule_goblin_bonus */
    /* TODO: weapon modifiers, missiles, skill_formula */

    free_battle(b);
    test_teardown();
}

static void test_loot_items(CuTest* tc)
{
    troop ta, td;
    region* r;
    faction *f;
    unit* ua, * ud;
    battle* b = NULL;
    const resource_type* rtype;
    race* rc;

    test_setup();
    config_set_int("rules.items.loot_divisor", 1); // everything is looted 100%
    test_create_horse();
    rc = test_create_race("ghost");
    rc->flags |= RCF_FLY; /* bug 2887 */

    r = test_create_plain(0, 0);
    ud = test_create_unit(f = test_create_faction(), r);
    ud->status = ST_FLEE; /* bug 2887 */
    ua = test_create_unit(f, r);
    u_setrace(ua, rc);
    td.fighter = setup_fighter(&b, ud);
    td.index = 0;
    ta.fighter = setup_fighter(&b, ua);
    ta.index = 0;

    ta.fighter->alive = 0;

    rtype = get_resourcetype(R_HORSE);
    i_change(&ua->items, rtype->itype, 1);
    loot_items(ta.fighter);
    CuAssertIntEquals(tc, 1, i_get(td.fighter->loot, rtype->itype));
    CuAssertIntEquals(tc, 0, i_get(ua->items, rtype->itype));

    free_battle(b);
    test_teardown();
}

static void test_loot_notlost_items(CuTest* tc)
{
    troop ta, td;
    region* r;
    unit* ua, * ud;
    battle* b = NULL;
    const resource_type* rtype;
    race* rc;

    test_setup();
    config_set_int("rules.items.loot_divisor", 0); // nothing is looted
    test_create_horse();
    rc = test_create_race("ghost");
    rc->flags |= RCF_FLY; /* bug 2887 */

    r = test_create_plain(0, 0);
    ud = test_create_unit(test_create_faction(), r);
    ud->status = ST_FLEE; /* bug 2887 */
    ua = test_create_unit(test_create_faction(), r);
    u_setrace(ua, rc);
    td.fighter = setup_fighter(&b, ud);
    td.index = 0;
    ta.fighter = setup_fighter(&b, ua);
    ta.index = 0;

    ta.fighter->alive = 0;
    ta.fighter->side->relations[td.fighter->side->index] |= E_ENEMY;
    ta.fighter->side->enemies[0] = td.fighter->side;
    ta.fighter->side->enemies[1] = NULL;
    td.fighter->side->relations[ta.fighter->side->index] |= E_ENEMY;
    td.fighter->side->enemies[0] = ta.fighter->side;
    td.fighter->side->enemies[1] = NULL;

    rtype = get_resourcetype(R_HORSE);
    rtype->itype->flags |= ITF_NOTLOST; /* must always be looted */
    i_change(&ua->items, rtype->itype, 1);
    loot_items(ta.fighter);
    CuAssertIntEquals(tc, 1, i_get(td.fighter->loot, rtype->itype));
    CuAssertIntEquals(tc, 0, i_get(ua->items, rtype->itype));

    free_battle(b);
    test_teardown();
}

static void test_loot_cursed_items_self(CuTest* tc)
{
    troop ta, td;
    region* r;
    faction *f;
    unit* ua, * ud;
    battle* b = NULL;
    const resource_type* rtype;
    race* rc;

    test_setup();
    config_set_int("rules.items.loot_divisor", 1); // everything is looted
    test_create_horse();
    rc = test_create_race("ghost");
    rc->flags |= RCF_FLY; /* bug 2887 */

    r = test_create_plain(0, 0);
    ud = test_create_unit(f = test_create_faction(), r);
    ud->status = ST_FLEE; /* bug 2887 */
    ua = test_create_unit(f, r);
    u_setrace(ua, rc);
    td.fighter = setup_fighter(&b, ud);
    td.index = 0;
    ta.fighter = setup_fighter(&b, ua);
    ta.index = 0;

    ta.fighter->alive = 0;

    rtype = get_resourcetype(R_HORSE);
    rtype->itype->flags |= ITF_CURSED; /* must be looted by own faction */
    i_change(&ua->items, rtype->itype, 1);
    loot_items(ta.fighter);
    CuAssertIntEquals(tc, 1, i_get(td.fighter->loot, rtype->itype));
    CuAssertIntEquals(tc, 0, i_get(ua->items, rtype->itype));

    free_battle(b);
    test_teardown();
}

static void test_loot_cursed_items_other(CuTest* tc)
{
    troop ta, td;
    region* r;
    unit* ua, * ud;
    battle* b = NULL;
    const resource_type* rtype;
    race* rc;

    test_setup();
    config_set_int("rules.items.loot_divisor", 1); // everything is looted
    test_create_horse();
    rc = test_create_race("ghost");
    rc->flags |= RCF_FLY; /* bug 2887 */

    r = test_create_plain(0, 0);
    ud = test_create_unit(test_create_faction(), r);
    ud->status = ST_FLEE; /* bug 2887 */
    ua = test_create_unit(test_create_faction(), r);
    u_setrace(ua, rc);
    td.fighter = setup_fighter(&b, ud);
    td.index = 0;
    ta.fighter = setup_fighter(&b, ua);
    ta.index = 0;

    ta.fighter->alive = 0;

    rtype = get_resourcetype(R_HORSE);
    rtype->itype->flags |= ITF_CURSED; /* must not be looted */
    i_change(&ua->items, rtype->itype, 1);
    loot_items(ta.fighter);
    CuAssertIntEquals(tc, 0, i_get(td.fighter->loot, rtype->itype));
    CuAssertIntEquals(tc, 0, i_get(ua->items, rtype->itype));

    free_battle(b);
    test_teardown();
}

static void test_no_loot_from_fleeing(CuTest* tc)
{
    troop ta, td;
    region* r;
    unit* ua, * ud;
    battle* b = NULL;
    const resource_type* rtype;
    race* rc;

    test_setup();
    test_create_horse();
    rc = test_create_race("ghost");
    rc->flags |= RCF_FLY; /* bug 2887 */

    r = test_create_plain(0, 0);
    ud = test_create_unit(test_create_faction(), r);
    ud->status = ST_FLEE; /* bug 2887 */
    ua = test_create_unit(test_create_faction(), r);
    u_setrace(ua, rc);
    td.fighter = setup_fighter(&b, ud);
    td.index = 0;
    ta.fighter = setup_fighter(&b, ua);
    ta.index = 0;

    ta.fighter->side->relations[td.fighter->side->index] |= E_ENEMY;
    ta.fighter->side->enemies[0] = td.fighter->side;
    ta.fighter->side->enemies[1] = NULL;
    td.fighter->side->relations[ta.fighter->side->index] |= E_ENEMY;
    td.fighter->side->enemies[0] = ta.fighter->side;
    td.fighter->side->enemies[1] = NULL;

    flee_all(ta.fighter);

    rtype = get_resourcetype(R_HORSE);
    rtype->itype->flags |= ITF_NOTLOST; /* must always be looted */
    i_change(&ua->items, rtype->itype, 1);
    loot_items(ta.fighter);
    CuAssertIntEquals(tc, 0, i_get(td.fighter->loot, rtype->itype));
    CuAssertIntEquals(tc, 1, i_get(ua->items, rtype->itype));

    free_battle(b);
    test_teardown();
}

static void test_terminate(CuTest * tc)
{
    troop at, dt;
    battle *b = NULL;
    region *r;
    unit *au, *du;
    race *rc;

    test_setup();
    r = test_create_plain(0, 0);

    rc = test_create_race("human");
    au = test_create_unit(test_create_faction_ex(rc, NULL), r);
    du = test_create_unit(test_create_faction_ex(rc, NULL), r);
    dt.index = 0;
    at.index = 0;

    at.fighter = setup_fighter(&b, au);
    dt.fighter = setup_fighter(&b, du);

    CuAssertIntEquals_Msg(tc, "not killed", 0, terminate(dt, at, AT_STANDARD, "1d1", false));
    b = NULL;
    at.fighter = setup_fighter(&b, au);
    dt.fighter = setup_fighter(&b, du);
    CuAssertIntEquals_Msg(tc, "killed", 1, terminate(dt, at, AT_STANDARD, "100d1", false));
    CuAssertIntEquals_Msg(tc, "number", 0, dt.fighter->person[0].hp);

    free_battle(b);
    test_teardown();
}


static void test_battle_report_one(CuTest *tc)
{
    battle * b = NULL;
    region *r;
    unit *u1, *u2;
    message *m;
    const char *expect;
    fighter *fig;

    test_setup();
    setup_messages();
    r = test_create_plain(0, 0);
    u1 = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), r);
    b = make_battle(r);
    join_battle(b, u1, true, &fig);
    join_battle(b, u2, false, &fig);
    faction_setname(u1->faction, "Monster");
    expect = factionname(u1->faction);

    report_battle_start(b);
    CuAssertPtrNotNull(tc, m = test_find_messagetype(u1->faction->battles->msgs, "start_battle"));
    CuAssertStrEquals(tc, expect, (const char *)m->parameters[0].v);

    free_battle(b);
    test_teardown();
}

static void test_battle_report_two(CuTest *tc)
{
    battle * b = NULL;
    region *r;
    unit *u1, *u2;
    message *m;
    char expect[64];
    fighter *fig;
    struct locale *lang;

    test_setup();
    lang = test_create_locale();
    locale_setstring(lang, "and", "and");
    setup_messages();
    r = test_create_plain(0, 0);
    u1 = test_create_unit(test_create_faction_ex(NULL, lang), r);
    u2 = test_create_unit(test_create_faction_ex(NULL, lang), r);
    u2->faction->locale = lang;

    str_slprintf(expect, sizeof(expect), "%s and %s", factionname(u1->faction), factionname(u2->faction));
    b = make_battle(r);
    join_battle(b, u1, true, &fig);
    join_battle(b, u2, true, &fig);
    report_battle_start(b);

    CuAssertPtrNotNull(tc, m = test_find_messagetype(u1->faction->battles->msgs, "start_battle"));
    CuAssertStrEquals(tc, expect, (const char *)m->parameters[0].v);

    free_battle(b);
    test_teardown();
}

static void test_battle_report_three(CuTest *tc)
{
    battle * b = NULL;
    region *r;
    unit *u1, *u2, *u3;
    message *m;
    char expect[64];
    fighter *fig;
    struct locale *lang;

    test_setup();
    lang = test_create_locale();
    locale_setstring(lang, "and", "and");
    setup_messages();
    r = test_create_plain(0, 0);
    u1 = test_create_unit(test_create_faction(), r);
    u1->faction->locale = lang;
    u2 = test_create_unit(test_create_faction(), r);
    u2->faction->locale = lang;
    u3 = test_create_unit(test_create_faction(), r);
    u3->faction->locale = lang;

    str_slprintf(expect, sizeof(expect), "%s, %s and %s", factionname(u1->faction), factionname(u2->faction), factionname(u3->faction));
    b = make_battle(r);
    join_battle(b, u1, true, &fig);
    join_battle(b, u2, true, &fig);
    join_battle(b, u3, true, &fig);
    report_battle_start(b);

    CuAssertPtrNotNull(tc, m = test_find_messagetype(u1->faction->battles->msgs, "start_battle"));
    CuAssertStrEquals(tc, expect, (const char *)m->parameters[0].v);

    free_battle(b);
    test_teardown();
}

static void test_battle_skilldiff_building(CuTest *tc)
{
    troop ta, td;
    region *r;
    unit *ua, *ud;
    battle *b = NULL;
    building_type *btype;

    test_setup();
    btype = setup_castle();

    r = test_create_plain(0, 0);
    ud = test_create_unit(test_create_faction(), r);
    ud->building = test_create_building(ud->region, btype);
    ua = test_create_unit(test_create_faction(), r);
    td.fighter = setup_fighter(&b, ud);
    td.index = 0;
    ta.fighter = setup_fighter(&b, ua);
    ta.index = 0;
    CuAssertIntEquals(tc, 0, buildingeffsize(ud->building, false));
    CuAssertIntEquals(tc, 0, skilldiff(ta, td, 0));

    ud->building->size = 10;
    CuAssertIntEquals(tc, 2, buildingeffsize(ud->building, false));
    CuAssertIntEquals(tc, 1, building_protection(ud->building));
    CuAssertIntEquals(tc, -1, skilldiff(ta, td, 0));

    create_curse(ud, &ud->building->attribs, &ct_magicwalls, 1, 1, 1, 1);
    CuAssertIntEquals(tc, -2, skilldiff(ta, td, 0));

    create_curse(ud, &ud->building->attribs, &ct_strongwall, 1, 1, 2, 1);
    CuAssertIntEquals(tc, -4, skilldiff(ta, td, 0));

    free_battle(b);
    test_teardown();
}

static void assert_skill(CuTest *tc, const char *msg, unit *u, skill_t sk, unsigned int level, unsigned int week, unsigned int weekmax)
{
    skill *sv = unit_skill(u, sk);
    if (sv) {
        char buf[256];
        sprintf(buf, "%s level %d != %d", msg, sv->level, level);
        CuAssertIntEquals_Msg(tc, buf, level, sv->level);
        sprintf(buf, "%s week %d !<= %d !<= %d", msg, week, sv->weeks, weekmax);
        CuAssert(tc, buf, sv->weeks >= week && sv->weeks <= weekmax);
    }
    else {
        CuAssertIntEquals_Msg(tc, msg, level, 0);
        CuAssertIntEquals_Msg(tc, msg, week, 0);
    }
}

static void test_drain_exp(CuTest *tc)
{
    unit *u;
    const char *msg;
    int i;
    double rand;

    test_setup();
    config_set("study.random_progress", "0");
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_level(u, SK_STAMINA, 3);

    CuAssertIntEquals(tc, 3, unit_skill(u, SK_STAMINA)->level);
    CuAssertIntEquals(tc, 4, unit_skill(u, SK_STAMINA)->weeks);

    assert_skill(tc, msg = "base", u, SK_STAMINA, 3, 4, 4);
    assert_skill(tc, msg, u, SK_STAMINA, 3, 4, 4);
    assert_skill(tc, msg, u, SK_MINING, 0, 0, 0);

    for (i = 0; i < 10; ++i) {
        set_level(u, SK_STAMINA, 3);
        drain_exp(u, 0);
        assert_skill(tc, msg = "0 change", u, SK_STAMINA, 3, 4, 4);
        assert_skill(tc, msg, u, SK_MINING, 0, 0, 0);

        for (rand = 0.0; rand < 2.0; rand += 1) {
            random_source_inject_constant(rand);

            set_level(u, SK_STAMINA, 3);
            drain_exp(u, 29);

            assert_skill(tc, msg = "no change yet", u, SK_STAMINA, 3, 4, rand == 0.0 ? 4 : 5);
            assert_skill(tc, msg, u, SK_MINING, 0, 0, 0);

            set_level(u, SK_STAMINA, 3);
            drain_exp(u, 1);

            assert_skill(tc, msg = "random change", u, SK_STAMINA, 3, 4, rand == 0.0 ? 4 : 5);
            assert_skill(tc, msg, u, SK_MINING, 0, 0, 0);

            set_level(u, SK_STAMINA, 3);
            drain_exp(u, 30);

            assert_skill(tc, msg = "plus one", u, SK_STAMINA, 3, 5, 5);
            assert_skill(tc, msg, u, SK_MINING, 0, 0, 0);
        }

        set_level(u, SK_STAMINA, 3);
        drain_exp(u, 90);

        assert_skill(tc, msg = "plus three", u, SK_STAMINA, 3, 7, 7);
        assert_skill(tc, msg, u, SK_MINING, 0, 0, 0);

        set_level(u, SK_STAMINA, 3);
        drain_exp(u, 120);

        assert_skill(tc, msg = "plus four", u, SK_STAMINA, 2, 5, 5);
        assert_skill(tc, msg, u, SK_MINING, 0, 0, 0);
    }
    test_teardown();
}

static void test_tactics_chance(CuTest *tc) {
    unit *u;
    ship_type *stype;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_ocean(0, 0));
    CuAssertDblEquals(tc, 0.1, tactics_chance(u, 1), 0.01);
    CuAssertDblEquals(tc, 0.3, tactics_chance(u, 3), 0.01);
    stype = test_create_shiptype("brot");
    u->ship = test_create_ship(u->region, stype);
    CuAssertDblEquals(tc, 0.2, tactics_chance(u, 2), 0.01);
    stype->tac_bonus = 2.0;
    CuAssertDblEquals(tc, 0.4, tactics_chance(u, 2), 0.01);
    test_teardown();
}

static void test_battle_attack_invisible(CuTest* tc) {
    region* r;
    unit* u1, * u2;
    faction *f;
    test_setup();
    setup_messages();
    r = test_create_plain(0, 0);
    u1 = test_create_unit(f = test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), test_create_plain(1, 1));
    unit_addorder(u1, create_order(K_ATTACK, f->locale, itoa36(u2->no)));
    do_battles();
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "feedback_unit_not_found"));
    test_clear_messages(f);
    move_unit(u2, r, NULL);
    do_battles();
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "feedback_unit_not_found"));
    test_teardown();
}

static void test_battle_fleeing(CuTest* tc) {
    region *r;
    unit *u1, *u2;
    test_setup();
    setup_messages();
    r = test_create_plain(0, 0);
    u1 = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), r);
    u1->status = ST_FLEE;
    u2->status = ST_AGGRO;
#if 0
    setguard(u1, true);
    CuAssertIntEquals(tc, UFL_GUARD, (u1->flags & UFL_GUARD));
    CuAssertIntEquals(tc, RF_GUARDED, (r->flags & RF_GUARDED));
#endif
    config_set_int("rules.combat.flee_chance_base", 100);
    config_set_int("rules.combat.flee_chance_limit", 100);
    unit_addorder(u2, create_order(K_ATTACK, u2->faction->locale, itoa36(u1->no)));
    do_battles();
    CuAssertIntEquals(tc, 1, u1->number);
    CuAssertIntEquals(tc, 1, u2->number);
#if 0
    CuAssertIntEquals(tc, 0, (u1->flags & UFL_GUARD));
    CuAssertIntEquals(tc, 0, (r->flags & RF_GUARDED));
#endif
    CuAssertIntEquals(tc, UFL_LONGACTION, (u1->flags & UFL_LONGACTION));
    CuAssertIntEquals(tc, UFL_LONGACTION | UFL_NOTMOVING, (u2->flags & (UFL_LONGACTION | UFL_NOTMOVING)));
    test_teardown();
}

CuSuite *get_battle_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_make_fighter);
    SUITE_ADD_TEST(suite, test_select_weapon);
    SUITE_ADD_TEST(suite, test_select_weapon_restricted);
    SUITE_ADD_TEST(suite, test_select_armor);
    SUITE_ADD_TEST(suite, test_battle_fleeing);
    SUITE_ADD_TEST(suite, test_battle_attack_invisible);
    SUITE_ADD_TEST(suite, test_battle_skilldiff);
    SUITE_ADD_TEST(suite, test_battle_skilldiff_building);
    SUITE_ADD_TEST(suite, test_battle_report_one);
    SUITE_ADD_TEST(suite, test_battle_report_two);
    SUITE_ADD_TEST(suite, test_battle_report_three);
    SUITE_ADD_TEST(suite, test_defenders_get_building_bonus);
    SUITE_ADD_TEST(suite, test_attackers_get_no_building_bonus);
    SUITE_ADD_TEST(suite, test_building_bonus_respects_size);
    SUITE_ADD_TEST(suite, test_building_defense_bonus);
    SUITE_ADD_TEST(suite, test_calculate_armor);
    SUITE_ADD_TEST(suite, test_natural_armor);
    SUITE_ADD_TEST(suite, test_magic_resistance);
    SUITE_ADD_TEST(suite, test_spells_reduce_damage);
    SUITE_ADD_TEST(suite, test_projectile_armor);
    SUITE_ADD_TEST(suite, test_tactics_chance);
    SUITE_ADD_TEST(suite, test_terminate);
    SUITE_ADD_TEST(suite, test_loot_items);
    SUITE_ADD_TEST(suite, test_loot_notlost_items);
    SUITE_ADD_TEST(suite, test_loot_cursed_items_self);
    SUITE_ADD_TEST(suite, test_loot_cursed_items_other);
    SUITE_ADD_TEST(suite, test_no_loot_from_fleeing);
    DISABLE_TEST(suite, test_drain_exp);
    return suite;
}
