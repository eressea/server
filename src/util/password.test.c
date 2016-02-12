#include <platform.h>
#include <CuTest.h>
#include "password.h"

static void test_passwords(CuTest *tc) {
    const char *hash, *expect;

    CuAssertIntEquals(tc, VERIFY_UNKNOWN, password_verify("$9$password", "password"));

    expect = "$apr1$FqQLkl8g$.icQqaDJpim4BVy.Ho5660";
    CuAssertIntEquals(tc, VERIFY_OK, password_verify(expect, "Hodor"));
    hash = password_hash("Hodor", "FqQLkl8g", PASSWORD_APACHE_MD5);
    CuAssertPtrNotNull(tc, hash);
    CuAssertStrEquals(tc, expect, hash);

    expect = "$1$ZouUn04i$yNnT1Oy8azJ5V.UM9ppP5/";
    CuAssertIntEquals(tc, VERIFY_OK, password_verify(expect, "jollygood"));
    hash = password_hash("jollygood", "ZouUn04i", PASSWORD_MD5);
    CuAssertPtrNotNull(tc, hash);
    CuAssertStrEquals(tc, expect, hash);

    expect = "$0$password";
    CuAssertIntEquals(tc, VERIFY_OK, password_verify(expect, "password"));
    CuAssertIntEquals(tc, VERIFY_FAIL, password_verify(expect, "arseword"));
    hash = password_hash("password", "hodor", PASSWORD_PLAIN);
    CuAssertPtrNotNull(tc, hash);
    CuAssertStrEquals(tc, expect, hash);

    expect = "$2y$05$RJ8qAhu.foXyJLdc2eHTLOaK4MDYn3/v4HtOVCq0Plv2yxcrEB7Wm";
    CuAssertIntEquals(tc, VERIFY_OK, password_verify(expect, "Hodor"));
}

CuSuite *get_password_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_passwords);
    return suite;
}
