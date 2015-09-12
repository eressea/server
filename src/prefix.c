#include <platform.h>
#include "prefix.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char **race_prefixes = NULL;
static size_t size = 4;
static unsigned int next = 0;

void add_raceprefix(const char *prefix)
{
    assert(prefix);
    if (race_prefixes == NULL) {
        next = 0;
        size = 4;
        race_prefixes = malloc(size * sizeof(char *));
    }
    if (next + 1 == size) {
        size *= 2;
        race_prefixes = realloc(race_prefixes, size * sizeof(char *));
    }
    race_prefixes[next++] = _strdup(prefix);
    race_prefixes[next] = NULL;
}

void free_prefixes(void) {
    int i;
    if (race_prefixes) {
        for (i = 0; race_prefixes[i]; ++i) {
            free(race_prefixes[i]);
        }
        free(race_prefixes);
        race_prefixes = 0;
    }
}
