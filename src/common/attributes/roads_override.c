/* vi: set ts=2:
 *
 * $Id: roads_override.c,v 1.1 2001/02/04 19:00:23 corwin Exp $
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

#include <config.h>
#include <eressea.h>
#include "roads_override.h"

attrib_type at_roads_override = { 
  "roads_override", NULL, NULL, NULL, &a_writestring, &a_readstring
};

