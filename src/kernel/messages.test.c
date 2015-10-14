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
    message_type *mtype = mt_new("custom", NULL);

    test_cleanup();
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

static void test_merge_split(CuTest *tc) {
    message_list *mlist = 0, *append = 0;
    struct mlist **split; // TODO: why is this a double asterisk?
    message_type *mtype = mt_new("custom", NULL);

    test_cleanup();
    mt_register(mtype);
    add_message(&mlist, msg_message(mtype->name, ""));
    add_message(&append, msg_message(mtype->name, ""));

    CuAssertPtrEquals(tc, 0, mlist->begin->next);
    CuAssertPtrEquals(tc, &mlist->begin->next, mlist->end);
    split = merge_messages(mlist, append);
    CuAssertPtrNotNull(tc, split);
    CuAssertPtrEquals(tc, &mlist->begin->next, split);
    CuAssertPtrEquals(tc, append->end, mlist->end);
    CuAssertPtrNotNull(tc, mlist->begin->next);
    CuAssertPtrEquals(tc, append->begin, mlist->begin->next);
    split_messages(mlist, split);
    CuAssertPtrEquals(tc, 0, mlist->begin->next);
    free_messagelist(*split);
    free_messagelist(mlist->begin);
    free(mlist);
    free_messagelist(append->begin);
    free(append);
    test_cleanup();
}

CuSuite *get_messages_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_missing_message);
    SUITE_ADD_TEST(suite, test_merge_split);
    SUITE_ADD_TEST(suite, test_message);
    return suite;
}