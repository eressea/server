#include "contact.h"

#include "tests.h"
#include <CuTest.h>

struct region;
struct unit;
struct faction;

static void test_contact(CuTest *tc) {
    struct unit *u1, *u2, *u3;
    struct region *r;
    struct faction *f;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction(NULL);
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    u3 = test_create_unit(test_create_faction(NULL), r);
    CuAssertTrue(tc, ucontact(u1, u1));
    CuAssertTrue(tc, ucontact(u1, u2));
    CuAssertTrue(tc, !ucontact(u1, u3));
    usetcontact(u1, u3);
    CuAssertTrue(tc, ucontact(u1, u3));
    CuAssertTrue(tc, !ucontact(u2, u3));
    test_teardown();
}

CuSuite *get_contact_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_contact);

    return suite;
}
