#include <platform.h>
#include <CuTest.h>
#include "password.h"

static void test_passwords(CuTest *tc) {
    const char *hash;

    hash = password_hash("password");
    CuAssertPtrNotNull(tc, hash);
    CuAssertStrEquals(tc, "$0$saltyfish$password", hash);
    CuAssertIntEquals(tc, VERIFY_OK, password_verify(hash, "password"));
    CuAssertIntEquals(tc, VERIFY_FAIL, password_verify(hash, "arseword"));
    CuAssertIntEquals(tc, VERIFY_UNKNOWN, password_verify("$9$saltyfish$password", "arseword"));
}

CuSuite *get_password_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_passwords);
    return suite;
}
