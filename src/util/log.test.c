#include <platform.h>
#include <CuTest.h>

#include "log.h"

#include <stdarg.h>
#include <string.h>

void log_string(void *data, int level, const char *module, const char *format, va_list args) {
    char *str = (char *)data;
    const char *arg = va_arg(args, const char *);
    unused_arg(format);
    unused_arg(module);
    unused_arg(level);
    strcpy(str, arg);
}

static void test_logging(CuTest * tc)
{
    char str1[32];
    char str2[32];
    struct log_t * id1 = log_create(LOG_CPWARNING, str1, log_string);
    struct log_t * id2 = log_create(LOG_CPWARNING, str2, log_string);
    CuAssertTrue(tc, id1!=id2);
    log_warning("Hello %s", "World");
    log_destroy(id1);
    log_destroy(id2);
    CuAssertStrEquals(tc, "World", str1);
    CuAssertStrEquals(tc, "World", str2);
}

CuSuite *get_log_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_logging);
    return suite;
}
