/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */
#ifndef MY_LOCALE_H
#define MY_LOCALE_H

typedef struct locale locale;

/** managing multiple locales: **/
extern locale * find_locale(const char * name);
extern locale * make_locale(const char * key);

/** operations on locales: **/
extern void locale_setstring(struct locale * lang, const char * key, const char * value);
extern const char * locale_getstring(const struct locale * lang, const char * key);
extern const char * locale_string(const struct locale * lang, const char * key); /* does fallback */
extern unsigned int locale_hashkey(const struct locale * lang);
extern const char * locale_name(const struct locale * lang);

extern const char * reverse_lookup(const struct locale * lang, const char * str);

extern void debug_language(const char * log);

#define LOC(s,l) locale_string(s, l)

#endif
