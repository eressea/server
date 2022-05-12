#include "teleport.h"

/* kernel includes */
#include "kernel/config.h"
#include "kernel/equipment.h"
#include "kernel/unit.h"
#include "kernel/region.h"
#include "kernel/race.h"
#include "kernel/skill.h"
#include "kernel/terrain.h"
#include "kernel/faction.h"
#include "kernel/plane.h"

/* util includes */
#include "util/log.h"
#include "util/stats.h"
#include "util/rng.h"

#include "monsters.h"

/* libc includes */
#include <assert.h>

#define TE_CENTER 1000

int real2tp(int rk)
{
    /* in C:
     * -4 / 5 = 0;
     * +4 / 5 = 0;
     * !!!!!!!!!!;
     */
    return TE_CENTER + (rk + (TP_DISTANCE * 5000)) / TP_DISTANCE - 5000;
}

static region *tpregion(const region * r)
{
    region *rt =
        findregion(real2tp(r->x), real2tp(r->y));
    if (!is_astral(rt))
        return NULL;
    return rt;
}

int regions_in_range(const region * r, int radius, bool(*valid) (const region *), region *result[])
{
    int x, y, num = 0;
    const struct plane *pl = rplane(r);
    for (x = -radius; x <= +radius; ++x) {
        for (y = -radius; y <= radius; ++y) {
            int dist = koor_distance(0, 0, x, y);

            if (dist <= radius) {
                region *rn;
                int nx = r->x + x, ny = r->y + y;
                pnormalize(&nx, &ny, pl);
                rn = findregion(nx, ny);
                if (rn != NULL && (valid == NULL || valid(rn))) {
                    if (result) {
                        result[num] = rn;
                    }
                    ++num;
                }
            }
        }
    }
    return num;
}

int get_astralregions(const region * r, bool(*valid) (const region *), region *result[])
{
    assert(is_astral(r));
    r = r_astral_to_standard(r);
    if (r) {
        return regions_in_range(r, TP_RADIUS, valid, result);
    }
    return 0;
}

region *r_standard_to_astral(const region * r)
{
    assert(!is_astral(r));
    return tpregion(r);
}

region *r_astral_to_standard(const region * r)
{
    int x, y;
    region *r2;

    assert(is_astral(r));
    x = (r->x - TE_CENTER) * TP_DISTANCE;
    y = (r->y - TE_CENTER) * TP_DISTANCE;
    pnormalize(&x, &y, NULL);
    r2 = findregion(x, y);
    if (r2 == NULL || rplane(r2))
        return NULL;

    return r2;
}

#define MAX_BRAIN_SIZE 100

void spawn_braineaters(float chance)
{
    const race * rc_brain = get_race(RC_HIRNTOETER);
    region *r;
    faction *f = get_monsters();
    int next = rng_int() % (int)(chance * 100);

    if (f == NULL || rc_brain == NULL) {
        return;
    }

    for (r = regions; r; r = r->next) {
        unit *u, *ub = NULL;
        if (!is_astral(r) || fval(r->terrain, FORBIDDEN_REGION))
            continue;

        for (u = r->units; u; u = u->next) {
            if (u->_race == rc_brain) {
                if (!ub) {
                    ub = u;
                }
                else {
                    int n = u->number + ub->number;
                    if (n <= MAX_BRAIN_SIZE) {
                        scale_number(ub, n);
                        u->number = 0;
                    }
                    else {
                        ub = u;
                    }
                }
            }
        }

        /* Neues Monster ? */
        if (next-- == 0) {
            u = create_unit(r, f, 1 + rng_int() % 10 + rng_int() % 10,
                rc_brain, 0, NULL, NULL);
            equip_unit(u, "seed_braineater");
            stats_count("monsters.create.braineater", 1);

            next = rng_int() % (int)(chance * 100);
        }
    }
}

bool is_astral(const region * r)
{
    plane *pl = get_astralplane();
    return (pl && rplane(r) == pl);
}

plane *get_astralplane(void)
{
    plane *astralspace = NULL;
    static int config;
    static bool rule_astralplane;
    
    if (config_changed(&config)) {
        rule_astralplane = config_get_int("modules.astralspace", 1) != 0;
    }
    if (!rule_astralplane) {
        return NULL;
    }
    astralspace = getplanebyname("Astralraum");
    if (!astralspace) {
        astralspace = create_new_plane(1, "Astralraum",
            TE_CENTER - 500, TE_CENTER + 500,
            TE_CENTER - 500, TE_CENTER + 500, 0);
    }
    return astralspace;
}

void create_teleport_plane(void)
{
    region *r;
    const struct plane *hplane = get_homeplane();
    struct plane* aplane = get_astralplane();

    const terrain_type *fog = get_terrain("fog");
    const terrain_type *tfog = get_terrain("thickfog");

    for (r = regions; r; r = r->next) {
        if (rplane(r) == hplane) {
            update_teleport_plane(r, aplane, fog, tfog);
        }
    }
}

void update_teleport_plane(const region* r, plane* aplane, const terrain_type* terrain, const struct terrain_type* blocked)
{
    static region* rlast = NULL;
    region* ra = tpregion(r);
    const terrain_type* fog = terrain ? terrain : get_terrain("fog");
    const terrain_type* tfog = blocked ? blocked : get_terrain("thickfog");
    const terrain_type* ter = (r->terrain->flags & FORBIDDEN_REGION) ? tfog : fog;

    if (ra == NULL) {
        int x = real2tp(r->x);
        int y = real2tp(r->y);
        pnormalize(&x, &y, aplane);

        ra = new_region(x, y, aplane, 0);
        terraform_region(ra, ter);
    }
    else if (ra->terrain == fog && ter == tfog) {
        /* try fixing this region, but only once: */
        if (ra != rlast) {
            rlast = ra;
            if (ra->units) {
                unit* u;
                for (u = ra->units; u; u = u->next) {
                    if (!is_monsters(u->faction)) {
                        log_warning("astral space above %s should be impassable, but there are units of %s here.", regionname(r, NULL), factionname(u->faction));
                        break;
                    }
                }
                if (!u) {
                    while (ra->units) {
                        erase_unit(&ra->units, ra->units);
                    }
                }
            }
        }
        terraform_region(ra, ter);
    }
}

bool inhabitable(const region * r)
{
    return fval(r->terrain, LAND_REGION);
}
