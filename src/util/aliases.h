#pragma once

struct locale;

#define MAXSTRINGS 2
#define MAXALIASES 16

typedef struct str_alias {
    char *key;
    char *strings[MAXSTRINGS];
} str_alias;

typedef struct str_aliases {
    struct str_aliases *next;
    const struct locale *lang;
    str_alias alternatives[MAXALIASES];
} str_aliases;

void free_aliases(void);

str_aliases *get_aliases(const struct locale *lang);
void alias_add(str_aliases *aliases, const char *key, const char *text);
const str_alias *alias_get(const struct locale *lang, const char *key);
