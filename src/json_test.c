#include <platform.h>
#include <CuTest.h>
#include <stream.h>
#include <memstream.h>
#include "json.h"
#include "bind_eressea.h"

static void test_export(CuTest * tc) {
    char buf[1024];
    stream out = { 0 };
    int err;
    
    mstream_init(&out);
    err = json_export(&out, EXPORT_REGIONS);
    CuAssertIntEquals(tc, 0, err);
    out.api->rewind(out.handle);
    out.api->read(out.handle, buf, sizeof(buf));
    CuAssertStrEquals(tc, "{\n}\n", buf);
    mstream_done(&out);
}
  
CuSuite *get_json_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_export);
    return suite;
}
