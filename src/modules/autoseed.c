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
#include "autoseed.h"

#include "market.h"

/* kernel includes */
#include <kernel/alliance.h>
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/plane.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

#include <attributes/key.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <selist.h>
#include <util/rng.h>
#include <util/unicode.h>

/* libc includes */
#include <limits.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

const terrain_type *random_terrain(const terrain_type * terrains[],
    int distribution[], int size)
{
    int ndistribution = size;
    const terrain_type *terrain;
    int n;

    assert(size > 0);
    if (distribution) {
        ndistribution = 0;
        for (n = 0; n != size; ++n) {
            ndistribution += distribution[n];
        }
    }

    n = rng_int() % ndistribution;
    if (distribution) {
        int i;
        for (i = 0; i != size; ++i) {
            n -= distribution[i];
            if (n < 0)
                break;
        }
        assert(i < size);
        terrain = terrains[i];
    }
    else {
        terrain = terrains[n];
    }
    return terrain;
}

int seed_adamantium(region * r, int base)
{
    const resource_type *rtype = rt_find("adamantium");
    rawmaterial *rm;
    for (rm = r->resources; rm; rm = rm->next) {
        if (rm->type->rtype == rtype)
            break;
    }
    if (!rm) {
        add_resource(r, 1, base, 150, rtype);
    }
    return 0;
}

static int count_demand(const region * r)
{
    struct demand *dmd;
    int c = 0;
    if (r->land) {
        for (dmd = r->land->demands; dmd; dmd = dmd->next)
            c++;
    }
    return c;
}

static int
recurse_regions(region * r, region_list ** rlist,
bool(*fun) (const region * r))
{
    if (!fun(r))
        return 0;
    else {
        int len = 0;
        direction_t d;
        region_list *rl = calloc(sizeof(region_list), 1);
        rl->next = *rlist;
        rl->data = r;
        (*rlist) = rl;
        fset(r, RF_MARK);
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            region *nr = rconnect(r, d);
            if (nr && !fval(nr, RF_MARK))
                len += recurse_regions(nr, rlist, fun);
        }
        return len + 1;
    }
}

static bool f_nolux(const region * r)
{
    return (r->land && count_demand(r) != get_maxluxuries());
}

int fix_all_demand(region *rd) {
    region_list *rl, *rlist = NULL;
    recurse_regions(rd, &rlist, f_nolux);
    for (rl = rlist; rl; rl = rl->next) {
        region *r = rl->data;
        freset(r, RF_MARK);         /* undo recursive marker */
        if (!fix_demand(r)) {
            return -1;
        }
    }
    return 0;
}

/* nach 150 Runden ist Neustart erlaubt */
#define MINAGE_MULTI 150
newfaction *read_newfactions(const char *filename)
{
    newfaction *newfactions = NULL;
    FILE *F = fopen(filename, "r");
    char buf[1024];

    if (F == NULL)
        return NULL;
    for (;;) {
        faction *f;
        char race[20], email[64], lang[8], password[16];
        newfaction *nf, **nfi;
        int bonus = 0, subscription = 0;
        int alliance = 0;

        if (fgets(buf, sizeof(buf), F) == NULL)
            break;

        email[0] = '\0';
        password[0] = '\0';

        if (sscanf(buf, "%54s %20s %8s %d %d %16s %d", email, race, lang, &bonus,
            &subscription, password, &alliance) < 3)
            break;
        if (email[0] == '\0')
            break;
        if (password[0] == '\0') {
            size_t sz;
            sz = strlcpy(password, itoa36(rng_int()), sizeof(password));
            sz += strlcat(password, itoa36(rng_int()), sizeof(password));
        }
        for (f = factions; f; f = f->next) {
            if (strcmp(f->email, email) == 0 && f->subscription
                && f->age < MINAGE_MULTI)
                break;
        }
        if (f && f->units)
            continue;                 /* skip the ones we've already got */
        for (nf = newfactions; nf; nf = nf->next) {
            if (strcmp(nf->email, email) == 0)
                break;
        }
        if (nf)
            continue;
        nf = calloc(sizeof(newfaction), 1);
        if (set_email(&nf->email, email) != 0) {
            log_error("Invalid email address for subscription %s: %s\n", itoa36(subscription), email);
            free(nf);
            continue;
        }
        nf->password = strdup(password);
        nf->race = rc_find(race);
        nf->subscription = subscription;
        if (alliances != NULL) {
            struct alliance *al = findalliance(alliance);
            if (al == NULL) {
                char zText[64];
                sprintf(zText, "Allianz %d", alliance);
                al = makealliance(alliance, zText);
            }
            nf->allies = al;
        }
        else {
            nf->allies = NULL;
        }
        if (nf->race == NULL) {
            /* if the script didn't supply the race as a token, then it gives us a
             * race in the default locale (which means that itis a UTF8 string) */
            nf->race = findrace(race, default_locale);
            if (nf->race == NULL) {
                char buffer[32];
                size_t outbytes = sizeof(buffer) - 1;
                size_t inbytes = strlen(race);
                unicode_latin1_to_utf8(buffer, &outbytes, race, &inbytes);
                buffer[outbytes] = 0;
                nf->race = findrace(buffer, default_locale);
                if (nf->race == NULL) {
                    log_error("new faction has unknown race '%s'.\n", race);
                    free(nf);
                    continue;
                }
            }
        }
        nf->lang = get_locale(lang);
        nf->bonus = bonus;
        assert(nf->race && nf->email && nf->lang);
        nfi = &newfactions;
        while (*nfi) {
            if ((*nfi)->race == nf->race)
                break;
            nfi = &(*nfi)->next;
        }
        nf->next = *nfi;
        *nfi = nf;
    }
    fclose(F);
    return newfactions;
}

extern int numnewbies;

static const terrain_type *preferred_terrain(const struct race *rc)
{
    terrain_t t = T_PLAIN;
    if (rc == rc_find("dwarf"))
        t = T_MOUNTAIN;
    if (rc == rc_find("insect"))
        t = T_DESERT;
    if (rc == rc_find("halfling"))
        t = T_SWAMP;
    if (rc == rc_find("troll"))
        t = T_MOUNTAIN;
    return newterrain(t);
}

#define REGIONS_PER_FACTION 2
#define PLAYERS_PER_ISLAND 20
#define MAXISLANDSIZE 50
#define MINFACTIONS 1
#define VOLCANO_CHANCE 100

static bool virgin_region(const region * r)
{
    direction_t d;
    if (r == NULL)
        return true;
    if (fval(r->terrain, FORBIDDEN_REGION))
        return false;
    if (r->units)
        return false;
    for (d = 0; d != MAXDIRECTIONS; ++d) {
        const region *rn = rconnect(r, d);
        if (rn) {
            if (rn->age > r->age + 1)
                return false;
            if (rn->units)
                return false;
            if (fval(rn->terrain, FORBIDDEN_REGION)) {
                /* because it kinda sucks to have islands that are adjacent to a firewall */
                return false;
            }
        }
    }
    return true;
}

static selist * get_island(region * root)
{
    selist * ql, *result = 0;
    int qi = 0;

    fset(root, RF_MARK);
    selist_push(&result, root);

    for (ql = result, qi = 0; ql; selist_advance(&ql, &qi, 1)) {
        int dir;
        region *r = (region *)selist_get(ql, qi);
        region * next[MAXDIRECTIONS];

        get_neighbours(r, next);

        for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
            region *rn = next[dir];
            if (rn != NULL && rn->land && !fval(rn, RF_MARK)) {
                fset(rn, RF_MARK);
                selist_push(&result, rn);
            }
        }
    }

    for (ql = result, qi = 0; ql; selist_advance(&ql, &qi, 1)) {
        region *r = (region *)selist_get(ql, qi);
        freset(r, RF_MARK);
    }
    return result;
}

static void
get_island_info(region * root, int *size_p, int *inhabited_p, int *maxage_p)
{
    int qi, size = 0, maxage = 0, inhabited = 0;
    selist *ql, *island = NULL;

    selist_push(&island, root);
    fset(root, RF_MARK);

    for (ql = island, qi = 0; ql; selist_advance(&ql, &qi, 1)) {
        int d;
        region *r = (region *)selist_get(ql, qi);
        if (r->units) {
            unit *u;
            for (u = r->units; u; u = u->next) {
                if (!fval(u->faction, FFL_NOIDLEOUT) && u->faction->age > maxage) {
                    maxage = u->faction->age;
                }
            }
            ++inhabited;
        }
        ++size;
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            region *rn = rconnect(r, d);
            if (rn && !fval(rn, RF_MARK) && rn->land) {
                selist_push(&island, rn);
                fset(rn, RF_MARK);
            }
        }
    }
    for (ql = island, qi = 0; ql; selist_advance(&ql, &qi, 1)) {
        region *r = (region *)selist_get(ql, qi);
        freset(r, RF_MARK);
    }
    selist_free(island);
    if (size_p)
        *size_p = size;
    if (inhabited_p)
        *inhabited_p = inhabited;
    if (maxage_p)
        *maxage_p = maxage;
}

void free_newfaction(newfaction * nf)
{
    free(nf->email);
    free(nf->password);
    free(nf);
}

static void frame_regions(int age, const terrain_type * terrain)
{
    plane *hplane = get_homeplane();
    region *r = regions;
    for (r = regions; r; r = r->next) {
        plane *pl = rplane(r);
        direction_t d;
        if (r->age < age)
            continue;
        if (pl != hplane)
            continue;                 /* only do this on the main world */
        if (r->terrain == terrain)
            continue;

        for (d = 0; d != MAXDIRECTIONS; ++d) {
            region *rn = rconnect(r, d);
            if (rn == NULL) {
                int x = r->x + delta_x[d];
                int y = r->y + delta_y[d];
                pnormalize(&x, &y, pl);
                rn = new_region(x, y, pl, 0);
                terraform_region(rn, terrain);
                rn->age = r->age;
            }
        }
    }
}

static void prepare_starting_region(region * r)
{
    int n, t;
    double p;

    assert(r->land);

    /* population between 30% and 60% of max */
    p = rng_double();
    n = (int)(r->terrain->size * (0.3 + p * 0.3));
    rsetpeasants(r, n);

    /* trees: don't squash the peasants, and at least 5% should be forrest */
    t = (rtrees(r, 2) + rtrees(r, 1) / 2) * TREESIZE;
    if (t < r->terrain->size / 20 || t + n > r->terrain->size) {
        double p2 = 0.05 + rng_double() * (1.0 - p - 0.05);
        int maxtrees = (int)(r->terrain->size / 1.25 / TREESIZE);   /* 1.25 = each young tree will take 1/2 the space of old trees */
        int trees = (int)(p2 * maxtrees);

        rsettrees(r, 2, trees);
        rsettrees(r, 1, trees / 2);
        rsettrees(r, 0, trees / 4);
    }

    /* horses: between 1% and 2% */
    p = rng_double();
    rsethorses(r, (int)(r->terrain->size * (0.01 + p * 0.01)));

    if (!markets_module()) {
        fix_demand(r);
    }
}

/** create new island with up to nsize players
 * returns the number of players placed on the new island.
 */
int autoseed(newfaction ** players, int nsize, int max_agediff)
{
    region *r = NULL;
    region_list *rlist = NULL;
    int rsize = 0, tsize = 0;
    int isize = REGIONS_PER_FACTION;      /* target size for the island */
    int psize = 0;                /* players on this island */
    const terrain_type *volcano_terrain = get_terrain("volcano");
    static int nterrains = -1;
    static const terrain_type **terrainarr = 0;
    static int *distribution;

    assert(players);
    if (nterrains < 0) {
        int n = 0;
        const terrain_type *terrain = terrains();
        for (nterrains = 0; terrain; terrain = terrain->next) {
            if (terrain->distribution) {
                ++nterrains;
            }
        }
        terrainarr = malloc(sizeof(terrain_type *) * nterrains);
        distribution = malloc(sizeof(int) * nterrains);
        for (terrain = terrains(); terrain; terrain = terrain->next) {
            if (terrain->distribution) {
                terrainarr[n] = terrain;
                distribution[n++] = terrain->distribution;
            }
        }
    }
    frame_regions(16, newterrain(T_FIREWALL));

    if (listlen(*players) < MINFACTIONS)
        return 0;

    if (max_agediff > 0) {
        region *rmin = NULL;
        plane *hplane = get_homeplane();
        /* find a spot that's adjacent to the previous island, but virgin.
         * like the last land virgin ocean region adjacent to land.
         */
        for (r = regions; r; r = r->next) {
            struct plane *pl = rplane(r);
            if (r->age <= max_agediff && r->terrain == newterrain(T_OCEAN)
                && pl == hplane && virgin_region(r)) {
                direction_t d;
                for (d = 0; d != MAXDIRECTIONS; ++d) {
                    region *rn = rconnect(r, d);
                    if (rn && rn->land && rn->age <= max_agediff && virgin_region(rn)) {
                        /* only expand islands that aren't single-islands and not too big already */
                        int size, inhabitants, maxage;
                        get_island_info(rn, &size, &inhabitants, &maxage);
                        if (maxage <= max_agediff && size >= 2 && size < MAXISLANDSIZE) {
                            rmin = rn;
                            break;
                        }
                    }
                }
            }
        }
        if (rmin != NULL) {
            faction *f;
            selist *ql, *rlist = get_island(rmin);
            int qi;

            for (ql = rlist, qi = 0; ql; selist_advance(&ql, &qi, 1)) {
                region *r = (region *)selist_get(ql, qi);
                unit *u;
                for (u = r->units; u; u = u->next) {
                    f = u->faction;
                    if (!fval(f, FFL_MARK)) {
                        ++psize;
                        fset(f, FFL_MARK);
                    }
                }
            }
            selist_free(rlist);
            if (psize > 0) {
                for (f = factions; f; f = f->next) {
                    freset(f, FFL_MARK);
                }
            }
            if (psize < PLAYERS_PER_ISLAND) {
                r = rmin;
            }
        }
    }
    if (r == NULL) {
        region *rmin = NULL;
        direction_t dmin = MAXDIRECTIONS;
        plane *hplane = get_homeplane();
        /* find an empty spot.
         * rmin = the youngest ocean region that has a missing neighbour
         * dmin = direction in which it's empty
         */
        for (r = regions; r; r = r->next) {
            struct plane *pl = rplane(r);
            if (r->terrain == newterrain(T_OCEAN) && pl == hplane && (rmin == NULL
                || r->age <= max_agediff)) {
                direction_t d;
                for (d = 0; d != MAXDIRECTIONS; ++d) {
                    region *rn = rconnect(r, d);
                    if (rn == NULL)
                        break;
                }
                if (d != MAXDIRECTIONS) {
                    rmin = r;
                    dmin = d;
                }
            }
        }

        /* create a new region where we found the empty spot, and make it the first
         * in our island. island regions are kept in rlist, so only new regions can
         * get populated, and old regions are not overwritten */
        if (rmin != NULL) {
            plane *pl = rplane(rmin);
            int x = rmin->x + delta_x[dmin];
            int y = rmin->y + delta_y[dmin];
            pnormalize(&x, &y, pl);
            assert(virgin_region(rconnect(rmin, dmin)));
            r = new_region(x, y, pl, 0);
            terraform_region(r, newterrain(T_OCEAN));
        }
    }
    if (r != NULL) {
        add_regionlist(&rlist, r);
        fset(r, RF_MARK);
        rsize = 1;
    }

    while (rsize && (nsize || isize >= REGIONS_PER_FACTION)) {
        int i = rng_int() % rsize;
        region_list **rnext = &rlist;
        region_list *rfind;
        direction_t d;
        while (i--)
            rnext = &(*rnext)->next;
        rfind = *rnext;
        r = rfind->data;
        freset(r, RF_MARK);
        *rnext = rfind->next;
        free(rfind);
        --rsize;
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            region *rn = rconnect(r, d);
            if (rn && fval(rn, RF_MARK))
                continue;
            if (rn == NULL) {
                plane *pl = rplane(r);
                int x = r->x + delta_x[d];
                int y = r->y + delta_y[d];
                pnormalize(&x, &y, pl);
                rn = new_region(x, y, pl, 0);
                terraform_region(rn, newterrain(T_OCEAN));
            }
            if (virgin_region(rn)) {
                add_regionlist(&rlist, rn);
                fset(rn, RF_MARK);
                ++rsize;
            }
        }
        if (volcano_terrain != NULL && (rng_int() % VOLCANO_CHANCE == 0)) {
            terraform_region(r, volcano_terrain);
        }
        else if (nsize && (rng_int() % isize == 0 || rsize == 0)) {
            newfaction **nfp, *nextf = *players;
            faction *f;
            unit *u;

            isize += REGIONS_PER_FACTION;
            terraform_region(r, preferred_terrain(nextf->race));
            prepare_starting_region(r);
            ++tsize;
            assert(r->land && r->units == 0);
            u = addplayer(r, addfaction(nextf->email, nextf->password, nextf->race,
                nextf->lang, nextf->subscription));
            f = u->faction;
            fset(f, FFL_ISNEW);
            f->alliance = nextf->allies;

            /* remove duplicate email addresses */
            nfp = &nextf->next;
            while (*nfp) {
                newfaction *nf = *nfp;
                if (strcmp(nextf->email, nf->email) == 0) {
                    *nfp = nf->next;
                    free_newfaction(nf);
                }
                else
                    nfp = &nf->next;
            }
            *players = nextf->next;
            free_newfaction(nextf);

            ++psize;
            --nsize;
            --isize;
            if (psize >= PLAYERS_PER_ISLAND)
                break;
        }
        else {
            terraform_region(r, random_terrain(terrainarr, distribution, nterrains));
            --isize;
        }
    }

    if (nsize != 0) {
        log_error(
            ("Could not place all factions on the same island as requested\n"));
    }

    if (rlist) {
#define MINOCEANDIST 3
#define MAXOCEANDIST 6
#define MAXFILLDIST 10
#define SPECIALCHANCE 80
        region_list **rbegin = &rlist;
        int special = 1;
        int oceandist = MINOCEANDIST + (rng_int() % (MAXOCEANDIST - MINOCEANDIST));
        while (oceandist--) {
            region_list **rend = rbegin;
            while (*rend)
                rend = &(*rend)->next;
            while (rbegin != rend) {
                direction_t d;
                region *r = (*rbegin)->data;
                rbegin = &(*rbegin)->next;
                for (d = 0; d != MAXDIRECTIONS; ++d) {
                    region *rn = rconnect(r, d);
                    if (rn == NULL) {
                        const struct terrain_type *terrain = newterrain(T_OCEAN);
                        plane *pl = rplane(r);
                        int x = r->x + delta_x[d];
                        int y = r->y + delta_y[d];
                        pnormalize(&x, &y, pl);
                        rn = new_region(x, y, pl, 0);
                        if (rng_int() % SPECIALCHANCE < special) {
                            terrain = random_terrain(terrainarr, distribution, nterrains);
                            special = SPECIALCHANCE / 3;      /* 33% chance auf noch eines */
                        }
                        else {
                            special = 1;
                        }
                        terraform_region(rn, terrain);
                        /* the new region has an extra 20% chance to have mallorn */
                        if (rng_int() % 100 < 20)
                            fset(r, RF_MALLORN);
                        add_regionlist(rend, rn);
                    }
                }
            }

        }
        while (*rbegin) {
            region *r = (*rbegin)->data;
            plane *pl = rplane(r);
            direction_t d;
            rbegin = &(*rbegin)->next;
            for (d = 0; d != MAXDIRECTIONS; ++d)
                if (rconnect(r, d) == NULL) {
                    int i;
                    for (i = 1; i != MAXFILLDIST; ++i) {
                        int x = r->x + delta_x[d] * i;
                        int y = r->y + delta_y[d] * i;
                        pnormalize(&x, &y, pl);
                        if (findregion(x, y)) {
                            break;
                        }
                    }
                    if (i != MAXFILLDIST) {
                        while (--i) {
                            region *rn;
                            int x = r->x + delta_x[d] * i;
                            int y = r->y + delta_y[d] * i;
                            pnormalize(&x, &y, pl);
                            rn = new_region(x, y, pl, 0);
                            terraform_region(rn, newterrain(T_OCEAN));
                        }
                    }
                }
        }
        while (rlist) {
            region_list *self = rlist;
            rlist = rlist->next;
            freset(self->data, RF_MARK);
            free(self);
        }
    }
    return tsize;
}

region_list *regionqueue_push(region_list ** rlist, region * r)
{
    region_list *rnew = malloc(sizeof(region_list));
    rnew->data = r;
    rnew->next = 0;
    while (*rlist) {
        rlist = &(*rlist)->next;
    }
    *rlist = rnew;
    return rnew;
}

region *regionqueue_pop(region_list ** rlist)
{
    if (*rlist) {
        region *r = (*rlist)->data;
        region_list *rpop = *rlist;
        *rlist = rpop->next;
        free(rpop);
        return r;
    }
    return 0;
}

#define GEOMAX 8
static struct geo {
    int distribution;
    terrain_t type;
} geography_e3[GEOMAX] = {
    {
        8, T_OCEAN }, {
            3, T_SWAMP }, {
                1, T_VOLCANO }, {
                    3, T_DESERT }, {
                        4, T_HIGHLAND }, {
                            3, T_MOUNTAIN }, {
                                2, T_GLACIER }, {
                                    1, T_PLAIN }
};

const terrain_type *random_terrain_e3(direction_t dir)
{
    static const terrain_type **terrainarr = 0;
    static int *distribution = 0;

    if (!distribution) {
        int n = 0;

        terrainarr = malloc(GEOMAX * sizeof(const terrain_type *));
        distribution = malloc(GEOMAX * sizeof(int));
        for (n = 0; n != GEOMAX; ++n) {
            terrainarr[n] = newterrain(geography_e3[n].type);
            distribution[n] = geography_e3[n].distribution;
        }
    }
    return random_terrain(terrainarr, distribution, GEOMAX);
}

int
random_neighbours(region * r, region_list ** rlist,
const terrain_type * (*terraformer) (direction_t))
{
    int nsize = 0;
    direction_t dir;
    for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
        region *rn = rconnect(r, dir);
        if (rn == NULL) {
            const terrain_type *terrain = terraformer(dir);
            plane *pl = rplane(r);
            int x = r->x + delta_x[dir];
            int y = r->y + delta_y[dir];
            pnormalize(&x, &y, pl);
            rn = new_region(x, y, pl, 0);
            terraform_region(rn, terrain);
            regionqueue_push(rlist, rn);
            if (rn->land) {
                ++nsize;
            }
        }
    }
    return nsize;
}

const terrain_type *get_ocean(direction_t dir)
{
    return newterrain(T_OCEAN);
}

int region_quality(const region * r, region * rn[])
{
    int n, result = 0;

    for (n = 0; n != MAXDIRECTIONS; ++n) {
        if (rn[n] && rn[n]->land) {
            if (rn[n]->terrain == newterrain(T_VOLCANO)) {
                /* nobody likes volcanoes */
                result -= 2000;
            }
            result += rpeasants(rn[n]);
        }
    }
    return result;
}

static void oceans_around(region * r, region * rn[])
{
    int n;
    for (n = 0; n != MAXDIRECTIONS; ++n) {
        region *rx = rn[n];
        if (rx == NULL) {
            plane *pl = rplane(r);
            int x = r->x + delta_x[n];
            int y = r->y + delta_y[n];
            pnormalize(&x, &y, pl);
            rx = new_region(x, y, pl, 0);
            terraform_region(rx, newterrain(T_OCEAN));
            rn[n] = rx;
        }
    }
}

#define SWAP_INTS(a, b) { a^=b; b^=a; a^=b; }

static void smooth_island(region_list * island)
{
    region *rn[MAXDIRECTIONS];
    region_list *rlist = NULL;
    for (rlist = island; rlist; rlist = rlist->next) {
        region *r = rlist->data;
        int n, nland = 0;

        if (r->land) {
            get_neighbours(r, rn);
            for (n = 0; n != MAXDIRECTIONS && nland <= 1; ++n) {
                if (rn[n]->land) {
                    ++nland;
                    r = rn[n];
                }
            }

            if (nland == 1) {
                get_neighbours(r, rn);
                oceans_around(r, rn);
                for (n = 0; n != MAXDIRECTIONS; ++n) {
                    int n1 = (n + 1) % MAXDIRECTIONS;
                    int n2 = (n + 1 + MAXDIRECTIONS) % MAXDIRECTIONS;
                    if (!rn[n]->land && rn[n1] != r && rn[n2] != r) {
                        r = rlist->data;
                        runhash(r);
                        runhash(rn[n]);
                        SWAP_INTS(r->x, rn[n]->x);
                        SWAP_INTS(r->y, rn[n]->y);
                        rhash(r);
                        rhash(rn[n]);
                        rlist->data = r;
                        oceans_around(r, rn);
                        break;
                    }
                }
            }
        }
    }
}

static void starting_region(newfaction ** players, region * r, region * rn[])
{
    int n;
    oceans_around(r, rn);
    freset(r, RF_MARK);
    for (n = 0; n != MAXDIRECTIONS; ++n) {
        freset(rn[n], RF_MARK);
    }
    terraform_region(r, newterrain(T_PLAIN));
    prepare_starting_region(r);
    if (players && *players) {
        newfaction *nf = *players;
        const struct race *rc = nf->race ? nf->race : races;
        const struct locale *lang = nf->lang ? nf->lang : default_locale;
        const char * passwd = nf->password ? nf->password : itoa36(rng_int());
        addplayer(r, addfaction(nf->email, passwd, rc, lang, 0));
        *players = nf->next;
        free_newfaction(nf);
    }
}

/* E3A island generation */
int build_island_e3(newfaction ** players, int x, int y, int numfactions, int minsize)
{
#define MIN_QUALITY 1000
    int nfactions = 0;
    region_list *rlist = NULL;
    region_list *island = NULL;
    plane *pl = findplane(x, y);
    region *r = findregion(x, y);
    int nsize = 1;
    int q, maxq = INT_MIN, minq = INT_MAX;

    if (!r)
        r = new_region(x, y, pl, 0);
    assert(!r->units);
    do {
        terraform_region(r, random_terrain_e3(NODIRECTION));
    } while (!r->land);

    while (r) {
        fset(r, RF_MARK);
        if (r->land) {
            if (nsize < minsize) {
                nsize += random_neighbours(r, &rlist, &random_terrain_e3);
            }
            else {
                nsize += random_neighbours(r, &rlist, &get_ocean);
            }
        }
        regionqueue_push(&island, r);
        r = regionqueue_pop(&rlist);
    }

    smooth_island(island);

    if (nsize > minsize / 2) {
        for (rlist = island; rlist; rlist = rlist->next) {
            r = rlist->data;
            if (r->land && fval(r, RF_MARK)) {
                region *rn[MAXDIRECTIONS];

                get_neighbours(r, rn);
                q = region_quality(r, rn);
                if (q >= MIN_QUALITY && nfactions < numfactions && *players) {
                    starting_region(players, r, rn);
                    minq = MIN(minq, q);
                    maxq = MAX(maxq, q);
                    ++nfactions;
                }
            }
        }

        for (rlist = island; rlist && nfactions < numfactions; rlist = rlist->next) {
            r = rlist->data;
            if (!r->land && fval(r, RF_MARK)) {
                region *rn[MAXDIRECTIONS];
                get_neighbours(r, rn);
                q = region_quality(r, rn);
                if (q >= MIN_QUALITY * 4 / 3 && nfactions < numfactions && *players) {
                    starting_region(players, r, rn);
                    minq = MIN(minq, q);
                    maxq = MAX(maxq, q);
                    ++nfactions;
                }
            }
        }
    }

    for (rlist = island; rlist; rlist = rlist->next) {
        r = rlist->data;
        if (r->units) {
            region *rn[MAXDIRECTIONS];
            get_neighbours(r, rn);
            q = region_quality(r, rn);
            if (q - minq > (maxq - minq) * 2 / 3) {
                terraform_region(r, newterrain(T_HIGHLAND));
                prepare_starting_region(r);
            }
            rsetmoney(r, 50000);   /* 2% = 1000 silver */
        }
        else if (r->land) {
            rsetmoney(r, rmoney(r) *4);
        }
    }
    return nfactions;
}
