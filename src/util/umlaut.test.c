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

static void test_transliterations(CuTest *tc) {
    const char * umlauts = "\xc3\xa4\xc3\xb6\xc3\xbc\xc3\x9f"; /* auml ouml uuml szlig nul */
    void * tokens = 0;
    variant id;
    int result;

    id.i = 3;
    addtoken(&tokens, umlauts, id);
    /* transliteration is the real magic */
    result = findtoken(tokens, "AEoeUEss", &id);
    CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
    CuAssertIntEquals(tc, 3, id.i);

    result = findtoken(tokens, umlauts, &id);
    CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
    CuAssertIntEquals(tc, 3, id.i);

    freetokens(tokens);
}

static void test_directions(CuTest * tc)
{
    void * tokens = 0;
    variant id;
    int result;

    id.i = 1;
    addtoken(&tokens, "OSTEN", id);
    addtoken(&tokens, "O", id);
    
    id.i = 2;
    addtoken(&tokens, "nw", id);
    addtoken(&tokens, "northwest", id);
    
    result = findtoken(tokens, "ost", &id);
    CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
    CuAssertIntEquals(tc, 1, id.i);
    result = findtoken(tokens, "northw", &id);
    CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
    CuAssertIntEquals(tc, 2, id.i);
    freetokens(tokens);
}

static void test_umlaut(CuTest * tc)
{
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
    addtoken(&tokens, "d", id);

    /* we can find substrings if they are significant */
    result = findtoken(tokens, "herp", &id);
    CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
    CuAssertIntEquals(tc, 1, id.i);

    CuAssertIntEquals(tc, E_TOK_SUCCESS, findtoken(tokens, "derp", &id));
    CuAssertIntEquals(tc, E_TOK_SUCCESS, findtoken(tokens, "Derp", &id));
    CuAssertIntEquals(tc, E_TOK_SUCCESS, findtoken(tokens, "dErp", &id));
    CuAssertIntEquals(tc, E_TOK_SUCCESS, findtoken(tokens, "deRp", &id));
    CuAssertIntEquals(tc, E_TOK_SUCCESS, findtoken(tokens, "derP", &id));
    CuAssertIntEquals(tc, E_TOK_SUCCESS, findtoken(tokens, "DERP", &id));
    CuAssertIntEquals(tc, 2, id.i);

    result = findtoken(tokens, "herp-a-derp", &id);
    CuAssertIntEquals(tc, E_TOK_NOMATCH, result);

    freetokens(tokens);
}

CuSuite *get_umlaut_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_umlaut);
    SUITE_ADD_TEST(suite, test_directions);
    SUITE_ADD_TEST(suite, test_transliterate);
    SUITE_ADD_TEST(suite, test_transliterations);
    return suite;
}
