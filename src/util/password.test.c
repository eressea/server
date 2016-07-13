#include <platform.h>
#include "password.h"
#include <CuTest.h>
#include <string.h>

static void test_passwords(CuTest *tc) {
    const char *hash, *expect;
    
    expect = "password";
    if (password_is_implemented(PASSWORD_PLAINTEXT)) {
        hash = password_encode("password", PASSWORD_PLAINTEXT);
        CuAssertPtrNotNull(tc, hash);
        CuAssertStrEquals(tc, hash, expect);
        CuAssertIntEquals(tc, VERIFY_OK, password_verify(expect, "password"));
        CuAssertIntEquals(tc, VERIFY_FAIL, password_verify(expect, "arseword"));
    } else {
        CuAssertIntEquals(tc, VERIFY_UNKNOWN, password_verify(expect, "password"));
    }
}

CuSuite *get_password_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_passwords);
    return suite;
}
