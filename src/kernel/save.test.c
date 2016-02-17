#include <platform.h>
#include <kernel/config.h>

#include "save.h"
#include "unit.h"
#include "group.h"
#include "ally.h"
#include "faction.h"
#include "version.h"
#include <CuTest.h>
#include <tests.h>

#include <stdio.h>

static void test_readwrite_data(CuTest * tc)
{
    const char *filename = "test.dat";
    char path[MAX_PATH];
    test_cleanup();
    sprintf(path, "%s/%s", datapath(), filename);
    CuAssertIntEquals(tc, 0, writegame(filename));
    CuAssertIntEquals(tc, 0, readgame(filename, false));
    CuAssertIntEquals(tc, RELEASE_VERSION, global.data_version);
    CuAssertIntEquals(tc, 0, remove(path));
    test_cleanup();
}

static void test_readwrite_unit(CuTest * tc)
{
    const char *filename = "test.dat";
    char path[MAX_PATH];
    gamedata *data;
    struct unit *u;
    struct region *r;
    struct faction *f;
    int fno;
    /* FIXME: at some point during this test, errno is set to 17 (File exists), why? */

    create_directories();
    test_cleanup();
    r = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    fno = f->no;
    u = test_create_unit(f, r);
    join_path(datapath(), filename, path, sizeof(path));

    data = gamedata_open(path, "w");
    CuAssertPtrNotNull(tc, data); // TODO: intermittent test
    write_unit(data, u);
    gamedata_close(data);

    free_gamedata();
    f = test_create_faction(0);
    renumber_faction(f, fno);
    data = gamedata_open(path, "r");
    u = read_unit(data);
    gamedata_close(data);

    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, f, u->faction);
    CuAssertPtrEquals(tc, 0, u->region);
    CuAssertIntEquals(tc, 0, remove(path));
    test_cleanup();
}

static void test_readwrite_dead_faction(CuTest *tc) {
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

CuSuite *get_save_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_readwrite_data);
    SUITE_ADD_TEST(suite, test_readwrite_unit);
    SUITE_ADD_TEST(suite, test_readwrite_dead_faction);
    return suite;
}
