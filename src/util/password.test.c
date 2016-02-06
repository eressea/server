#include <platform.h>
#include <CuTest.h>
#include "password.h"

static void test_passwords(CuTest *tc) {
    const char *hash;

    hash = password_hash("Hodor", "FqQLkl8g", PASSWORD_APACHE_MD5);
    CuAssertPtrNotNull(tc, hash);
    CuAssertStrEquals(tc, "$apr1$FqQLkl8g$.icQqaDJpim4BVy.Ho5660", hash);
    CuAssertIntEquals(tc, VERIFY_OK, password_verify(hash, "Hodor"));

    hash = password_hash("jollygood", "ZouUn04i", PASSWORD_MD5);
    CuAssertPtrNotNull(tc, hash);
    CuAssertStrEquals(tc, "$1$ZouUn04i$yNnT1Oy8azJ5V.UM9ppP5/", hash);
    CuAssertIntEquals(tc, VERIFY_OK, password_verify(hash, "jollygood"));

    hash = password_hash("password", "hodor", PASSWORD_PLAIN);
    CuAssertPtrNotNull(tc, hash);
    CuAssertStrEquals(tc, "$0$hodor$password", hash);
    CuAssertIntEquals(tc, VERIFY_OK, password_verify(hash, "password"));
    CuAssertIntEquals(tc, VERIFY_FAIL, password_verify(hash, "arseword"));
    CuAssertIntEquals(tc, VERIFY_UNKNOWN, password_verify("$9$saltyfish$password", "password"));
}

CuSuite *get_password_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_passwords);
    return suite;
}
