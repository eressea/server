#include "prefix.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char **race_prefixes = NULL;

void add_raceprefix(const char *prefix)
{
    static size_t size = 4;
    static unsigned int next = 0;
    if (race_prefixes == NULL)
        race_prefixes = malloc(size * sizeof(char *));
    if (next + 1 == size) {
        size *= 2;
        race_prefixes = realloc(race_prefixes, size * sizeof(char *));
    }
    race_prefixes[next++] = _strdup(prefix);
    race_prefixes[next] = NULL;
}
