/* vi: set ts=2:
 *
 *	$Id: cr.h,v 1.1 2001/01/27 18:15:32 enno Exp $
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

#ifndef _CR_H
#define _CR_H

void cr_faction(FILE * out, struct faction * f);
void cr_region(FILE * out, struct region * r);

#endif
