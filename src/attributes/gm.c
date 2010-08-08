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
#include "gm.h"

/* kernel includes */
#include <kernel/plane.h>

/* util includes */
#include <util/attrib.h>
#include <util/storage.h>

static void
write_gm(const attrib * a, const void * owner, struct storage * store)
{
  write_plane_reference((plane*)a->data.v, store);
}

static int
read_gm(attrib * a, void * owner, struct storage * store)
{
  plane * pl;
  int result = read_plane_reference(&pl, store);
  a->data.v = pl;
  return result;
}


attrib_type at_gm = {
	"gm",
	NULL,
	NULL,
	NULL,
	write_gm,
	read_gm,
};

attrib *
make_gm(const struct plane * pl)
{
	attrib * a = a_new(&at_gm);
	a->data.v = (void*)pl;
	return a;
}
