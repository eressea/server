#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "unicode.h"

#include <CuTest.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static const char utf8_nbsp[] = { 0xc2, 0xa0, 0 };          // NON BREAKING SPACE
static const char utf8_202f[] = { 0xe2, 0x80, 0xaf, 0 };    // NARROW NO-BREAK SPACE
static const char utf8_nel[] = { 0xc2, 0x85, 0 };           // NEXT LINE
static const char utf8_2007[] = { 0xe2, 0x80, 0x87, 0 };    // FIGURE SPACE
static const char utf8_lrm[] = { 0xe2, 0x80, 0x8e, 0 };     // LEFT-TO-RIGHT MARK
static const char utf8_33f0[] = { 0xE2, 0x8F, 0xB0, 0x00 }; // ALARM CLOCK
static const char utf8_zwnj[] = { 0xe2, 0x80, 0x8c, 0 };    // ZERO-WIDTH NON-JOINER 

static void test_unicode_clean(CuTest* tc)
{
    char buffer[32];

    strcpy(buffer, "He\n\tll\ro");
    unicode_utf8_clean(buffer);
    CuAssertStrEquals(tc, "Hello", buffer);

    memcpy(buffer + 2, utf8_nel, 2);
    unicode_utf8_clean(buffer);
    CuAssertStrEquals(tc, "Heo", buffer);
}

static void test_unicode_trim(CuTest* tc)
{
    char buffer[32];

    strcpy(buffer, "Hello");
    CuAssertIntEquals(tc, 0, (int)unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello", buffer);

    strcpy(buffer, "Hello World");
    CuAssertIntEquals(tc, 0, (int)unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello World", buffer);

    strcpy(buffer, "  Hello World");
    CuAssertIntEquals(tc, 2, (int)unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello World", buffer);

    strcpy(buffer, "Hello World  ");
    CuAssertIntEquals(tc, 2, (int)unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello World", buffer);

    strcpy(buffer, " Hello  World ");
    CuAssertIntEquals(tc, 2, (int)unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello  World", buffer);

    strcpy(buffer, "LRM");
    buffer[3] = -30;
    buffer[4] = -128;
    buffer[5] = -114;
    buffer[6] = 0;
    CuAssertIntEquals(tc, 3, (int)unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "LRM", buffer);

    strcpy(buffer, "  Hello Word  ");
    CuAssertIntEquals(tc, 4, (int)unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello Word", buffer);

    strcpy(buffer, "Hello Word\n");
    CuAssertIntEquals(tc, 1, (int)unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello Word", buffer);

    strcpy(buffer, "  Hello Word\t\n");
    CuAssertIntEquals(tc, 4, (int)unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello Word", buffer);

    strcpy(buffer, " \t Hello Word");
    CuAssertIntEquals(tc, 3, (int)unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello Word", buffer);

    strcpy(buffer, "Hello World");
    buffer[9] = -61;
    CuAssertIntEquals(tc, EILSEQ, (int)unicode_utf8_trim(buffer));
    CuAssertStrEquals(tc, "Hello Wor", buffer);
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
    const unsigned char uchar_str[] = { 0xc3, 0x98, 0xc5, 0xb8, 0xc2, 0x9d, 'l', 0 }; /* &Oslash;&Yuml;&#157;l */
    char *utf8_str = (char *)uchar_str;
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
    wchar_t wc;
    size_t sz;

    CuAssertIntEquals(tc, 0, unicode_utf8_decode(&wc, "a", &sz));
    CuAssertIntEquals(tc, 'a', wc);
    CuAssertIntEquals(tc, 1, (int)sz);
}

static void test_unicode_bug2262(CuTest *tc) {
    char name[7];
    wchar_t wc;
    size_t sz;

    strcpy(name, "utende");
    CuAssertIntEquals(tc, 0, unicode_utf8_decode(&wc, name, &sz));
    CuAssertIntEquals(tc, 1, (int)sz);
    CuAssertIntEquals(tc, 'u', wc);
    CuAssertIntEquals(tc, 0, (int)unicode_utf8_trim(name));

    name[0] = -4; /* latin1: &uuml; should fail to decode */
    CuAssertIntEquals(tc, EILSEQ, unicode_utf8_decode(&wc, name, &sz));
    CuAssertIntEquals(tc, EILSEQ, (int)unicode_utf8_trim(name));
}

static void test_unicode_compare(CuTest *tc)
{
    CuAssertIntEquals(tc, 0, unicode_utf8_strcasecmp("ABCDEFG", "abcdefg"));
    CuAssertIntEquals(tc, 0, unicode_utf8_strcasecmp("abcdefg123", "ABCDEFG123"));
    CuAssertIntEquals(tc, 1, unicode_utf8_strcasecmp("bacdefg123", "ABCDEFG123"));
    CuAssertIntEquals(tc, -1, unicode_utf8_strcasecmp("abc", "abcD"));
}

static void test_unicode_trim_zwnj(CuTest *tc) {
    char name[64];
    char expect[64];
    snprintf(name, sizeof(name), "%sA%sB%s  ", utf8_zwnj, utf8_zwnj, utf8_zwnj);
    snprintf(expect, sizeof(expect), "A%sB", utf8_zwnj);
    CuAssertIntEquals(tc, 8, (int)unicode_utf8_trim(name));
    CuAssertStrEquals(tc, expect, name);
}

static void test_unicode_trim_nbsp(CuTest *tc) {
    char name[64];
    char expect[64];
    snprintf(name, sizeof(name), "%sA%sB%s  ", utf8_nbsp, utf8_nbsp, utf8_nbsp);
    snprintf(expect, sizeof(expect), "A%sB", utf8_nbsp);
    CuAssertIntEquals(tc, 6, (int)unicode_utf8_trim(name));
    CuAssertStrEquals(tc, expect, name);
}

static void test_unicode_trim_nnbsp(CuTest *tc) {
    char name[64];
    char expect[64];
    snprintf(name, sizeof(name), "%sA%sB%s  ", utf8_202f, utf8_202f, utf8_202f);
    snprintf(expect, sizeof(expect), "A%sB", utf8_202f);
    CuAssertIntEquals(tc, 8, (int)unicode_utf8_trim(name));
    CuAssertStrEquals(tc, expect, name);
}

static void test_unicode_trim_figure_space(CuTest *tc) {
    char name[64];
    char expect[64];
    snprintf(name, sizeof(name), "%sA%sB%s  ", utf8_2007, utf8_2007, utf8_2007);
    snprintf(expect, sizeof(expect), "A%sB", utf8_2007);
    CuAssertIntEquals(tc, 8, (int)unicode_utf8_trim(name));
    CuAssertStrEquals(tc, expect, name);
}

static void test_unicode_trim_lrm(CuTest *tc) {
    char name[64];
    char expect[64];
    snprintf(name, sizeof(name), "%sBrot%szeit%s  ", utf8_lrm, utf8_lrm, utf8_lrm);
    snprintf(expect, sizeof(expect), "Brot%szeit", utf8_lrm);
    CuAssertIntEquals(tc, 8, (int)unicode_utf8_trim(name));
    CuAssertStrEquals(tc, expect, name);
}

static void test_unicode_trim_emoji(CuTest *tc) {
    char name[64];
    char expect[64];
    snprintf(name, sizeof(name), "%s Alarm%sClock %s", utf8_33f0, utf8_33f0, utf8_33f0);
    strcpy(expect, name);
    CuAssertIntEquals(tc, 0, (int)unicode_utf8_trim(name));
    CuAssertStrEquals(tc, expect, name);
}

CuSuite *get_unicode_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_unicode_clean);
    SUITE_ADD_TEST(suite, test_unicode_trim);
    SUITE_ADD_TEST(suite, test_unicode_trim_zwnj);
    SUITE_ADD_TEST(suite, test_unicode_trim_nbsp);
    SUITE_ADD_TEST(suite, test_unicode_trim_nnbsp);
    SUITE_ADD_TEST(suite, test_unicode_trim_figure_space);
    SUITE_ADD_TEST(suite, test_unicode_trim_lrm);
    SUITE_ADD_TEST(suite, test_unicode_trim_emoji);
    SUITE_ADD_TEST(suite, test_unicode_utf8_to_other);
    SUITE_ADD_TEST(suite, test_unicode_utf8_to_ucs);
    SUITE_ADD_TEST(suite, test_unicode_compare);
    SUITE_ADD_TEST(suite, test_unicode_bug2262);
    SUITE_ADD_TEST(suite, test_unicode_tolower);
    return suite;
}
