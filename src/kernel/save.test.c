#include <platform.h>
#include <kernel/config.h>
#include <util/attrib.h>
#include <util/gamedata.h>
#include <attributes/key.h>

#include "save.h"
#include "version.h"
#include "building.h"
#include "ship.h"
#include "unit.h"
#include "group.h"
#include "ally.h"
#include "faction.h"
#include "plane.h"
#include "region.h"

#include <triggers/changefaction.h>
#include <triggers/createunit.h>
#include <triggers/timeout.h>
#include <util/attrib.h>
#include <util/event.h>
#include <util/base36.h>
#include <util/password.h>

#include <storage.h>
#include <memstream.h>
#include <CuTest.h>
#include <tests.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

static void test_readwrite_data(CuTest * tc)
{
    const char *filename = "test.dat";
    char path[MAX_PATH];
    test_setup();
    CuAssertIntEquals(tc, 0, writegame(filename));
    CuAssertIntEquals(tc, 0, readgame(filename, false));
    join_path(datapath(), filename, path, sizeof(path));
    CuAssertIntEquals(tc, 0, remove(path));
    test_cleanup();
}

static void test_readwrite_unit(CuTest * tc)
{
    gamedata data;
    storage store;
    struct unit *u;
    struct region *r;
    struct faction *f;
    int fno;

    test_setup();
    r = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    fno = f->no;
    u = test_create_unit(f, r);
    unit_setname(u, "  Hodor  ");
    CuAssertStrEquals(tc, "  Hodor  ", u->_name);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_unit(&data, u);
    
    data.strm.api->rewind(data.strm.handle);
    free_gamedata();
    f = test_create_faction(0);
    r = test_create_region(0, 0, 0);
    renumber_faction(f, fno);
    gamedata_init(&data, &store, RELEASE_VERSION);
    u = read_unit(&data);
    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, f, u->faction);
    CuAssertStrEquals(tc, "Hodor", u->_name);
    CuAssertPtrEquals(tc, 0, u->region);

    mstream_done(&data.strm);
    gamedata_done(&data);
    move_unit(u, r, NULL); // this makes sure that u doesn't leak
    test_cleanup();
}

static void test_readwrite_building(CuTest * tc)
{
    gamedata data;
    storage store;
    building *b;
    region *r;

    test_setup();
    r = test_create_region(0, 0, 0);
    b = test_create_building(r, 0);
    free(b->name);
    b->name = _strdup("  Hodor  ");
    CuAssertStrEquals(tc, "  Hodor  ", b->name);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_building(&data, b);
    
    data.strm.api->rewind(data.strm.handle);
    free_gamedata();
    r = test_create_region(0, 0, 0);
    gamedata_init(&data, &store, RELEASE_VERSION);
    b = read_building(&data);
    CuAssertPtrNotNull(tc, b);
    CuAssertStrEquals(tc, "Hodor", b->name);
    CuAssertPtrEquals(tc, 0, b->region);
    b->region = r;
    r->buildings = b;

    mstream_done(&data.strm);
    gamedata_done(&data);
    test_cleanup();
}

static void test_readwrite_ship(CuTest * tc)
{
    gamedata data;
    storage store;
    ship *sh;
    region *r;

    test_setup();
    r = test_create_region(0, 0, 0);
    sh = test_create_ship(r, 0);
    free(sh->name);
    sh->name = _strdup("  Hodor  ");
    CuAssertStrEquals(tc, "  Hodor  ", sh->name);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_ship(&data, sh);
    
    data.strm.api->rewind(data.strm.handle);
    free_gamedata();
    r = test_create_region(0, 0, 0);
    gamedata_init(&data, &store, RELEASE_VERSION);
    sh = read_ship(&data);
    CuAssertPtrNotNull(tc, sh);
    CuAssertStrEquals(tc, "Hodor", sh->name);
    CuAssertPtrEquals(tc, 0, sh->region);
    sh->region = r;
    r->ships = sh;

    mstream_done(&data.strm);
    gamedata_done(&data);
    test_cleanup();
}

static void test_readwrite_attrib(CuTest *tc) {
    gamedata data;
    storage store;
    attrib *a = NULL;

    test_setup();
    key_set(&a, 41);
    key_set(&a, 42);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_attribs(data.store, a, NULL);
    a_removeall(&a, NULL);
    CuAssertPtrEquals(tc, 0, a);

    data.strm.api->rewind(data.strm.handle);
    read_attribs(&data, &a, NULL);
    mstream_done(&data.strm);
    gamedata_done(&data);
    CuAssertTrue(tc, key_get(a, 41));
    CuAssertTrue(tc, key_get(a, 42));
    a_removeall(&a, NULL);

    test_cleanup();
}

static void test_readwrite_dead_faction_group(CuTest *tc) {
    faction *f, *f2;
    unit * u;
    group *g;
    ally *al;
    int fno;
    gamedata data;
    storage store;

    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);

    test_cleanup();
    f = test_create_faction(0);
    fno = f->no;
    CuAssertPtrEquals(tc, f, factions);
    CuAssertPtrEquals(tc, 0, f->next);
    f2 = test_create_faction(0);
    CuAssertPtrEquals(tc, f2, factions->next);
    u = test_create_unit(f2, test_create_region(0, 0, 0));
    CuAssertPtrNotNull(tc, u);
    g = join_group(u, "group");
    CuAssertPtrNotNull(tc, g);
    al = ally_add(&g->allies, f);
    CuAssertPtrNotNull(tc, al);

    CuAssertPtrEquals(tc, f, factions);
    destroyfaction(&factions);
    CuAssertTrue(tc, !f->_alive);
    CuAssertPtrEquals(tc, f2, factions);
    write_game(&data);
    free_gamedata();
    f = f2 = NULL;
    data.strm.api->rewind(data.strm.handle);
    read_game(&data);
    CuAssertPtrEquals(tc, 0, findfaction(fno));
    f2 = factions;
    CuAssertPtrNotNull(tc, f2);
    u = f2->units;
    CuAssertPtrNotNull(tc, u);
    g = get_group(u);
    CuAssertPtrNotNull(tc, g);
    CuAssertPtrEquals(tc, 0, g->allies);
    mstream_done(&data.strm);
    gamedata_done(&data);
    test_cleanup();
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
    f = test_create_faction(0);
    test_create_unit(f, r = test_create_region(0, 0, 0));
    region_set_owner(r, f, 0);
    destroyfaction(&factions);
    CuAssertTrue(tc, !f->_alive);
    remove_empty_units();
    write_game(&data);
    free_gamedata();
    f = NULL;
    data.strm.api->rewind(data.strm.handle);
    read_game(&data);
    mstream_done(&data.strm);
    gamedata_done(&data);
    f = factions;
    CuAssertPtrEquals(tc, 0, f);
    r = regions;
    CuAssertPtrNotNull(tc, r);
    CuAssertPtrEquals(tc, 0, region_get_owner(r));
    test_cleanup();
}

static void test_readwrite_dead_faction_changefaction(CuTest *tc) {
    gamedata data;
    storage store;
    faction *f, *f2;
    region *r;
    trigger *tr;
    unit * u;

    test_cleanup();
    f = test_create_faction(0);
    f2 = test_create_faction(0);
    u = test_create_unit(f2, r = test_create_region(0, 0, 0));
    tr = trigger_changefaction(u, f);
    add_trigger(&u->attribs, "timer", trigger_timeout(10, tr));
    CuAssertPtrNotNull(tc, a_find(u->attribs, &at_eventhandler));
    destroyfaction(&factions);
    CuAssertTrue(tc, !f->_alive);
    remove_empty_units();
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_game(&data);
    free_gamedata();
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
    CuAssertPtrEquals(tc, 0, a_find(u->attribs, &at_eventhandler));
    test_cleanup();
}

static void test_readwrite_dead_faction_createunit(CuTest *tc) {
    gamedata data;
    storage store;
    faction *f, *f2;
    region *r;
    trigger *tr;
    unit * u;

    test_cleanup();
    f = test_create_faction(0);
    f2 = test_create_faction(0);
    u = test_create_unit(f2, r = test_create_region(0, 0, 0));
    tr = trigger_createunit(r, f, f->race, 1);
    add_trigger(&u->attribs, "timer", trigger_timeout(10, tr));
    CuAssertPtrNotNull(tc, a_find(u->attribs, &at_eventhandler));
    destroyfaction(&factions);
    CuAssertTrue(tc, !f->_alive);
    remove_empty_units();
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_game(&data);
    free_gamedata();
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
    CuAssertPtrEquals(tc, 0, a_find(u->attribs, &at_eventhandler));
    test_cleanup();
}

static void test_read_password(CuTest *tc) {
    gamedata data;
    storage store;
    faction *f;

    test_setup();
    f = test_create_faction(0);
    faction_setpassword(f, password_encode("secret", PASSWORD_DEFAULT));
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    _test_write_password(&data, f);
    data.strm.api->rewind(data.strm.handle);
    _test_read_password(&data, f);
    mstream_done(&data.strm);
    gamedata_done(&data);
    CuAssertTrue(tc, checkpasswd(f, "secret"));
    test_cleanup();
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
    f = test_create_faction(0);
    faction_setpassword(f, password_encode("secret", PASSWORD_DEFAULT));
    CuAssertPtrNotNull(tc, f->_password);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    WRITE_TOK(data.store, "newpassword");
    WRITE_TOK(data.store, "secret");
    WRITE_TOK(data.store, "$brokenhash");
    data.strm.api->rewind(data.strm.handle);
    data.version = NOCRYPT_VERSION;
    _test_read_password(&data, f);
    CuAssertStrEquals(tc, "newpassword", f->_password);
    data.version = BADCRYPT_VERSION;
    _test_read_password(&data, f);
    CuAssertStrEquals(tc, "secret", f->_password);
    F = fopen(pwfile, "wt");
    fprintf(F, "%s:pwfile\n", itoa36(f->no));
    fclose(F);
    CuAssertTrue(tc, checkpasswd(f, "secret"));
    _test_read_password(&data, f);
    CuAssertStrEquals(tc, "pwfile", f->_password);
    CuAssertTrue(tc, checkpasswd(f, "pwfile"));
    mstream_done(&data.strm);
    gamedata_done(&data);
    CuAssertIntEquals(tc, 0, remove(pwfile));
    test_cleanup();
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
