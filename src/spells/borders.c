#include "borders.h"

#include "vortex.h"

#include "kernel/connection.h"
#include "kernel/curse.h"
#include "kernel/config.h"
#include "kernel/faction.h"
#include "kernel/messages.h"
#include "kernel/region.h"
#include "kernel/terrain.h"
#include "kernel/unit.h"

#include "kernel/attrib.h"
#include "kernel/gamedata.h"
#include "util/language.h"
#include "util/macros.h"
#include "util/message.h"
#include "util/rand.h"
#include "util/rng.h"
#include "util/resolve.h"

#include <storage.h>

#include <assert.h>
#include <stdlib.h>

static int cw_read_depr(variant *var, void *target, gamedata *data)
{
    storage *store = data->store;
    curse c;
    var->v = &c;
    curse_read(var, store, target);
    READ_INT(store, NULL);
    return AT_READ_DEPR;
}

/* ------------------------------------------------------------- */
/* Name:       Feuerwand
 * Stufe:
 * Gebiet:     Draig
 * Kategorie:  Region, negativ
 * Flag:
 * Kosten:     SPC_LINEAR
 * Aura:
 * Komponenten:
 *
 * Wirkung:
 *   eine Wand aus Feuer entsteht in der angegebenen Richtung
 *
 *   Was fuer eine Wirkung hat die?
 */

static void wall_init(connection * b)
{
    wall_data *fd = (wall_data *)calloc(1, sizeof(wall_data));
    if (!fd) abort();
    fd->countdown = -1;           /* infinite */
    b->data.v = fd;
    fd->active = false;
}

static void wall_destroy(connection * b)
{
    free(b->data.v);
}

static void wall_read(connection * b, gamedata * data)
{
    static wall_data dummy;
    wall_data *fd = b->data.v ? (wall_data *)b->data.v : &dummy;

    if (data->version < WALL_DATA_VERSION) {
        read_unit_reference(data, NULL, NULL);
    }
    READ_INT(data->store, &fd->force);
    READ_INT(data->store, &fd->countdown);
    fd->active = true;
}

static void wall_write(const connection * b, storage * store)
{
    wall_data *fd = (wall_data *)b->data.v;
    WRITE_INT(store, fd->force);
    WRITE_INT(store, fd->countdown);
}

static region *wall_move(const connection * b, struct unit *u,
struct region *from, struct region *to, bool routing)
{
    wall_data *fd = (wall_data *)b->data.v;
    if (!routing && fd->active) {
        int hp = dice(3, fd->force) * u->number;
        if (u->hp < hp) hp = u->hp;
        u->hp -= hp;
        if (u->hp) {
            ADDMSG(&u->faction->msgs, msg_message("firewall_damage",
                "region unit", from, u));
        }
        else
            ADDMSG(&u->faction->msgs, msg_message("firewall_death", "region unit",
            from, u));
        if (u->number > u->hp) {
            scale_number(u, u->hp);
            u->hp = u->number;
        }
    }
    return to;
}

static const char *b_namefirewall(const connection * b, const region * r,
    const faction * f, int gflags)
{
    const char *bname;
    UNUSED_ARG(f);
    UNUSED_ARG(r);
    UNUSED_ARG(b);
    if (gflags & GF_ARTICLE)
        bname = "a_firewall";
    else
        bname = "firewall";

    if (gflags & GF_PURE)
        return bname;
    return LOC(f->locale, mkname("border", bname));
}

border_type bt_firewall = {
    "firewall", VAR_VOIDPTR, 0,
    b_transparent,                /* transparent */
    wall_init,                    /* init */
    wall_destroy,                 /* destroy */
    wall_read,                    /* read */
    wall_write,                   /* write */
    b_blocknone,                  /* block */
    b_namefirewall,               /* name */
    b_rvisible,                   /* rvisible */
    b_finvisible,                 /* fvisible */
    b_uinvisible,                 /* uvisible */
    NULL,
    wall_move
};

void convert_firewall_timeouts(connection * b, attrib * a)
{
    while (a) {
        if (b->type == &bt_firewall && a->type == &at_countdown) {
            wall_data *fd = (wall_data *)b->data.v;
            fd->countdown = a->data.i;
        }
        a = a->next;
    }
}

border_type bt_wisps = { /* only here for reading old data */
    "wisps", VAR_VOIDPTR, 0,
    b_transparent,                /* transparent */
    0,                   /* init */
    wall_destroy,                 /* destroy */
    wall_read,                    /* read */
    0,                   /* write */
    b_blocknone,                  /* block */
    0,                   /* name */
    b_rvisible,                   /* rvisible */
    b_fvisible,                   /* fvisible */
    b_uvisible,                   /* uvisible */
    NULL,                         /* visible */
    0
};

static bool chaosgate_valid(const connection * b)
{
    const attrib *a = a_find(b->from->attribs, &at_direction);
    if (!a)
        a = a_find(b->to->attribs, &at_direction);
    if (!a)
        return false;
    return true;
}

static struct region *chaosgate_move(const connection * b, struct unit *u,
    struct region *from, struct region *to, bool routing)
{
    UNUSED_ARG(from);
    UNUSED_ARG(b);
    if (!routing) {
        int maxhp = u->hp / 4;
        if (maxhp < u->number)
            maxhp = u->number;
        u->hp = maxhp;
    }
    return to;
}

border_type bt_chaosgate = {
    "chaosgate", VAR_NONE, 0,
    b_transparent,                /* transparent */
    NULL,                         /* init */
    NULL,                         /* destroy */
    NULL,                         /* read */
    NULL,                         /* write */
    b_blocknone,                  /* block */
    NULL,                         /* name */
    b_rinvisible,                 /* rvisible */
    b_finvisible,                 /* fvisible */
    b_uinvisible,                 /* uvisible */
    chaosgate_valid,
    chaosgate_move
};

void register_borders(void)
{
    border_convert_cb = &convert_firewall_timeouts;
    at_deprecate("cursewall", cw_read_depr);

    register_bordertype(&bt_firewall);
    register_bordertype(&bt_wisps);
    register_bordertype(&bt_chaosgate);
}
