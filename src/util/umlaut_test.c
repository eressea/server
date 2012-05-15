#include <cutest/CuTest.h>
#include <stdio.h>
#include <string.h>
#include "umlaut.h"


static void test_umlaut(CuTest * tc)
{
  char umlauts[] = { 0xc3, 0xa4, 0xc3, 0xb6, 0xc3, 0xbc, 0xc3, 0x9f, 0 }; /* auml ouml uuml szlig nul */
  tnode tokens = { 0 };
  variant id;
  int result;

  id.i = 1;
  addtoken(&tokens, "herp", id);
  id.i = 2;
  addtoken(&tokens, "derp", id);
  id.i = 3;
  addtoken(&tokens, umlauts, id);

  result = findtoken(&tokens, "herp", &id);
  CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
  CuAssertIntEquals(tc, 1, id.i);

  result = findtoken(&tokens, "derp", &id);
  CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
  CuAssertIntEquals(tc, 2, id.i);

  result = findtoken(&tokens, umlauts, &id);
  CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
  CuAssertIntEquals(tc, 3, id.i);

  result = findtoken(&tokens, "AEoeUEss", &id);
  CuAssertIntEquals(tc, E_TOK_SUCCESS, result);
  CuAssertIntEquals(tc, 3, id.i);

  result = findtoken(&tokens, "herpderp", &id);
  CuAssertIntEquals(tc, E_TOK_NOMATCH, result);
}

CuSuite *get_umlaut_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_umlaut);
  return suite;
}
