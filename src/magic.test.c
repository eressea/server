#include "magic.h"

#include "contact.h"
#include "teleport.h"

#include <triggers/changerace.h>
#include <triggers/timeout.h>

#include <util/keyword.h>      // for K_CAST
#include <util/variant.h>      // for frac_make, frac_sub, frac_equal, variant
#include <util/language.h>

#include "kernel/skill.h"      // for SK_MAGIC, enable_skill, SK_STAMINA
#include "kernel/types.h"      // for M_TYBIED, M_GWYRRD, M_CERDDOR, M_GRAY
#include <kernel/ally.h>
#include <kernel/attrib.h>
#include <kernel/building.h>
#include <kernel/callbacks.h>
#include <kernel/equipment.h>
#include <kernel/event.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/skills.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/unit.h>
#include <kernel/objtypes.h>
#include <kernel/pool.h>

#include <strings.h>
#include <selist.h>
#include <stb_ds.h>

#include <tests.h>

#include <CuTest.h>

#include <stdbool.h>           // for true, bool
#include <stdlib.h>

void test_updatespells(CuTest * tc)
{
    faction * f;
    spell * sp;
    spellbook *book = NULL;

    test_setup();
    test_create_race("human");

    f = test_create_faction();
    sp = create_spell("testspell");
    CuAssertPtrNotNull(tc, sp);

    book = create_spellbook("spells");
    CuAssertPtrNotNull(tc, book);
    spellbook_add(book, sp, 1);

    CuAssertPtrEquals(tc, NULL, f->spellbook);
    pick_random_spells(f, 1, book, 1);
    CuAssertPtrNotNull(tc, f->spellbook);
    CuAssertIntEquals(tc, 1, (int) arrlen(f->spellbook->spells));
    CuAssertPtrNotNull(tc, spellbook_get(f->spellbook, sp));
    free_spellbook(book);
    test_teardown();
}

static void test_get_spellbook(CuTest * tc)
{
    spellbook *sb;

    test_setup();
    CuAssertPtrNotNull(tc, sb = get_spellbook("hodorhodorhodor"));
    CuAssertPtrEquals(tc, sb, get_spellbook("hodorhodorhodor"));
    CuAssertTrue(tc, sb != get_spellbook("hodor"));
    test_teardown();
}

static void test_spellbooks(CuTest * tc)
{
    spell *sp;
    spellbook *herp, *derp;
    spellbook_entry *entry;
    const char * sname = "herpderp";
    test_setup();

    herp = get_spellbook("herp");
    CuAssertPtrNotNull(tc, herp);
    CuAssertPtrEquals(tc, herp, get_spellbook("herp"));
    derp = get_spellbook("derp");
    CuAssertPtrNotNull(tc, derp);
    CuAssertTrue(tc, derp != herp);
    CuAssertStrEquals(tc, "herp", herp->name);
    CuAssertStrEquals(tc, "derp", derp->name);

    sp = create_spell(sname);
    spellbook_add(herp, sp, 1);
    CuAssertPtrNotNull(tc, sp);
    entry = spellbook_get(herp, sp);
    CuAssertPtrNotNull(tc, entry);
    CuAssertPtrEquals(tc, sp, spellref_get(&entry->spref));

    test_teardown();
    test_setup();

    herp = get_spellbook("herp");
    CuAssertPtrNotNull(tc, herp);

    test_teardown();
}

void test_pay_spell(CuTest * tc)
{
    spell *sp;
    unit * u;
    faction * f;
    region * r;
    int level;

    test_setup();
    init_resources();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);

    sp = test_create_spell();
    CuAssertPtrNotNull(tc, sp);

    set_level(u, SK_MAGIC, 5);
    unit_add_spell(u, sp, 1);

    change_resource(u, get_resourcetype(R_SILVER), 1);
    change_resource(u, get_resourcetype(R_AURA), 3);
    change_resource(u, get_resourcetype(R_HORSE), 3);

    level = max_spell_level(u, u, sp, 3, 1, NULL);
    CuAssertIntEquals(tc, 3, level);
    pay_spell(u, NULL, sp, level, 1);
    CuAssertIntEquals(tc, 0, get_resource(u, get_resourcetype(R_SILVER)));
    CuAssertIntEquals(tc, 0, get_resource(u, get_resourcetype(R_AURA)));
    CuAssertIntEquals(tc, 0, get_resource(u, get_resourcetype(R_HORSE)));
    test_teardown();
}

void test_pay_spell_failure(CuTest * tc)
{
    spell *sp;
    struct unit * u;
    struct faction * f;
    struct region * r;
    int level;

    test_setup();
    init_resources();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);

    sp = test_create_spell();
    CuAssertPtrNotNull(tc, sp);

    set_level(u, SK_MAGIC, 5);
    unit_add_spell(u, sp, 1);

    CuAssertIntEquals(tc, 1, change_resource(u, get_resourcetype(R_SILVER), 1));
    CuAssertIntEquals(tc, 2, change_resource(u, get_resourcetype(R_AURA), 2));
    CuAssertIntEquals(tc, 3, change_resource(u, get_resourcetype(R_HORSE), 3));

    level = max_spell_level(u, u, sp, 3, 1, NULL);
    CuAssertIntEquals(tc, 2, level);
    pay_spell(u, NULL, sp, level, 1);
    CuAssertIntEquals(tc, 1, change_resource(u, get_resourcetype(R_SILVER), 1));
    CuAssertIntEquals(tc, 3, change_resource(u, get_resourcetype(R_AURA), 3));
    CuAssertIntEquals(tc, 2, change_resource(u, get_resourcetype(R_HORSE), 1));

    CuAssertIntEquals(tc, 0, max_spell_level(u, u, sp, 3, 1, NULL));
    CuAssertIntEquals(tc, 0, change_resource(u, get_resourcetype(R_SILVER), -1));
    CuAssertIntEquals(tc, 0, max_spell_level(u, u, sp, 2, 1, NULL));
    test_teardown();
}

void test_getspell_unit(CuTest * tc)
{
    spell *sp;
    struct unit * u;
    struct faction * f;
    struct region * r;
    struct locale * lang;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    create_mage(u, M_GWYRRD);
    enable_skill(SK_MAGIC, true);

    set_level(u, SK_MAGIC, 1);

    lang = test_create_locale();
    sp = create_spell("testspell");
    locale_setstring(lang, mkname_spell(sp), "Herp-a-derp");

    CuAssertPtrEquals(tc, NULL, unit_getspell(u, "Herp-a-derp", lang));

    unit_add_spell(u, sp, 1);
    CuAssertPtrNotNull(tc, unit_getspell(u, "Herp-a-derp", lang));
    test_teardown();
}

void test_getspell_faction(CuTest * tc)
{
    spell *sp;
    struct unit * u;
    struct faction * f;
    struct region * r;
    struct locale * lang;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    create_mage(u, f->magiegebiet);
    enable_skill(SK_MAGIC, true);

    set_level(u, SK_MAGIC, 1);

    lang = test_create_locale();
    sp = create_spell("testspell");
    locale_setstring(lang, mkname("spell", sp->sname), "Herp-a-derp");

    CuAssertPtrEquals(tc, NULL, unit_getspell(u, "Herp-a-derp", lang));

    f->spellbook = create_spellbook(0);
    spellbook_add(f->spellbook, sp, 1);
    CuAssertPtrEquals(tc, sp, unit_getspell(u, "Herp-a-derp", lang));
    test_teardown();
}

void test_getspell_school(CuTest * tc)
{
    spell *sp;
    struct unit * u;
    struct faction * f;
    struct region * r;
    struct locale * lang;
    struct spellbook * book;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    create_mage(u, f->magiegebiet);
    enable_skill(SK_MAGIC, true);
    set_level(u, SK_MAGIC, 1);

    lang = test_create_locale();
    sp = create_spell("testspell");
    locale_setstring(lang, mkname_spell(sp), "Herp-a-derp");

    CuAssertPtrEquals(tc, NULL, unit_getspell(u, "Herp-a-derp", lang));

    book = faction_get_spellbook(f);
    CuAssertPtrNotNull(tc, book);
    spellbook_add(book, sp, 1);
    CuAssertPtrEquals(tc, sp, unit_getspell(u, "Herp-a-derp", lang));
    test_teardown();
}

void test_set_pre_combatspell(CuTest * tc)
{
    spell *sp;
    struct unit * u;
    struct faction * f;
    struct region * r;
    const int index = 0;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    enable_skill(SK_MAGIC, true);
    set_level(u, SK_MAGIC, 1);
    sp = create_spell("testspell");
    sp->sptyp |= PRECOMBATSPELL;

    unit_add_spell(u, sp, 1);

    set_combatspell(u, sp, 0, 2);
    CuAssertPtrEquals(tc, sp, (spell *)get_combatspell(u, index));
    set_level(u, SK_MAGIC, 2);
    CuAssertIntEquals(tc, 2, get_combatspelllevel(u, index));
    set_level(u, SK_MAGIC, 1);
    CuAssertIntEquals(tc, 1, get_combatspelllevel(u, index));
    unset_combatspell(u, sp);
    CuAssertIntEquals(tc, 0, get_combatspelllevel(u, index));
    CuAssertPtrEquals(tc, NULL, (spell *)get_combatspell(u, index));
    test_teardown();
}

void test_set_main_combatspell(CuTest * tc)
{
    spell *sp;
    struct unit * u;
    struct faction * f;
    struct region * r;
    const int index = 1;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    enable_skill(SK_MAGIC, true);
    set_level(u, SK_MAGIC, 1);
    sp = create_spell("testspell");
    sp->sptyp |= COMBATSPELL;

    unit_add_spell(u, sp, 1);

    set_combatspell(u, sp, 0, 2);
    CuAssertPtrEquals(tc, sp, (spell *)get_combatspell(u, index));
    set_level(u, SK_MAGIC, 2);
    CuAssertIntEquals(tc, 2, get_combatspelllevel(u, index));
    set_level(u, SK_MAGIC, 1);
    CuAssertIntEquals(tc, 1, get_combatspelllevel(u, index));
    unset_combatspell(u, sp);
    CuAssertIntEquals(tc, 0, get_combatspelllevel(u, index));
    CuAssertPtrEquals(tc, NULL, (spell *)get_combatspell(u, index));
    test_teardown();
}

void test_set_post_combatspell(CuTest * tc)
{
    spell *sp;
    struct unit * u;
    struct faction * f;
    struct region * r;
    const int index = 2;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    enable_skill(SK_MAGIC, true);
    set_level(u, SK_MAGIC, 1);
    sp = create_spell("testspell");
    sp->sptyp |= POSTCOMBATSPELL;

    unit_add_spell(u, sp, 1);

    set_combatspell(u, sp, 0, 2);
    CuAssertPtrEquals(tc, sp, (spell *)get_combatspell(u, index));
    set_level(u, SK_MAGIC, 2);
    CuAssertIntEquals(tc, 2, get_combatspelllevel(u, index));
    set_level(u, SK_MAGIC, 1);
    CuAssertIntEquals(tc, 1, get_combatspelllevel(u, index));
    unset_combatspell(u, sp);
    CuAssertIntEquals(tc, 0, get_combatspelllevel(u, index));
    CuAssertPtrEquals(tc, NULL, (spell *)get_combatspell(u, index));
    test_teardown();
}

void test_hasspell(CuTest * tc)
{
    spell *sp;
    struct unit * u;
    struct faction * f;
    struct region * r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    enable_skill(SK_MAGIC, true);
    sp = create_spell("testspell");
    sp->sptyp |= POSTCOMBATSPELL;

    unit_add_spell(u, sp, 2);

    set_level(u, SK_MAGIC, 1);
    CuAssertTrue(tc, !u_hasspell(u, sp));

    set_level(u, SK_MAGIC, 2);
    CuAssertTrue(tc, u_hasspell(u, sp));

    set_level(u, SK_MAGIC, 1);
    CuAssertTrue(tc, !u_hasspell(u, sp));
    test_teardown();
}

static selist * casts;

static int cast_fireball(struct castorder * co) {
    selist_push(&casts, co);
    return 0;
}

void test_multi_cast(CuTest *tc) {
    unit *u;
    spell *sp;
    struct locale * lang;

    test_setup();
    add_spellcast("fireball", cast_fireball);
    sp = create_spell("fireball");
    CuAssertPtrEquals(tc, sp, find_spell("fireball"));

    lang = test_create_locale();
    locale_setstring(lang, mkname_spell(sp), "Feuerball");
    CuAssertStrEquals(tc, "Feuerball", spell_name(mkname_spell(sp), lang));

    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_level(u, SK_MAGIC, 10);
    unit_add_spell(u, sp, 1);
    CuAssertPtrEquals(tc, sp, unit_getspell(u, "Feuerball", lang));

    unit_addorder(u, create_order(K_CAST, u->faction->locale, "Feuerball"));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, "Feuerball"));
    CuAssertPtrEquals(tc, casts, 0);
    magic();
    CuAssertPtrNotNull(tc, casts);
    CuAssertIntEquals(tc, 2, selist_length(casts));
    selist_free(casts);
    test_teardown();
}

static void test_resist_chance(CuTest *tc) {
    unit *u1, *u2, *u3, *u4;
    variant prob;

    test_setup();
    u1 = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), u1->region);
    u3 = test_create_unit(test_create_faction(), u1->region);
    u4 = test_create_unit(u1->faction, u1->region);

    /* my own units never resist: */
    prob = resist_chance(u1, u4, TYP_UNIT, 0);
    CuAssertIntEquals(tc, 0, prob.sa[0]);
    prob = resist_chance(u1, u4, TYP_UNIT, 10);
    CuAssertIntEquals(tc, 0, prob.sa[0]);

    /* allied units can still resist */
    ally_set(&u3->faction->allies, u1->faction, HELP_GUARD);
    prob = resist_chance(u1, u3, TYP_UNIT, 0);
    prob = frac_sub(prob, frac_make(1, 2)); /* resist at 50% base chance */
    CuAssertIntEquals(tc, 0, prob.sa[0]);
    prob = resist_chance(u1, u3, TYP_UNIT, 10);
    prob = frac_sub(prob, frac_make(3, 5)); /* resist at 10% + 50% base chance */
    CuAssertIntEquals(tc, 0, prob.sa[0]);
    ally_set(&u3->faction->allies, u1->faction, 0);

    contact_unit(u3, u1);
    prob = resist_chance(u1, u3, TYP_UNIT, 0);
    CuAssertIntEquals(tc, 0, prob.sa[0]);
    prob = resist_chance(u1, u3, TYP_UNIT, 10);
    CuAssertIntEquals(tc, 0, prob.sa[0]);

    /* 50% base chance for equal skills */
    prob = resist_chance(u1, u2, TYP_UNIT, 0);
    prob = frac_sub(prob, frac_make(1, 2));
    CuAssertIntEquals(tc, 0, prob.sa[0]);

    /* 8 levels better = 40% reduced */
    set_level(u1, SK_MAGIC, 8);
    prob = resist_chance(u1, u2, TYP_UNIT, 0);
    prob = frac_sub(prob, frac_make(1, 10));
    CuAssertIntEquals(tc, 0, prob.sa[0]);

    /* apply a 10% bonus */
    prob = resist_chance(u1, u2, TYP_UNIT, 10);
    prob = frac_sub(prob, frac_make(1, 5));
    CuAssertIntEquals(tc, 0, prob.sa[0]);

    /* never less than 2 percent */
    set_level(u1, SK_MAGIC, 10); /* negates the 50% base chance */
    prob = resist_chance(u1, u2, TYP_UNIT, 0);
    prob = frac_sub(prob, frac_make(1, 50));
    CuAssertIntEquals(tc, 0, prob.sa[0]);

    /* never more than 98 percent */
    prob = resist_chance(u1, u2, TYP_UNIT, 100);
    prob = frac_sub(prob, frac_make(98, 100));
    CuAssertIntEquals(tc, 0, prob.sa[0]);

    /* 5% bonus per best skill of defender */
    set_level(u1, SK_MAGIC, 10);
    set_level(u2, SK_ENTERTAINMENT, 10); /* negate magic skill */
    prob = resist_chance(u1, u2, TYP_UNIT, 0);
    prob = frac_sub(prob, frac_make(1, 2)); /* only 50% base chance remains */
    CuAssertIntEquals(tc, 0, prob.sa[0]);

    /* 5% bonus in magic resistance for each magic level */
    set_level(u2, SK_MAGIC, 4); /* makes for 20% */
    prob = magic_resistance(u2);
    prob = frac_sub(prob, frac_make(1, 5)); /* 20% from magic levels */
    CuAssertIntEquals(tc, 0, prob.sa[0]);

    prob = resist_chance(u1, u2, TYP_UNIT, 0);
    prob = frac_sub(prob, frac_make(1, 2)); /* 50% basic resistance */
    prob = frac_sub(prob, frac_make(1, 5)); /* 20% from magic levels */
    CuAssertIntEquals(tc, 0, prob.sa[0]);

    test_teardown();
}

static void test_magic_resistance(CuTest *tc) {
    unit *u;
    race *rc;
    building_type *btype;

    test_setup();
    test_use_astral();

    rc = test_create_race("human");
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));

    btype = test_create_buildingtype("stonecircle");
    btype->magresbonus = 20; /* this building gives +20% magic resistance */
    u->building = test_create_building(u->region, btype);
    CuAssertTrue(tc, frac_equal(frac_make(1, 5), magic_resistance(u)));
    u->building = NULL;

    /* 5% bonus in magic resistance for each magic level */
    set_level(u, SK_MAGIC, 4); /* makes for 20% */
    CuAssertTrue(tc, frac_equal(frac_make(1, 5), magic_resistance(u)));
    set_level(u, SK_MAGIC, 0);

    CuAssertTrue(tc, frac_equal(rc->magres, magic_resistance(u)));
    rc->magres = frac_one;
    CuAssert(tc, "magic resistance is capped at 0.9", frac_equal(magic_resistance(u), frac_make(9, 10)));
    rc = test_create_race("braineater");
    rc->magres = frac_one;
    u_setrace(u, rc);
    CuAssert(tc, "brain eaters outside astral space have 50% magres", frac_equal(magic_resistance(u), frac_make(1, 2)));
    u->region->_plane = get_astralplane();
    CuAssert(tc, "brain eaters in astral space have full magres", frac_equal(magic_resistance(u), frac_make(9, 10)));
    test_teardown();
}

static void test_max_spellpoints(CuTest *tc) {
    unit *u;
    race *rc;
    item_type *it_aura;

    test_setup();
    rc = test_create_race("human");
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));
    CuAssertIntEquals(tc, 0, max_spellpoints(u, u->region));
    CuAssertIntEquals(tc, 0, max_spellpoints(u, NULL));
    create_mage(u, M_GWYRRD);
    rc->maxaura = 100;
    CuAssertIntEquals(tc, 1, max_spellpoints(u, NULL));
    rc->maxaura = 200;
    CuAssertIntEquals(tc, 2, max_spellpoints(u, NULL));
    set_level(u, SK_MAGIC, 1);
    CuAssertIntEquals(tc, 3, max_spellpoints(u, NULL));
    set_level(u, SK_MAGIC, 2);
    CuAssertIntEquals(tc, 9, max_spellpoints(u, NULL));
    /* permanent aura loss: */
    CuAssertIntEquals(tc, 7, change_maxspellpoints(u, -2));
    CuAssertIntEquals(tc, 7, max_spellpoints(u, NULL));
    /* aurafocus: */
    it_aura = test_create_itemtype("aurafocus");
    i_change(&u->items, it_aura, 1);
    CuAssertIntEquals(tc, 9, max_spellpoints(u, NULL));
    test_teardown();
}

static void test_regenerate_aura(CuTest *tc) {
    unit *u;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    create_mage(u, M_GWYRRD);
    CuAssertIntEquals(tc, 0, get_spellpoints(u));
    CuAssertIntEquals(tc, 1, max_spellpoints(u, NULL));
    regenerate_aura();
    CuAssertIntEquals(tc, 1, get_spellpoints(u));

    u = test_create_unit(u->faction, u->region);
    create_mage(u, M_GRAY);
    CuAssertIntEquals(tc, 0, get_spellpoints(u));
    CuAssertIntEquals(tc, 1, max_spellpoints(u, NULL));
    regenerate_aura();
    CuAssertIntEquals(tc, 1, get_spellpoints(u));
    test_teardown();
}

/**
 * Test for Bug 2582.
 *
 * Migrant units that are not familiars, but whose race has a maxaura
 * must not regenerate aura.
 */
static void test_regenerate_aura_migrants(CuTest *tc) {
    unit *u;
    race *rc;

    test_setup();
    rc = test_create_race("demon");
    rc->maxaura = 100;
    rc->flags |= RCF_FAMILIAR;

    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u_setrace(u, rc);
    CuAssertIntEquals(tc, 0, get_spellpoints(u));
    regenerate_aura();
    CuAssertIntEquals(tc, 0, get_spellpoints(u));
    test_teardown();
}

static void test_fix_fam_migrants(CuTest *tc) {
    unit *u, *mage;
    race *rc;

    test_setup();
    rc = test_create_race("demon");
    rc->maxaura = 100;
    rc->flags |= RCF_FAMILIAR;

    /* u is a migrant with at_mage attribute, but not a familiar */
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u_setrace(u, rc);
    create_mage(u, M_GRAY);
    CuAssertTrue(tc, !is_familiar(u));
    CuAssertPtrNotNull(tc, get_mage(u));
    fix_fam_migrant(u);
    CuAssertTrue(tc, !is_familiar(u));
    CuAssertPtrEquals(tc, NULL, get_mage(u));

    /* u is a familiar, and stays unchanged: */
    mage = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u_setrace(u, rc);
    /* reproduce the bug, create a broken familiar: */
    create_newfamiliar(mage, u);
    set_level(u, SK_MAGIC, 1);
    CuAssertTrue(tc, is_familiar(u));
    CuAssertPtrNotNull(tc, get_mage(u));
    fix_fam_migrant(u);
    CuAssertTrue(tc, is_familiar(u));
    CuAssertPtrNotNull(tc, get_mage(u));

    test_teardown();
}

static bool equip_spell(unit *u, const char *eqname, int mask) {
    spell * sp = find_spell("test");
    unit_add_spell(u, sp, 1);
    return true;
}

static void test_fix_fam_spells(CuTest *tc) {
    unit *u, *mage;
    race *rc;
    spell * sp;

    test_setup();
    sp = create_spell("test");
    rc = test_create_race("demon");
    rc->maxaura = 100;
    rc->flags |= RCF_FAMILIAR;

    /* u is a familiar, and gets equipped: */
    mage = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u_setrace(u, rc);
    /* reproduce the bug, create a broken familiar: */
    callbacks.equip_unit = NULL;
    create_newfamiliar(mage, u);
    set_level(u, SK_MAGIC, 1);
    CuAssertPtrEquals(tc, NULL, unit_get_spellbook(u));
    CuAssertTrue(tc, !u_hasspell(u, sp));
    callbacks.equip_unit = equip_spell;
    CuAssertTrue(tc, is_familiar(u));
    fix_fam_spells(u);
    CuAssertTrue(tc, is_familiar(u));
    CuAssertPtrNotNull(tc, unit_get_spellbook(u));
    CuAssertTrue(tc, u_hasspell(u, sp));

    /* u is a migrant, and does not get equipped: */
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u_setrace(u, rc);
    CuAssertTrue(tc, !is_familiar(u));
    fix_fam_spells(u);
    CuAssertTrue(tc, !is_familiar(u));
    CuAssertPtrEquals(tc, NULL, unit_get_spellbook(u));

    test_teardown();
}

static void test_illusioncastle(CuTest *tc)
{
    building *b;
    building_type *btype, *bt_icastle;
    const attrib *a;
    test_setup();
    btype = test_create_buildingtype("hodor");
    CuAssertPtrEquals(tc, NULL, btype->a_stages[0].name);
    bt_icastle = test_create_buildingtype("illusioncastle");
    b = test_create_building(test_create_plain(0, 0), bt_icastle);
    b->size = 1;
    make_icastle(b, btype, 10);
    a = a_find(b->attribs, &at_icastle);
    CuAssertPtrNotNull(tc, a);
    CuAssertPtrEquals(tc, btype, (void *)icastle_type(a));
    CuAssertPtrEquals(tc, bt_icastle, (void *)b->type);
    CuAssertStrEquals(tc, "hodor", buildingtype(btype, b, b->size));
    btype->a_stages[0].name = str_strdup("chaos");
    CuAssertStrEquals(tc, "chaos", buildingtype(btype, b, b->size));
    test_teardown();
}

static void test_is_mage(CuTest *tc) {
    unit *u;
    struct sc_mage *mage;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    CuAssertPtrEquals(tc, NULL, get_mage(u));
    CuAssertTrue(tc, !is_mage(u));
    set_level(u, SK_MAGIC, 1);
    CuAssertTrue(tc, !is_mage(u));
    CuAssertPtrEquals(tc, NULL, get_mage(u));
    CuAssertPtrNotNull(tc, mage = create_mage(u, M_CERDDOR));
    CuAssertPtrEquals(tc, mage, get_mage(u));
    CuAssertTrue(tc, is_mage(u));
    test_teardown();
}

static void test_get_mage(CuTest *tc) {
    unit *u;
    struct sc_mage *mage;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    CuAssertPtrEquals(tc, NULL, get_mage(u));
    CuAssertPtrNotNull(tc, mage = create_mage(u, M_CERDDOR));
    CuAssertPtrEquals(tc, mage, get_mage(u));
    test_teardown();
}

static void test_familiar_set(CuTest *tc) {
    unit *mag, *fam;

    test_setup();

    mag = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    fam = test_create_unit(mag->faction, test_create_plain(0, 0));
    CuAssertPtrEquals(tc, NULL, get_familiar(mag));
    CuAssertPtrEquals(tc, NULL, get_familiar_mage(fam));
    CuAssertPtrEquals(tc, NULL, a_find(mag->attribs, &at_skillmod));
    set_familiar(mag, fam);
    CuAssertPtrEquals(tc, fam, get_familiar(mag));
    CuAssertPtrEquals(tc, mag, get_familiar_mage(fam));
    CuAssertPtrNotNull(tc, a_find(mag->attribs, &at_skillmod));
    remove_familiar(mag);
    CuAssertPtrEquals(tc, NULL, get_familiar(mag));
    CuAssertPtrEquals(tc, NULL, a_find(mag->attribs, &at_skillmod));
    test_teardown();
}

static void test_familiar_skillmod(CuTest* tc) {
    unit* mag, * fam;

    test_setup();

    mag = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    fam = test_create_unit(mag->faction, test_create_plain(0, 0));
    set_level(mag, SK_STAMINA, 5);
    set_familiar(mag, fam);
    set_level(mag, SK_STAMINA, 5);
    CuAssertIntEquals(tc, 5, effskill(mag, SK_STAMINA, mag->region));
    set_level(fam, SK_STAMINA, 6);
    CuAssertIntEquals(tc, 8, effskill(mag, SK_STAMINA, mag->region));
    remove_familiar(mag);
    CuAssertIntEquals(tc, 5, effskill(mag, SK_STAMINA, mag->region));
    test_teardown();
}

static void test_familiar_age(CuTest *tc) {
    unit *mag, *fam;

    test_setup();

    mag = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    fam = test_create_unit(mag->faction, test_create_plain(0, 0));
    set_familiar(mag, fam);
    CuAssertPtrEquals(tc, fam, get_familiar(mag));
    CuAssertPtrEquals(tc, mag, get_familiar_mage(fam));
    a_age(&fam->attribs, fam);
    a_age(&mag->attribs, mag);
    CuAssertPtrEquals(tc, fam, get_familiar(mag));
    CuAssertPtrEquals(tc, mag, get_familiar_mage(fam));
    set_number(fam, 0);
    a_age(&mag->attribs, mag);
    CuAssertPtrEquals(tc, NULL, get_familiar(mag));
    test_teardown();
}

static unit *eq_unit;
static char *eq_name;
static int eq_mask;

static bool equip_callback(unit *u, const char *eqname, int mask) {
    eq_unit = u;
    eq_name = str_strdup(eqname);
    eq_mask = mask;
    return true;
}

static void test_familiar_equip(CuTest *tc) {
    unit *mag, *u;

    test_setup();
    callbacks.equip_unit = equip_callback;
    mag = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u = test_create_unit(mag->faction, test_create_plain(0, 0));
    CuAssertStrEquals(tc, "human", u->_race->_name);
    set_familiar(mag, u);
    create_newfamiliar(mag, u);

    CuAssertIntEquals(tc, EQUIP_ALL, eq_mask);
    CuAssertPtrEquals(tc, u, eq_unit);
    CuAssertStrEquals(tc, "fam_human", eq_name);
    free(eq_name);

    test_teardown();
}

static void test_fumble_toad(CuTest *tc) {
    unit* u;
    struct race* rc_toad;
    const struct race* rc;
    trigger* t;
    changerace_data* crd;

    test_setup();
    rc_toad = test_create_race("toad");
    CuAssertPtrEquals(tc, rc_toad, (race *)get_race(RC_TOAD));

    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    rc = u_race(u);
    fumble_toad(u, NULL, 6);
    CuAssertPtrEquals(tc, rc_toad, (race*)u_race(u));
    CuAssertPtrNotNull(tc, t = get_timeout(u->attribs, "timer", &tt_changerace));
    CuAssertPtrNotNull(tc, crd = (changerace_data*)t->data.v);
    CuAssertPtrEquals(tc, (race*)rc, (race*)crd->race);
    CuAssertPtrEquals(tc, NULL, (race*)crd->irace);

    test_teardown();
}

CuSuite *get_magic_suite(void)
{
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_is_mage);
    SUITE_ADD_TEST(suite, test_get_mage);
    SUITE_ADD_TEST(suite, test_multi_cast);
    SUITE_ADD_TEST(suite, test_updatespells);
    SUITE_ADD_TEST(suite, test_spellbooks);
    SUITE_ADD_TEST(suite, test_get_spellbook);
    SUITE_ADD_TEST(suite, test_pay_spell);
    SUITE_ADD_TEST(suite, test_pay_spell_failure);
    SUITE_ADD_TEST(suite, test_getspell_unit);
    SUITE_ADD_TEST(suite, test_getspell_faction);
    SUITE_ADD_TEST(suite, test_getspell_school);
    SUITE_ADD_TEST(suite, test_set_pre_combatspell);
    SUITE_ADD_TEST(suite, test_set_main_combatspell);
    SUITE_ADD_TEST(suite, test_set_post_combatspell);
    SUITE_ADD_TEST(suite, test_hasspell);
    SUITE_ADD_TEST(suite, test_magic_resistance);
    SUITE_ADD_TEST(suite, test_resist_chance);
    SUITE_ADD_TEST(suite, test_max_spellpoints);
    SUITE_ADD_TEST(suite, test_illusioncastle);
    SUITE_ADD_TEST(suite, test_regenerate_aura);
    SUITE_ADD_TEST(suite, test_regenerate_aura_migrants);
    SUITE_ADD_TEST(suite, test_fix_fam_spells);
    SUITE_ADD_TEST(suite, test_fix_fam_migrants);
    SUITE_ADD_TEST(suite, test_fumble_toad);
    return suite;
}

CuSuite* get_familiar_suite(void)
{
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_familiar_equip);
    SUITE_ADD_TEST(suite, test_familiar_skillmod);
    SUITE_ADD_TEST(suite, test_familiar_set);
    SUITE_ADD_TEST(suite, test_familiar_age);
    return suite;
}

