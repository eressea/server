/* vi: set ts=2:
 *
 *	$Id: language.h,v 1.2 2001/01/26 16:19:41 enno Exp $
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
extern const char * locale_string(const locale * lang, const char * key);
extern void locale_setstring(locale * lang, const char * key, const char * value);
extern int locale_hashkey(const locale * lang);
extern const char * locale_name(const locale * lang);

extern const char * reverse_lookup(const locale * lang, const char * str);
#endif
