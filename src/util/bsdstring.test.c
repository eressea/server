#ifdef _MSC_VER
#include <platform.h>
#endif
#include <CuTest.h>
#include "bsdstring.h"
#include <errno.h>
#include <string.h>

CuSuite *get_bsdstring_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    return suite;
}
