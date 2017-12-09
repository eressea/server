#include <platform.h>

#include "json.h"
#include "tests.h"

#include <kernel/region.h>
#include <kernel/terrain.h>

#include <stream.h>
#include <memstream.h>

#include <cJSON.h>
#include <CuTest.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


static char *strip(char *str) {
    char *s = str, *b = str, *e = str;
    /* b is where text begins, e where it ends, s where we insert it. */
    for (; *b && isspace(*b); ++b) {};
    for (e = b; *e && !isspace(*e); ++e) {};
    while (*b) {
        if (s != b) {
            memcpy(s, b, e - b);
        }
        s += e - b;
        for (b = e; *b && isspace(*b); ++b) {};
        for (e = b; *e && !isspace(*e); ++e) {};
    }
    *s = 0;
    return str;
}

static void test_export_no_regions(CuTest * tc) {
    char buf[1024];
    stream out = { 0 };
    int err;
    size_t len;

    mstream_init(&out);
    err = json_export(&out, EXPORT_REGIONS);
    CuAssertIntEquals(tc, 0, err);
    out.api->rewind(out.handle);
    len = out.api->read(out.handle, buf, sizeof(buf));
    buf[len] = '\0';
    CuAssertStrEquals(tc, "{}", strip(buf));
    mstream_done(&out);
    test_cleanup();
}

static cJSON *export_a_region(CuTest * tc, const struct terrain_type *terrain, region **_r) {
    char buf[1024];
    stream out = { 0 };
    int err;
    region *r;
    cJSON *json, *attr, *result, *regs;

    r = test_create_region(0, 0, terrain);

    mstream_init(&out);
    err = json_export(&out, EXPORT_REGIONS);
    CuAssertIntEquals(tc, 0, err);
    out.api->rewind(out.handle);
    out.api->read(out.handle, buf, sizeof(buf));
    mstream_done(&out);

    json = cJSON_Parse(buf);
    CuAssertPtrNotNull(tc, json);
    CuAssertIntEquals(tc, cJSON_Object, json->type);
    CuAssertPtrEquals(tc, 0, cJSON_GetObjectItem(json, "factions"));
    CuAssertPtrEquals(tc, 0, cJSON_GetObjectItem(json, "units"));
    CuAssertPtrNotNull(tc, regs = cJSON_GetObjectItem(json, "regions"));
    CuAssertIntEquals(tc, cJSON_Object, regs->type);
    result = regs->child;
    regs->child = 0;
    cJSON_Delete(json);
    CuAssertIntEquals(tc, r->uid, atoi(result->string));
    CuAssertPtrNotNull(tc, attr = cJSON_GetObjectItem(result, "x"));
    CuAssertIntEquals(tc, r->x, attr->valueint);
    CuAssertPtrNotNull(tc, attr = cJSON_GetObjectItem(result, "y"));
    CuAssertIntEquals(tc, r->y, attr->valueint);
    CuAssertPtrNotNull(tc, attr = cJSON_GetObjectItem(result, "type"));
    CuAssertStrEquals(tc, terrain->_name, attr->valuestring);

    if (_r) *_r = r;
    return result;
}

static void test_export_land_region(CuTest * tc) {
    region *r;
    struct terrain_type *terrain;
    cJSON *json, *attr;
    test_cleanup();
    terrain = test_create_terrain("plain", LAND_REGION);
    json = export_a_region(tc, terrain, &r);
    CuAssertPtrNotNull(tc, attr = cJSON_GetObjectItem(json, "name"));
    CuAssertStrEquals(tc, r->land->name, attr->valuestring);
    cJSON_Delete(json);
    test_cleanup();
}

static void test_export_ocean_region(CuTest * tc) {
    struct terrain_type *terrain;
    cJSON *json;
    test_cleanup();
    terrain = test_create_terrain("ocean", SEA_REGION);
    json = export_a_region(tc, terrain, 0);
    CuAssertPtrEquals(tc, 0, cJSON_GetObjectItem(json, "name"));
    cJSON_Delete(json);
    test_cleanup();
}

static void test_export_no_factions(CuTest * tc) {
    char buf[1024];
    stream out = { 0 };
    int err;
    size_t len;

    mstream_init(&out);
    err = json_export(&out, EXPORT_FACTIONS);
    CuAssertIntEquals(tc, 0, err);
    out.api->rewind(out.handle);
    len = out.api->read(out.handle, buf, sizeof(buf));
    buf[len] = 0;
    CuAssertStrEquals(tc, "{}", strip(buf));
    mstream_done(&out);
    test_cleanup();
}

CuSuite *get_json_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_export_no_regions);
    SUITE_ADD_TEST(suite, test_export_ocean_region);
    SUITE_ADD_TEST(suite, test_export_land_region);
    SUITE_ADD_TEST(suite, test_export_no_factions);
    return suite;
}
