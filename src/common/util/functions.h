/* vi: set ts=2:
 *
 *	$Id: functions.h,v 1.2 2001/01/26 16:19:41 enno Exp $
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

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

typedef void (*pf_generic)(void);

extern const char *get_functionname(pf_generic fun);
extern pf_generic get_function(const char * name);
extern void register_function(pf_generic fun, const char * name);

#endif
