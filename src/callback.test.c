#include "callback.h"
#include <stdlib.h>
#include <CuTest.h>

void callback(va_list ap) {
    int i = (int)va_arg(ap, int);
    int *p = va_arg(ap, int *);
    *p += i;
}

static void test_find_callback(CuTest *tc) {
    HCALLBACK cb;
    reset_callbacks();
    CuAssertIntEquals(tc, -1, find_callback("test", &cb));
    cb = register_callback("test", callback);
    CuAssertIntEquals(tc, 0, find_callback("test", &cb));
    reset_callbacks();
}

static void test_call_by_handle(CuTest *tc) {
    HCALLBACK cb;
    int x = 0;
    reset_callbacks();
    cb = create_callback(callback);
    CuAssertIntEquals(tc, 0, call_callback(cb, 0, 42, &x));
    CuAssertIntEquals(tc, 42, x);
    reset_callbacks();
}

static void test_call_by_name(CuTest *tc) {
    HCALLBACK cb = { 0 };
    HCALLBACK ca = { 0 };
    int x = 0;
    reset_callbacks();
    CuAssertIntEquals(tc, -1, call_callback(cb, "test", 42, &x));
    cb = register_callback("test", callback);
    CuAssertIntEquals(tc, 0, call_callback(cb, "test", 42, &x));
    CuAssertIntEquals(tc, 42, x);
    CuAssertIntEquals(tc, 0, call_callback(ca, "test", 42, &x));
    CuAssertIntEquals(tc, 84, x);
    reset_callbacks();
}

CuSuite *get_callback_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_find_callback);
    SUITE_ADD_TEST(suite, test_call_by_name);
    SUITE_ADD_TEST(suite, test_call_by_handle);
    return suite;
}
