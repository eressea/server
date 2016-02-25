#include <platform.h>
#include "password.h"
#include <CuTest.h>
#include <string.h>

static void test_passwords(CuTest *tc) {
    const char *hash, *expect;
    expect = "$apr1$FqQLkl8g$.icQqaDJpim4BVy.Ho5660";
    if (password_is_implemented(PASSWORD_APACHE_MD5)) {
        CuAssertIntEquals(tc, VERIFY_OK, password_verify(expect, "Hodor"));
        hash = password_encode("Hodor", PASSWORD_APACHE_MD5);
        CuAssertPtrNotNull(tc, hash);
        CuAssertIntEquals(tc, 0, strncmp(hash, expect, 6));
    } else {
        CuAssertIntEquals(tc, VERIFY_UNKNOWN, password_verify(expect, "Hodor"));
    }

    expect = "$1$ZouUn04i$yNnT1Oy8azJ5V.UM9ppP5/";
    if (password_is_implemented(PASSWORD_MD5)) {
        CuAssertIntEquals(tc, VERIFY_OK, password_verify(expect, "jollygood"));
        hash = password_encode("jollygood", PASSWORD_MD5);
        CuAssertPtrNotNull(tc, hash);
        CuAssertIntEquals(tc, 0, strncmp(hash, expect, 3));
    } else {
        CuAssertIntEquals(tc, VERIFY_UNKNOWN, password_verify(expect, "jollygood"));
    }

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
    
    expect = "$0$password";
    if (password_is_implemented(PASSWORD_NOCRYPT)) {
        hash = password_encode("password", PASSWORD_NOCRYPT);
        CuAssertPtrNotNull(tc, hash);
        CuAssertStrEquals(tc, hash, expect);
        CuAssertIntEquals(tc, VERIFY_OK, password_verify(expect, "password"));
        CuAssertIntEquals(tc, VERIFY_FAIL, password_verify(expect, "arseword"));
    } else {
        CuAssertIntEquals(tc, VERIFY_UNKNOWN, password_verify(expect, "password"));
    }

    expect = "$2y$05$RJ8qAhu.foXyJLdc2eHTLOaK4MDYn3/v4HtOVCq0Plv2yxcrEB7Wm";
    if (password_is_implemented(PASSWORD_BCRYPT)) {
        CuAssertIntEquals(tc, VERIFY_OK, password_verify(expect, "Hodor"));
        hash = password_encode("Hodor", PASSWORD_BCRYPT);
        CuAssertPtrNotNull(tc, hash);
        CuAssertIntEquals(tc, 0, strncmp(hash, expect, 7));
    } else {
        CuAssertIntEquals(tc, VERIFY_UNKNOWN, password_verify(expect, "Hodor"));
    }
    
    CuAssertIntEquals(tc, VERIFY_UNKNOWN, password_verify("$9$saltyfish$password", "password"));
}

CuSuite *get_password_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_passwords);
    return suite;
}
