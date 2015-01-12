#include <platform.h>
#include "types.h"
#include "curse.h"

#include <kernel/region.h>
#include <kernel/unit.h>
#include <util/attrib.h>
#include <util/message.h>
#include <tests.h>

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
  destroy_curse(c);
  result = findcurse(cid);
  CuAssertPtrEquals(tc, NULL, result);
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

static void test_magicstreet(CuTest *tc) {
    curse_fixture fix;
    message *msg;
    setup_curse(&fix, "magicstreet");
    fix.c->duration = 2;
    msg = fix.c->type->curseinfo(fix.r, TYP_REGION, fix.c, 0);
    CuAssertStrEquals(tc, "curseinfo::magicstreet", test_get_messagetype(msg));
    msg_release(msg);
    test_cleanup();
}

static void test_magicstreet_warning(CuTest *tc) {
    curse_fixture fix;
    message *msg;
    setup_curse(&fix, "magicstreet");
    fix.c->duration = 1;
    msg = fix.c->type->curseinfo(fix.r, TYP_REGION, fix.c, 0);
    CuAssertStrEquals(tc, "curseinfo::magicstreetwarn", test_get_messagetype(msg));
    msg_release(msg);
    test_cleanup();
}

static void test_good_dreams(CuTest *tc) {
    curse_fixture fix;
    message *msg;
    setup_curse(&fix, "gbdream");
    fix.c->effect = 1;
    msg = fix.c->type->curseinfo(fix.r, TYP_REGION, fix.c, 0);
    CuAssertStrEquals(tc, "curseinfo::gooddream", test_get_messagetype(msg));
    msg_release(msg);
    test_cleanup();
}

static void test_bad_dreams(CuTest *tc) {
    curse_fixture fix;
    message *msg;
    setup_curse(&fix, "gbdream");
    fix.c->effect = -1;
    msg = fix.c->type->curseinfo(fix.r, TYP_REGION, fix.c, 0);
    CuAssertStrEquals(tc, "curseinfo::baddream", test_get_messagetype(msg));
    msg_release(msg);
    test_cleanup();
}

CuSuite *get_curse_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_curse);
  SUITE_ADD_TEST(suite, test_magicstreet);
  SUITE_ADD_TEST(suite, test_magicstreet_warning);
  SUITE_ADD_TEST(suite, test_good_dreams);
  SUITE_ADD_TEST(suite, test_bad_dreams);
  return suite;
}
