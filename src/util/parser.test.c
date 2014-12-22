#include <platform.h>
#include "parser.h"

#include <CuTest.h>

static void test_parser(CuTest *tc) {
    init_tokens_str("HELP ONE TWO THREE");
}

CuSuite *get_parser_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_parser);
    return suite;
}
