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

#if !(defined(OLD_TRIGGER) || defined(CONVERT_TRIGGER))
# error "Do not include unless for old code or to enable conversion"
#endif

#ifndef POINTERTAGS_H
#define POINTERTAGS_H

/* Tags */
typedef enum {
	TAG_NORMAL,		/* Std-Tag, Ptr wird NULL, wenn Objekt vernichtet wird */
	TAG_RELATION,	/*unit* in relation-Attribs */

	/* Achtung: neue Tags nur über dieser Zeile anfügen, aber unter bereits
	 * bestehenden!  Die Reihenfolge nicht verändern!  */
	MAXTAGS,
	TAG_NOTAG = -1
} tag_t;

#ifndef OBJTYPES_H
#include <objtypes.h>
#endif

extern void tag_pointer(void *objPP, typ_t typ, tag_t tag);
extern void untag_pointer(void *objPP, typ_t typ, tag_t tag);

extern void change_all_pointers(void *obj1, typ_t typ, void *obj2);

extern int count_all_pointers(void *obj, typ_t typ);
extern int count_tagged_pointers(void *obj, typ_t typ, tag_t tag);

#include "attrib.h"
extern attrib_type at_pointer_tag;

#if defined(OLD_TRIGGER) || defined (CONVERT_TRIGGER)
extern void add_ID_resolve2(obj_ID id, void *objPP, typ_t typ, tag_t tag);
#endif

#endif /* POINTERTAGS_H */
