#include <CuTest.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "umlaut.h"

static void test_transliterate(CuTest * tc)
{
    char buffer[32];

    CuAssertStrEquals(tc, "", transliterate(buffer, sizeof(buffer), ""));
    CuAssertStrEquals(tc, "herpderp", transliterate(buffer, sizeof(buffer), "herpderp"));
    CuAssertStrEquals(tc, "herpderp", buffer);

    CuAssertStrEquals(tc, "herpderp", transliterate(buffer, sizeof(buffer), "HERPDERP"));
    CuAssertStrEquals(tc, "haerpdaerp", transliterate(buffer, sizeof(buffer), "h\xc3\xa4rpd\xc3\xa4rp"));
    CuAssertStrEquals(tc, "aeoeuess", transliterate(buffer, sizeof(buffer), "\xc3\xa4\xc3\xb6\xc3\xbc\xc3\x9f"));
    CuAssertStrEquals(tc, "aeoeuess", transliterate(buffer, sizeof(buffer), "\xc3\x84\xc3\x96\xc3\x9c\xe1\xba\x9e"));

    /* handle buffer that is too small */
    CuAssertStrEquals(tc, 0, transliterate(buffer, 1, "herpderp"));
    CuAssertStrEquals(tc, "", buffer);
    CuAssertStrEquals(tc, 0, transliterate(buffer, 3, "herpderp"));
    CuAssertStrEquals(tc, "he", buffer);
    CuAssertStrEquals(tc, 0, transliterate(buffer, 3, "h\xc3\xa4rpd\xc3\xa4rp"));
    CuAssertStrEquals(tc, "h?", buffer);
}

static void test_umlaut(CuTest * tc)
{
    const char * umlauts = "\xc3\xa4\xc3\xb6\xc3\xbc\xc3\x9f"; /* auml ouml uuml szlig nul */
    void * tokens = 0;
    variant id;
    int result;

    /* don't crash on an empty set */
    result = findtoken(tokens, "herpderp", &id);
    CuAssertIntEquals(tc, E_TOK_NOMATCH, result);

    id.i = 1;
    addtoken(&tokens, "herpderp", id);
    id.i = 2;
    addtoken(&tokens, "derp", id);
    id.i = 3;
    addtoken(&tokens, umlauts, id);

    /* we can find substrings if they are significant */
    result = findtoken(tokens, "herp", &id);
    CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
    CuAssertIntEquals(tc, 1, id.i);

    result = findtoken(tokens, "DERP", &id);
    CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
    CuAssertIntEquals(tc, 2, id.i);

    result = findtoken(tokens, umlauts, &id);
    CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
    CuAssertIntEquals(tc, 3, id.i);

    /* transliteration is the real magic */
    result = findtoken(tokens, "AEoeUEss", &id);
    CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
    CuAssertIntEquals(tc, 3, id.i);

    result = findtoken(tokens, "herp-a-derp", &id);
    CuAssertIntEquals(tc, E_TOK_NOMATCH, result);

    freetokens(tokens);
}

CuSuite *get_umlaut_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_umlaut);
    SUITE_ADD_TEST(suite, test_transliterate);
    return suite;
}
