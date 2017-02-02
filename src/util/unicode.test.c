#include <platform.h>
#include <CuTest.h>
#include "unicode.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void test_unicode_trim(CuTest * tc)
{
    char buffer[32];

    strcpy(buffer, "Hello Word");
    CuAssertIntEquals(tc, 0, unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello Word", buffer);

    strcpy(buffer, "  Hello Word  ");
    CuAssertIntEquals(tc, 4, unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello Word", buffer);

    strcpy(buffer, "Hello Word\n");
    CuAssertIntEquals(tc, 1, unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello Word", buffer);

    strcpy(buffer, "  Hello Word\t\n");
    CuAssertIntEquals(tc, 4, unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello Word", buffer);

    strcpy(buffer, " \t Hello Word");
    CuAssertIntEquals(tc, 3, unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello Word", buffer);

    buffer[9] = -61;
    CuAssertIntEquals(tc, 1, unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello Wor?", buffer);
}

static void test_unicode_tolower(CuTest * tc)
{
    char buffer[32];
    CuAssertIntEquals(tc, 0, unicode_utf8_tolower(buffer, sizeof(buffer), "HeLlO W0Rld"));
    CuAssertStrEquals(tc, "hello w0rld", buffer);
    memset(buffer, 0, sizeof(buffer));
    buffer[5] = 'X';
    CuAssertIntEquals(tc, ENOMEM, unicode_utf8_tolower(buffer, 5, "HeLlO W0Rld"));
    CuAssertStrEquals(tc, "helloX", buffer);
}

static void test_unicode_utf8_to_other(CuTest *tc)
{
    const unsigned char uchar_str[] = { 0xc3, 0x98, 0xc5, 0xb8, 0xc2, 0x9d, 'l', 0 }; // &Oslash;&Yuml;&#157;l
    utf8_t *utf8_str = (utf8_t *)uchar_str;
    unsigned char ch;
    size_t sz;
    CuAssertIntEquals(tc, 0, unicode_utf8_to_cp437(&ch, utf8_str, &sz));
    CuAssertIntEquals(tc, 2, (int)sz);
    CuAssertIntEquals(tc, '?', ch);
    CuAssertIntEquals(tc, 0, unicode_utf8_to_cp437(&ch, utf8_str+2, &sz));
    CuAssertIntEquals(tc, 2, (int)sz);
    CuAssertIntEquals(tc, '?', ch);
    CuAssertIntEquals(tc, 0, unicode_utf8_to_cp437(&ch, utf8_str+4, &sz));
    CuAssertIntEquals(tc, 2, (int)sz);
    CuAssertIntEquals(tc, '?', ch);
    CuAssertIntEquals(tc, 0, unicode_utf8_to_cp437(&ch, utf8_str + 6, &sz));
    CuAssertIntEquals(tc, 1, (int)sz);
    CuAssertIntEquals(tc, 'l', ch);

    CuAssertIntEquals(tc, 0, unicode_utf8_to_cp1252(&ch, utf8_str, &sz));
    CuAssertIntEquals(tc, 2, (int)sz);
    CuAssertIntEquals(tc, 216, ch);
    CuAssertIntEquals(tc, 0, unicode_utf8_to_cp1252(&ch, utf8_str+2, &sz));
    CuAssertIntEquals(tc, 2, (int)sz);
    CuAssertIntEquals(tc, 120, ch);
    CuAssertIntEquals(tc, 0, unicode_utf8_to_cp1252(&ch, utf8_str + 4, &sz));
    CuAssertIntEquals(tc, 2, (int)sz);
    CuAssertIntEquals(tc, 0x9d, ch);
    CuAssertIntEquals(tc, 0, unicode_utf8_to_cp1252(&ch, utf8_str + 6, &sz));
    CuAssertIntEquals(tc, 1, (int)sz);
    CuAssertIntEquals(tc, 'l', ch);

    CuAssertIntEquals(tc, 0, unicode_utf8_to_ascii(&ch, utf8_str, &sz));
    CuAssertIntEquals(tc, 2, (int)sz);
    CuAssertIntEquals(tc, '?', ch);
    CuAssertIntEquals(tc, 0, unicode_utf8_to_ascii(&ch, utf8_str + 2, &sz));
    CuAssertIntEquals(tc, 2, (int)sz);
    CuAssertIntEquals(tc, '?', ch);
    CuAssertIntEquals(tc, 0, unicode_utf8_to_ascii(&ch, utf8_str + 4, &sz));
    CuAssertIntEquals(tc, 2, (int)sz);
    CuAssertIntEquals(tc, '?', ch);
    CuAssertIntEquals(tc, 0, unicode_utf8_to_ascii(&ch, utf8_str + 6, &sz));
    CuAssertIntEquals(tc, 1, (int)sz);
    CuAssertIntEquals(tc, 'l', ch);
}

static void test_unicode_utf8_to_ucs(CuTest *tc) {
    ucs4_t ucs;
    size_t sz;

    CuAssertIntEquals(tc, 0, unicode_utf8_to_ucs4(&ucs, "a", &sz));
    CuAssertIntEquals(tc, 'a', ucs);
    CuAssertIntEquals(tc, 1, sz);
}

static void test_unicode_bug2262(CuTest *tc) {
    char name[7];
    ucs4_t ucs;
    size_t sz;

    strcpy(name, "utende");
    CuAssertIntEquals(tc, 0, unicode_utf8_to_ucs4(&ucs, name, &sz));
    CuAssertIntEquals(tc, 1, sz);
    CuAssertIntEquals(tc, 'u', ucs);
    CuAssertIntEquals(tc, 0, unicode_utf8_trim(name));

    name[0] = -4; // latin1: &uuml; should fail to decode
    CuAssertIntEquals(tc, EILSEQ, unicode_utf8_to_ucs4(&ucs, name, &sz));
    CuAssertIntEquals(tc, EILSEQ, unicode_utf8_trim(name));
}

static void test_unicode_compare(CuTest *tc)
{
    CuAssertIntEquals(tc, 0, unicode_utf8_strcasecmp("ABCDEFG", "abcdefg"));
    CuAssertIntEquals(tc, 0, unicode_utf8_strcasecmp("abcdefg123", "ABCDEFG123"));
    CuAssertIntEquals(tc, 1, unicode_utf8_strcasecmp("bacdefg123", "ABCDEFG123"));
}

CuSuite *get_unicode_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_unicode_bug2262);
    SUITE_ADD_TEST(suite, test_unicode_tolower);
    SUITE_ADD_TEST(suite, test_unicode_trim);
    SUITE_ADD_TEST(suite, test_unicode_utf8_to_other);
    SUITE_ADD_TEST(suite, test_unicode_utf8_to_ucs);
    SUITE_ADD_TEST(suite, test_unicode_compare);
    return suite;
}
