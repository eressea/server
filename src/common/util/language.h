/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */
#ifndef MY_LOCALE_H
#define MY_LOCALE_H
#ifdef __cplusplus
extern "C" {
#endif

struct locale;

/** managing multiple locales: **/
extern struct locale * find_locale(const char * name);
extern struct locale * make_locale(const char * key);

/** operations on locales: **/
extern void locale_setstring(struct locale * lang, const char * key, const char * value);
extern const char * locale_getstring(const struct locale * lang, const char * key);
extern const char * locale_string(const struct locale * lang, const char * key); /* does fallback */
extern unsigned int locale_hashkey(const struct locale * lang);
extern const char * locale_name(const struct locale * lang);

extern const char * mkname(const char * namespc, const char * key);
extern char * mkname_buf(const char * namespc, const char * key, char * buffer);

extern void debug_language(const char * log);
extern void make_locales(const char * str);

#define LOC(lang, s) locale_string(lang, s)

extern struct locale * default_locale;
extern struct locale * locales;
extern struct locale * nextlocale(const struct locale * lang);

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

struct tnode * get_translations(const struct locale * lang, int index);

#ifdef __cplusplus
}
#endif
#endif
