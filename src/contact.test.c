#include "contact.h"

#include "kernel/attrib.h"
#include "kernel/faction.h"
#include "kernel/order.h"
#include "kernel/unit.h"

#include "util/keyword.h"
#include "util/param.h"

#include "tests.h"
#include <CuTest.h>

#include <stddef.h>

static void test_contact(CuTest *tc) {
    struct unit *u1, *u2, *u3;
    struct region *r;
    struct faction *f;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    u3 = test_create_unit(test_create_faction(), r);
    CuAssertTrue(tc, ucontact(u1, u1));
    CuAssertTrue(tc, ucontact(u1, u2));
    CuAssertTrue(tc, !ucontact(u1, u3));
    contact_unit(u1, u3);
    CuAssertTrue(tc, ucontact(u1, u3));
    CuAssertTrue(tc, !ucontact(u2, u3));
    test_teardown();
}

static void test_contact_cmd(CuTest *tc) {
    struct unit *u, *u2;
    struct region *r;
    const struct locale *lang;
    struct order *ord;

    test_setup();
    r = test_create_plain(0, 0);
    u = test_create_unit(test_create_faction(), r);
    lang = u->faction->locale;

    u2 = test_create_unit(test_create_faction(), r);
    ord = create_order(K_CONTACT, u->faction->locale, "%s %i", param_name(P_UNIT, lang), u2->no);
    a_removeall(&u->attribs, NULL);
    contact_cmd(u, ord);
    CuAssertTrue(tc, ucontact(u, u2));
    free_order(ord);

    u2 = test_create_unit(test_create_faction(), r);
    ord = create_order(K_CONTACT, u->faction->locale, "%s %i", param_name(P_FACTION, lang),
        u2->faction->no);
    a_removeall(&u->attribs, NULL);
    contact_cmd(u, ord);
    CuAssertTrue(tc, ucontact(u, u2));
    free_order(ord);

    u2 = test_create_unit(test_create_faction(), r);
    ord = create_order(K_CONTACT, u->faction->locale, "%i", u2->no);
    a_removeall(&u->attribs, NULL);
    contact_cmd(u, ord);
    CuAssertTrue(tc, ucontact(u, u2));
    free_order(ord);

    u2 = test_create_unit(test_create_faction(), r);
    usetalias(u2, 42);
    ord = create_order(K_CONTACT, u->faction->locale, "%s %s %i",
        param_name(P_UNIT, lang), param_name(P_TEMP, lang), ualias(u2));
    a_removeall(&u->attribs, NULL);
    contact_cmd(u, ord);
    CuAssertTrue(tc, ucontact(u, u2));
    free_order(ord);

    u2 = test_create_unit(test_create_faction(), r);
    usetalias(u2, 47);
    ord = create_order(K_CONTACT, u->faction->locale, "%s %i",
        param_name(P_TEMP, lang), ualias(u2));
    a_removeall(&u->attribs, NULL);
    contact_cmd(u, ord);
    CuAssertTrue(tc, ucontact(u, u2));
    free_order(ord);

    test_teardown();
}

static void test_contact_cmd_not_local(CuTest *tc) {
    struct unit *u, *u2;
    struct region *r, *r2;
    const struct locale *lang;
    struct order *ord;

    test_setup();
    r = test_create_plain(0, 0);
    u = test_create_unit(test_create_faction(), r);
    lang = u->faction->locale;

    /* can contact units in other regions */
    r2 = test_create_plain(0, 1);
    u2 = test_create_unit(test_create_faction(), r2);
    ord = create_order(K_CONTACT, u->faction->locale, "%s %i",
        param_name(P_UNIT, lang), u2->no);
    a_removeall(&u->attribs, NULL);
    contact_cmd(u, ord);
    CuAssertTrue(tc, ucontact(u, u2));
    free_order(ord);

    /* two units with same TEMP id in differnt regions */
    u2 = test_create_unit(test_create_faction(), r);
    usetalias(u2, 42);
    ord = create_order(K_CONTACT, u->faction->locale, "%s %s %i",
        param_name(P_UNIT, lang), param_name(P_TEMP, lang), ualias(u2));
    a_removeall(&u->attribs, NULL);
    contact_cmd(u, ord);
    CuAssertTrue(tc, ucontact(u, u2));
    free_order(ord);

    /* cannot contact TEMP units in other regions */
    u2 = test_create_unit(test_create_faction(), r2);
    usetalias(u2, 47);
    ord = create_order(K_CONTACT, u->faction->locale, "%s %s %i",
        param_name(P_UNIT, lang), param_name(P_TEMP, lang), ualias(u2));
    test_clear_messages(u->faction);
    a_removeall(&u->attribs, NULL);
    contact_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "feedback_unit_not_found"));
    CuAssertTrue(tc, !ucontact(u, u2));
    free_order(ord);

    test_teardown();
}

static void test_contact_cmd_invalid(CuTest *tc) {
    struct unit *u;
    struct region *r;
    const struct locale *lang;
    struct order *ord;

    test_setup();
    r = test_create_plain(0, 0);
    u = test_create_unit(test_create_faction(), r);
    lang = u->faction->locale;

    /* KONTAKTIERE EINHEIT <not-found> */
    ord = create_order(K_CONTACT, u->faction->locale, "%s %i",
        param_name(P_UNIT, lang), u->no + 1);
    contact_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "feedback_unit_not_found"));
    free_order(ord);
    test_clear_messages(u->faction);

    /* KONTAKTIERE EINHEIT TEMP <not-found> */
    ord = create_order(K_CONTACT, u->faction->locale, "%s %s %i",
        param_name(P_UNIT, lang), param_name(P_TEMP, lang), u->no + 1);
    contact_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "feedback_unit_not_found"));
    free_order(ord);
    test_clear_messages(u->faction);

    /* KONTAKTIERE EINHEIT TEMP */
    ord = create_order(K_CONTACT, u->faction->locale, "%s %s",
        param_name(P_UNIT, lang), param_name(P_TEMP, lang));
    contact_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "feedback_unit_not_found"));
    free_order(ord);
    test_clear_messages(u->faction);

    /* KONTAKTIERE EINHEIT */
    ord = create_order(K_CONTACT, u->faction->locale,
        param_name(P_UNIT, lang));
    contact_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "feedback_unit_not_found"));
    free_order(ord);
    test_clear_messages(u->faction);

    /* KONTAKTIERE TEMP */
    ord = create_order(K_CONTACT, u->faction->locale,
        param_name(P_TEMP, lang));
    contact_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "feedback_unit_not_found"));
    free_order(ord);
    test_clear_messages(u->faction);

    /* KONTAKTIERE */
    ord = create_order(K_CONTACT, u->faction->locale, NULL);
    contact_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "feedback_unit_not_found"));
    free_order(ord);
    test_clear_messages(u->faction);

    test_teardown();
}

CuSuite *get_contact_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_contact);
    SUITE_ADD_TEST(suite, test_contact_cmd);
    SUITE_ADD_TEST(suite, test_contact_cmd_not_local);
    SUITE_ADD_TEST(suite, test_contact_cmd_invalid);

    return suite;
}
