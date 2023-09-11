#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "ally.h"
#include "alliance.h"
#include "calendar.h"
#include "callbacks.h"
#include "config.h"
#include "faction.h"
#include "item.h"
#include "magic.h"                // for set_familiar
#include "order.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "skill.h"         // for SK_ALCHEMY, SK_MAGIC, SK_ENTERTAINMENT
#include "types.h"         // for M_GRAY
#include "unit.h"

#include <util/goodies.h>
#include <util/keyword.h>         // for K_ENTERTAIN, K_WORK, K_AUTOSTUDY
#include <util/language.h>
#include <util/password.h>

#include <attributes/racename.h>

#include <CuTest.h>
#include <tests.h>
#include <selist.h>

#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>              // for false, true

static void test_destroyfaction(CuTest *tc) {
    faction *f;
    region *r;
    unit* u;

    test_setup();
    init_resources();

    r = test_create_plain(0, 0);
    rsetpeasants(r, 100);
    f = test_create_faction();
    u = test_create_unit(f, r);
    scale_number(u, 100);
    CuAssertPtrEquals(tc, f, factions);
    CuAssertPtrEquals(tc, NULL, f->next);
    destroyfaction(&factions);
    CuAssertPtrEquals(tc, NULL, factions);
    CuAssertIntEquals(tc, 200, rpeasants(r));
    test_teardown();
}

static void test_destroyfaction_items(CuTest *tc) {
    faction *f;
    region *r;
    unit* u;

    test_setup();
    init_resources();

    r = test_create_plain(0, 0);
    rsethorses(r, 10);
    rsetmoney(r, 1000);
    f = test_create_faction();
    u = test_create_unit(f, r);
    i_change(&u->items, it_find("horse"), 10);
    i_change(&u->items, it_find("money"), 1000);
    CuAssertPtrEquals(tc, f, factions);
    CuAssertPtrEquals(tc, NULL, f->next);
    destroyfaction(&factions);
    CuAssertPtrEquals(tc, NULL, factions);
    CuAssertIntEquals(tc, 20, rhorses(r));
    CuAssertIntEquals(tc, 2000, rmoney(r));
    test_teardown();
}

static void test_destroyfaction_undead(CuTest *tc) {
    faction *f;
    region *r;
    unit* u;
    race* rc;

    test_setup();
    init_resources();

    r = test_create_plain(0, 0);
    rsethorses(r, 10);
    rsetpeasants(r, 100);
    rsetmoney(r, 1000);
    rc = test_create_race("undead");
    rc->flags -= RCF_PLAYABLE;
    f = test_create_faction_ex(rc, NULL);
    u = test_create_unit(f, r);
    i_change(&u->items, it_find("horse"), 10);
    i_change(&u->items, it_find("money"), 1000);
    scale_number(u, 100);
    CuAssertPtrEquals(tc, f, factions);
    CuAssertPtrEquals(tc, NULL, f->next);
    destroyfaction(&factions);
    CuAssertPtrEquals(tc, NULL, factions);
    CuAssertIntEquals(tc, 20, rhorses(r));
    CuAssertIntEquals(tc, 100, rpeasants(r));
    CuAssertIntEquals(tc, 2000, rmoney(r));
    test_teardown();
}

static void test_destroyfaction_demon(CuTest *tc) {
    faction *f;
    region *r;
    unit* u;
    race* rc;

    test_setup();
    init_resources();

    r = test_create_plain(0, 0);
    rsethorses(r, 10);
    rsetpeasants(r, 100);
    rsetmoney(r, 1000);
    rc = test_create_race("demon");
    rc->ec_flags -= ECF_REC_ETHEREAL;
    f = test_create_faction_ex(rc, NULL);
    u = test_create_unit(f, r);
    i_change(&u->items, it_find("horse"), 10);
    i_change(&u->items, it_find("money"), 1000);
    scale_number(u, 100);
    CuAssertPtrEquals(tc, f, factions);
    CuAssertPtrEquals(tc, NULL, f->next);
    destroyfaction(&factions);
    CuAssertPtrEquals(tc, NULL, factions);
    CuAssertIntEquals(tc, 20, rhorses(r));
    CuAssertIntEquals(tc, 100, rpeasants(r));
    CuAssertIntEquals(tc, 2000, rmoney(r));
    test_teardown();
}

static void test_destroyfaction_orc(CuTest *tc) {
    faction *f;
    region *r;
    unit* u;
    race* rc;

    test_setup();
    init_resources();

    r = test_create_plain(0, 0);
    rsethorses(r, 10);
    rsetpeasants(r, 100);
    rsetmoney(r, 1000);
    rc = test_create_race("orc");
    rc->recruit_multi = 2;
    f = test_create_faction_ex(rc, NULL);
    u = test_create_unit(f, r);
    i_change(&u->items, it_find("horse"), 10);
    i_change(&u->items, it_find("money"), 1000);
    scale_number(u, 100);
    CuAssertPtrEquals(tc, f, factions);
    CuAssertPtrEquals(tc, NULL, f->next);
    destroyfaction(&factions);
    CuAssertPtrEquals(tc, NULL, factions);
    CuAssertIntEquals(tc, 20, rhorses(r));
    CuAssertIntEquals(tc, 150, rpeasants(r));
    CuAssertIntEquals(tc, 2000, rmoney(r));
    test_teardown();
}

static void test_destroyfaction_allies(CuTest *tc) {
    faction *f1, *f2;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    test_create_unit(f1, r);
    f2 = test_create_faction();
    ally_set(&f1->allies, f2, HELP_FIGHT);
    CuAssertIntEquals(tc, HELP_FIGHT, alliedfaction(f1, f2, HELP_ALL));
    CuAssertPtrEquals(tc, f2, f1->next);
    destroyfaction(&f1->next);
    CuAssertIntEquals(tc, false, faction_alive(f2));
    CuAssertIntEquals(tc, 0, alliedfaction(f1, f2, HELP_ALL));
    test_teardown();
}

static void test_remove_empty_factions_alliance(CuTest *tc) {
    faction *f;
    struct alliance *al;

    test_setup();
    f = test_create_faction();
    al = makealliance(0, "Hodor");
    setalliance(f, al);
    CuAssertPtrEquals(tc, f, alliance_get_leader(al));
    CuAssertIntEquals(tc, 1, selist_length(al->members));
    remove_empty_factions();
    CuAssertPtrEquals(tc, NULL, al->_leader);
    CuAssertIntEquals(tc, 0, selist_length(al->members));
    test_teardown();
}

static void test_remove_empty_factions(CuTest *tc) {
    faction *f, *fm;
    int fno;

    test_setup();
    fm = get_or_create_monsters();
    assert(fm);
    f = test_create_faction();
    fno = f->no;
    remove_empty_factions();
    CuAssertIntEquals(tc, false, f->_alive);
    CuAssertPtrEquals(tc, fm, factions);
    CuAssertPtrEquals(tc, NULL, fm->next);
    CuAssertPtrEquals(tc, NULL, findfaction(fno));
    CuAssertPtrEquals(tc, fm, get_monsters());
    test_teardown();
}

static void test_remove_dead_factions(CuTest *tc) {
    faction *f, *fm;
    region *r;
    int fno;

    test_setup();
    r = test_create_plain(0, 0);
    fm = get_or_create_monsters();
    f = test_create_faction();
    assert(fm && r && f);
    test_create_unit(f, r);
    test_create_unit(fm, r);
    remove_empty_factions();
    CuAssertPtrEquals(tc, f, findfaction(f->no));
    CuAssertPtrNotNull(tc, get_monsters());
    fm->units = 0;
    f->_alive = false;
    fno = f->no;
    remove_empty_factions();
    CuAssertPtrEquals(tc, NULL, findfaction(fno));
    CuAssertPtrEquals(tc, fm, get_monsters());
    test_teardown();
}

static void test_addfaction(CuTest *tc) {
    faction *f = NULL;
    const struct race *rc;
    const struct locale *lang;

    test_setup();
    rc = rc_get_or_create("human");
    lang = test_create_locale();
    f = addfaction("test@example.com", NULL, rc, lang);
    CuAssertPtrNotNull(tc, f);
    CuAssertPtrNotNull(tc, f->name);
    CuAssertPtrEquals(tc, NULL, (void *)f->units);
    CuAssertPtrEquals(tc, NULL, (void *)f->next);
    CuAssertPtrEquals(tc, NULL, (void *)faction_getbanner(f));
    CuAssertPtrEquals(tc, NULL, (void *)f->spellbook);
    CuAssertPtrEquals(tc, NULL, (void *)f->origin);
    CuAssertPtrEquals(tc, (void *)factions, (void *)f);
    CuAssertStrEquals(tc, "test@example.com", f->email);
    CuAssertTrue(tc, checkpasswd(f, "hurrdurr"));
    CuAssertPtrEquals(tc, (void *)lang, (void *)f->locale);
    CuAssertIntEquals(tc, FFL_ISNEW|FFL_PWMSG, f->flags);
    CuAssertIntEquals(tc, 0, faction_age(f));
    CuAssertTrue(tc, faction_alive(f));
    CuAssertIntEquals(tc, M_GRAY, f->magiegebiet);
    CuAssertIntEquals(tc, 0, f->lastorders);
    CuAssertPtrEquals(tc, f, findfaction(f->no));
    test_teardown();
}

static void test_check_passwd(CuTest *tc) {
    faction *f;
    
    test_setup();
    f = test_create_faction();
    faction_setpassword(f, password_hash("password", PASSWORD_DEFAULT));
    CuAssertTrue(tc, checkpasswd(f, "password"));
    CuAssertTrue(tc, !checkpasswd(f, "assword"));
    CuAssertTrue(tc, !checkpasswd(f, "PASSWORD"));
    test_teardown();
}

static void test_change_locale(CuTest *tc) {
    faction *f;
    unit *u;
    order *ord;
    struct locale *lang;
    
    test_setup();
    f = test_create_faction();
    lang = get_or_create_locale("en");
    u = test_create_unit(f, test_create_plain(0, 0));
    u->thisorder = create_order(K_ENTERTAIN, f->locale, NULL);

    u->defaults = create_order(K_WORK, f->locale, NULL);
    ord = u->defaults->next = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ALCHEMY]);
    CuAssertIntEquals(tc, SK_ALCHEMY - 100, ord->id);
    ord->next = create_order(K_GIVE, f->locale, "abcd 1 Schwert");

    /* NAME, GUARD not implemented: */
    unit_addorder(u, create_order(K_NAME, f->locale, "EINHEIT Hodor"));
    unit_addorder(u, create_order(K_GUARD, f->locale, NULL));
    unit_addorder(u, create_order(K_ENTERTAIN, f->locale, NULL));
    unit_addorder(u, create_order(K_KOMMENTAR, f->locale, "ich bin kein Tintenfisch"));
    unit_addorder(u, ord = create_order(K_STUDY, f->locale, skillnames[SK_ENTERTAINMENT]));
    CuAssertIntEquals(tc, SK_ENTERTAINMENT - 100, ord->id);

    change_locale(f, lang, true);
    CuAssertPtrEquals(tc, lang, (void *)f->locale);
    CuAssertPtrNotNull(tc, u->thisorder);

    CuAssertPtrNotNull(tc, ord = u->defaults);
    CuAssertIntEquals(tc, K_WORK, ord->command);
    CuAssertPtrNotNull(tc, ord = ord->next);
    CuAssertIntEquals(tc, K_AUTOSTUDY, ord->command);
    CuAssertIntEquals(tc, SK_ALCHEMY - 100, ord->id);
    CuAssertPtrEquals(tc, NULL, ord->next);

    CuAssertPtrNotNull(tc, ord = u->orders);
    /* GIVE and NAME get skipped, they're not implemented */
    CuAssertIntEquals(tc, K_GUARD, ord->command);
    CuAssertPtrNotNull(tc, ord = ord->next);
    CuAssertIntEquals(tc, K_ENTERTAIN, ord->command);
    CuAssertPtrNotNull(tc, ord = ord->next);
    CuAssertIntEquals(tc, K_KOMMENTAR, ord->command);
    CuAssertPtrNotNull(tc, ord = ord->next);
    CuAssertIntEquals(tc, K_STUDY, ord->command);
    CuAssertIntEquals(tc, SK_ENTERTAINMENT - 100, ord->id);
    CuAssertPtrEquals(tc, NULL, ord->next);

    test_teardown();
}

static void test_get_monsters(CuTest *tc) {
    faction *f;

    test_setup();
    CuAssertPtrNotNull(tc, (f = get_monsters()));
    CuAssertPtrEquals(tc, f, get_monsters());
    CuAssertIntEquals(tc, 666, f->no);
    CuAssertStrEquals(tc, "Monster", f->name);
    test_teardown();
}

static void test_set_origin(CuTest *tc) {
    faction *f;
    int x = 0, y = 0;
    plane *pl;

    test_setup();
    pl = create_new_plane(0, "", 0, 19, 0, 19, 0);
    f = test_create_faction();
    CuAssertPtrEquals(tc, NULL, f->origin);
    faction_setorigin(f, 0, 1, 1);
    CuAssertIntEquals(tc, 0, f->origin->id);
    CuAssertIntEquals(tc, 1, f->origin->x);
    CuAssertIntEquals(tc, 1, f->origin->y);
    faction_getorigin(f, 0, &x, &y);
    CuAssertIntEquals(tc, 1, x);
    CuAssertIntEquals(tc, 1, y);
    adjust_coordinates(f, &x, &y, pl);
    CuAssertIntEquals(tc, -9, x);
    CuAssertIntEquals(tc, -9, y);
    adjust_coordinates(f, &x, &y, 0);
    CuAssertIntEquals(tc, -10, x);
    CuAssertIntEquals(tc, -10, y);
    test_teardown();
}

static void test_set_origin_bug(CuTest *tc) {
    faction *f;
    plane *pl;
    int x = 17, y = 10;

    test_setup();
    pl = create_new_plane(0, "", 0, 19, 0, 19, 0);
    f = test_create_faction();
    faction_setorigin(f, 0, -10, 3);
    faction_setorigin(f, 0, -13, -4);
    adjust_coordinates(f, &x, &y, pl);
    CuAssertIntEquals(tc, 0, f->origin->id);
    CuAssertIntEquals(tc, -9, x);
    CuAssertIntEquals(tc, 2, y);
    test_teardown();
}

static void test_max_migrants(CuTest *tc) {
    faction *f;
    unit *u;
    race *rc;

    test_setup();
    rc = test_create_race("human");
    f = test_create_faction_ex(rc, NULL);
    u = test_create_unit(f, test_create_plain(0, 0));
    CuAssertIntEquals(tc, 0, count_maxmigrants(f));
    rc->flags |= RCF_MIGRANTS;
    CuAssertIntEquals(tc, 0, count_maxmigrants(f));
    scale_number(u, 250);
    CuAssertIntEquals(tc, 13, count_maxmigrants(f));
    test_teardown();
}

static void test_count_skill(CuTest* tc) {
    unit* u;
    region* r;
    faction* f;

    test_setup();
    f = test_create_faction();
    r = test_create_plain(0, 0);
    u = test_create_unit(f, r);
    set_level(u, SK_MAGIC, 1);
    set_level(u, SK_ALCHEMY, 1);
    CuAssertIntEquals(tc, 1, faction_count_skill(f, SK_MAGIC));
    CuAssertIntEquals(tc, 1, faction_count_skill(f, SK_ALCHEMY));
    u = test_create_unit(f, r);
    set_level(u, SK_MAGIC, 1);
    set_level(u, SK_ALCHEMY, 1);
    CuAssertIntEquals(tc, 2, faction_count_skill(f, SK_MAGIC));
    CuAssertIntEquals(tc, 2, faction_count_skill(f, SK_ALCHEMY));

    // familiars do not count as magicians:
    set_familiar(u, u);
    CuAssertIntEquals(tc, 1, faction_count_skill(f, SK_MAGIC));
    CuAssertIntEquals(tc, 2, faction_count_skill(f, SK_ALCHEMY));
    test_teardown();
}

static void test_skill_limit(CuTest *tc) {
    faction *f;

    test_setup();
    f = test_create_faction();
    CuAssertIntEquals(tc, INT_MAX, faction_skill_limit(f, SK_ENTERTAINMENT));
    CuAssertIntEquals(tc, 3, faction_skill_limit(f, SK_ALCHEMY));
    config_set_int("rules.maxskills.alchemy", 4);
    CuAssertIntEquals(tc, 4, faction_skill_limit(f, SK_ALCHEMY));
    CuAssertIntEquals(tc, 3, faction_skill_limit(f, SK_MAGIC));
    CuAssertIntEquals(tc, 3, max_magicians(f));
    config_set_int("rules.maxskills.magic", 4);
    CuAssertIntEquals(tc, 4, faction_skill_limit(f, SK_MAGIC));
    CuAssertIntEquals(tc, 4, max_magicians(f));
    f->race = test_create_race(racenames[RC_ELF]);
    CuAssertIntEquals(tc, 5, faction_skill_limit(f, SK_MAGIC));
    CuAssertIntEquals(tc, 5, max_magicians(f));
    test_teardown();
}

static void test_valid_race(CuTest *tc) {
    race * rc1, *rc2;
    faction *f;

    test_setup();
    rc1 = test_create_race("human");
    rc2 = test_create_race("elf");
    f = test_create_faction_ex(rc1, NULL);
    CuAssertTrue(tc, valid_race(f, rc1));
    CuAssertTrue(tc, !valid_race(f, rc2));
    rc_set_param(rc1, "other_race", "elf");
    CuAssertTrue(tc, valid_race(f, rc1));
    CuAssertTrue(tc, valid_race(f, rc2));
    test_teardown();
}

static void test_dbstrings(CuTest *tc) {
    const char *lipsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
    faction *f;
    test_setup();
    f = test_create_faction();
    faction_setbanner(f, lipsum);
    faction_setpassword(f, lipsum + 12);
    CuAssertStrEquals(tc, lipsum, faction_getbanner(f));
    CuAssertStrEquals(tc, lipsum + 12, faction_getpassword(f));
    test_teardown();
}

static void test_set_email(CuTest *tc) {
    faction *f;
    char email[10];
    test_setup();
    CuAssertIntEquals(tc, 0, check_email("enno@eressea.de"));
    CuAssertIntEquals(tc, 0, check_email("bugs@eressea.de"));
    CuAssertIntEquals(tc, -1, check_email("bad@@eressea.de"));
    CuAssertIntEquals(tc, -1, check_email("eressea.de"));
    CuAssertIntEquals(tc, -1, check_email("eressea@"));
    CuAssertIntEquals(tc, -1, check_email(""));
    CuAssertIntEquals(tc, -1, check_email(NULL));
    f = test_create_faction();

    sprintf(email, "enno");
    faction_setemail(f, email);
    CuAssertTrue(tc, email != f->email);
    CuAssertStrEquals(tc, "enno", f->email);
    CuAssertStrEquals(tc, "enno", faction_getemail(f));
    faction_setemail(f, "bugs@eressea.de");
    CuAssertStrEquals(tc, "bugs@eressea.de", f->email);
    faction_setemail(f, NULL);
    CuAssertPtrEquals(tc, NULL, f->email);
    CuAssertStrEquals(tc, "", faction_getemail(f));
    test_teardown();
}

static void test_save_special_items(CuTest *tc) {
    unit *u, *ug;
    race * rc;
    struct item_type *itype, *it_silver, *it_horse;

    test_setup();
    it_horse = test_create_horse();
    it_silver = test_create_silver();
    itype = test_create_itemtype("banana");
    itype->flags |= ITF_NOTLOST;
    rc = test_create_race("template");
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    i_change(&u->items, itype, 1);

    /* when there is no monster in the region, a ghost of the dead unit is created: */
    save_special_items(u);
    CuAssertPtrNotNull(tc, u->next);
    ug = u->next;
    CuAssertPtrEquals(tc, NULL, ug->next);
    CuAssertPtrEquals(tc, rc, (void *)ug->_race);
    CuAssertIntEquals(tc, 1, u->number);
    CuAssertIntEquals(tc, 0, i_get(u->items, itype));
    CuAssertIntEquals(tc, 1, i_get(ug->items, itype));
    CuAssertStrEquals(tc, "ghost", get_racename(ug->attribs));
    CuAssertStrEquals(tc, u->_name, ug->_name);

    i_change(&u->items, itype, 1);
    /* when there is a monster, it takes all special items: */
    save_special_items(u);
    CuAssertPtrEquals(tc, NULL, ug->next);
    CuAssertIntEquals(tc, 2, i_get(ug->items, itype));
    CuAssertPtrEquals(tc, NULL, u->items);

    i_change(&u->items, itype, 1);
    i_change(&u->items, it_horse, 5);
    i_change(&u->items, it_silver, 10);
    /* horses and money need to go to the region and are not taken: */
    save_special_items(u);
    CuAssertIntEquals(tc, 3, i_get(ug->items, itype));
    CuAssertIntEquals(tc, 5, i_get(u->items, it_horse));
    CuAssertIntEquals(tc, 10, i_get(u->items, it_silver));
    test_teardown();
}

static void test_addplayer(CuTest *tc) {
    unit *u;
    region *r;
    faction *f;
    item_type *itype;
    test_setup();
    callbacks.equip_unit = NULL;
    itype = test_create_silver();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = addplayer(r, f);
    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, r, u->region);
    CuAssertPtrEquals(tc, f, u->faction);
    CuAssertIntEquals(tc, turn, u->faction->lastorders);
    CuAssertIntEquals(tc, i_get(u->items, itype), 10);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertIntEquals(tc, K_WORK, getkeyword(u->orders));
    test_teardown();
}

CuSuite *get_faction_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_addplayer);
    SUITE_ADD_TEST(suite, test_max_migrants);
    SUITE_ADD_TEST(suite, test_skill_limit);
    SUITE_ADD_TEST(suite, test_count_skill);
    SUITE_ADD_TEST(suite, test_addfaction);
    SUITE_ADD_TEST(suite, test_remove_empty_factions);
    SUITE_ADD_TEST(suite, test_destroyfaction);
    SUITE_ADD_TEST(suite, test_destroyfaction_items);
    SUITE_ADD_TEST(suite, test_destroyfaction_undead);
    SUITE_ADD_TEST(suite, test_destroyfaction_demon);
    SUITE_ADD_TEST(suite, test_destroyfaction_orc);
    SUITE_ADD_TEST(suite, test_destroyfaction_allies);
    SUITE_ADD_TEST(suite, test_remove_empty_factions_alliance);
    SUITE_ADD_TEST(suite, test_remove_dead_factions);
    SUITE_ADD_TEST(suite, test_get_monsters);
    SUITE_ADD_TEST(suite, test_set_origin);
    SUITE_ADD_TEST(suite, test_set_origin_bug);
    SUITE_ADD_TEST(suite, test_check_passwd);
    SUITE_ADD_TEST(suite, test_change_locale);
    SUITE_ADD_TEST(suite, test_valid_race);
    SUITE_ADD_TEST(suite, test_set_email);
    SUITE_ADD_TEST(suite, test_dbstrings);
    SUITE_ADD_TEST(suite, test_save_special_items);
    return suite;
}
