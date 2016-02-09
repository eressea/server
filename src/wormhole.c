/* 
 +-------------------+
 |                   |  Christian Schlittchen <corwin@amber.kn-bremen.de>
 | Eressea PBEM host |  Enno Rehling <enno@eressea.de>
 | (c) 1998 - 2004   |  Katja Zedel <katze@felidae.kn-bremen.de>
 |                   |
 +-------------------+

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#include <platform.h>
#include <kernel/config.h>
#include "settings.h"

#include "wormhole.h"

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/version.h>

/* util includes */
#include <util/attrib.h>
#include <util/language.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <quicklist.h>

#include <storage.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>

static bool good_region(const region * r)
{
    return (!fval(r, RF_CHAOTIC) && r->age > 30 && rplane(r) == NULL
        && r->units != NULL && r->land != NULL);
}

static int cmp_age(const void *v1, const void *v2)
{
    const region **r1 = (const region **)v1;
    const region **r2 = (const region **)v2;
    if ((*r1)->age < (*r2)->age)
        return -1;
    if ((*r1)->age > (*r2)->age)
        return 1;
    return 0;
}

static int wormhole_age(struct attrib *a, void *owner)
{
    building *entry = (building *)owner;
    region *exit = (region *)a->data.v;
    int maxtransport = entry->size;
    region *r = entry->region;
    unit *u = r->units;

    unused_arg(owner);
    for (; u != NULL && maxtransport != 0; u = u->next) {
        if (u->building == entry) {
            message *m = NULL;
            if (u->number > maxtransport || has_limited_skills(u)) {
                m = msg_message("wormhole_requirements", "unit region", u, u->region);
            }
            else if (exit != NULL) {
                move_unit(u, exit, NULL);
                maxtransport -= u->number;
                m = msg_message("wormhole_exit", "unit region", u, exit);
                add_message(&exit->msgs, m);
            }
            if (m != NULL) {
                add_message(&u->faction->msgs, m);
                msg_release(m);
            }
        }
    }

    remove_building(&r->buildings, entry);
    ADDMSG(&r->msgs, msg_message("wormhole_dissolve", "region", r));

    /* age returns 0 if the attribute needs to be removed, !=0 otherwise */
    return AT_AGE_KEEP;
}

static void wormhole_write(const struct attrib *a, const void *owner, struct storage *store)
{
    region *exit = (region *)a->data.v;
    write_region_reference(exit, store);
}

/** conversion code, turn 573, 2008-05-23 */
static int resolve_exit(variant id, void *address)
{
    building *b = findbuilding(id.i);
    region **rp = address;
    if (b) {
        *rp = b->region;
        return 0;
    }
    *rp = NULL;
    return -1;
}

static int wormhole_read(struct attrib *a, void *owner, struct storage *store)
{
    resolve_fun resolver = (global.data_version < UIDHASH_VERSION)
        ? resolve_exit : resolve_region_id;
    read_fun reader = (global.data_version < UIDHASH_VERSION)
        ? read_building_reference : read_region_reference;

    if (global.data_version < ATTRIBOWNER_VERSION) {
        READ_INT(store, NULL);
    }
    if (read_reference(&a->data.v, store, reader, resolver) == 0) {
        if (!a->data.v) {
            return AT_READ_FAIL;
        }
    }
    return AT_READ_OK;
}

static attrib_type at_wormhole = {
    "wormhole",
    NULL,
    NULL,
    wormhole_age,
    wormhole_write,
    wormhole_read,
    NULL,
    ATF_UNIQUE
};

static void
make_wormhole(const building_type * bt_wormhole, region * r1, region * r2)
{
    building *b1 = new_building(bt_wormhole, r1, default_locale);
    building *b2 = new_building(bt_wormhole, r2, default_locale);
    attrib *a1 = a_add(&b1->attribs, a_new(&at_wormhole));
    attrib *a2 = a_add(&b2->attribs, a_new(&at_wormhole));
    a1->data.v = b2->region;
    a2->data.v = b1->region;
    b1->size = bt_wormhole->maxsize;
    b2->size = bt_wormhole->maxsize;
    ADDMSG(&r1->msgs, msg_message("wormhole_appear", "region", r1));
    ADDMSG(&r2->msgs, msg_message("wormhole_appear", "region", r2));
}

#define WORMHOLE_CHANCE 10000

static void select_wormhole_regions(quicklist **rlistp, int *countp) {
    quicklist *rlist = 0;
    region *r = regions;
    int count = 0;

    while (r != NULL) {
        int next = rng_int() % (2 * WORMHOLE_CHANCE);
        while (r != NULL && (next != 0 || !good_region(r))) {
            if (good_region(r)) {
                --next;
            }
            r = r->next;
        }
        if (r == NULL)
            break;
        ql_push(&rlist, r);
        ++count;
        r = r->next;
    }

    *countp = count;
    *rlistp = rlist;
}

void sort_wormhole_regions(quicklist *rlist, region **match, int count) {
    quicklist *ql;
    int qi, i = 0;

    for (ql = rlist, qi = 0; i != count; ql_advance(&ql, &qi, 1)) {
        match[i++] = (region *)ql_get(ql, qi);
    }
    qsort(match, count, sizeof(region *), cmp_age);
}

void make_wormholes(region **match, int count, const building_type *bt_wormhole) {
    int i;
    count /= 2;
    for (i = 0; i != count; ++i) {
        make_wormhole(bt_wormhole, match[i], match[i + count]);
    }
}

void wormholes_update(void)
{
    const building_type *bt_wormhole = bt_find("wormhole");
    quicklist *rlist = 0;
    int count = 0;
    region **match;

    if (bt_wormhole == NULL) {
        return;
    }

    select_wormhole_regions(&rlist, &count);
    if (count < 2) {
        return;
    }
    match = (region **)malloc(sizeof(region *) * count);
    sort_wormhole_regions(rlist, match, count);
    ql_free(rlist);
    make_wormholes(match, count, bt_wormhole);
    free(match);
}

void wormholes_register(void)
{
    at_register(&at_wormhole);
}
