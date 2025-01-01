#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "config.h"
#include "ally.h"

#include "unit.h"
#include "region.h"
#include "group.h"
#include "faction.h"
#include "objtypes.h"

#include <kernel/attrib.h>
#include <kernel/gamedata.h>

#include <storage.h>
#include <strings.h>

#include <stb_ds.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct ally {
    struct faction *faction;
    int status;
} ally;

int allies_walk(struct ally *a_allies, cb_allies_walk callback, void *udata)
{
    size_t i, len = arrlen(a_allies);
    for (i = 0; i != len; ++i) {
        ally *al = a_allies + i;
        int e = callback(al->faction, al->status, udata);
        if (e != 0) {
            return e;
        }
    }
    return 0;
}

int ally_get(ally *a_allies, const struct faction *f)
{
    size_t i, len = arrlen(a_allies);
    for (i = 0; i != len; ++i) {
        ally *al = a_allies + i;
        if (al->faction == f) {
            return al->status;
        }
    }
    return 0;
}

struct ally *ally_set(ally **p_allies, struct faction *f, int status)
{
    ally *a_allies = *p_allies;
    size_t i, len = arrlen(a_allies);
    for (i = 0; i != len; ++i) {
        ally *al = a_allies + i;
        if (al->faction == f) {
            if (status == 0) {
                if (len == 1) {
                    arrfree(*p_allies);
                }
                else {
                    arrdelswap(*p_allies, i);
                }
                return NULL;
            }
            else {
                al->status = status;
            }
            return al;
        }
    }
    if (status > 0) {
        ally *al = arraddnptr(*p_allies, 1);
        al->faction = f;
        al->status = status;
        return al;
    }
    return NULL;
}

void write_allies(struct gamedata * data, const struct ally *a_allies)
{
    size_t i, len = arrlen(a_allies);
    for (i = 0; i != len; ++i) {
        const ally *al = a_allies + i;
        const faction * f = al->faction;
        if (f && faction_alive(f)) {
            write_faction_reference(f, data->store);
            assert(al->status > 0);
            WRITE_INT(data->store, al->status);
        }
    }
    write_faction_reference(NULL, data->store);
}

void read_allies(gamedata * data, ally **p_allies)
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
        /* NB: some data files have allies with status=0 */
        if (status > 0) {
            ally_set(p_allies, f, status);
        }
    }
}

void allies_free(ally *al)
{
    arrfree(al);
}

ally *allies_clone(const ally *al) {
    if (al) {
        ally *al_clone = NULL;
        size_t len = arrlen(al);

        arraddnptr(al_clone, len);
        if (al_clone) {
            memcpy(al_clone, al, len * sizeof(ally));
            return al_clone;
        }
    }
    return NULL;
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

int ally_status(const char *s)
{
    if (strcmp(s, "give") == 0) {
        return HELP_GIVE;
    }
    else if (strcmp(s, "fight") == 0) {
        return HELP_FIGHT;
    }
    else if (strcmp(s, "money") == 0) {
        return HELP_MONEY;
    }
    else if (strcmp(s, "travel") == 0) {
        return HELP_TRAVEL;
    }
    else if (strcmp(s, "guard") == 0) {
        return HELP_GUARD;
    }
    else if (strcmp(s, "all") == 0) {
        return HELP_ALL;
    }
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
    ally *all = g ? g->allies : f->allies;
    int status;

    if (!(faction_alive(f) && faction_alive(f2))) {
        return 0;
    }
    status = ally_get(all, f2) & mask;
    if (IS_PAUSED(f)) {
        status &= HELP_GUARD;
    }
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
    if (u->faction == f2) {
        return mask;
    }
    if (!faction_alive(f2)) {
        return mask;
    }
    if (u->faction != NULL && f2 != NULL) {
        group *g;
        g = get_group(u);
        if (g) {
            return alliedgroup(u->faction, f2, g, mask);
        }
        return alliedfaction(u->faction, f2, mask);
    }
    return 0;
}
