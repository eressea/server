/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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

#ifndef MY_LOCALE_H
#define MY_LOCALE_H
#ifdef __cplusplus
extern "C" {
#endif

#define MAXLOCALES 3

  struct locale;

/** managing multiple locales: **/
  extern struct locale *get_locale(const char *name);
  extern struct locale *get_or_create_locale(const char *key);
  extern void free_locales(void);

  /** operations on locales: **/
  extern void locale_setstring(struct locale *lang, const char *key,
    const char *value);
  extern const char *locale_getstring(const struct locale *lang,
    const char *key);
  extern const char *locale_string(const struct locale *lang, const char *key); /* does fallback */
  extern unsigned int locale_index(const struct locale *lang);
  extern const char *locale_name(const struct locale *lang);

  extern const char *mkname(const char *namespc, const char *key);
  extern char *mkname_buf(const char *namespc, const char *key, char *buffer);

  extern void make_locales(const char *str);

#define LOC(lang, s) (lang?locale_string(lang, s):s)

  extern struct locale *default_locale;
  extern struct locale *locales;
  extern struct locale *nextlocale(const struct locale *lang);

  enum {
    UT_PARAMS,
    UT_KEYWORDS,
    UT_SKILLS,
    UT_RACES,
    UT_OPTIONS,
    UT_DIRECTIONS,
    UT_ARCHETYPES,
    UT_MAGIC,
    UT_TERRAINS,
    UT_SPECDIR,
    UT_MAX
  };

  void ** get_translations(const struct locale *lang, int index);

#ifdef __cplusplus
}
#endif
#endif
