#include <platform.h>
#include <kernel/config.h>
#include <util/attrib.h>
#include <attributes/key.h>

#include "save.h"
#include "unit.h"
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
    CuAssertIntEquals(tc, 0, writegame(filename));
    CuAssertIntEquals(tc, 0, readgame(filename, false));
    CuAssertIntEquals(tc, RELEASE_VERSION, global.data_version);
    join_path(datapath(), filename, path, sizeof(path));
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

    data = gamedata_open(path, "wb");
    CuAssertPtrNotNull(tc, data); // TODO: intermittent test (even after the 'b' fix!)
    write_unit(data, u);
    gamedata_close(data);

    free_gamedata();
    f = test_create_faction(0);
    renumber_faction(f, fno);
    data = gamedata_open(path, "rb");
    u = read_unit(data);
    gamedata_close(data);

    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, f, u->faction);
    CuAssertPtrEquals(tc, 0, u->region);
    CuAssertIntEquals(tc, 0, remove(path));
    test_cleanup();
}

static void test_readwrite_attrib(CuTest *tc) {
    gamedata *data;
    attrib *a = NULL;
    const char *path = "attrib.dat";
    test_cleanup();
    data = gamedata_open(path, "wb");
    CuAssertPtrNotNull(tc, data);
    add_key(&a, 41);
    add_key(&a, 42);
    write_attribs(data->store, a, NULL);
    gamedata_close(data);
    a_removeall(&a, NULL);
    CuAssertPtrEquals(tc, 0, a);

    data = gamedata_open(path, "rb");
    CuAssertPtrNotNull(tc, data);
    read_attribs(data->store, &a, NULL);
    gamedata_close(data);
    CuAssertPtrNotNull(tc, find_key(a, 41));
    CuAssertPtrNotNull(tc, find_key(a, 42));
    a_removeall(&a, NULL);

    CuAssertIntEquals(tc, 0, remove(path));
    test_cleanup();
}

CuSuite *get_save_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_readwrite_attrib);
    SUITE_ADD_TEST(suite, test_readwrite_data);
    SUITE_ADD_TEST(suite, test_readwrite_unit);
    return suite;
}
