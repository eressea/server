#include <platform.h>
#include <eressea.h>
#include <kernel/config.h>
#include <CuTest.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util/log.h>

#pragma warning(disable: 4210)

typedef struct suite {
    struct suite *next;
    CuSuite *csuite;
    char *name;
} suite;

static suite *suites;

static void add_suite(CuSuite *(*csuite)(void), const char *name, int argc, char *argv[]) {
    suite *s = 0;
    if (argc > 0) {
        int i;
        for (i = 0; i != argc; ++i) {
            if (strcmp(argv[i], name) == 0) {
                s = malloc(sizeof(suite));
                break;
            }
        }
    }
    else {
        s = malloc(sizeof(suite));
    }
    if (s) {
        s->next = suites;
        s->name = _strdup(name);
        s->csuite = csuite();
        suites = s;
    }
}

void RunTests(CuSuite * suite, const char *name) {
    CuString *output = CuStringNew();

    CuSuiteRun(suite);
    CuSuiteDetails(suite, output);
    if (suite->failCount) CuSuiteSummary(suite, output);
    printf("%s: %s", name, output->buffer);
    CuStringDelete(output);
}

bool list = false;

#define ADD_SUITE(name) \
    CuSuite *get_##name##_suite(void); \
    if (list) printf("%s\n", #name); \
    if (!list || argc>0) add_suite(get_##name##_suite, #name, argc, argv)

int RunAllTests(int argc, char *argv[])
{
    /* self-test */
    ADD_SUITE(tests);
    ADD_SUITE(callback);
    ADD_SUITE(seen);
    ADD_SUITE(json);
    ADD_SUITE(jsonconf);
    ADD_SUITE(direction);
    ADD_SUITE(skill);
    ADD_SUITE(keyword);
    ADD_SUITE(order);
    ADD_SUITE(race);
    /* util */
    ADD_SUITE(config);
    ADD_SUITE(attrib);
    ADD_SUITE(base36);
    ADD_SUITE(bsdstring);
    ADD_SUITE(functions);
    ADD_SUITE(gamedata);
    ADD_SUITE(parser);
    ADD_SUITE(password);
    ADD_SUITE(umlaut);
    ADD_SUITE(unicode);
    ADD_SUITE(strings);
    ADD_SUITE(log);
    ADD_SUITE(rng);
    /* items */
    ADD_SUITE(xerewards);
    /* kernel */
    ADD_SUITE(alliance);
    ADD_SUITE(unit);
    ADD_SUITE(faction);
    ADD_SUITE(group);
    ADD_SUITE(build);
    ADD_SUITE(pool);
    ADD_SUITE(curse);
    ADD_SUITE(equipment);
    ADD_SUITE(item);
    ADD_SUITE(magic);
    ADD_SUITE(alchemy);
    ADD_SUITE(reports);
    ADD_SUITE(save);
    ADD_SUITE(ship);
    ADD_SUITE(spellbook);
    ADD_SUITE(building);
    ADD_SUITE(spell);
    ADD_SUITE(spells);
    ADD_SUITE(magicresistance);
    ADD_SUITE(ally);
    ADD_SUITE(messages);
    /* gamecode */
    ADD_SUITE(prefix);
    ADD_SUITE(battle);
    ADD_SUITE(donations);
    ADD_SUITE(travelthru);
    ADD_SUITE(economy);
    ADD_SUITE(flyingship);
    ADD_SUITE(give);
    ADD_SUITE(laws);
    ADD_SUITE(market);
    ADD_SUITE(monsters);
    ADD_SUITE(move);
    ADD_SUITE(piracy);
    ADD_SUITE(key);
    ADD_SUITE(stealth);
    ADD_SUITE(otherfaction);
    ADD_SUITE(upkeep);
    ADD_SUITE(vortex);
    ADD_SUITE(wormhole);
    ADD_SUITE(spy);
    ADD_SUITE(study);
    ADD_SUITE(shock);

    if (suites) {
        CuSuite *summary = CuSuiteNew();
        int fail_count;
        game_init();
        while (suites) {
            suite *s = suites->next;
            RunTests(suites->csuite, suites->name);
            summary->failCount += suites->csuite->failCount;
            summary->count += suites->csuite->count;
            CuSuiteDelete(suites->csuite);
            free(suites->name);
            free(suites);
            suites = s;
        }
        printf("\ntest summary: %d tests, %d failed\n", summary->count, summary->failCount);
        fail_count = summary->failCount;
        CuSuiteDelete(summary);
        game_done();
        return fail_count;
    }
    return 0;
}

int main(int argc, char ** argv) {
    log_to_file(LOG_CPERROR, stderr);
    ++argv;
    --argc;
    if (argc > 0 && strcmp("--list", argv[0]) == 0) {
        list = true;
        ++argv;
        --argc;
    }
    return RunAllTests(argc, argv);
}
