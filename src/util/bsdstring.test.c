#include <CuTest.h>
#include "bsdstring.h"
#include <errno.h>
#include <string.h>

static void test_strlcat(CuTest * tc)
{
    char buffer[32];

    memset(buffer, 0x7f, sizeof(buffer));

    buffer[0] = '\0';
    CuAssertIntEquals(tc, 4, (int)strlcat(buffer, "herp", 4));
    CuAssertStrEquals(tc, "her", buffer);

    buffer[0] = '\0';
    CuAssertIntEquals(tc, 4, (int)strlcat(buffer, "herp", 8));
    CuAssertStrEquals(tc, "herp", buffer);
    CuAssertIntEquals(tc, 0x7f, buffer[5]);

    CuAssertIntEquals(tc, 8, (int)strlcat(buffer, "derp", 8));
    CuAssertStrEquals(tc, "herpder", buffer);
    CuAssertIntEquals(tc, 0x7f, buffer[8]);
}

static void test_strlcpy(CuTest * tc)
{
    char buffer[32];

    memset(buffer, 0x7f, sizeof(buffer));

    CuAssertIntEquals(tc, 4, (int)strlcpy(buffer, "herp", 4));
    CuAssertStrEquals(tc, "her", buffer);

    CuAssertIntEquals(tc, 4, (int)strlcpy(buffer, "herp", 8)); /*-V666 */
    CuAssertStrEquals(tc, "herp", buffer);
    CuAssertIntEquals(tc, 0x7f, buffer[5]);

    CuAssertIntEquals(tc, 8, (int)strlcpy(buffer, "herpderp", 8));
    CuAssertStrEquals(tc, "herpder", buffer);
    CuAssertIntEquals(tc, 0x7f, buffer[8]);
    errno = 0;
}

CuSuite *get_bsdstring_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_strlcat);
    SUITE_ADD_TEST(suite, test_strlcpy);
    return suite;
}
