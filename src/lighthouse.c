#include <platform.h>
#include "lighthouse.h"

#include <kernel/config.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include <util/log.h>
#include <util/attrib.h>

#include <assert.h>
#include <math.h>

attrib_type at_lighthouse = {
    "lighthouse"
    /* Rest ist NULL; tempor�res, nicht alterndes Attribut */
};

bool is_lighthouse(const building_type *btype)
{
    return is_building_type(btype, "lighthouse");
}

/* update_lighthouse: call this function whenever the size of a lighthouse changes
* it adds temporary markers to the surrounding regions.
* The existence of markers says nothing about the quality of the observer in
* the lighthouse, for this may change more frequently.
*/
void update_lighthouse(building * lh)
{
    region *r = lh->region;
    assert(is_lighthouse(lh->type));

    r->flags |= RF_LIGHTHOUSE;
    if (lh->size >= 10) {
        int d = lighthouse_range(lh, NULL, NULL);
        int x;
        for (x = -d; x <= d; ++x) {
            int y;
            for (y = -d; y <= d; ++y) {
                attrib *a;
                region *r2;
                int px = r->x + x, py = r->y + y;

                pnormalize(&px, &py, rplane(r));
                r2 = findregion(px, py);
                if (!r2 || !fval(r2->terrain, SEA_REGION))
                    continue;
                if (distance(r, r2) > d)
                    continue;
                a = a_find(r2->attribs, &at_lighthouse);
                while (a && a->type == &at_lighthouse) {
                    building *b = (building *)a->data.v;
                    if (b == lh)
                        break;
                    a = a->next;
                }
                if (!a) {
                    a = a_add(&r2->attribs, a_new(&at_lighthouse));
                    a->data.v = (void *)lh;
                }
            }
        }
    }
}

void remove_lighthouse(const building *lh) {
    building *b;
    region * r = lh->region;

    r->flags &= ~RF_LIGHTHOUSE;
    for (b = r->buildings; b; b = b->next) {
        if (b != lh && is_lighthouse(b->type)) {
            update_lighthouse(b);
        }
    }
}

int lighthouse_range(const building * b, const faction * f, const unit *u)
{
    if (b->size >= 10 && (b->flags & BLD_MAINTAINED)) {
        int maxd = (int)log10(b->size) + 1;

        if (u && skill_enabled(SK_PERCEPTION)) {
            int sk = effskill(u, SK_PERCEPTION, 0) / 3;
            assert(u->building == b);
            assert(u->faction == f);
            if (maxd > sk) maxd = sk;
        }
        /* E3A rule: no perception req'd */
        return maxd;
    }
    return 0;
}

bool lighthouse_guarded(const region * r)
{
    attrib *a;

    if (!r->attribs || !(r->terrain->flags & SEA_REGION)) {
        return false;
    }
    for (a = a_find(r->attribs, &at_lighthouse); a && a->type == &at_lighthouse;
        a = a->next) {
        building *b = (building *)a->data.v;

        assert(is_building_type(b->type, "lighthouse"));
        if ((b->flags & BLD_MAINTAINED) && b->size >= 10) {
            int maxd = (int)log10(b->size) + 1;
            int d = distance(r, b->region);
            assert(maxd >= d);
            return true;
        }
    }

    return false;
}
