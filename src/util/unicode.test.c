#include <platform.h>
#include <CuTest.h>
#include "unicode.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void test_unicode_mkname(CuTest * tc)
{
    char buffer[32];
    CuAssertIntEquals(tc, 0, unicode_utf8_mkname(buffer, sizeof(buffer), "HeLlO W0Rld"));
    CuAssertStrEquals(tc, "HeLlO W0Rld", buffer);
    memset(buffer, 0, sizeof(buffer));
    CuAssertIntEquals(tc, 1, unicode_utf8_mkname(buffer, sizeof(buffer), "HeLlO\nW0Rld"));
    CuAssertStrEquals(tc, "HeLlOW0Rld", buffer);
    memset(buffer, 0, sizeof(buffer));
    buffer[5] = 'X';
    CuAssertIntEquals(tc, ENOMEM, unicode_utf8_mkname(buffer, 5, "HeLl\n W0Rld"));
    CuAssertStrEquals(tc, "HeLl X", buffer);
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

CuSuite *get_unicode_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_unicode_tolower);
    SUITE_ADD_TEST(suite, test_unicode_mkname);
    SUITE_ADD_TEST(suite, test_unicode_utf8_to_other);
    return suite;
}
