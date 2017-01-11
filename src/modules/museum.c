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

#if MUSEUM_MODULE
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

void warden_add_give(unit * src, unit * u, const item_type * itype, int n)
{
    attrib *aw = a_find(u->attribs, &at_warden);
    museumgiveback *gb = NULL;
    museumgivebackcookie *gbc;
    attrib *a;

    /* has the giver a cookie corresponding to the warden */
    for (a = a_find(src->attribs, &at_museumgivebackcookie);
        a && a->type == &at_museumgivebackcookie; a = a->next) {
        if (((museumgivebackcookie *)(a->data.v))->warden_no == u->no)
            break;
    }

    /* if not give it one */
    if (a == NULL || a->type != &at_museumgivebackcookie) {
        a = a_add(&src->attribs, a_new(&at_museumgivebackcookie));
        gbc = (museumgivebackcookie *)a->data.v;
        gbc->warden_no = u->no;
        gbc->cookie = aw->data.i;
        assert(aw->data.i < INT_MAX);
        aw->data.i++;
    }
    else {
        gbc = (museumgivebackcookie *)(a->data.v);
    }

    /* now we search for the warden's corresponding item list */
    for (a = a_find(u->attribs, &at_museumgiveback);
        a && a->type == &at_museumgiveback; a = a->next) {
        gb = (museumgiveback *)a->data.v;
        if (gb->cookie == gbc->cookie) {
            break;
        }
    }

    /* if there's none, give it one */
    if (!gb) {
        a = a_add(&u->attribs, a_new(&at_museumgiveback));
        gb = (museumgiveback *)a->data.v;
        gb->cookie = gbc->cookie;
    }

    /* now register the items */
    i_change(&gb->items, itype, n);

    /* done */

    /* this has a caveat: If the src-unit is destroyed while inside
     * the museum, the corresponding itemlist of the warden will never
     * be removed. to circumvent that in a generic way will be extremly
     * difficult. */
}

void create_museum(void)
{
#if 0                           /* TODO: move this to Lua. It should be possible. */
    unsigned int museum_id = hashstring("museum");
    plane *museum = getplanebyid(museum_id);
    region *r;
    building *b;
    const terrain_type *terrain_hall = get_terrain("hall1");
    const terrain_type *terrain_corridor = get_terrain("corridor1");

    assert(terrain_corridor && terrain_hall);

    if (!museum) {
        museum = create_new_plane(museum_id, "Museum", 9500, 9550,
            9500, 9550, PFL_MUSEUM);
    }

    if (findregion(9525, 9525) == NULL) {
        /* Eingangshalle */
        r = new_region(9525, 9525, 0);
        terraform_region(r, terrain_hall);
        r->planep = museum;
        rsetname(r, "Eingangshalle");
        rsethorses(r, 0);
        rsetmoney(r, 0);
        rsetpeasants(r, 0);
        set_string(&r->display,
            "Die Eingangshalle des Großen Museum der 1. Welt ist bereits jetzt ein beeindruckender Anblick. Obwohl das Museum noch nicht eröffnet ist, vermittelt sie bereits einen Flair exotischer Welten. In den Boden ist ein großer Kompass eingelassen, der den Besuchern bei Orientierung helfen soll.");
    }

    r = findregion(9526, 9525);
    if (!r) {
        /* Lounge */
        r = new_region(9526, 9525, 0);
        terraform_region(r, terrain_hall);
        r->planep = museum;
        rsetname(r, "Lounge");
        rsethorses(r, 0);
        rsetmoney(r, 0);
        rsetpeasants(r, 0);
        set_string(&r->display,
            "Die Lounge des großen Museums ist ein Platz, in dem sich die Besucher treffen, um die Eindrücke, die sie gewonnen haben, zu verarbeiten. Gemütliche Sitzgruppen laden zum Verweilen ein.");
    }

    r = findregion(9526, 9525);
    if (!r->buildings) {
        const building_type *bt_generic = bt_find("generic");
        b = new_building(bt_generic, r, NULL);
        set_string(&b->name, "Séparée im dämonischen Stil");
        set_string(&b->display,
            "Diese ganz im dämonischen Stil gehaltene Sitzgruppe ist ganz in dunklen Schwarztönen gehalten. Muster fremdartiger Runen bedecken das merkwürdig geformte Mobiliar, das unangenehm lebendig wirkt.");

        b = new_building(bt_generic, r, NULL);
        set_string(&b->name, "Séparée im elfischen Stil");
        set_string(&b->display,
            "Ganz in Grün- und Brauntönen gehalten wirkt die Sitzgruppe fast lebendig. Bei näherer Betrachtung erschließt sich dem Betrachter, daß sie tatsächlich aus lebenden Pflanzen erstellt ist. So ist der Tisch aus einem eizigen Baum gewachsen, und die Polster bestehen aus weichen Grassoden. Ein wunderschön gemusterter Webteppich mit tausenden naturgetreu eingestickter Blumensarten bedeckt den Boden.");

        b = new_building(bt_generic, r, NULL);
        set_string(&b->name, "Séparée im halblingschen Stil");
        set_string(&b->display,
            "Dieses rustikale Mobiliar ist aus einem einzigen, gewaltigen Baum hergestellt worden. Den Stamm haben fleißige Halblinge der Länge nach gevierteilt und aus den vier langen Viertelstämmen die Sitzbänke geschnitzt, während der verbleibende Stumpf als Tisch dient. Schon von weitem steigen dem Besucher die Gerüche der Köstlichkeiten entgegen, die auf dem Tisch stapeln.");

        b = new_building(bt_generic, r, NULL);
        set_string(&b->name, "Séparée im orkischen Stil");
        set_string(&b->display,
            "Grobgeschreinerte, elfenhautbespannte Stühle und ein Tisch aus Knochen, über deren Herkunft man sich lieber keine Gedanken macht, bilden die Sitzgruppe im orkischen Stil. Überall haben Orks ihre Namen, und anderes wenig zitierenswertes in das Holz und Gebein geritzt.");

        b = new_building(bt_generic, r, NULL);
        set_string(&b->name, "Séparée im Meermenschenstil");
        set_string(&b->display,
            "Ganz in Blau- und Grüntönen gehalten, mit Algen und Muscheln verziert wirken die aus altem Meerholz geschnitzten Stühle immer ein wenig feucht. Seltsammerweise hat der schwere aus alten Planken gezimmerte Tisch einen Mast mit kompletten Segel in der Mitte.");

        b = new_building(bt_generic, r, NULL);
        set_string(&b->name, "Séparée im Katzenstil");
        set_string(&b->display,
            "Die Wände dieses Séparée sind aus dunklem Holz. Was aus der Ferne wie ein chaotisch durchbrochenes Flechtwerk wirkt, entpuppt sich bei näherer Betrachtung als eine bis in winzige Details gestaltete dschungelartige Landschaft, in die eine Vielzahl von kleinen Bildergeschichten eingewoben sind. Wie es scheint hat sich der Künstler Mühe gegeben wirklich jedes Katzenvolk Eresseas zu porträtieren. Das schummrige Innere wird von einem Kamin dominiert, vor dem einige Sessel und weiche Kissen zu einem gemütlichen Nickerchen einladen. Feiner Anduner Sisal bezieht die Lehnen der Sessel und verlockt dazu, seine Krallen hinein zu versenken. Auf einem kleinen Ecktisch steht ein großer Korb mit roten Wollknäulen und grauen und braunen Spielmäusen.");
    } else {
        for (b = r->buildings; b; b = b->next) {
            b->size = b->type->maxsize;
        }
    }

    r = findregion(9524, 9526);
    if (!r) {
        r = new_region(9524, 9526, 0);
        terraform_region(r, terrain_corridor);
        r->planep = museum;
        rsetname(r, "Nördliche Promenade");
        rsethorses(r, 0);
        rsetmoney(r, 0);
        rsetpeasants(r, 0);
        set_string(&r->display,
            "Die Nördliche Promenade führt direkt in den naturgeschichtlichen Teil des Museums.");
    }
    r = findregion(9525, 9524);
    if (!r) {
        r = new_region(9525, 9524, 0);
        terraform_region(r, terrain_corridor);
        r->planep = museum;
        rsetname(r, "Südliche Promenade");
        rsethorses(r, 0);
        rsetmoney(r, 0);
        rsetpeasants(r, 0);
        set_string(&r->display,
            "Die Südliche Promenade führt den Besucher in den kulturgeschichtlichen Teil des Museums.");
    }
#endif
}

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
            msg_message("use_item", "unit item", u, itype->rtype));
        b->data.i &= 0xFE;
    }
}

static void use_key2(connection *b, void *data) {
    unit * u = (unit *)data;
    if (b->type == &bt_questportal) {
        const struct item_type *itype = it_find("questkey2");
        ADDMSG(&u->faction->msgs,
            msg_message("use_item", "unit item", u, itype->rtype));
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

    register_item_use(use_museumticket, "use_museumticket");
    register_item_use(use_museumkey, "use_museumkey");
    register_item_use(use_museumexitticket, "use_museumexitticket");
}

#endif
