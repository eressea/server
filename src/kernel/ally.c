#include "platform.h"
#include "config.h"
#include "types.h"
#include "ally.h"

#include "unit.h"
#include "region.h"
#include "group.h"
#include "faction.h"
#include "objtypes.h"

#include <kernel/attrib.h>
#include <util/strings.h>
#include <kernel/gamedata.h>

#include <storage.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define BLOCKSIZE 15
typedef struct allies {
    struct allies *next;
    int num;
    struct faction *factions[BLOCKSIZE];
    int status[BLOCKSIZE];
} allies;

static void block_insert(allies *al, struct faction *f, int status) {
    int i = al->num++;
    al->status[i] = status;
    al->factions[i] = f;
    /* TODO: heapify */
}

static int block_search(allies *al, const struct faction *f) {
    int i;
    /* TODO: binary search */
    for (i = 0; i != al->num; ++i) {
        if (al->factions[i] == f) {
            return i;
        }
    }
    return BLOCKSIZE;
}

int allies_walk(struct allies *all, cb_allies_walk callback, void *udata)
{
     allies *al;
    for (al = all; al; al = al->next) {
        int i;
        for (i = 0; i != al->num; ++i) {
            int e = callback(all, al->factions[i], al->status[i], udata);
            if (e != 0) {
                return e;
            }
        }
    }
    return 0;
}

int ally_get(allies *al, const struct faction *f)
{
    for (; al; al =  al->next) {
        int i = block_search(al, f);
        if (i != BLOCKSIZE) {
            return al->status[i];
        }
    }
    return 0;
}

void ally_set(allies **p_al, struct faction *f, int status)
{
    while (*p_al) {
        allies *al = *p_al;
        int i = block_search(al, f);
        if (i != BLOCKSIZE) {
            if (status == 0) {
                if (--al->num != i) {
                    al->factions[i] = al->factions[al->num];
                    al->status[i] = al->status[al->num];
                    /* TODO: repair heap up or down */
                }
                else if (al->num == 0) {
                    *p_al = al->next;
                    free(al);
                    return;
                }
            }
            else {
                al->status[i] = status;
            }
            return;
        }
        if (al->num < BLOCKSIZE) {
            block_insert(al, f, status);
            return;
        }
        p_al = &al->next;
    }
    *p_al = calloc(1, sizeof(allies));
    block_insert(*p_al, f, status);
}

void write_allies(gamedata * data, const allies *alist)
{
    const allies *al;
    for (al = alist; al; al = al->next) {
        int i;
        for (i = 0; i != al->num; ++i) {
            const faction * f = al->factions[i];
            if (f && f->_alive) {
                write_faction_reference(f, data->store);
                WRITE_INT(data->store, al->status[i]);
            }
        }
    }
    write_faction_reference(NULL, data->store);
}

void read_allies(gamedata * data, allies **p_al)
{
    for (;;) {
        faction *f;
        int aid, status;
        READ_INT(data->store, &aid);
        /* TODO: deal with unresolved factions, somehow */
        if (aid <= 0) {
            break;
        }
        f = findfaction(aid);
        if (!f) f = faction_create(aid);
        READ_INT(data->store, &status);
        ally_set(p_al, f, status);
    }
}

void allies_free(allies *al)
{
    while (al) {
        allies * an = al->next;
        free(al);
        al = an;
    }
}

allies *allies_clone(const allies *al) {
    allies *al_clone = NULL, **al_end = &al_clone;

    for (; al; al = al->next) {
        allies *al_new = calloc(1, sizeof(allies));
        memcpy(al_new, al, sizeof(allies));
        *al_end = al_new;
        al_end = &al_new->next;
    }
    *al_end = NULL;
    return al_clone;
}

static int ally_flag(const char *s, int help_mask)
{
    if ((help_mask & HELP_MONEY) && strcmp(s, "money") == 0)
        return HELP_MONEY;
    if ((help_mask & HELP_FIGHT) && strcmp(s, "fight") == 0)
        return HELP_FIGHT;
    if ((help_mask & HELP_GIVE) && strcmp(s, "give") == 0)
        return HELP_GIVE;
    if ((help_mask & HELP_GUARD) && strcmp(s, "guard") == 0)
        return HELP_GUARD;
    if ((help_mask & HELP_FSTEALTH) && strcmp(s, "stealth") == 0)
        return HELP_FSTEALTH;
    if ((help_mask & HELP_TRAVEL) && strcmp(s, "travel") == 0)
        return HELP_TRAVEL;
    return 0;
}

/** Specifies automatic alliance modes.
* If this returns a value then the bits set are immutable between alliance
* partners (faction::alliance) and cannot be changed with the HELP command.
*/
int AllianceAuto(void)
{
    int value;
    const char *str = config_get("alliance.auto");
    value = 0;
    if (str != NULL) {
        char *sstr = str_strdup(str);
        char *tok = strtok(sstr, " ");
        while (tok) {
            value |= ally_flag(tok, -1);
            tok = strtok(NULL, " ");
        }
        free(sstr);
    }
    return value & HelpMask();
}

static int
autoalliance(const faction * sf, const faction * f2)
{
    if (sf->alliance == f2->alliance) {
        return AllianceAuto();
    }
    return 0;
}

static void init_npcfaction(variant *var)
{
    var->i = 1;
}

attrib_type at_npcfaction = {
    "npcfaction",
    init_npcfaction,
    NULL,
    NULL,
    a_writeint,
    a_readint,
    NULL,
    ATF_UNIQUE
};

/** Limits the available help modes
* The bitfield returned by this function specifies the available help modes
* in this game (so you can, for example, disable HELP GIVE globally).
* Disabling a status will disable the command sequence entirely (order parsing
* uses this function).
*/
int HelpMask(void)
{
    static int config, rule;
    if (config_changed(&config)) {
        const char *str = config_get("rules.help.mask");
        if (str != NULL) {
            char *sstr = str_strdup(str);
            char *tok = strtok(sstr, " ");
            rule = 0;
            while (tok) {
                rule |= ally_flag(tok, -1);
                tok = strtok(NULL, " ");
            }
            free(sstr);
        }
        else {
            rule = HELP_ALL;
        }
    }
    return rule;
}

static int AllianceRestricted(void)
{
    static int config, rule;
    if (config_changed(&config)) {
        const char *str = config_get("alliance.restricted");
        rule = 0;
        if (str != NULL) {
            char *sstr = str_strdup(str);
            char *tok = strtok(sstr, " ");
            while (tok) {
                rule |= ally_flag(tok, -1);
                tok = strtok(NULL, " ");
            }
            free(sstr);
        }
        rule &= HelpMask();
    }
    return rule;
}

int alliance_status(const faction *f, const faction *f2, int status) {
    status |= autoalliance(f, f2);
    if (status > 0) {
        int mask = AllianceRestricted();
        if (mask) {
            if (a_find(f->attribs, &at_npcfaction)) {
                return status;
            }
            if (a_find(f2->attribs, &at_npcfaction)) {
                return status;
            }
            if (f->alliance != f2->alliance) {
                status &= ~mask;
            }
        }
    }
    return status;
}

int
alliedgroup(const struct faction *f,
    const struct faction *f2, const struct group *g, int mask)
{
    allies *all = g ? g->allies : f->allies;
    int status;

    if (!(faction_alive(f) && faction_alive(f2))) {
        return 0;
    }
    status = ally_get(all, f2) & mask;
    return alliance_status(f, f2, status);
}

int
alliedfaction(const struct faction *f, const struct faction *f2, int mask)
{
    return alliedgroup(f, f2, NULL, mask);
}

/* Die Gruppe von Einheit u hat helfe zu f2 gesetzt. */
int alliedunit(const unit * u, const faction * f2, int mask)
{
    assert(u);
    assert(f2);
    assert(u->region);            /* the unit should be in a region, but it's possible that u->number==0 (TEMP units) */
    if (u->faction == f2) {
        return mask;
    }
    if (!faction_alive(f2)) {
        return 0;
    }
    if (u->faction != NULL && f2 != NULL) {
        group *g;

        if (mask & HELP_FIGHT) {
            if ((u->flags & UFL_DEFENDER) || (u->faction->flags & FFL_DEFENDER)) {
                faction *owner = region_get_owner(u->region);
                /* helps the owner of the region */
                if (owner == f2) {
                    return HELP_FIGHT;
                }
            }
        }

        g = get_group(u);
        if (g) {
            return alliedgroup(u->faction, f2, g, mask);
        }
        return alliedfaction(u->faction, f2, mask);
    }
    return 0;
}

