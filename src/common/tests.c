#include <external/cutest/CuTest.h>
#include <stdio.h>

CuSuite* get_base36_suite();

void RunAllTests(void) {
  CuString *output = CuStringNew();
  CuSuite* suite = CuSuiteNew();

  CuSuiteAddSuite(suite, get_base36_suite());

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);
  printf("%s\n", output->buffer);
}

int main(int argc, char **argv) {
  RunAllTests();
}
