/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
                         Katja Zedel <katze@felidae.kn-bremen.de
                         Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <kernel/config.h>
#include "creation.h"
#include "monster.h"
#include "alchemy.h"

/* kernel includes */
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

faction *createmonsters(int no)
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

  f->email = _strdup("monsters@eressea.de");
  f->name = _strdup("Monster");
  f->alive = 1;
  f->options = (char)(1 << O_REPORT);
  addlist(&factions, f);
  fhash(f);
  return f;
}
