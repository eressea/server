#include <platform.h>

#include "magic.h"
#include "teleport.h"
#include "give.h"

#include <kernel/callbacks.h>
#include <kernel/building.h>
#include <kernel/race.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/unit.h>
#include <kernel/pool.h>

#include <util/attrib.h>
#include <util/language.h>
#include <util/strings.h>

#include <CuTest.h>
#include <selist.h>
#include <tests.h>

#include <stdlib.h>
#include <string.h>

void test_updatespells(CuTest * tc)
{
    faction * f;
    spell * sp;
    spellbook *book = 0;

    test_setup();
    test_create_race("human");

    f = test_create_faction(NULL);
    sp = create_spell("testspell");
    CuAssertPtrNotNull(tc, sp);

    book = create_spellbook("spells");
    CuAssertPtrNotNull(tc, book);
    spellbook_add(book, sp, 1);

    CuAssertPtrEquals(tc, 0, f->spellbook);
    pick_random_spells(f, 1, book, 1);
    CuAssertPtrNotNull(tc, f->spellbook);
    CuAssertIntEquals(tc, 1, selist_length(f->spellbook->spells));
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
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);

    sp = test_create_spell();
    CuAssertPtrNotNull(tc, sp);

    set_level(u, SK_MAGIC, 5);
    unit_add_spell(u, 0, sp, 1);

    change_resource(u, get_resourcetype(R_SILVER), 1);
    change_resource(u, get_resourcetype(R_AURA), 3);
    change_resource(u, get_resourcetype(R_HORSE), 3);

    level = eff_spelllevel(u, sp, 3, 1);
    CuAssertIntEquals(tc, 3, level);
    pay_spell(u, sp, level, 1);
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
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);

    sp = test_create_spell();
    CuAssertPtrNotNull(tc, sp);

    set_level(u, SK_MAGIC, 5);
    unit_add_spell(u, 0, sp, 1);

    CuAssertIntEquals(tc, 1, change_resource(u, get_resourcetype(R_SILVER), 1));
    CuAssertIntEquals(tc, 2, change_resource(u, get_resourcetype(R_AURA), 2));
    CuAssertIntEquals(tc, 3, change_resource(u, get_resourcetype(R_HORSE), 3));

    level = eff_spelllevel(u, sp, 3, 1);
    CuAssertIntEquals(tc, 2, level);
    pay_spell(u, sp, level, 1);
    CuAssertIntEquals(tc, 1, change_resource(u, get_resourcetype(R_SILVER), 1));
    CuAssertIntEquals(tc, 3, change_resource(u, get_resourcetype(R_AURA), 3));
    CuAssertIntEquals(tc, 2, change_resource(u, get_resourcetype(R_HORSE), 1));

    CuAssertIntEquals(tc, 0, eff_spelllevel(u, sp, 3, 1));
    CuAssertIntEquals(tc, 0, change_resource(u, get_resourcetype(R_SILVER), -1));
    CuAssertIntEquals(tc, 0, eff_spelllevel(u, sp, 2, 1));
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
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    create_mage(u, M_GRAY);
    enable_skill(SK_MAGIC, true);

    set_level(u, SK_MAGIC, 1);

    lang = test_create_locale();
    sp = create_spell("testspell");
    locale_setstring(lang, mkname("spell", sp->sname), "Herp-a-derp");

    CuAssertPtrEquals(tc, 0, unit_getspell(u, "Herp-a-derp", lang));

    unit_add_spell(u, 0, sp, 1);
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
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    create_mage(u, f->magiegebiet);
    enable_skill(SK_MAGIC, true);

    set_level(u, SK_MAGIC, 1);

    lang = test_create_locale();
    sp = create_spell("testspell");
    locale_setstring(lang, mkname("spell", sp->sname), "Herp-a-derp");

    CuAssertPtrEquals(tc, 0, unit_getspell(u, "Herp-a-derp", lang));

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
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    create_mage(u, f->magiegebiet);
    enable_skill(SK_MAGIC, true);
    set_level(u, SK_MAGIC, 1);

    lang = test_create_locale();
    sp = create_spell("testspell");
    locale_setstring(lang, mkname("spell", sp->sname), "Herp-a-derp");

    CuAssertPtrEquals(tc, 0, unit_getspell(u, "Herp-a-derp", lang));

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
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    enable_skill(SK_MAGIC, true);
    set_level(u, SK_MAGIC, 1);
    sp = create_spell("testspell");
    sp->sptyp |= PRECOMBATSPELL;

    unit_add_spell(u, 0, sp, 1);

    set_combatspell(u, sp, 0, 2);
    CuAssertPtrEquals(tc, sp, (spell *)get_combatspell(u, index));
    set_level(u, SK_MAGIC, 2);
    CuAssertIntEquals(tc, 2, get_combatspelllevel(u, index));
    set_level(u, SK_MAGIC, 1);
    CuAssertIntEquals(tc, 1, get_combatspelllevel(u, index));
    unset_combatspell(u, sp);
    CuAssertIntEquals(tc, 0, get_combatspelllevel(u, index));
    CuAssertPtrEquals(tc, 0, (spell *)get_combatspell(u, index));
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
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    enable_skill(SK_MAGIC, true);
    set_level(u, SK_MAGIC, 1);
    sp = create_spell("testspell");
    sp->sptyp |= COMBATSPELL;

    unit_add_spell(u, 0, sp, 1);

    set_combatspell(u, sp, 0, 2);
    CuAssertPtrEquals(tc, sp, (spell *)get_combatspell(u, index));
    set_level(u, SK_MAGIC, 2);
    CuAssertIntEquals(tc, 2, get_combatspelllevel(u, index));
    set_level(u, SK_MAGIC, 1);
    CuAssertIntEquals(tc, 1, get_combatspelllevel(u, index));
    unset_combatspell(u, sp);
    CuAssertIntEquals(tc, 0, get_combatspelllevel(u, index));
    CuAssertPtrEquals(tc, 0, (spell *)get_combatspell(u, index));
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
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    enable_skill(SK_MAGIC, true);
    set_level(u, SK_MAGIC, 1);
    sp = create_spell("testspell");
    sp->sptyp |= POSTCOMBATSPELL;

    unit_add_spell(u, 0, sp, 1);

    set_combatspell(u, sp, 0, 2);
    CuAssertPtrEquals(tc, sp, (spell *)get_combatspell(u, index));
    set_level(u, SK_MAGIC, 2);
    CuAssertIntEquals(tc, 2, get_combatspelllevel(u, index));
    set_level(u, SK_MAGIC, 1);
    CuAssertIntEquals(tc, 1, get_combatspelllevel(u, index));
    unset_combatspell(u, sp);
    CuAssertIntEquals(tc, 0, get_combatspelllevel(u, index));
    CuAssertPtrEquals(tc, 0, (spell *)get_combatspell(u, index));
    test_teardown();
}

void test_hasspell(CuTest * tc)
{
    spell *sp;
    struct unit * u;
    struct faction * f;
    struct region * r;

    test_setup();
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    f->magiegebiet = M_TYBIED;
    u = test_create_unit(f, r);
    enable_skill(SK_MAGIC, true);
    sp = create_spell("testspell");
    sp->sptyp |= POSTCOMBATSPELL;

    unit_add_spell(u, 0, sp, 2);

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
    locale_setstring(lang, mkname("spell", sp->sname), "Feuerball");
    CuAssertStrEquals(tc, "Feuerball", spell_name(sp, lang));

    u = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
    set_level(u, SK_MAGIC, 10);
    unit_add_spell(u, 0, sp, 1);
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

static void test_magic_resistance(CuTest *tc) {
    unit *u;
    race *rc;

    test_setup();
    rc = test_create_race("human");
    u = test_create_unit(test_create_faction(rc), test_create_region(0, 0, NULL));
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

    test_setup();
    rc = test_create_race("human");
    u = test_create_unit(test_create_faction(rc), test_create_region(0, 0, NULL));
    CuAssertIntEquals(tc, 1, max_spellpoints(u->region, u));
    rc->maxaura = 100;
    CuAssertIntEquals(tc, 1, max_spellpoints(u->region, u));
    rc->maxaura = 200;
    CuAssertIntEquals(tc, 2, max_spellpoints(u->region, u));
    create_mage(u, M_GRAY);
    set_level(u, SK_MAGIC, 1);
    CuAssertIntEquals(tc, 3, max_spellpoints(u->region, u));
    set_level(u, SK_MAGIC, 2);
    CuAssertIntEquals(tc, 9, max_spellpoints(u->region, u));
    /* permanent aura loss: */
    CuAssertIntEquals(tc, 7, change_maxspellpoints(u, -2));
    CuAssertIntEquals(tc, 7, max_spellpoints(u->region, u));
    test_teardown();
}

static void test_illusioncastle(CuTest *tc)
{
    building *b;
    building_type *btype, *bt_icastle;
    const attrib *a;
    test_setup();
    btype = test_create_buildingtype("castle");
    bt_icastle = test_create_buildingtype("illusioncastle");
    b = test_create_building(test_create_region(0, 0, NULL), bt_icastle);
    b->size = 1;
    make_icastle(b, btype, 10);
    a = a_find(b->attribs, &at_icastle);
    CuAssertPtrNotNull(tc, a);
    CuAssertPtrEquals(tc, btype, (void *)icastle_type(a));
    CuAssertPtrEquals(tc, bt_icastle, (void *)b->type);
    CuAssertStrEquals(tc, "castle", buildingtype(btype, b, b->size));
    btype->stages->name = str_strdup("site");
    CuAssertStrEquals(tc, "site", buildingtype(btype, b, b->size));
    test_teardown();
}

static void test_is_mage(CuTest *tc) {
    unit *u;
    sc_mage *mage;

    test_setup();
    u = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
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
    sc_mage *mage;

    test_setup();
    u = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
    CuAssertPtrEquals(tc, NULL, get_mage(u));
    CuAssertPtrEquals(tc, NULL, get_mage_depr(u));
    CuAssertPtrNotNull(tc, mage = create_mage(u, M_CERDDOR));
    CuAssertPtrEquals(tc, mage, get_mage(u));
    CuAssertPtrEquals(tc, NULL, get_mage_depr(u));
    set_level(u, SK_MAGIC, 1);
    CuAssertPtrEquals(tc, mage, get_mage(u));
    CuAssertPtrEquals(tc, mage, get_mage_depr(u));
    test_teardown();
}

static void test_familiar_set(CuTest *tc) {
    unit *mag, *fam;

    test_setup();

    mag = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
    fam = test_create_unit(mag->faction, test_create_region(0, 0, NULL));
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

static void test_familiar_age(CuTest *tc) {
    unit *mag, *fam;

    test_setup();

    mag = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
    fam = test_create_unit(mag->faction, test_create_region(0, 0, NULL));
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
    mag = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
    u = test_create_unit(mag->faction, test_create_region(0, 0, NULL));
    CuAssertStrEquals(tc, "human", u->_race->_name);
    set_familiar(mag, u);
    create_newfamiliar(mag, u);

    CuAssertIntEquals(tc, EQUIP_ALL, eq_mask);
    CuAssertPtrEquals(tc, u, eq_unit);
    CuAssertStrEquals(tc, "fam_human", eq_name);
    free(eq_name);

    test_teardown();
}

CuSuite *get_familiar_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_familiar_equip);
    SUITE_ADD_TEST(suite, test_familiar_set);
    SUITE_ADD_TEST(suite, test_familiar_age);
    return suite;
}

CuSuite *get_magic_suite(void)
{
    CuSuite *suite = CuSuiteNew();
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
    SUITE_ADD_TEST(suite, test_max_spellpoints);
    SUITE_ADD_TEST(suite, test_illusioncastle);
    return suite;
}
