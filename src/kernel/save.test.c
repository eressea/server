#include <platform.h>
#include <kernel/config.h>
#include <util/attrib.h>
#include <util/gamedata.h>
#include <attributes/key.h>

#include "save.h"
#include "unit.h"
#include "group.h"
#include "ally.h"
#include "faction.h"
#include "plane.h"
#include "region.h"
#include "version.h"

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

#include <storage.h>

#include <stdio.h>

static void test_readwrite_data(CuTest * tc)
{
    const char *filename = "test.dat";
    char path[MAX_PATH];
    test_cleanup();
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

    test_cleanup();
    r = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    fno = f->no;
    u = test_create_unit(f, r);

    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_unit(&data, u);
    
    data.strm.api->rewind(data.strm.handle);
    free_gamedata();
    f = test_create_faction(0);
    renumber_faction(f, fno);
    gamedata_init(&data, &store, RELEASE_VERSION);
    u = read_unit(&data);
    mstream_done(&data.strm);
    gamedata_done(&data);

    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, f, u->faction);
    CuAssertPtrEquals(tc, 0, u->region);
    test_cleanup();
}

static void test_readwrite_attrib(CuTest *tc) {
    gamedata data;
    storage store;
    attrib *a = NULL;

    test_cleanup();
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
    writegame("test.dat");
    free_gamedata();
    f = f2 = NULL;
    readgame("test.dat", false);
    CuAssertPtrEquals(tc, 0, findfaction(fno));
    f2 = factions;
    CuAssertPtrNotNull(tc, f2);
    u = f2->units;
    CuAssertPtrNotNull(tc, u);
    g = get_group(u);
    CuAssertPtrNotNull(tc, g);
    CuAssertPtrEquals(tc, 0, g->allies);
    test_cleanup();
}

static void test_readwrite_dead_faction_regionowner(CuTest *tc) {
    faction *f;
    region *r;

    test_cleanup();
    config_set("rules.region_owners", "1");
    f = test_create_faction(0);
    test_create_unit(f, r = test_create_region(0, 0, 0));
    region_set_owner(r, f, turn);
    destroyfaction(&factions);
    CuAssertTrue(tc, !f->_alive);
    remove_empty_units();
    writegame("test.dat");
    free_gamedata();
    f = NULL;
    readgame("test.dat", false);
    f = factions;
    CuAssertPtrEquals(tc, 0, f);
    r = regions;
    CuAssertPtrNotNull(tc, r);
    CuAssertPtrEquals(tc, 0, region_get_owner(r));
    test_cleanup();
}

static void test_readwrite_dead_faction_changefaction(CuTest *tc) {
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
    writegame("test.dat");
    free_gamedata();
    f = NULL;
    readgame("test.dat", false);
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
    writegame("test.dat");
    free_gamedata();
    f = NULL;
    readgame("test.dat", false);
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
    const char *path = "test.dat";
    gamedata *data;
    faction *f;
    f = test_create_faction(0);
    faction_setpassword(f, password_encode("secret", PASSWORD_DEFAULT));
    data = gamedata_open(path, "wb", RELEASE_VERSION);
    CuAssertPtrNotNull(tc, data);
    _test_write_password(data, f);
    gamedata_close(data);
    data = gamedata_open(path, "rb", RELEASE_VERSION);
    CuAssertPtrNotNull(tc, data);
    _test_read_password(data, f);
    gamedata_close(data);
    CuAssertTrue(tc, checkpasswd(f, "secret"));
    CuAssertIntEquals(tc, 0, remove(path));
}

static void test_read_password_external(CuTest *tc) {
    const char *path = "test.dat", *pwfile = "passwords.txt";
    gamedata *data;
    faction *f;
    FILE * F;

    remove(pwfile);
    f = test_create_faction(0);
    faction_setpassword(f, password_encode("secret", PASSWORD_DEFAULT));
    CuAssertPtrNotNull(tc, f->_password);
    data = gamedata_open(path, "wb", RELEASE_VERSION);
    CuAssertPtrNotNull(tc, data);
    WRITE_TOK(data->store, (const char *)f->_password);
    WRITE_TOK(data->store, (const char *)f->_password);
    gamedata_close(data);
    data = gamedata_open(path, "rb", RELEASE_VERSION);
    CuAssertPtrNotNull(tc, data);
    data->version = BADCRYPT_VERSION;
    _test_read_password(data, f);
    CuAssertPtrEquals(tc, 0, f->_password);
    F = fopen(pwfile, "wt");
    fprintf(F, "%s:secret\n", itoa36(f->no));
    fclose(F);
    _test_read_password(data, f);
    CuAssertPtrNotNull(tc, f->_password);
    gamedata_close(data);
    CuAssertTrue(tc, checkpasswd(f, "secret"));
    CuAssertIntEquals(tc, 0, remove(path));
    CuAssertIntEquals(tc, 0, remove(pwfile));
}

CuSuite *get_save_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_readwrite_attrib);
    SUITE_ADD_TEST(suite, test_readwrite_data);
    SUITE_ADD_TEST(suite, test_readwrite_unit);
    SUITE_ADD_TEST(suite, test_readwrite_dead_faction_createunit);
    SUITE_ADD_TEST(suite, test_readwrite_dead_faction_changefaction);
    SUITE_ADD_TEST(suite, test_readwrite_dead_faction_regionowner);
    DISABLE_TEST(suite, test_readwrite_dead_faction_group);
    SUITE_ADD_TEST(suite, test_read_password);
    SUITE_ADD_TEST(suite, test_read_password_external);
    return suite;
}
