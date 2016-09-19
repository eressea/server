#include <platform.h>

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/unit.h>
#include <util/attrib.h>
#include <util/gamedata.h>
#include <util/message.h>
#include <binarystore.h>
#include <filestream.h>
#include <memstream.h>
#include <storage.h>
#include <stream.h>
#include <tests.h>

#include "curse.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <CuTest.h>


static void test_curse(CuTest * tc)
{
    attrib *attrs = NULL;
    curse *c, *result;
    int cid;

    curse_type ct_dummy = { "dummy", CURSETYP_NORM, 0, M_SUMEFFECT, NULL };
    c = create_curse(NULL, &attrs, &ct_dummy, 1.0, 1, 1, 1);
    cid = c->no;
    result = findcurse(cid);
    CuAssertPtrEquals(tc, c, result);
    a_remove(&attrs, attrs);
    result = findcurse(cid);
    CuAssertPtrEquals(tc, NULL, result);
    test_cleanup();
}

typedef struct {
    curse *c;
    region *r;
    unit *u;
} curse_fixture;

static void setup_curse(curse_fixture *fix, const char *name) {
    test_cleanup();
    fix->r = test_create_region(0, 0, NULL);
    fix->u = test_create_unit(test_create_faction(NULL), fix->r);
    fix->c = create_curse(fix->u, &fix->r->attribs, ct_find(name), 1.0, 1, 1.0, 0);
}

static void cleanup_curse(curse_fixture *fix) {
    // destroy_curse(fix->c);
    test_cleanup();
}

static void test_magicstreet(CuTest *tc) {
    curse_fixture fix;
    message *msg;
    setup_curse(&fix, "magicstreet");
    fix.c->duration = 2;
    msg = fix.c->type->curseinfo(fix.r, TYP_REGION, fix.c, 0);
    CuAssertStrEquals(tc, "curseinfo::magicstreet", test_get_messagetype(msg));
    msg_release(msg);
    cleanup_curse(&fix);
}

static void test_magicstreet_warning(CuTest *tc) {
    curse_fixture fix;
    message *msg;
    setup_curse(&fix, "magicstreet");
    fix.c->duration = 1;
    msg = fix.c->type->curseinfo(fix.r, TYP_REGION, fix.c, 0);
    CuAssertStrEquals(tc, "curseinfo::magicstreetwarn", test_get_messagetype(msg));
    msg_release(msg);
    cleanup_curse(&fix);
}

static void test_good_dreams(CuTest *tc) {
    curse_fixture fix;
    message *msg;
    setup_curse(&fix, "gbdream");
    fix.c->effect = 1;
    msg = fix.c->type->curseinfo(fix.r, TYP_REGION, fix.c, 0);
    CuAssertStrEquals(tc, "curseinfo::gooddream", test_get_messagetype(msg));
    msg_release(msg);
    cleanup_curse(&fix);
}

static void test_bad_dreams(CuTest *tc) {
    curse_fixture fix;
    message *msg;
    setup_curse(&fix, "gbdream");
    fix.c->effect = -1;
    msg = fix.c->type->curseinfo(fix.r, TYP_REGION, fix.c, 0);
    CuAssertStrEquals(tc, "curseinfo::baddream", test_get_messagetype(msg));
    msg_release(msg);
    cleanup_curse(&fix);
}

static void test_memstream(CuTest *tc) {
    storage store;
    stream out = { 0 };
    char buf[1024];
    int val=0;

    mstream_init(&out);
    binstore_init(&store, &out);
    store.handle.data = &out;

    WRITE_INT(&store, 999999);
    WRITE_TOK(&store, "fortytwo");
    WRITE_INT(&store, 42);

    out.api->rewind(out.handle);
    READ_INT(&store, &val);
    READ_TOK(&store, buf, 1024);
    CuAssertIntEquals(tc, 999999, val);
    CuAssertStrEquals(tc, "fortytwo", buf);
    READ_INT(&store, &val);
    CuAssertIntEquals(tc, 42, val);
    mstream_done(&out);
}

static void test_write_flag(CuTest *tc) {
    curse_fixture fix;
    gamedata data;
    storage store;
    region * r;
    curse * c;
    int uid;

    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);

    setup_curse(&fix, "gbdream");
    c = fix.c;
    r = fix.r;
    uid = r->uid;
    c->flags = CURSE_ISNEW;
    write_game(&data);
    free_gamedata();
    data.strm.api->rewind(data.strm.handle);
    read_game(&data);
    r = findregionbyid(uid);
    CuAssertPtrNotNull(tc, r);
    CuAssertPtrNotNull(tc, r->attribs);
    c = (curse *)r->attribs->data.v;
    CuAssertIntEquals(tc, CURSE_ISNEW, c->flags);

    mstream_done(&data.strm);
    gamedata_done(&data);
    cleanup_curse(&fix);
}

static void test_curse_cache(CuTest *tc)
{
    int cache = 0;
    const curse_type ct_dummy = { "dummy", CURSETYP_NORM, 0, M_SUMEFFECT, NULL };    
    test_setup();
    CuAssertIntEquals(tc, true, ct_changed(&cache));
    CuAssertIntEquals(tc, false, ct_changed(&cache));
    CuAssertPtrEquals(tc, NULL, (void *)ct_find(ct_dummy.cname));
    ct_register(&ct_dummy);
    CuAssertIntEquals(tc, true, ct_changed(&cache));
    CuAssertPtrEquals(tc, (void *)&ct_dummy, (void *)ct_find(ct_dummy.cname));
    ct_remove(ct_dummy.cname);
    CuAssertIntEquals(tc, true, ct_changed(&cache));
    CuAssertIntEquals(tc, false, ct_changed(&cache));
    CuAssertPtrEquals(tc, NULL, (void *)ct_find(ct_dummy.cname));
    test_cleanup();
}

CuSuite *get_curse_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_curse);
    SUITE_ADD_TEST(suite, test_curse_cache);
    SUITE_ADD_TEST(suite, test_magicstreet);
    SUITE_ADD_TEST(suite, test_magicstreet_warning);
    SUITE_ADD_TEST(suite, test_good_dreams);
    SUITE_ADD_TEST(suite, test_bad_dreams);
    SUITE_ADD_TEST(suite, test_memstream);
    SUITE_ADD_TEST(suite, test_write_flag);
    return suite;
}
