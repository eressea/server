#include <cutest/CuTest.h>
#include <stdio.h>
#include <string.h>
#include "quicklist.h"

static const char * hello = "Hello World";

static void test_insert(CuTest * tc) {
  struct quicklist * ql = NULL;
  int i;
  for (i=0;i!=32;++i) {
    CuAssertIntEquals(tc, i, ql_length(ql));
    ql_insert(&ql, 0, (void*)i);
  }
  for (i=0;i!=32;++i) {
    CuAssertIntEquals(tc, 31-i, (int)ql_get(ql, i));
  }
}

static void test_advance(CuTest * tc) {
  struct quicklist * ql = NULL, *qli;
  int i, n = 31;
  for (i=0;i!=32;++i) {
    ql_insert(&ql, 0, (void*)i);
  }
  for (i=0,qli=ql;qli;ql_advance(&qli, &i, 1),n--) {
    int g = (int)ql_get(qli, i);
    CuAssertIntEquals(tc, n, g);
  }
}

static void test_push(CuTest * tc) {
  struct quicklist * ql = NULL;
  CuAssertIntEquals(tc, 0, ql_length(ql));
  ql_push(&ql, (void*)hello);
  CuAssertIntEquals(tc, 1, ql_length(ql));
  CuAssertStrEquals(tc, "Hello World", (const char *)ql_get(ql, 0));
  ql_delete(&ql, 0);
  CuAssertPtrEquals(tc, 0, ql);
}

static void test_delete_edgecases(CuTest * tc) {
  struct quicklist * ql = NULL;
  ql_delete(&ql, 0);
  CuAssertPtrEquals(tc, 0, ql);
  ql_push(&ql, (void*)hello);
  ql_delete(&ql, -1);
  ql_delete(&ql, 32);
  CuAssertIntEquals(tc, 1, ql_length(ql));
}

static void test_insert_many(CuTest * tc) {
  struct quicklist * ql = NULL;
  int i;
  for (i=0;i!=32;++i) {
    ql_push(&ql, (void*)i);
  }
  for (i=0;i!=32;++i) {
    CuAssertIntEquals(tc, 32-i, ql_length(ql));
    CuAssertIntEquals(tc, i, (int)ql_get(ql, 0));
    CuAssertIntEquals(tc, 31, (int)ql_get(ql, ql_length(ql)-1));
    ql_delete(&ql, 0);
  }
  CuAssertPtrEquals(tc, 0, ql);
}

static void test_delete_rand(CuTest * tc) {
  struct quicklist * ql = NULL;
  int i;
  for (i=0;i!=32;++i) {
    ql_push(&ql, (void*)i);
  }
  CuAssertIntEquals(tc, 32, ql_length(ql));
  ql_delete(&ql, 0);
  CuAssertIntEquals(tc, 1, (int)ql_get(ql, 0));
  CuAssertIntEquals(tc, 31, ql_length(ql));
  ql_delete(&ql, 30);
  CuAssertIntEquals(tc, 30, ql_length(ql));
}

CuSuite* get_quicklist_suite(void)
{
  CuSuite* suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_advance);
  SUITE_ADD_TEST(suite, test_push);
  SUITE_ADD_TEST(suite, test_insert);
  SUITE_ADD_TEST(suite, test_insert_many);
  SUITE_ADD_TEST(suite, test_delete_rand);
  SUITE_ADD_TEST(suite, test_delete_edgecases);
  return suite;
}
