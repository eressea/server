#ifndef CLASS_LANGUAGE_STRUCT
#define CLASS_LANGUAGE_STRUCT

/* This file should not be included by anything in the server. If you
 * feel that you need to include it, it's a sure sign that you're trying to
 * do something BAD. */

#define SMAXHASH 2048
typedef struct locale_str {
    unsigned int hashkey;
    struct locale_str *nexthash;
    char *str;
    char *key;
} locale_str;

typedef struct locale {
    char *name;
    unsigned int index;
    struct locale *next;
    unsigned int hashkey;
    struct locale_str *strings[SMAXHASH];
    struct locale *fallback;
} locale;

extern locale *default_locale;
extern locale *locales;

#endif
