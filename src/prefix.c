#include <platform.h>
#include "prefix.h"

#include <util/log.h>
#include <util/strings.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char **race_prefixes = NULL;
static size_t size = 4;
static unsigned int next = 0;

int add_raceprefix(const char *prefix)
{
    assert(prefix);
    if (race_prefixes == NULL) {
        next = 0;
        size = 4;
        race_prefixes = malloc(size * sizeof(char *));
        if (!race_prefixes) abort();
    }
    if (next + 1 == size) {
        char **tmp;
        tmp = realloc(race_prefixes, 2 * size * sizeof(char *));
        if (!tmp) abort();
        race_prefixes = tmp;
        size *= 2;
    }
    race_prefixes[next++] = str_strdup(prefix);
    race_prefixes[next] = NULL;
    return 0;
}

void free_prefixes(void) {
    if (race_prefixes) {
        int i;
        for (i = 0; race_prefixes[i]; ++i) {
            free(race_prefixes[i]);
        }
        free(race_prefixes);
        race_prefixes = 0;
    }
}
