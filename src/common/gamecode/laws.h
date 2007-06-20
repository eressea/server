/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#ifndef H_GC_LAWS
#define H_GC_LAWS
#ifdef __cplusplus
extern "C" {
#endif

extern int writepasswd(void);
int getoption(void);
int wanderoff(struct region * r, int p);
void demographics(void);
void last_orders(void);
void find_address(void);
void update_guards(void);
  extern void deliverMail(struct faction * f, struct region * r, struct unit * u, const char *s, struct unit * receiver);

/* eressea-specific. put somewhere else, please. */
#include "resolve.h"
void processorders(void);
extern struct attrib_type at_germs;

#ifdef __cplusplus
}
#endif

#endif
