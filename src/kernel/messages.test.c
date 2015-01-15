#include <platform.h>
#include "messages.h"

#include <CuTest.h>
#include <tests.h>

void test_missing_message(CuTest *tc) {
    message *msg;
    msg = msg_message("unknown", "unit", NULL);
    CuAssertPtrNotNull(tc, msg);
    CuAssertPtrNotNull(tc, msg->type);
    CuAssertStrEquals(tc, msg->type->name, "missing_message");
    msg_release(msg);
}

void test_message(CuTest *tc) {
    message *msg;
//    const char * args[] = { }
    message_type *mtype = mt_new("custom", NULL);
    mt_register(mtype);
    CuAssertPtrEquals(tc, mtype, (void *)mt_find("custom"));
    CuAssertIntEquals(tc, 0, mtype->nparameters);
    CuAssertPtrEquals(tc, NULL, (void *)mtype->pnames);
    CuAssertPtrEquals(tc, NULL, (void *)mtype->types);
    msg = msg_message("custom", "");
    CuAssertPtrNotNull(tc, msg);
    CuAssertIntEquals(tc, 1, msg->refcount);
    CuAssertPtrEquals(tc, NULL, msg->parameters);
    CuAssertPtrEquals(tc, mtype, (void *)msg->type);

    CuAssertPtrEquals(tc, msg, msg_addref(msg));
    CuAssertIntEquals(tc, 2, msg->refcount);
    msg_release(msg);
    CuAssertIntEquals(tc, 1, msg->refcount);
    msg_release(msg);
    test_cleanup();
}

CuSuite *get_messages_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_missing_message);
    SUITE_ADD_TEST(suite, test_message);
    return suite;
}