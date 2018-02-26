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

#include "museum.h"

/* kernel includes */
#include <kernel/building.h>
#include <kernel/connection.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include <kernel/faction.h>

#include <move.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/functions.h>
#include <util/gamedata.h>
#include <util/strings.h>
#include <util/language.h>
#include <util/macros.h>

#include <storage.h>

/* libc includes */
#include <limits.h>
#include <stdlib.h>
#include <assert.h>

#define PFL_MUSEUM PFL_NOMONSTERS | PFL_NORECRUITS | PFL_NOGIVE | PFL_NOATTACK | PFL_NOTERRAIN | PFL_NOMAGIC | PFL_NOSTEALTH | PFL_NOTEACH | PFL_NOBUILD | PFL_NOFEED

attrib_type at_museumexit = {
    "museumexit", NULL, NULL, NULL, a_writeshorts, a_readshorts
};

static void a_initmuseumgivebackcookie(attrib * a)
{
    a->data.v = calloc(1, sizeof(museumgivebackcookie));
}

static void a_finalizemuseumgivebackcookie(attrib * a)
{
    free(a->data.v);
}

static void
a_writemuseumgivebackcookie(const attrib * a, const void *owner,
struct storage *store)
{
    museumgivebackcookie *gbc = (museumgivebackcookie *)a->data.v;
    WRITE_INT(store, gbc->warden_no);
    WRITE_INT(store, gbc->cookie);
}

static int
a_readmuseumgivebackcookie(attrib * a, void *owner, gamedata *data)
{
    museumgivebackcookie *gbc = (museumgivebackcookie *)a->data.v;
    READ_INT(data->store, &gbc->warden_no);
    READ_INT(data->store, &gbc->cookie);
    return AT_READ_OK;
}

attrib_type at_museumgivebackcookie = {
    "museumgivebackcookie",
    a_initmuseumgivebackcookie,
    a_finalizemuseumgivebackcookie,
    NULL,
    a_writemuseumgivebackcookie,
    a_readmuseumgivebackcookie
};

attrib_type at_warden = {
    "itemwarden", NULL, NULL, NULL, a_writeint, a_readint
};

static void a_initmuseumgiveback(attrib * a)
{
    a->data.v = calloc(1, sizeof(museumgiveback));
}

static void a_finalizemuseumgiveback(attrib * a)
{
    museumgiveback *gb = (museumgiveback *)a->data.v;
    i_freeall(&gb->items);
    free(a->data.v);
}

static void
a_writemuseumgiveback(const attrib * a, const void *owner,
struct storage *store)
{
    museumgiveback *gb = (museumgiveback *)a->data.v;
    WRITE_INT(store, gb->cookie);
    write_items(store, gb->items);
}

static int a_readmuseumgiveback(attrib * a, void *owner, struct gamedata *data)
{
    museumgiveback *gb = (museumgiveback *)a->data.v;
    READ_INT(data->store, &gb->cookie);
    read_items(data->store, &gb->items);
    return AT_READ_OK;
}

attrib_type at_museumgiveback = {
    "museumgiveback",
    a_initmuseumgiveback,
    a_finalizemuseumgiveback,
    NULL,
    a_writemuseumgiveback,
    a_readmuseumgiveback
};

static int
use_museumexitticket(unit * u, const struct item_type *itype, int amount,
order * ord)
{
    attrib *a;
    region *r;
    unit *warden = findunit(atoi36("mwar"));
    int unit_cookie;

    UNUSED_ARG(amount);

    /* Prüfen ob in Eingangshalle */
    if (u->region->x != 9525 || u->region->y != 9525) {
        cmistake(u, ord, 266, MSG_MAGIC);
        return 0;
    }

    a = a_find(u->attribs, &at_museumexit);
    assert(a);
    r = findregion(a->data.sa[0], a->data.sa[1]);
    assert(r);
    a_remove(&u->attribs, a);
    /* Übergebene Gegenstände zurückgeben */

    a = a_find(u->attribs, &at_museumgivebackcookie);
    if (a) {
        unit_cookie = a->data.i;
        a_remove(&u->attribs, a);

        for (a = a_find(warden->attribs, &at_museumgiveback);
            a && a->type == &at_museumgiveback; a = a->next) {
            if (((museumgiveback *)(a->data.v))->cookie == unit_cookie)
                break;
        }
        if (a && a->type == &at_museumgiveback) {
            museumgiveback *gb = (museumgiveback *)(a->data.v);
            item *it;

            for (it = gb->items; it; it = it->next) {
                i_change(&u->items, it->type, it->number);
            }
            ADDMSG(&u->faction->msgs, msg_message("museumgiveback",
                "region unit sender items", r, u, warden, gb->items));
            a_remove(&warden->attribs, a);
        }
    }

    /* Benutzer zurück teleportieren */
    move_unit(u, r, NULL);

    /* Exitticket abziehen */
    i_change(&u->items, itype, -1);

    return 0;
}

static int
use_museumticket(unit * u, const struct item_type *itype, int amount,
order * ord)
{
    attrib *a;
    region *r = u->region;
    plane *pl = rplane(r);

    UNUSED_ARG(amount);

    /* Pruefen ob in normaler Plane und nur eine Person */
    if (pl != get_homeplane()) {
        cmistake(u, ord, 265, MSG_MAGIC);
        return 0;
    }
    if (u->number != 1) {
        cmistake(u, ord, 267, MSG_MAGIC);
        return 0;
    }
    if (has_horses(u)) {
        cmistake(u, ord, 272, MSG_MAGIC);
        return 0;
    }

    /* In diesem Attribut merken wir uns, wohin die Einheit zurückgesetzt
     * wird, wenn sie das Museum verläßt. */

    a = a_add(&u->attribs, a_new(&at_museumexit));
    a->data.sa[0] = (short)r->x;
    a->data.sa[1] = (short)r->y;

    /* Benutzer in die Halle teleportieren */
    move_unit(u, findregion(9525, 9525), NULL);

    /* Ticket abziehen */
    i_change(&u->items, itype, -1);

    /* Benutzer ein Exitticket geben */
    i_change(&u->items, itype, 1);

    return 0;
}

/***
* special quest door
***/

bool b_blockquestportal(const connection * b, const unit * u,
    const region * r)
{
    if (b->data.i > 0)
        return true;
    return false;
}

static const char *b_namequestportal(const connection * b, const region * r,
    const struct faction *f, int gflags)
{
    const char *bname;
    int lock = b->data.i;
    UNUSED_ARG(b);
    UNUSED_ARG(r);

    if (gflags & GF_ARTICLE) {
        if (lock > 0) {
            bname = "a_gate_locked";
        }
        else {
            bname = "a_gate_open";
        }
    }
    else {
        if (lock > 0) {
            bname = "gate_locked";
        }
        else {
            bname = "gate_open";
        }
    }
    if (gflags & GF_PURE)
        return bname;
    return LOC(f->locale, mkname("border", bname));
}

border_type bt_questportal = {
    "questportal", VAR_INT,
    b_opaque,
    NULL,                         /* init */
    NULL,                         /* destroy */
    b_read,                       /* read */
    b_write,                      /* write */
    b_blockquestportal,           /* block */
    b_namequestportal,            /* name */
    b_rvisible,                   /* rvisible */
    b_fvisible,                   /* fvisible */
    b_uvisible,                   /* uvisible */
};

static void use_key1(connection *b, void *data) {
    unit * u = (unit *)data;
    if (b->type == &bt_questportal) {
        const struct item_type *itype = it_find("questkey1");
        ADDMSG(&u->faction->msgs,
            msg_message("use_item", "unit amount item", u, 1, itype->rtype));
        b->data.i &= 0xFE;
    }
}

static void use_key2(connection *b, void *data) {
    unit * u = (unit *)data;
    if (b->type == &bt_questportal) {
        const struct item_type *itype = it_find("questkey2");
        ADDMSG(&u->faction->msgs,
            msg_message("use_item", "unit amount item", u, 1, itype->rtype));
        b->data.i &= 0xFD;
    }
}

static int
use_museumkey(unit * u, const struct item_type *itype, int amount,
order * ord)
{
    const struct item_type *ikey = it_find("questkey1");
    assert(u);
    assert(itype && ikey);
    assert(amount >= 1);

    walk_connections(u->region, itype == ikey ? use_key1 : use_key2, u);
    return 0;
}

void register_museum(void)
{
    register_bordertype(&bt_questportal);

    at_register(&at_warden);
    at_register(&at_museumexit);
    at_register(&at_museumgivebackcookie);
    at_register(&at_museumgiveback);

    register_item_use(use_museumexitticket, "use_museumexitticket");
    register_item_use(use_museumticket, "use_museumticket");
    register_item_use(use_museumkey, "use_questkey1");
    register_item_use(use_museumkey, "use_questkey2");
}
