#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "save.h"

#include "config.h"
#include "race.h"
#include "version.h"
#include "building.h"
#include "ship.h"
#include "unit.h"
#include "group.h"
#include "ally.h"
#include "faction.h"
#include "region.h"
#include "attrib.h"
#include "event.h"
#include "gamedata.h"

#include <attributes/key.h>

#include <triggers/changefaction.h>
#include <triggers/createunit.h>
#include <triggers/timeout.h>

#include <util/base36.h>
#include <util/password.h>
#include <util/path.h>
#include <util/strings.h>

#include <storage.h>
#include <stream.h>
#include <memstream.h>

#include <CuTest.h>
#include <tests.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

static void test_readwrite_data(CuTest * tc)
{
    const char *filename = "test.dat";
    char path[PATH_MAX];
    test_setup();
    CuAssertIntEquals(tc, 0, writegame(filename));
    CuAssertIntEquals(tc, 0, readgame(filename));
    path_join(datapath(), filename, path, sizeof(path));
    CuAssertIntEquals(tc, 0, remove(path));
    test_teardown();
}

static void test_readwrite_unit(CuTest * tc)
{
    gamedata data;
    storage store;
    struct unit *u;
    struct region *r;
    struct faction *f;
    struct race *irace;
    int fno;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    fno = f->no;
    u = test_create_unit(f, r);
    unit_setname(u, "Hodor");
    irace = test_create_race("halfling");
    CuAssertTrue(tc, playerrace(irace));
    u->irace = irace;
    CuAssertTrue(tc, irace == u_irace(u));
    fset(u, UFL_MOVED);
    
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_unit(&data, u);
    
    data.strm.api->rewind(data.strm.handle);
    test_reset();
    f = test_create_faction();
    r = test_create_plain(0, 0);
    renumber_faction(f, fno);
    gamedata_init(&data, &store, RELEASE_VERSION);
    u = read_unit(&data);
    // The UFL_MOVED flag must be saved, monsters use it for planning:
    CuAssertIntEquals(tc, UFL_MOVED, fval(u, UFL_MOVED));
    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, f, u->faction);
    CuAssertStrEquals(tc, "Hodor", u->_name);
    CuAssertTrue(tc, irace == u_irace(u));
    CuAssertPtrEquals(tc, NULL, u->region);

    mstream_done(&data.strm);
    gamedata_done(&data);
    move_unit(u, r, NULL); /* this makes sure that u doesn't leak */
    test_teardown();
}

static void test_readwrite_faction(CuTest * tc)
{
    gamedata data;
    storage store;
    faction *f;

    test_setup();
    f = test_create_faction();
    free(f->name);
    f->name = str_strdup("Hodor");
    CuAssertStrEquals(tc, "Hodor", f->name);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_faction(&data, f);
    
    data.strm.api->rewind(data.strm.handle);
    test_reset();
    gamedata_init(&data, &store, RELEASE_VERSION);
    f = read_faction(&data);
    CuAssertPtrNotNull(tc, f);
    CuAssertStrEquals(tc, "Hodor", f->name);
    CuAssertPtrEquals(tc, NULL, f->units);
    factions = f;

    mstream_done(&data.strm);
    gamedata_done(&data);
    test_teardown();
}

static void test_readwrite_region(CuTest * tc)
{
    gamedata data;
    storage store;
    region *r;
    const char * lipsum = "Lorem ipsum dolor sit amet";

    test_setup();
    r = test_create_plain(0, 0);
    free(r->land->name);
    r->land->name = str_strdup("Hodor");
    CuAssertStrEquals(tc, "Hodor", r->land->name);
    region_setinfo(r, lipsum);
    CuAssertStrEquals(tc, lipsum, r->land->display);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_region(&data, r);
    
    data.strm.api->rewind(data.strm.handle);
    test_reset();
    gamedata_init(&data, &store, RELEASE_VERSION);
    r = read_region(&data);
    CuAssertPtrNotNull(tc, r);
    CuAssertStrEquals(tc, "Hodor", r->land->name);
    CuAssertStrEquals(tc, lipsum, r->land->display);
    regions = r;

    mstream_done(&data.strm);
    gamedata_done(&data);
    test_teardown();
}

static void test_readwrite_building(CuTest * tc)
{
    gamedata data;
    storage store;
    building *b;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    b = test_create_building(r, NULL);
    free(b->name);
    b->name = str_strdup("Hodor");
    CuAssertStrEquals(tc, "Hodor", b->name);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_building(&data, b);
    
    data.strm.api->rewind(data.strm.handle);
    test_reset();
    r = test_create_plain(0, 0);
    gamedata_init(&data, &store, RELEASE_VERSION);
    b = read_building(&data);
    CuAssertPtrNotNull(tc, b);
    CuAssertStrEquals(tc, "Hodor", b->name);
    CuAssertPtrEquals(tc, NULL, b->region);
    b->region = r;
    r->buildings = b;

    mstream_done(&data.strm);
    gamedata_done(&data);
    test_teardown();
}

static void test_readwrite_ship(CuTest * tc)
{
    gamedata data;
    storage store;
    ship *sh;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    sh = test_create_ship(r, NULL);
    free(sh->name);
    sh->name = str_strdup("Hodor");
    CuAssertStrEquals(tc, "Hodor", sh->name);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_ship(&data, sh);
    
    data.strm.api->rewind(data.strm.handle);
    test_reset();
    r = test_create_plain(0, 0);
    gamedata_init(&data, &store, RELEASE_VERSION);
    sh = read_ship(&data);
    CuAssertPtrNotNull(tc, sh);
    CuAssertStrEquals(tc, "Hodor", sh->name);
    CuAssertPtrEquals(tc, NULL, sh->region);
    sh->region = r;
    r->ships = sh;

    mstream_done(&data.strm);
    gamedata_done(&data);
    test_teardown();
}

static void test_readwrite_attrib(CuTest *tc) {
    gamedata data;
    storage store;
    attrib *a = NULL;

    test_setup();
    key_set(&a, 41, 42);
    key_set(&a, 42, 43);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_attribs(data.store, a, NULL);
    a_removeall(&a, NULL);
    CuAssertPtrEquals(tc, NULL, a);

    data.strm.api->rewind(data.strm.handle);
    read_attribs(&data, &a, NULL);
    mstream_done(&data.strm);
    gamedata_done(&data);
    CuAssertIntEquals(tc, 42, key_get(a, 41));
    CuAssertIntEquals(tc, 43, key_get(a, 42));
    a_removeall(&a, NULL);

    test_teardown();
}

static void test_readwrite_dead_faction_group(CuTest *tc) {
    faction *f, *f2;
    unit * u;
    group *g;
    int fno;
    gamedata data;
    storage store;

    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);

    test_setup();
    f = test_create_faction();
    fno = f->no;
    CuAssertPtrEquals(tc, f, factions);
    CuAssertPtrEquals(tc, NULL, f->next);
    f2 = test_create_faction();
    CuAssertPtrEquals(tc, f2, factions->next);
    u = test_create_unit(f2, test_create_plain(0, 0));
    CuAssertPtrNotNull(tc, u);
    g = join_group(u, "group");
    CuAssertPtrNotNull(tc, g);
    CuAssertPtrEquals(tc, NULL, g->allies);
    ally_set(&g->allies, f, HELP_GIVE);
    CuAssertPtrNotNull(tc, g->allies);

    CuAssertPtrEquals(tc, f, factions);
    destroyfaction(&factions);
    CuAssertTrue(tc, !f->_alive);
    CuAssertPtrEquals(tc, f2, factions);
    write_game(&data);
    test_reset();
    data.strm.api->rewind(data.strm.handle);
    read_game(&data);
    CuAssertPtrEquals(tc, NULL, findfaction(fno));
    CuAssertPtrNotNull(tc, factions);
    u = factions->units;
    CuAssertPtrNotNull(tc, u);
    g = get_group(u);
    CuAssertPtrNotNull(tc, g);
    CuAssertPtrEquals(tc, NULL, g->allies);
    mstream_done(&data.strm);
    gamedata_done(&data);
    test_teardown();
}

static void test_readwrite_dead_faction_regionowner(CuTest *tc) {
    faction *f;
    region *r;
    gamedata data;
    storage store;

    test_setup();
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);

    config_set("rules.region_owners", "1");
    f = test_create_faction();
    test_create_unit(f, r = test_create_plain(0, 0));
    region_set_owner(r, f, 0);
    destroyfaction(&factions);
    CuAssertTrue(tc, !f->_alive);
    remove_empty_units();
    write_game(&data);
    test_reset();
    data.strm.api->rewind(data.strm.handle);
    read_game(&data);
    mstream_done(&data.strm);
    gamedata_done(&data);
    CuAssertPtrEquals(tc, NULL, factions);
    r = regions;
    CuAssertPtrNotNull(tc, r);
    CuAssertPtrEquals(tc, NULL, region_get_owner(r));
    test_teardown();
}

static void test_readwrite_dead_faction_changefaction(CuTest *tc) {
    gamedata data;
    storage store;
    faction *f, *f2;
    region *r;
    trigger *tr;
    unit * u;

    test_setup();
    f = test_create_faction();
    f2 = test_create_faction();
    u = test_create_unit(f2, r = test_create_plain(0, 0));
    tr = trigger_changefaction(u, f);
    add_trigger(&u->attribs, "timer", trigger_timeout(10, tr));
    CuAssertPtrNotNull(tc, a_find(u->attribs, &at_eventhandler));
    destroyfaction(&factions);
    CuAssertTrue(tc, !f->_alive);
    remove_empty_units();
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_game(&data);
    test_reset();
    f = NULL;
    data.strm.api->rewind(data.strm.handle);
    read_game(&data);
    mstream_done(&data.strm);
    gamedata_done(&data);
    f = factions;
    CuAssertPtrNotNull(tc, f);
    r = regions;
    CuAssertPtrNotNull(tc, r);
    u = r->units;
    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, NULL, a_find(u->attribs, &at_eventhandler));
    test_teardown();
}

static void test_readwrite_dead_faction_createunit(CuTest *tc) {
    gamedata data;
    storage store;
    faction *f, *f2;
    region *r;
    trigger *tr;
    unit * u;

    test_setup();
    f = test_create_faction();
    f2 = test_create_faction();
    u = test_create_unit(f2, r = test_create_plain(0, 0));
    tr = trigger_createunit(r, f, f->race, 1);
    add_trigger(&u->attribs, "timer", trigger_timeout(10, tr));
    CuAssertPtrNotNull(tc, a_find(u->attribs, &at_eventhandler));
    destroyfaction(&factions);
    CuAssertTrue(tc, !f->_alive);
    remove_empty_units();
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_game(&data);
    test_reset();
    f = NULL;
    data.strm.api->rewind(data.strm.handle);
    read_game(&data);
    mstream_done(&data.strm);
    gamedata_done(&data);
    f = factions;
    CuAssertPtrNotNull(tc, f);
    r = regions;
    CuAssertPtrNotNull(tc, r);
    u = r->units;
    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, NULL, a_find(u->attribs, &at_eventhandler));
    test_teardown();
}

static void test_read_password(CuTest *tc) {
    gamedata data;
    storage store;
    faction *f;

    test_setup();
    f = test_create_faction();
    faction_setpassword(f, password_hash("secret", PASSWORD_DEFAULT));
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    _test_write_password(&data, f);
    data.strm.api->rewind(data.strm.handle);
    _test_read_password(&data, f);
    mstream_done(&data.strm);
    gamedata_done(&data);
    CuAssertTrue(tc, checkpasswd(f, "secret"));
    test_teardown();
}

static void test_read_password_external(CuTest *tc) {
    gamedata data;
    storage store;
    const char *pwfile = "passwords.txt";
    faction *f;
    FILE * F;

    test_setup();
    if (remove(pwfile) != 0) {
        errno = 0;
    }
    f = test_create_faction();
    faction_setpassword(f, password_hash("secret", PASSWORD_DEFAULT));
    CuAssertPtrNotNull(tc, faction_getpassword(f));
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    WRITE_TOK(data.store, "newpassword");
    WRITE_TOK(data.store, "secret");
    WRITE_TOK(data.store, "$brokenhash");
    data.strm.api->rewind(data.strm.handle);
    data.version = NOCRYPT_VERSION;
    _test_read_password(&data, f);
    CuAssertTrue(tc, checkpasswd(f, "newpassword"));
    data.version = BADCRYPT_VERSION;
    _test_read_password(&data, f);
    CuAssertTrue(tc, checkpasswd(f, "secret"));
    F = fopen(pwfile, "wt");
    if (F) {
        fprintf(F, "%s:pwfile\n", itoa36(f->no));
        fclose(F);
    }
    CuAssertTrue(tc, checkpasswd(f, "secret"));
    _test_read_password(&data, f);
    CuAssertTrue(tc, checkpasswd(f, "pwfile"));
    mstream_done(&data.strm);
    gamedata_done(&data);
    CuAssertIntEquals(tc, 0, remove(pwfile));
    test_teardown();
}

static void test_version_no(CuTest *tc) {
    CuAssertIntEquals(tc, 0, version_no("0.0.0-devel"));
    CuAssertIntEquals(tc, 0x10000, version_no("1.0.0-test"));
    CuAssertIntEquals(tc, 0x10203, version_no("1.2.3-what.is.42"));
}

CuSuite *get_save_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_readwrite_attrib);
    SUITE_ADD_TEST(suite, test_readwrite_data);
    SUITE_ADD_TEST(suite, test_readwrite_unit);
    SUITE_ADD_TEST(suite, test_readwrite_faction);
    SUITE_ADD_TEST(suite, test_readwrite_region);
    SUITE_ADD_TEST(suite, test_readwrite_building);
    SUITE_ADD_TEST(suite, test_readwrite_ship);
    SUITE_ADD_TEST(suite, test_readwrite_dead_faction_createunit);
    SUITE_ADD_TEST(suite, test_readwrite_dead_faction_changefaction);
    SUITE_ADD_TEST(suite, test_readwrite_dead_faction_regionowner);
    SUITE_ADD_TEST(suite, test_readwrite_dead_faction_group);
    SUITE_ADD_TEST(suite, test_read_password);
    SUITE_ADD_TEST(suite, test_read_password_external);
    SUITE_ADD_TEST(suite, test_version_no);

    return suite;
}
