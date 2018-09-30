#ifdef _MSC_VER
#include <platform.h>
#endif

#include "order_parser.h"
#include "strings.h"

#include <CuTest.h>

#include <string.h>

static void test_parse_noop(CuTest *tc) {
    OP_Parser parser;
    parser = OP_ParserCreate();
    CuAssertPtrNotNull(tc, parser);
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello World", 11, 1));
    OP_ParserFree(parser);
}

static void copy_line(void *udata, const char *str) {
    char *dst = (char *)udata;
    if (dst) {
        strcpy(dst, str);
    }
}

static void test_parse_orders(CuTest *tc) {
    OP_Parser parser;
    char lastline[1024];

    parser = OP_ParserCreate();
    OP_SetUserData(parser, lastline);
    OP_SetOrderHandler(parser, copy_line);
    CuAssertPtrNotNull(tc, parser);
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello World", 11, 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello World\n", 12, 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello\\\n World", 13, 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello;World", 11, 1));
    CuAssertStrEquals(tc, "Hello", lastline);
    OP_ParserReset(parser);
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello\\World", 11, 1));
    CuAssertStrEquals(tc, "Hello\\World", lastline);
    OP_ParserReset(parser);
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello ", 6, 0));
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "World", 5, 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello\\World  \\", 14, 1));
    CuAssertStrEquals(tc, "Hello\\World  ", lastline);
    OP_ParserFree(parser);
}

CuSuite *get_order_parser_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_parse_noop);
    SUITE_ADD_TEST(suite, test_parse_orders);
    return suite;
}
