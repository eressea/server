/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
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

#include <config.h>
#include <kernel/eressea.h>
#include "creation.h"
#include "monster.h"

/* kernel includes */
#include <kernel/alchemy.h>
#include <kernel/build.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/magic.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

/* util includes */
#include <util/goodies.h>
#include <util/lists.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

faction *
createmonsters(int no)
{
  faction *f = findfaction(no);

  if (f) {
    puts("* Fehler! Die Monster Partei gibt es schon.");
    return f;
  }
  f = (faction *) calloc(1, sizeof(faction));
  f->no = no;
  /* alles ist auf null gesetzt, ausser dem folgenden. achtung - partei
  * no 0 muss keine orders einreichen! */

  f->email = strdup("monsters@eressea.de");
  f->name = strdup("Monster");
  f->alive = 1;
  f->options = (char)(1<<O_REPORT);
  addlist(&factions, f);
  fhash(f);
  return f;
}
