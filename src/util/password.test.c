#include <platform.h>
#include "password.h"
#include <CuTest.h>
#include <string.h>

static void test_passwords(CuTest *tc) {
    const char *hash;
    
    if (password_is_implemented(PASSWORD_BCRYPT)) {
        int wf = bcrypt_workfactor;
        bcrypt_workfactor = 4;
        hash = password_encode("password", PASSWORD_BCRYPT);
        CuAssertPtrNotNull(tc, hash);
        CuAssertIntEquals(tc, '$', hash[0]);
        CuAssertIntEquals(tc, '2', hash[1]);
        CuAssertIntEquals(tc, '$', hash[3]);
        CuAssertIntEquals(tc, '0', hash[4]);
        CuAssertIntEquals(tc, '4', hash[5]);
        CuAssertIntEquals(tc, '$', hash[6]);
        CuAssertIntEquals(tc, VERIFY_OK, password_verify(hash, "password"));
        CuAssertIntEquals(tc, VERIFY_FAIL, password_verify(hash, "arseword"));
        bcrypt_workfactor = wf;
    }
    if (password_is_implemented(PASSWORD_PLAINTEXT)) {
        hash = password_encode("password", PASSWORD_PLAINTEXT);
        CuAssertPtrNotNull(tc, hash);
        CuAssertStrEquals(tc, hash, "password");
        CuAssertIntEquals(tc, VERIFY_OK, password_verify(hash, "password"));
        CuAssertIntEquals(tc, VERIFY_FAIL, password_verify(hash, "arseword"));
    }
}

CuSuite *get_password_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_passwords);
    return suite;
}
