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

#if !(defined(OLD_TRIGGER) || defined(CONVERT_TRIGGER))
# error "Do not include unless for old code or to enable conversion"
#endif

#ifndef RELATION_H
#define RELATION_H
#if defined(OLD_TRIGGER) || defined(CONVERT_TRIGGER)
/* Einfache Beziehungen zwischen Objekten herstellen ("ich verfolge A",
 * "B ist mein Vater", "X ist mein Heimatort").  Werden automatisch
 * gespeichert und geladen.  Jedes Objekt kann von jedem Relations-Typ
 * (d.h. von jeder REL_XXX-Id) nur je eine besitzen.  Will man einem
 * Objekt zwei "gleiche" Relationen anhängen ("meine Väter sind C und D"),
 * so nehme man zwei Ids:
 * 		create_relation(u, TYP_UNIT, REL_DADDY1, uC, TYP_UNIT, SPREAD_ALWAYS);
 * 		create_relation(u, TYP_UNIT, REL_DADDY2, uD, TYP_UNIT, SPREAD_ALWAYS);
 *
 * Relations werden automatisch gelöscht, wenn das referierte Objekt
 * zerstört wird.
 */

#include "objtypes.h"
#include "attrspread.h"

typedef enum {
	REL_TARGET,		/* Objekt ist mein Ziel */
	REL_CREATOR,	/* Objekt hat mich erschaffen */
	REL_FAMILIAR,	/* Zauberer: Objekt ist Vertrauter */

	MAXRELATIONS
} relation_t;

void create_relation(void *obj1, typ_t typ1, relation_t id,
		void *obj2, typ_t typ2, spread_t spread);
void remove_relation(void *obj1, typ_t typ1, relation_t id);
void *get_relation(const void *obj, typ_t typ, relation_t id);
void *get_relation2(const void *obj, typ_t typ, relation_t id, typ_t *typ2P);
/* umgekehrte Richtung */
void *get_rev_relation(void *obj, typ_t typ, relation_t id);
void *get_rev_relation2(void *obj, typ_t typ, relation_t id, typ_t *typ2P);

#include "attrib.h"
extern attrib_type at_relation;
extern attrib_type at_relbackref;
#endif
#endif
