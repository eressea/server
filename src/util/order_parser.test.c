#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "order_parser.h"

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
    const char* input;

    parser = OP_ParserCreate();
    OP_SetUserData(parser, lastline);
    OP_SetOrderHandler(parser, copy_line);
    CuAssertPtrNotNull(tc, parser);

    lastline[0] = 0;
    input = "Hello World";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    input = "Error;\nHello World";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    input = "Hello World; Comment\nError";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 1));
    CuAssertStrEquals(tc, "Error", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    input = "Hello \"World;\"";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 1));
    CuAssertStrEquals(tc, input, lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    input = "Hello 'World;'";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 1));
    CuAssertStrEquals(tc, input, lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    input = "Hello \"';World;\"";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 1));
    CuAssertStrEquals(tc, input, lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    input = "Hello World;\\\nError";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    input = "SAY \"Hello; World\"";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 1));
    CuAssertStrEquals(tc, "SAY \"Hello; World\"", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    input = "Hello World;\\";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 0));
    input = "\nError";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    input = "Hello \\";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 0));
    input = "\nWorld";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    input = "Hello World\n";
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, input, strlen(input), 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello\\\n World", 13, 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello;World", 11, 1));
    CuAssertStrEquals(tc, "Hello", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello\\World", 11, 1));
    CuAssertStrEquals(tc, "Hello\\World", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, ";\n", 2, 0));
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello World", 11, 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, ";Hello \\", 8, 0));
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "\nWorld\n", 7, 0));
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Enno", 4, 1));
    CuAssertStrEquals(tc, "Enno", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, ";Hello", 6, 0));
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "World\n", 6, 0));
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Enno", 4, 1));
    CuAssertStrEquals(tc, "Enno", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello\\World  \\", 14, 1));
    CuAssertStrEquals(tc, "Hello\\World  ", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello\n", 6, 0));
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "World\n", 6, 1));
    CuAssertStrEquals(tc, "World", lastline);
    OP_ParserReset(parser);

    lastline[0] = 0;
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello\n", 6, 0));
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "World\n", 6, 1));
    CuAssertStrEquals(tc, "World", lastline);
    OP_ParserReset(parser);
    lastline[0] = 0;
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "Hello \\", 7, 0));
    CuAssertIntEquals(tc, OP_STATUS_OK, OP_Parse(parser, "\nWorld", 6, 1));
    CuAssertStrEquals(tc, "Hello World", lastline);
    OP_ParserFree(parser);
}

CuSuite *get_order_parser_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_parse_noop);
    SUITE_ADD_TEST(suite, test_parse_orders);
    return suite;
}
