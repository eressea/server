/*
+-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
| (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
|                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
+-------------------+  Stefan Reich <reich@halbling.de>

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <platform.h>
#include <kernel/config.h>
#include "alliance.h"

#include <attributes/key.h>

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/command.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/language.h>
#include <util/parser.h>
#include <quicklist.h>
#include <util/rng.h>
#include <util/umlaut.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

alliance *alliances = NULL;

void free_alliances(void)
{
    while (alliances) {
        alliance *al = alliances;
        alliances = al->next;
        free(al->name);
        if (al->members) {
            ql_free(al->members);
        }
        free(al);
    }
}

alliance *makealliance(int id, const char *name)
{
    alliance *al;

    for (;;) {
        if (id > 0) {
            for (al = alliances; al; al = al->next) {
                if (al->id == id) {
                    id = 0;
                    break;
                }
            }
            if (id > 0)
                break;
        }
        id = id ? id : (1 + (rng_int() % MAX_UNIT_NR));
    }
    return new_alliance(id, name);
}

alliance *new_alliance(int id, const char *name) {
    alliance *al;
    assert(id>0);

    al = calloc(1, sizeof(alliance));
    al->id = id;
    if (name) {
        al->name = strdup(name);
    }
    else {
        al->flags |= ALF_NON_ALLIED;
    }
    al->next = alliances;
    return alliances = al;
}

alliance *findalliance(int id)
{
    alliance *al;
    for (al = alliances; al; al = al->next) {
        if (al->id == id)
            return al;
    }
    return NULL;
}

typedef struct alliance_transaction {
    struct alliance_transaction *next;
    unit *u;
    order *ord;
    /*    alliance * al; */
    /*    variant userdata; */
} alliance_transaction;

static struct alliance_transaction *transactions[ALLIANCE_MAX];

faction *alliance_get_leader(alliance * al)
{
    if (!al->_leader) {
        if (al->members) {
            al->_leader = (faction *)ql_get(al->members, 0);
        }
    }
    return al->_leader;
}

static void create_transaction(int type, unit * u, order * ord)
{
    alliance_transaction *tr =
        (alliance_transaction *)calloc(1, sizeof(alliance_transaction));
    tr->ord = ord;
    tr->u = u;
    tr->next = transactions[type];
    transactions[type] = tr;
}

static void cmd_kick(const void *tnext, struct unit *u, struct order *ord)
{
    create_transaction(ALLIANCE_KICK, u, ord);
}

static void cmd_leave(const void *tnext, struct unit *u, struct order *ord)
{
    create_transaction(ALLIANCE_LEAVE, u, ord);
}

static void cmd_transfer(const void *tnext, struct unit *u, struct order *ord)
{
    create_transaction(ALLIANCE_TRANSFER, u, ord);
}

static void cmd_new(const void *tnext, struct unit *u, struct order *ord)
{
    create_transaction(ALLIANCE_NEW, u, ord);
}

static void cmd_invite(const void *tnext, struct unit *u, struct order *ord)
{
    create_transaction(ALLIANCE_INVITE, u, ord);
}

static void cmd_join(const void *tnext, struct unit *u, struct order *ord)
{
    create_transaction(ALLIANCE_JOIN, u, ord);
}

static void perform_kick(void)
{
    alliance_transaction **tap = transactions + ALLIANCE_KICK;
    while (*tap) {
        alliance_transaction *ta = *tap;
        alliance *al = f_get_alliance(ta->u->faction);

        if (al && alliance_get_leader(al) == ta->u->faction) {
            faction *f;
            init_order(ta->ord);
            skip_token();
            f = getfaction();
            if (f && f_get_alliance(f) == al) {
                setalliance(f, NULL);
            }
        }
        *tap = ta->next;
        free(ta);
    }
}

static void perform_new(void)
{
    alliance_transaction **tap = transactions + ALLIANCE_NEW;
    while (*tap) {
        alliance_transaction *ta = *tap;
        alliance *al;
        int id;
        faction *f = ta->u->faction;

        init_order(ta->ord);
        skip_token();
        id = getid();

        al = makealliance(id, itoa36(id));
        setalliance(f, al);

        *tap = ta->next;
        free(ta);
    }
}

static void perform_leave(void)
{
    alliance_transaction **tap = transactions + ALLIANCE_LEAVE;
    while (*tap) {
        alliance_transaction *ta = *tap;
        faction *f = ta->u->faction;

        setalliance(f, NULL);

        *tap = ta->next;
        free(ta);
    }
}

static void perform_transfer(void)
{
    alliance_transaction **tap = transactions + ALLIANCE_TRANSFER;
    while (*tap) {
        alliance_transaction *ta = *tap;
        alliance *al = f_get_alliance(ta->u->faction);

        if (al && alliance_get_leader(al) == ta->u->faction) {
            faction *f;
            init_order(ta->ord);
            skip_token();
            f = getfaction();
            if (f && f_get_alliance(f) == al) {
                al->_leader = f;
            }
        }
        *tap = ta->next;
        free(ta);
    }
}

static void perform_join(void)
{
    alliance_transaction **tap = transactions + ALLIANCE_JOIN;
    while (*tap) {
        alliance_transaction *ta = *tap;
        faction *fj = ta->u->faction;
        int aid;

        init_order(ta->ord);
        skip_token();
        aid = getid();
        if (aid) {
            alliance *al = findalliance(aid);
            if (al && f_get_alliance(fj) != al) {
                alliance_transaction **tip = transactions + ALLIANCE_INVITE;
                alliance_transaction *ti = *tip;
                while (ti) {
                    faction *fi = ti->u->faction;
                    if (fi && f_get_alliance(fi) == al) {
                        int fid;
                        init_order(ti->ord);
                        skip_token();
                        fid = getid();
                        if (fid == fj->no) {
                            break;
                        }
                    }
                    tip = &ti->next;
                    ti = *tip;
                }
                if (ti) {
                    setalliance(fj, al);
                    *tip = ti->next;
                    free(ti);
                }
                else {
                    /* TODO: error message */
                }
            }
        }
        *tap = ta->next;
        free(ta);
    }
}


static syntaxtree * build_syntax(void) {
    syntaxtree *slang, *stree = stree_create();
    for (slang = stree; slang; slang = slang->next) {
        stree_add(slang, LOC(slang->lang, alliance_kwd[ALLIANCE_KICK]), &cmd_kick);
        stree_add(slang, LOC(slang->lang, alliance_kwd[ALLIANCE_LEAVE]), &cmd_leave);
        stree_add(slang, LOC(slang->lang, alliance_kwd[ALLIANCE_TRANSFER]), &cmd_transfer);
        stree_add(slang, LOC(slang->lang, alliance_kwd[ALLIANCE_NEW]), &cmd_new);
        stree_add(slang, LOC(slang->lang, alliance_kwd[ALLIANCE_INVITE]), &cmd_invite);
        stree_add(slang, LOC(slang->lang, alliance_kwd[ALLIANCE_JOIN]), &cmd_join);
    }
    return stree;
}

static void execute(keyword_t kwd)
{
    struct syntaxtree *syntax = 0;

    region **rp = &regions;
    while (*rp) {
        region *r = *rp;
        unit **up = &r->units;
        while (*up) {
            unit *u = *up;
            if (u->number) {
                const struct locale *lang = u->faction->locale;
                order *ord;
                for (ord = u->orders; ord; ord = ord->next) {
                    if (getkeyword(ord) == kwd) {
                        void *root;
                        if (!syntax) syntax = build_syntax();
                        root = stree_find(syntax, lang);
                        do_command(root, u, ord);
                    }
                }
            }
            if (u == *up)
                up = &u->next;
        }
        if (*rp == r)
            rp = &r->next;
    }
    if (syntax) {
        stree_free(syntax);
        perform_kick();
        perform_leave();
        perform_transfer();
        perform_new();
        perform_join();
    }
}

const char* alliance_kwd[ALLIANCE_MAX] = {
    "kick",
    "leave",
    "command",
    "new",
    "invite",
    "join"
};

void alliance_cmd(void)
{
    execute(K_ALLIANCE);
    /* some may have been kicked, must remove f->alliance==NULL */
}

void setalliance(faction * f, alliance * al)
{
    if (f->alliance == al)
        return;
    if (f->alliance != NULL) {
        int qi;
        quicklist **flistp = &f->alliance->members;

        for (qi = 0; *flistp; ql_advance(flistp, &qi, 1)) {
            faction *data = (faction *)ql_get(*flistp, qi);
            if (data == f) {
                ql_delete(flistp, qi);
                break;
            }
        }

        if (f->alliance->_leader == f) {
            if (f->alliance->members) {
                f->alliance->_leader = (faction *)ql_get(f->alliance->members, 0);
            }
            else {
                f->alliance->_leader = NULL;
            }
        }
    }
    f->alliance = al;
    f->alliance_joindate = turn;
    if (al != NULL) {
        ql_push(&al->members, f);
        if (al->_leader == NULL) {
            al->_leader = f;
        }
    }
}

const char *alliancename(const alliance * al)
{
    typedef char name[OBJECTIDSIZE + 1];
    static name idbuf[8];
    static int nextbuf = 0;

    char *ibuf = idbuf[(++nextbuf) % 8];

    if (al && al->name) {
        slprintf(ibuf, sizeof(name), "%s (%s)", al->name, itoa36(al->id));
    }
    else {
        return NULL;
    }
    return ibuf;
}

void alliancevictory(void)
{
    const struct building_type *btype = bt_find("stronghold");
    region *r = regions;
    alliance *al = alliances;
    if (btype == NULL)
        return;
    while (r != NULL) {
        building *b = r->buildings;
        while (b != NULL) {
            if (b->type == btype) {
                unit *u = building_owner(b);
                if (u) {
                    fset(u->faction->alliance, FFL_MARK);
                }
            }
            b = b->next;
        }
        r = r->next;
    }
    while (al != NULL) {
        if (!fval(al, FFL_MARK)) {
            faction **fp;
            for (fp = &factions; *fp; ) {
                faction *f = *fp;
                if (f->alliance == al) {
                    ADDMSG(&f->msgs, msg_message("alliance::lost", "alliance", al));
                    destroyfaction(fp);
                } else {
                    fp = &f->next;
                }
            }
        }
        else {
            freset(al, FFL_MARK);
        }
        al = al->next;
    }
}

void alliance_setname(alliance * self, const char *name)
{
    free(self->name);
    if (name)
        self->name = strdup(name);
    else
        self->name = NULL;
}

bool is_allied(const struct faction *f1, const struct faction *f2) {
    return (f1 == f2 || (f1->alliance && f1->alliance == f2->alliance));
}
