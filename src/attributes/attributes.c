/*
Copyright (c) 1998-2015, Enno Rehling Rehling <enno@eressea.de>
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
#include "attributes.h"

/* attributes includes */
#include "follow.h"
#include "hate.h"
#include "iceberg.h"
#include "key.h"
#include "stealth.h"
#include "moved.h"
#include "movement.h"
#include "dict.h"
#include "orcification.h"
#include "otherfaction.h"
#include "overrideroads.h"
#include "racename.h"
#include "raceprefix.h"
#include "reduceproduction.h"
#include "targetregion.h"

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/building.h>

/* util includes */
#include <util/attrib.h>

attrib_type at_unitdissolve = {
    "unitdissolve", NULL, NULL, NULL, a_writechars, a_readchars
};

void register_attributes(void)
{
    at_deprecate("gm", a_readint);
    at_register(&at_stealth);
    at_register(&at_dict);
    at_register(&at_unitdissolve);
    at_register(&at_overrideroads);
    at_register(&at_raceprefix);
    at_register(&at_iceberg);
    at_register(&at_key);
    at_register(&at_follow);
    at_register(&at_targetregion);
    at_register(&at_orcification);
    at_register(&at_hate);
    at_register(&at_reduceproduction);
    at_register(&at_otherfaction);
    at_register(&at_racename);
    at_register(&at_speedup);
    at_register(&at_movement);
    at_register(&at_moved);
}
