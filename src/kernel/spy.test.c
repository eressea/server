#include <platform.h>
#include "types.h"
#include "spy.h"
#include "magic.h"

#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <util/attrib.h>
#include <util/message.h>
#include <util/crmessage.h>
#include <tests.h>

#include <stdio.h>

#include <CuTest.h>

typedef struct {
    region *r;
    unit *spy;
    unit *victim;
} spy_fixture;

static void setup_spy(spy_fixture *fix) {
    test_cleanup();
    fix->r = test_create_region(0, 0, NULL);
    fix->spy = test_create_unit(test_create_faction(NULL), fix->r);
    fix->victim = test_create_unit(test_create_faction(NULL), fix->r);
}


static const message_type *register_msg(const char *type, int n_param, ...) {
    char **argv;
    va_list args;
    int i;

    va_start(args, n_param);

    argv = malloc(sizeof(char *) * (n_param+1));
    for (i=0; i<n_param; ++i) {
	argv[i] = va_arg(args, char *);
    }
    argv[n_param] = 0;
    va_end(args);
    return mt_register(mt_new(type, (const char **)argv));
}

static void test_spy_message(CuTest *tc) {
    spy_fixture fix;
    struct mlist *msglist;
    struct message *msg;
    int m, p;
    const message_type *expected[3];
    variant v;

    setup_spy(&fix);
    enable_skill(SK_MAGIC, true);
    set_level(fix.victim, SK_MINING, 2);
    set_level(fix.victim, SK_MAGIC, 2);
    create_mage(fix.victim, M_DRAIG);

    expected[0] = register_msg("spyreport", 3, "spy:unit", "target:unit", "status:string");
    expected[1] = register_msg("spyreport_mage", 3, "spy:unit", "target:unit", "type:string");
    expected[2] = register_msg("spyreport_skills", 3, "spy:unit", "target:unit", "skills:string");
    register_msg("spyreport_faction", 3, "spy:unit", "target:unit", "faction:faction");
    register_msg("spyreport_items", 3, "spy:unit", "target:unit", "items:items");

    spy_message(99, fix.spy, fix.victim);
    msglist = fix.spy->faction->msgs->begin;
    m = 0;
    while (msglist) {
	msg = msglist->msg;
	CuAssertStrEquals(tc, expected[m]->name, msg->type->name);
	/* I'm not sure how to test for correct number and maybe even type of parameters */
	for (p = 0; p != msg->type->nparameters; ++p) {
	    v = msg->parameters[p];
	    if (v.v || !v.v)
		v = msg->parameters[p];
	}
	msglist = msglist->next;
	++m;
    }
    CuAssertIntEquals(tc, 3, m);

    test_cleanup();
}

CuSuite *get_spy_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_spy_message);
  return suite;
}
