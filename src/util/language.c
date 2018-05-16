/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifdef _MSC_VER
#include <platform.h>
#endif

#include "language.h"

#include "log.h"
#include "strings.h"
#include "umlaut.h"
#include "assert.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <critbit.h>

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
} locale;

locale *default_locale;
locale *locales;

unsigned int locale_index(const locale * lang)
{
    assert(lang);
    return lang->index;
}

locale *get_locale(const char *name)
{
    unsigned int hkey = str_hash(name);
    locale *l = locales;
    while (l && l->hashkey != hkey)
        l = l->next;
    return l;
}

static unsigned int nextlocaleindex = 0;

locale *get_or_create_locale(const char *name)
{
    locale *l;
    unsigned int hkey = str_hash(name);
    locale **lp = &locales;

    if (!locales) {
        nextlocaleindex = 0;
    }
    else {
        while (*lp && (*lp)->hashkey != hkey) lp = &(*lp)->next;
        if (*lp) {
            return *lp;
        }
    }
    *lp = l = (locale *)calloc(sizeof(locale), 1);
    assert_alloc(l);
    l->hashkey = hkey;
    l->name = str_strdup(name);
    l->index = nextlocaleindex++;
    assert(nextlocaleindex <= MAXLOCALES);
    if (default_locale == NULL) default_locale = l;
    return l;
}

/** creates a list of locales
 * This function takes a comma-delimited list of locale-names and creates
 * the locales using the make_locale function (useful for ini-files).
 * For maximum performance, locales should be created in order of popularity.
 */
void make_locales(const char *str)
{
    const char *tok = str;
    while (tok) {
        char zText[16];
        size_t len;

        tok = strchr(str, ',');
        if (tok) {
            len = tok - str;
            assert(sizeof(zText) > len);
            memcpy(zText, str, len);
            str = tok + 1;
        }
        else {
            len = strlen(str);
            memcpy(zText, str, len);
        }
        zText[len] = 0;
        get_or_create_locale(zText);
    }
}

const char *locale_getstring(const locale * lang, const char *key)
{
    unsigned int hkey = str_hash(key);
    unsigned int id = hkey & (SMAXHASH - 1);
    const struct locale_str *find;

    assert(lang);
    if (key == NULL || *key == 0)
        return NULL;
    find = lang->strings[id];
    while (find) {
        if (find->hashkey == hkey) {
            if (find->nexthash == NULL) {
                /* if this is the only entry with this hash, fine. */
                assert(strcmp(key, find->key) == 0);
                return find->str;
            }
            if (strcmp(key, find->key) == 0) {
                return find->str;
            }
        }
        find = find->nexthash;
    }
    return NULL;
}

const char *locale_string(const locale * lang, const char *key, bool warn)
{
    assert(lang);
    assert(key);

    if (key != NULL) {
        unsigned int hkey = str_hash(key);
        unsigned int id = hkey & (SMAXHASH - 1);
        struct locale_str *find;

        if (*key == 0) return 0;
        find = lang->strings[id];
        while (find) {
            if (find->hashkey == hkey) {
                if (!find->nexthash) {
                    /* if this is the only entry with this hash, fine. */
                    assert(strcmp(key, find->key) == 0);
                    break;
                }
                if (strcmp(key, find->key) == 0)
                    break;
            }
            find = find->nexthash;
        }
        if (find) {
            return find->str;
        }
        if (warn) {
            log_warning("missing translation for \"%s\" in locale %s\n", key, lang->name);
        }
        if (default_locale && lang != default_locale) {
            const char * value = locale_string(default_locale, key, warn);
            if (value) {
                /* TODO: evil side-effects for a const function */
                locale_setstring(get_or_create_locale(lang->name), key, value);
            }
            return value;
        }
    }
    return NULL;
}

void locale_setstring(locale * lang, const char *key, const char *value)
{
    unsigned int hkey = str_hash(key);
    unsigned int id = hkey & (SMAXHASH - 1);
    struct locale_str *find;
    if (!lang) {
        lang = default_locale;
    }
    assert(lang);
    find = lang->strings[id];
    while (find) {
        if (find->hashkey == hkey && strcmp(key, find->key) == 0)
            break;
        find = find->nexthash;
    }
    if (!find) {
        find = calloc(1, sizeof(struct locale_str));
        find->nexthash = lang->strings[id];
        lang->strings[id] = find;
        find->hashkey = hkey;
        find->key = str_strdup(key);
        find->str = str_strdup(value);
    }
    else {
        if (strcmp(find->str, value) != 0) {
            log_warning("multiple translations for key %s\n", key);
        }
        free(find->str);
        find->str = str_strdup(value);
    }
}

static const char *escape_str(const char *in, FILE *F) {
    while (*in) {
        char buffer[64];
        size_t len = 0;
        char *out = buffer;
        while (sizeof(buffer) > len + 2) {
            if (*in == '\0') break;
            if (strchr("\\\"", *in) != NULL) {
                *out++ = '\\';
                ++len;
            }
            *out++ = *in++;
            ++len;
        }
        if (len > 0) {
            fwrite(buffer, 1, len, F);
        }
    }
    return in;
}

void po_write_msg(FILE *F, const char *id, const char *str, const char *ctxt) {
    if (ctxt) {
        fputs("msgctxt \"", F);
        escape_str(ctxt, F);
        fputs("\"\nmsgid \"", F);
        escape_str(id, F);
    }
    else {
        fputs("msgid \"", F);
        escape_str(id, F);
    }
    fputs("\"\nmsgstr \"", F);
    escape_str(str, F);
    fputs("\"\n\n", F);
}

void export_strings(const struct locale * lang, FILE *F) {
    int i;

    for (i = 0; i != SMAXHASH; ++i) {
        const struct locale_str *find = lang->strings[i];
        while (find) {
            const char *dcolon = strstr(find->key, "::");
            if (dcolon) {
                size_t len = dcolon - find->key;
                char ctxname[16];
                assert(sizeof(ctxname) > len);
                memcpy(ctxname, find->key, len);
                ctxname[len] = '\0';
                po_write_msg(F, dcolon + 2, find->str, ctxname);
            }
            else {
                po_write_msg(F, find->key, find->str, NULL);
            }
            find = find->nexthash;
        }
    }
}

const char *locale_name(const locale * lang)
{
    return lang ? lang->name : "(null)";
}

char *mkname_buf(const char *space, const char *name, char *buffer)
{
    if (space && *space) {
        sprintf(buffer, "%s::%s", space, name);
    }
    else {
        strcpy(buffer, name);
    }
    return buffer;
}

const char *mkname(const char *space, const char *name)
{
    static char zBuffer[128]; /* FIXME: static return value */
    return mkname_buf(space, name, zBuffer);
}

locale *nextlocale(const struct locale * lang)
{
    return lang->next;
}

typedef struct lstr {
    void * tokens[UT_MAX];
} lstr;

static lstr lstrs[MAXLOCALES];

void ** get_translations(const struct locale *lang, int index)
{
    assert(lang);
    assert(lang->index < MAXLOCALES
        || !"you have to increase MAXLOCALES and recompile");
    if (lang->index < MAXLOCALES) {
        return lstrs[lang->index].tokens + index;
    }
    return lstrs[0].tokens + index;
}

void add_translation(struct critbit_tree **cbp, const char *key, int i) {
    char buffer[256];
    char * str = transliterate(buffer, sizeof(buffer) - sizeof(int), key);
    struct critbit_tree * cb = *cbp;
    if (str) {
        size_t len = strlen(str);
        if (!cb) {
            /* TODO: this will leak, because we do not know how to clean it up */
            *cbp = cb = (struct critbit_tree *)calloc(1, sizeof(struct critbit_tree));
        }
        len = cb_new_kv(str, len, &i, sizeof(int), buffer);
        cb_insert(cb, buffer, len);
    }
    else {
        log_error("could not transliterate '%s'\n", key);
    }
}

void init_translations(const struct locale *lang, int ut, const char * (*string_cb)(int i), int maxstrings)
{
    void **tokens;
    int i;

    assert(string_cb);
    assert(maxstrings > 0);
    tokens = get_translations(lang, ut);
    for (i = 0; i != maxstrings; ++i) {
        /* TODO: swap the name of s and key */
        const char * s = string_cb(i);
        if (s) {
            const char * key = locale_string(lang, s, false);
            if (key) {
                struct critbit_tree ** cb = (struct critbit_tree **)tokens;
                add_translation(cb, key, i);
            }
            else {
                log_warning("no translation for %s in locale %s", s, lang->name);
            }
        }
    }
}

void *get_translation(const struct locale *lang, const char *str, int index) {
    void **tokens = get_translations(lang, index);
    variant var;
    if (findtoken(*tokens, str, &var) == E_TOK_SUCCESS) {
        return var.v;
    }
    return NULL;
}

void locale_foreach(void(*callback)(const struct locale *, const char *)) {
    const locale * lang;
    for (lang = locales; lang; lang = lang->next) {
        callback(lang, lang->name);
    }
}

const char *localenames[] = {
    "de", "en",
    NULL
};

extern void init_locale(struct locale *lang);

static int locale_init = 0;

void init_locales(void)
{
    locale * lang;
    if (locale_init) return;
    assert(locales);
    for (lang = locales; lang; lang = lang->next) {
        init_locale(lang);
    }
    locale_init = 1;
}

void reset_locales(void) {
    locale_init = 0;
}

void free_locales(void) {
    locale_init = 0;
    while (locales) {
        int i, index = locales->index;
        locale * next = locales->next;

        for (i = UT_PARAMS; i != UT_RACES; ++i) {
            struct critbit_tree ** cb = (struct critbit_tree **)get_translations(locales, i);
            if (*cb) {
                /* TODO: this crashes? */
                cb_clear(*cb);
                free(*cb);
            }
        }
        for (i = UT_RACES; i != UT_MAX; ++i) {
            void *tokens = lstrs[index].tokens[i];
            if (tokens) {
                freetokens(tokens);
            }
        }
        for (i = 0; i != SMAXHASH; ++i) {
            while (locales->strings[i]) {
                struct locale_str * strings = locales->strings[i];
                free(strings->key);
                free(strings->str);
                locales->strings[i] = strings->nexthash;
                free(strings);
            }
        }
        free(locales->name);
        free(locales);
        locales = next;
    }
    memset(lstrs, 0, sizeof(lstrs)); /* TODO: does this data need to be free'd? */
}
