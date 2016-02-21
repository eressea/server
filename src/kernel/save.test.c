#include <platform.h>
#include <kernel/config.h>

#include "save.h"
#include "unit.h"
#include "faction.h"
#include "version.h"
#include <util/base36.h>
#include <CuTest.h>
#include <tests.h>

#include <storage.h>

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

    test_cleanup();
    r = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    fno = f->no;
    u = test_create_unit(f, r);
    _mkdir(datapath());
    sprintf(path, "%s/%s", datapath(), filename);

    data = gamedata_open(path, "wb");
    CuAssertPtrNotNull(tc, data);

    write_unit(data, u);
    gamedata_close(data);

    free_gamedata();
    f = test_create_faction(0);
    renumber_faction(f, fno);
    data = gamedata_open(path, "rb");
    CuAssertPtrNotNull(tc, data);
    u = read_unit(data);
    gamedata_close(data);

    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, f, u->faction);
    CuAssertPtrEquals(tc, 0, u->region);
    CuAssertIntEquals(tc, 0, remove(path));
    test_cleanup();
}

static void test_read_password(CuTest *tc) {
    const char *path = "test.dat";
    gamedata *data;
    faction *f;
    f = test_create_faction(0);
    faction_setpassword(f, "secret");
    data = gamedata_open(path, "wb");
    CuAssertPtrNotNull(tc, data);
    _test_write_password(data, f);
    gamedata_close(data);
    data = gamedata_open(path, "rb");
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
    faction_setpassword(f, "secret");
    CuAssertPtrNotNull(tc, f->passw);
    data = gamedata_open(path, "wb");
    CuAssertPtrNotNull(tc, data);
    WRITE_TOK(data->store, (const char *)f->passw);
    WRITE_TOK(data->store, (const char *)f->passw);
    gamedata_close(data);
    data = gamedata_open(path, "rb");
    CuAssertPtrNotNull(tc, data);
    data->version = BADCRYPT_VERSION;
    _test_read_password(data, f);
    CuAssertPtrEquals(tc, 0, f->passw);
    F = fopen(pwfile, "wt");
    fprintf(F, "%s:secret\n", itoa36(f->no));
    fclose(F);
    _test_read_password(data, f);
    CuAssertPtrNotNull(tc, f->passw);
    gamedata_close(data);
    CuAssertTrue(tc, checkpasswd(f, "secret"));
    CuAssertIntEquals(tc, 0, remove(path));
    CuAssertIntEquals(tc, 0, remove(pwfile));
}

CuSuite *get_save_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_readwrite_data);
    SUITE_ADD_TEST(suite, test_readwrite_unit);
    SUITE_ADD_TEST(suite, test_read_password);
    SUITE_ADD_TEST(suite, test_read_password_external);
    return suite;
}
