#ifdef _MSC_VER
#include <platform.h>
#endif

#include "log.h"
#include "macros.h"

#include <CuTest.h>
#include <tests.h>

#include <stdarg.h>
#include <string.h>

void log_string(void *data, int level, const char *module, const char *format, va_list args) {
    char *str = (char *)data;
    const char *arg = va_arg(args, const char *);
    UNUSED_ARG(format);
    UNUSED_ARG(module);
    UNUSED_ARG(level);
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

static int stats_cb(const char *stat, int num, void *udata) {
    int *counter = (int *)udata;
    if (counter) {
        *counter += num;
    }
    return 0;
}

static void test_stats(CuTest * tc)
{
    int n = 0;
    test_setup();
    CuAssertIntEquals(tc, 1, stats_count("foobar", 1));
    CuAssertIntEquals(tc, 2, stats_count("test.one", 2));
    CuAssertIntEquals(tc, 1, stats_count("test.two", 1));
    CuAssertIntEquals(tc, 4, stats_count("test.one", 2));
    CuAssertIntEquals(tc, 1, stats_count("test.two", 0));

    n = 0;
    CuAssertIntEquals(tc, 0, stats_walk("", stats_cb, &n));
    CuAssertIntEquals(tc, 6, n);

    n = 0;
    CuAssertIntEquals(tc, 0, stats_walk("test", stats_cb, &n));
    CuAssertIntEquals(tc, 5, n);

    n = 0;
    CuAssertIntEquals(tc, 0, stats_walk("test.one", stats_cb, &n));
    CuAssertIntEquals(tc, 4, n);

    n = 0;
    CuAssertIntEquals(tc, 0, stats_walk("foobar", stats_cb, &n));
    CuAssertIntEquals(tc, 1, n);

    test_teardown();
}

CuSuite *get_log_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_logging);
    SUITE_ADD_TEST(suite, test_stats);
    return suite;
}
