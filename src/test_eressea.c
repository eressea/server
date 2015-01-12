#include <platform.h>
#include <eressea.h>
#include <kernel/config.h>
#include <CuTest.h>
#include <stdio.h>
#include <util/log.h>

#pragma warning(disable: 4210)

void RunTests(CuSuite * suite, const char *name) {
    CuString *output = CuStringNew();

    CuSuiteRun(suite);
    CuSuiteDetails(suite, output);
    if (suite->failCount) CuSuiteSummary(suite, output);
    printf("%s: %s", name, output->buffer);
    CuStringDelete(output);
}

#define RUN_TESTS(suite, name) \
    CuSuite *get_##name##_suite(void); \
    CuSuite *name = get_##name##_suite(); \
    RunTests(name, #name); \
    suite->failCount += name->failCount; \
    suite->count += name->count; \
    CuSuiteDelete(name);

int RunAllTests(void)
{
  CuSuite *suite = CuSuiteNew();
  int fail_count, flags = log_flags;

  log_flags = LOG_FLUSH | LOG_CPERROR;
  game_init();

  /* self-test */
  RUN_TESTS(suite, tests);
  RUN_TESTS(suite, callback);
  RUN_TESTS(suite, json);
  RUN_TESTS(suite, jsonconf);
  RUN_TESTS(suite, direction);
  RUN_TESTS(suite, skill);
  RUN_TESTS(suite, keyword);
  RUN_TESTS(suite, order);
  RUN_TESTS(suite, race);
  /* util */
  RUN_TESTS(suite, config);
  RUN_TESTS(suite, attrib);
  RUN_TESTS(suite, base36);
  RUN_TESTS(suite, bsdstring);
  RUN_TESTS(suite, functions);
  RUN_TESTS(suite, parser);
  RUN_TESTS(suite, umlaut);
  RUN_TESTS(suite, unicode);
  RUN_TESTS(suite, strings);
  /* kernel */
  RUN_TESTS(suite, alliance);
  RUN_TESTS(suite, unit);
  RUN_TESTS(suite, faction);
  RUN_TESTS(suite, group);
  RUN_TESTS(suite, build);
  RUN_TESTS(suite, pool);
  RUN_TESTS(suite, curse);
  RUN_TESTS(suite, equipment);
  RUN_TESTS(suite, item);
  RUN_TESTS(suite, magic);
  RUN_TESTS(suite, reports);
  RUN_TESTS(suite, save);
  RUN_TESTS(suite, ship);
  RUN_TESTS(suite, spellbook);
  RUN_TESTS(suite, building);
  RUN_TESTS(suite, spell);
  RUN_TESTS(suite, ally);
  /* gamecode */
  RUN_TESTS(suite, battle);
  RUN_TESTS(suite, economy);
  RUN_TESTS(suite, give);
  RUN_TESTS(suite, laws);
  RUN_TESTS(suite, market);
  RUN_TESTS(suite, move);
  RUN_TESTS(suite, stealth);
  RUN_TESTS(suite, upkeep);
  RUN_TESTS(suite, vortex);
  RUN_TESTS(suite, wormhole);

  printf("\ntest summary: %d tests, %d failed\n", suite->count, suite->failCount);
  log_flags = flags;
  fail_count = suite->failCount;
  CuSuiteDelete(suite);
  game_done();
  return fail_count;
}

int main(int argc, char ** argv) {
    log_stderr = 0;
    return RunAllTests();
}
