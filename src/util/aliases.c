#include "aliases.h"

#include <strings.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

static struct str_aliases *g_aliases;

void free_aliases(void)
{
    while (g_aliases) {
        int i;
        struct str_aliases * anext = g_aliases->next;
        for (i = 0; i != MAXALIASES && g_aliases->alternatives[i].key; ++i) {
            int j;
            struct str_alias *alias = g_aliases->alternatives + i;
            free(alias->key);
            for (j = 0; j != MAXSTRINGS && alias->strings[j]; ++j) {
                free(alias->strings[j]);
            }
        }
        free(g_aliases);
        g_aliases = anext;
    }
}

str_aliases *get_aliases(const struct locale *lang) {
    str_aliases *aliases = g_aliases;
    while (aliases && aliases->lang != lang) {
        aliases = aliases->next;
    }
    if (aliases == NULL) {
        aliases = calloc(1, sizeof(struct str_aliases));
        aliases->next = g_aliases;
        aliases->lang = lang;
        g_aliases = aliases;
    }
    return aliases;
}

void alias_add(struct str_aliases *aliases, const char *key, const char *text) {
    int i, j;
    struct str_alias *alias;

    for (i = 0; i != MAXALIASES && aliases->alternatives[i].key; ++i) {
        if (0 == strcmp(key, aliases->alternatives[i].key)) {
            break;
        }
    }
    assert(i != MAXALIASES);
    alias = aliases->alternatives + i;
    alias->key = str_strdup(key);
    for (j = 0; j != MAXSTRINGS; ++j) {
        if (alias->strings[j] == NULL) {
            alias->strings[j] = str_strdup(text);
            break;
        }
    }
    assert(j != MAXSTRINGS);
}

const struct str_alias *alias_get(const struct locale *lang, const char *key)
{
    str_aliases *aliases = g_aliases;
    while (aliases && aliases->lang != lang) {
        aliases = aliases->next;
    }
    if (aliases) {
        int i;
        for (i = 0; i != MAXALIASES && aliases->alternatives[i].key; ++i) {
            if (0 == strcmp(key, aliases->alternatives[i].key)) {
                return aliases->alternatives + i;
            }
        }
    }
    return NULL;
}
