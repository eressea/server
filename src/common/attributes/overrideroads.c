/* vi: set ts=2:
 *
 * $Id: overrideroads.c,v 1.1 2001/02/10 10:57:49 enno Exp $
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
#include "overrideroads.h"

attrib_type at_overrideroads = { 
  "roads_override", NULL, NULL, NULL, &a_writestring, &a_readstring
};

