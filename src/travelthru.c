/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
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

#include "travelthru.h"
#include "laws.h"
#include "report.h"

#include <kernel/unit.h>
#include <kernel/ship.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <util/attrib.h>
#include <util/bsdstring.h>
#include <util/log.h>
#include <util/language.h>
#include <stream.h>

#include <assert.h>
#include <string.h>

/*********************/
/*   at_travelunit   */
/*********************/
attrib_type at_travelunit = {
    "travelunit",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

static int count_travelthru(const struct region *r, const struct faction *f, attrib *alist) {
    int maxtravel = 0;
    attrib *a;
    for (a = alist; a && a->type == &at_travelunit; a = a->next) {
        unit *u = (unit *)a->data.v;

        if (r != u->region && (!u->ship || ship_owner(u->ship) == u)) {
            if (cansee_durchgezogen(f, r, u, 0)) {
                ++maxtravel;
            }
        }
    }
    return maxtravel;
}

void write_travelthru(stream *out, const region * r, const faction * f)
{
    attrib *abegin, *a;
    int counter = 0, maxtravel = 0;
    char buf[8192];
    char *bufp = buf;
    int bytes;
    size_t size = sizeof(buf) - 1;

    assert(r);
    assert(f);
    if (!fval(r, RF_TRAVELUNIT)) {
        return;
    }
    abegin = a_find(r->attribs, &at_travelunit);

    /* How many are we listing? For grammar. */
    maxtravel = count_travelthru(r, f, abegin);
    if (maxtravel == 0) {
        return;
    }

    /* Auflisten. */
    for (a = abegin; a && a->type == &at_travelunit; a = a->next) {
        unit *u = (unit *)a->data.v;

        if (r != u->region && (!u->ship || ship_owner(u->ship) == u)) {
            if (cansee_durchgezogen(f, r, u, 0)) {
                ++counter;
                if (u->ship != NULL) {
                    bytes = (int)strlcpy(bufp, shipname(u->ship), size);
                }
                else {
                    bytes = (int)strlcpy(bufp, unitname(u), size);
                }
                if (wrptr(&bufp, &size, bytes) != 0) {
                    INFO_STATIC_BUFFER();
                    break;
                }

                if (counter + 1 < maxtravel) {
                    bytes = (int)strlcpy(bufp, ", ", size);
                    if (wrptr(&bufp, &size, bytes) != 0) {
                        INFO_STATIC_BUFFER();
                        break;
                    }
                }
                else if (counter + 1 == maxtravel) {
                    bytes = (int)strlcpy(bufp, LOC(f->locale, "list_and"), size);
                    if (wrptr(&bufp, &size, bytes) != 0) {
                        INFO_STATIC_BUFFER();
                        break;
                    }
                }
            }
        }
    }
    if (size > 0) {
        if (maxtravel == 1) {
            bytes = _snprintf(bufp, size, " %s", LOC(f->locale, "has_moved_one"));
        }
        else {
            bytes = _snprintf(bufp, size, " %s", LOC(f->locale, "has_moved_many"));
        }
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER_EX("write_travelthru");
    }
    *bufp = 0;
    paragraph(out, buf, 0, 0, 0);
}

/** sets a marker in the region telling that the unit has travelled through it
* this is used for two distinctly different purposes:
* - to report that a unit has travelled through. the report function
*   makes sure to only report the ships of travellers, not the travellers
*   themselves
* - to report the region to the traveller
*/
void travelthru(const unit * u, region * r)
{
    attrib *ru = a_add(&r->attribs, a_new(&at_travelunit));

    fset(r, RF_TRAVELUNIT);

    ru->data.v = (void *)u;

    /* the first and last region of the faction gets reset, because travelthrough
    * could be in regions that are located before the [first, last] interval,
    * and recalculation is needed */
#ifdef SMART_INTERVALS
    update_interval(u->faction, r);
#endif
}

