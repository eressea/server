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

#ifndef SPREAD_H
#define SPREAD_H

/* Verhalten von Attibuten auf Units bei GIB PERSONEN */

typedef enum {
	SPREAD_NEVER,			/* Wird nie mit übertragen */
	SPREAD_ALWAYS,		/* Wird immer mit übertragen */
	SPREAD_MODULO,		/* Personenweise Weitergabe */
	SPREAD_CHANCE,		/* Ansteckungschance je nach Mengenverhältnis */
	SPREAD_TRANSFER		/* Attribut wird nicht kopiert, sondern "wandert" */
} spread_t;

#endif /* SPREAD_H */
