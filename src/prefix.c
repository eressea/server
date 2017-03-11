#include <platform.h>
#include "prefix.h"

#include <util/log.h>

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
    }
    if (next + 1 == size) {
        char **tmp;
        tmp = realloc(race_prefixes, 2 * size * sizeof(char *));
        if (!tmp) {
            log_fatal("allocation failure");
            return 1;
        }
        race_prefixes = tmp;
        size *= 2;
    }
    race_prefixes[next++] = strdup(prefix);
    race_prefixes[next] = NULL;
    return 0;
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
