/* vi: set ts=2:
 *
 * $Id: render.h,v 1.3 2001/02/24 12:50:48 enno Exp $
 * Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifdef OLD_MESSAGES
struct message;
struct locale;

extern const char * render(const struct message * m, const struct locale * lang);
extern void render_init(void);
#endif
