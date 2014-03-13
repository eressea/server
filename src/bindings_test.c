#include <platform.h>
#include <CuTest.h>
#include <stream.h>
#include <memstream.h>
#include "export.h"
#include "bind_eressea.h"

static void test_export(CuTest * tc) {
    char buf[1024];
    stream out = { 0 };
    int err;
    
    mstream_init(&out);
    err = export_json(&out, EXPORT_REGIONS);
    CuAssertIntEquals(tc, 0, err);
    out.api->rewind(out.handle);
    out.api->readln(out.handle, buf, sizeof(buf));
    CuAssertStrEquals(tc, "{}", buf);
    mstream_done(&out);
}
  
CuSuite *get_bindings_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_export);
    return suite;
}
