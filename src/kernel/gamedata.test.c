#include "gamedata.h"

#include <CuTest.h>
#include <stdio.h>
#include <stdlib.h>

static void test_gamedata(CuTest * tc)
{
    gamedata *data;
    data = gamedata_open("test.dat", "wb", 0);
    CuAssertPtrNotNull(tc, data);
    gamedata_close(data);
    free(data);
    data = gamedata_open("test.dat", "rb", 0);
    CuAssertPtrNotNull(tc, data);
    gamedata_close(data);
    free(data);
    CuAssertIntEquals(tc, 0, remove("test.dat"));
}

CuSuite *get_gamedata_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_gamedata);
    return suite;
}
