#include <CuTest.h>

#include "log.h"

#include <stdarg.h>
#include <string.h>

void log_string(void *data, int level, const char *module, const char *format, va_list args) {
    char *str = (char *)data;
    const char *arg = va_arg(args, const char *);
    strncpy(str, arg, 32);
}

static void test_logging(CuTest * tc)
{
    char str1[32];
    char str2[32];
    int id1 = log_create(LOG_CPWARNING, str1, log_string);
    int id2 = log_create(LOG_CPWARNING, str2, log_string);
    CuAssertTrue(tc, id1!=id2);
    log_warning("Hello %s", "World");
    CuAssertStrEquals(tc, str1, "World");
    CuAssertStrEquals(tc, str2, "World");
    log_destroy(id1);
    log_destroy(id2);
}

CuSuite *get_log_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_logging);
    return suite;
}
