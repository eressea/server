#include "firewall.h"

#include "spells.h"
#include "borders.h"

#include <magic.h>

#include <kernel/attrib.h>
#include <kernel/connection.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/gamedata.h>
#include <kernel/messages.h>
#include <kernel/region.h>
#include <kernel/unit.h>

#include <storage.h>

/* libc includes */
#include <assert.h>
#include <math.h>

static connection *find_firewall(const region *r1, const region *r2)
{
    connection *b = get_borders(r1, r2);
    for (; b; b = b->next) {
        if (b->type == &bt_firewall) {
            return b;
        }
    }
    return NULL;
}

static struct message *firewall_info(const void *obj, objtype_t typ,
    const struct curse * c, int self)
{
    (void)obj;
    (void)typ;
    (void)self;
    return msg_message("curseinfo::firewall", "direction id", c->data.sa[0], c->no);
}

static void firewall_change(curse *c, double delta, void *owner)
{
    assert(c);
    c->vigour += delta;
    if (c->vigour <= 0) {
        direction_t dir = c->data.sa[0];
        region *r = (region *)owner;
        region *r2 = rconnect(r, dir);
        connection *b = find_firewall(r, r2);
        if (b) {
            erase_border(b);
        }
    }
}

static int firewall_read(gamedata *data, curse *c, void *owner) {
    int i;
    READ_INT(data->store, &i); /* direction */
    c->data.sa[0] = (direction_t)i;
    return 0;
}

static int firewall_write(storage *store, const curse *c, const void *owner) {
    WRITE_INT(store, c->data.sa[0]); /* direction */
    return 0;
}

static int firewall_age(struct curse *c, void *owner)
{
    direction_t dir = c->data.sa[0];
    region *r = (region *)owner;
    region *r2 = rconnect(r, dir);
    connection *b = find_firewall(r, r2);
    if (b) {
        wall_data *wd = (wall_data *)b->data.v;
        if (0 == wd->countdown || c->vigour <= 0.0) {
            curse *c2 = get_curse(r2->attribs, &ct_firewall);
            if (c2) {
                remove_curse(&r2->attribs, c2);
            }
            erase_border(b);
            return AT_AGE_REMOVE;
        }
        else {
            wd->active = true;
            --wd->countdown;
            return AT_AGE_KEEP;
        }
    }

    /* if the firewall is gone, we no longer need the curse */
    return AT_AGE_REMOVE;
}

const struct curse_type ct_firewall = {
    "firewall", CURSETYP_REGION, 0, NO_MERGE,
    firewall_info,
    firewall_change,
    firewall_read,
    firewall_write,
    NULL,
    firewall_age,
};

void create_firewall(struct unit *mage, struct region *r, enum direction_t d, double force, int duration)
{
    region *r2 = rconnect(r, d);
    connection *b = get_borders(r, r2);
    wall_data *fd;
    curse *c;
    while (b != NULL) {
        if (b->type == &bt_firewall)
            break;
        b = b->next;
    }
    if (b == NULL) {
        b = create_border(&bt_firewall, r, r2);
        fd = (wall_data *)b->data.v;
        fd->force = (int)force;
        fd->active = false;
        fd->countdown = duration * 2;
    }
    else {
        fd = (wall_data *)b->data.v;
        fd->force = (int)fmax(fd->force, force);
        if (fd->countdown < duration)
            fd->countdown = duration;
    }
    c = create_curse(mage, &r->attribs, &ct_firewall, force, duration + 1, 0.0, 0);
    c->data.sa[0] = d; /* direction of r2 */
    c = create_curse(mage, &r2->attribs, &ct_firewall, force, duration + 1, 0.0, 0);
    c->data.sa[0] = d_reverse(d); /* direction of r */
}

int sp_firewall(castorder *co)
{
    region *r = co_get_region(co);
    unit *caster = co_get_caster(co);
    int cast_level = co->level;
    double force = co->force;
    spellparameter *param = co->a_params;
    int dir;
    region *r2;

    dir = (int)get_direction(param->data.xs, caster->faction->locale);
    if (dir >= 0) {
        r2 = rconnect(r, dir);
    }
    else {
        report_spell_failure(caster, co->order);
        return 0;
    }

    if (!r2 || r2 == r) {
        report_spell_failure(caster, co->order);
        return 0;
    }

    create_firewall(caster, r, dir, force / 2 + 0.5, cast_level);

    /* melden, 1x pro Partei */
    {
        message *seen = msg_message("firewall_effect", "mage region", caster, r);
        message *unseen = msg_message("firewall_effect", "mage region", (unit *)NULL, r);
        report_spell_effect(r, caster, seen, unseen);
        msg_release(seen);
        msg_release(unseen);
    }

    return cast_level;
}

