#include "build.h"

#include "alchemy.h"
#include "direction.h"
#include "move.h"
#include "study.h"
#include "guard.h"
#include "laws.h"
#include "skill.h"
#include "lighthouse.h"

/* kernel includes */
#include <kernel/ally.h>
#include <kernel/alliance.h>
#include <kernel/attrib.h>
#include <kernel/building.h>
#include <kernel/config.h>
#include <kernel/connection.h>
#include <kernel/curse.h>
#include <kernel/event.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

/* from libutil */
#include <util/goodies.h>
#include <util/language.h>
#include <util/log.h>
#include <util/message.h>
#include <util/param.h>
#include <util/parser.h>
#include <util/resolve.h>
#include <util/strings.h>

/* from libc */
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct building *getbuilding(const struct region *r)
{
    building *b = findbuilding(getid());
    if (b != NULL && r == b->region) {
        return b;
    }
    return NULL;
}

/* ------------------------------------------------------------- */

int recycle(unit *u, construction *con, int size) {
    /* TODO: Nicht an ZERSTOERE mit Punktangabe angepasst! */
    int c;
    for (c = 0; con->materials[c].number; ++c) {
        const requirement *rq = con->materials + c;
        int num = (rq->number * size / con->reqsize) / 2;
        if (num) {
            change_resource(u, rq->rtype, num);
        }
    }
    return size;
}

/* ------------------------------------------------------------- */

void build_road(unit * u, int size, direction_t d)
{
    region *r = u->region;
    int n, left, effsk;
    region *rn = rconnect(r, d);

    assert(u->number);
    effsk = effskill(u, SK_ROAD_BUILDING, NULL);
    if (!effsk) {
        cmistake(u, u->thisorder, 103, MSG_PRODUCE);
        return;
    }

    if (rn == NULL || rn->terrain->max_road < 0) {
        cmistake(u, u->thisorder, 94, MSG_PRODUCE);
        return;
    }

    if (r->terrain->max_road < 0) {
        cmistake(u, u->thisorder, 94, MSG_PRODUCE);
        return;
    }

    if (r->terrain == newterrain(T_SWAMP)) {
        /* wenn kein Damm existiert */
        const struct building_type *bt_dam = bt_find("dam");
        if (!bt_dam || !buildingtype_exists(r, bt_dam, true)) {
            cmistake(u, u->thisorder, 132, MSG_PRODUCE);
            return;
        }
    }
    else if (r->terrain == newterrain(T_DESERT)) {
        const struct building_type *bt_caravan = bt_find("caravan");
        /* wenn keine Karawanserei existiert */
        if (!bt_caravan || !buildingtype_exists(r, bt_caravan, true)) {
            cmistake(u, u->thisorder, 133, MSG_PRODUCE);
            return;
        }
    }
    else if (r->terrain == newterrain(T_GLACIER)) {
        const struct building_type *bt_tunnel = bt_find("tunnel");
        /* wenn kein Tunnel existiert */
        if (!bt_tunnel || !buildingtype_exists(r, bt_tunnel, true)) {
            cmistake(u, u->thisorder, 131, MSG_PRODUCE);
            return;
        }
    }

    /* left kann man noch bauen */
    left = r->terrain->max_road - rroad(r, d);

    /* hoffentlich ist r->road <= r->terrain->max_road, n also >= 0 */
    if (left <= 0) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder,
            "error_roads_finished", ""));
        return;
    }

    if (size > 0 && left > size) {
        left = size;
    }
    /* baumaximum anhand der rohstoffe */
    if (u_race(u) == get_race(RC_STONEGOLEM)) {
        n = u->number * GOLEM_STONE;
    }
    else {
        n = get_pooled(u, get_resourcetype(R_STONE), GET_DEFAULT, left);
        if (n == 0) {
            cmistake(u, u->thisorder, 151, MSG_PRODUCE);
            return;
        }
    }
    if (n < left) left = n;

    /* n = maximum by skill. try to maximize it */
    n = build_skill(u, effsk, 0, SK_ROAD_BUILDING);

    /* make minimum of possible and available: */
    if (n > left) n = left;

    /* n is now modified by several special effects, so we have to
     * minimize it again to make sure the road will not grow beyond
     * maximum. */
    rsetroad(r, d, rroad(r, d) + n);

    if (u_race(u) == get_race(RC_STONEGOLEM)) {
        int golemsused = n / GOLEM_STONE;
        if (n % GOLEM_STONE != 0) {
            ++golemsused;
        }
        scale_number(u, u->number - golemsused);
    }
    else {
        use_pooled(u, get_resourcetype(R_STONE), GET_DEFAULT, n);
        /* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */
        produceexp(u, SK_ROAD_BUILDING, (n < u->number) ? n : u->number);
    }
    ADDMSG(&u->faction->msgs, msg_message("buildroad",
        "region unit size", r, u, n));
}

/* ------------------------------------------------------------- */

/* ** ** ** ** ** ** *
 *  new build rules  *
 * ** ** ** ** ** ** */

static int required(int size, int msize, int maxneed)
/* um size von msize Punkten zu bauen,
 * braucht man required von maxneed resourcen */
{
    int used;
    assert(msize > 0);
    used = size * maxneed / msize;
    if (size * maxneed % msize)
        ++used;
    return used;
}

static int matmod(const unit * u, const resource_type * rtype, int value)
{
    if (rtype->modifiers) {
        variant save = frac_one;
        const struct building_type *btype = NULL;
        const struct race *rc = u_race(u);
        resource_mod *mod;
        if (u->building && inside_building(u)) {
            btype = u->building->type;
        }
        for (mod = rtype->modifiers; mod->type != RMT_END; ++mod) {
            if (mod->type == RMT_USE_SAVE) {
                if (!mod->btype || mod->btype == btype) {
                    if (!mod->race_mask || (mod->race_mask & rc->mask_item)) {
                        save = frac_mul(save, mod->value);
                    }
                }
            }
        }
        return value * save.sa[0] / save.sa[1];
    }
    return value;
}

int roqf_factor(void)
{
    static int config;
    static int value;
    if (config_changed(&config)) {
        value = config_get_int("rules.economy.roqf", 10);
    }
    return value;
}

static int use_materials(unit *u, const construction *type, int n, int completed) {
    if (type->materials) {
        int c;
        for (c = 0; type->materials[c].number; c++) {
            const struct resource_type *rtype = type->materials[c].rtype;
            int prebuilt =
                required(completed, type->reqsize, type->materials[c].number);
            int need =
                required(completed + n, type->reqsize, type->materials[c].number);
            int multi, canuse = 100;       /* normalization */
            canuse = matmod(u, rtype, canuse);
            assert(canuse >= 0);
            assert(canuse % 100 == 0
                || !"only constant multipliers are implemented in build()");
            multi = canuse / 100;
            if (canuse < 0) {
                return canuse;        /* pass errors to caller */
            }
            use_pooled(u, rtype, GET_DEFAULT,
                (need - prebuilt + multi - 1) / multi);
        }
    }
    return 0;
}

static int count_materials(unit *u, const construction *type, int n, int completed)
{
    if (type->materials) {
        int c;
        for (c = 0; n > 0 && type->materials[c].number; c++) {
            const struct resource_type *rtype = type->materials[c].rtype;
            int canuse = get_pooled(u, rtype, GET_DEFAULT, INT_MAX);
            canuse = matmod(u, rtype, canuse);

            assert(canuse >= 0);
            if (type->reqsize > 1) {
                int prebuilt = required(completed, type->reqsize,
                    type->materials[c].number);
                while (n > 0) {
                    int need =
                        required(completed + n, type->reqsize, type->materials[c].number);
                    if (need - prebuilt <= canuse) {
                        break;
                    }
                    --n;                /* TODO: optimieren? */
                }
            }
            else {
                int maxn = canuse / type->materials[c].number;
                if (maxn < n) {
                    n = maxn;
                }
            }
        }
    }
    return n;
}

int build_skill(unit *u, int basesk, int skill_mod, skill_t sk)
{
    int effsk, skills;
    int dm = get_effect(u, oldpotiontype[P_DOMORE]);

    effsk = basesk + skill_mod;
    assert(effsk >= 0);

    skills = effsk * u->number;

    /* technically, nimblefinge and domore should be in a global set of
     * "game"-attributes, (as at_skillmod) but for a while, we're leaving
     * them in here. */

     /* Flinkfingerring wirkt nicht auf Mengenbegrenzte (magische)
      * Talente */
    if (faction_skill_limit(u->faction, sk) == INT_MAX) {
        const resource_type* ring = get_resourcetype(R_RING_OF_NIMBLEFINGER);
        item* itm = ring ? *i_find(&u->items, ring->itype) : 0;
        int i = itm ? itm->number : 0;
        if (i > 0) {
            int rings = (u->number < i) ? u->number : i;
            skills = skills * ((roqf_factor() - 1) * rings + u->number) / u->number;
        }
    }

    if (dm != 0) {
        /* Auswirkung Schaffenstrunk */
        if (dm > u->number) dm = u->number;
        change_effect(u, oldpotiontype[P_DOMORE], -dm);
        skills += dm * effsk;
    }
    return skills;
}

/** Use up resources for building an object.
* Build up to 'size' points of 'type', where 'completed'
* of the first object have already been finished. return the
* actual size that could be built.
*/
static int build_limited(unit * u, const construction * con, int completed, int number, int want, int basesk, int *skill_total) {
    int skills = *skill_total;
    int made = 0, maxsize;

    assert(con);
    if (want <= 0) {
        return 0;
    }
    maxsize = con->maxsize * number;
    if (completed == maxsize) {
        return ECOMPLETE;
    }
    for (; want > 0 && skills > 0;) {
        int err, n;

        /*  Hier ist entweder maxsize == -1, oder completed < maxsize.
         *  Andernfalls ist das Datenfile oder sonstwas kaputt...
         *  (enno): Nein, das ist fuer Dinge, bei denen die naechste Ausbaustufe
         *  die gleiche wie die vorherige ist. z.b. Gegenstaende.
         */
        if (maxsize > 0) {
            completed = completed % (maxsize);
        }
        else {
            completed = 0;
            assert(con->reqsize >= 1);
        }

        if (basesk < con->minskill) {
            if (made == 0)
                return ELOWSKILL;
            else
                break;              /* not good enough to go on */
        }

        /* n = maximum buildable size */
        if (con->minskill > 1) {
            n = skills / con->minskill;
        }
        else {
            n = skills;
        }

        if (want < n) n = want;

        if (maxsize > 0) {
            int req = maxsize - completed;
            if (req < n) n = req;
            want = n;
        }

        n = count_materials(u, con, n, completed);
        if (n <= 0) {
            if (made == 0)
                return ENOMATERIALS;
            else
                break;
        }
        err = use_materials(u, con, n, completed);
        if (err < 0) {
            return err;
        }
        made += n;
        skills -= n * con->minskill;
        want -= n;
        completed = completed + n;
    }
    *skill_total = skills;
    return made;
}

int build(unit * u, int number, const construction * con, int completed, int want, int skill_mod)
{
    int skills = INT_MAX;         /* number of skill points remainig */
    int made, basesk = 0;

    assert(number >= 1);
    assert(con->skill != NOSKILL);
    basesk = effskill(u, con->skill, NULL);
    if (basesk == 0) {
        return ENEEDSKILL;
    }

    skills = build_skill(u, basesk, skill_mod, con->skill);
    made = build_limited(u, con, completed, number, want, basesk, &skills);
    /* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */
    if (made > 0) {
        produceexp(u, con->skill, (made < u->number) ? made : u->number);
    }
    return made;
}

message *msg_materials_required(unit * u, order * ord,
    const construction * ctype, int multi)
{
    int c;
    message *msg;
    /* something missing from the list of materials */
    resource *reslist = NULL;

    if (multi <= 0 || multi == INT_MAX)
        multi = 1;
    for (c = 0; ctype && ctype->materials[c].number; ++c) {
        resource *res = malloc(sizeof(resource));
        if (!res) abort();
        res->number = multi * ctype->materials[c].number / ctype->reqsize;
        res->type = ctype->materials[c].rtype;
        res->next = reslist;
        reslist = res;
    }
    msg = msg_feedback(u, ord, "build_required", "required", reslist);
    while (reslist) {
        resource *res = reslist->next;
        free(reslist);
        reslist = res;
    }
    return msg;
}

int maxbuild(const unit * u, const construction * cons)
/* calculate maximum size that can be built from available material */
{
    int c;
    int maximum = INT_MAX;
    for (c = 0; cons->materials[c].number; c++) {
        const resource_type *rtype = cons->materials[c].rtype;
        int have = get_pooled(u, rtype, GET_DEFAULT, INT_MAX);
        int need = required(1, cons->reqsize, cons->materials[c].number);
        if (have < need) {
            return 0;
        }
        else {
            int b = have / need;
            if (maximum > b) maximum = b;
        }
    }
    return maximum;
}

static int build_failure(unit *u, order *ord, const building_type *btype, int want, int err) {
    switch (err) {
    case ECOMPLETE:
        /* the building is already complete */
        cmistake(u, ord, 4, MSG_PRODUCE);
        break;
    case ENOMATERIALS:
        ADDMSG(&u->faction->msgs, msg_materials_required(u, ord,
            &btype->stages->construction, want));
        break;
    case ELOWSKILL:
    case ENEEDSKILL:
        /* no skill, or not enough skill points to build */
        cmistake(u, ord, 50, MSG_PRODUCE);
        break;
    }
    return err;
}

static int build_stages(unit *u, const building_type *btype, int built, int n, int basesk, int *skill_total) {
    
    const building_stage *stage;
    int made = 0;

    for (stage = btype->stages; stage; stage = stage->next) {
        const construction * con = &stage->construction;
        if (con->maxsize < 0 || con->maxsize > built) {
            int err, want = INT_MAX;
            if (n < INT_MAX) {
                /* do not build more than n total */
                want = n - made;
            }
            if (con->maxsize > 0) {
                /* do not build more than the rest of the stage */
                int todo = con->maxsize - built;
                if (todo < want) {
                    want = todo;
                }
            }
            err = build_limited(u, con, 1, built, want, basesk, skill_total);
            if (err < 0) {
                if (made == 0) {
                    /* could not make any part at all */
                    return err;
                }
                else {
                    /* could not build any part of this stage (low skill, etc). */
                    break;
                }
            }
            else {
                /* err is the amount we built of this stage */
                built += err;
                made += err;
                if (con->maxsize > 0 && built < con->maxsize) {
                    /* we did not finish the stage, can quit here */
                    break;
                }
            }
        }
        /* build the next stage: */
        if (built >= con->maxsize && con->maxsize > 0) {
            built -= con->maxsize;
        }
    }
    return made;
}

int
build_building(unit * u, const building_type * btype, int id, int want, order * ord)
{
    region *r = u->region;
    int n = want, built = 0;
    building *b = NULL;
    /* einmalige Korrektur */
    const char *btname;
    order *new_order = NULL;
    const struct locale *lang = u->faction->locale;
    int skills, basesk;         /* number of skill points remainig */

    assert(u->number);
    assert(btype->stages);

    basesk = effskill(u, SK_BUILDING, NULL);
    skills = build_skill(u, basesk, 0, SK_BUILDING);
    if (skills == 0) {
        cmistake(u, ord, 101, MSG_PRODUCE);
        return 0;
    }

    /* Falls eine Nummer angegeben worden ist, und ein Gebaeude mit der
     * betreffenden Nummer existiert, ist b nun gueltig. Wenn keine Burg
     * gefunden wurde, dann wird nicht einfach eine neue erbaut. Ansonsten
     * baut man an der eigenen burg weiter. */

     /* Wenn die angegebene Nummer falsch ist, KEINE Burg bauen! */
    if (id > 0) {                 /* eine Nummer angegeben, keine neue Burg bauen */
        b = findbuilding(id);
        if (!b || b->region != u->region) { /* eine Burg mit dieser Nummer gibt es hier nicht */
            /* vieleicht Tippfehler und die eigene Burg ist gemeint? */
            if (u->building && u->building->type == btype) {
                b = u->building;
            }
            else {
                /* keine neue Burg anfangen wenn eine Nummer angegeben war */
                cmistake(u, ord, 6, MSG_PRODUCE);
                return 0;
            }
        }
    }
    else if (u->building && u->building->type == btype) {
        b = u->building;
    }

    if (b) {
        btype = b->type;
    }

    if (fval(btype, BTF_UNIQUE) && buildingtype_exists(r, btype, false)) {
        /* only one of these per region */
        cmistake(u, ord, 93, MSG_PRODUCE);
        return 0;
    }
    if (btype->flags & BTF_NOBUILD) {
        /* special building, cannot be built */
        cmistake(u, ord, 221, MSG_PRODUCE);
        return 0;
    }
    if ((r->terrain->flags & LAND_REGION) == 0) {
        /* special terrain, cannot build */
        cmistake(u, ord, 221, MSG_PRODUCE);
        return 0;
    }
    if (btype->flags & BTF_ONEPERTURN) {
        if (b && fval(b, BLD_EXPANDED)) {
            cmistake(u, ord, 318, MSG_PRODUCE);
            return 0;
        }
        n = 1;
    }
    if (b) {
        bool rule_other = config_get_int("rules.build.other_buildings", 1) != 0;
        if (!rule_other) {
            unit *owner = building_owner(b);
            if (!owner || owner->faction != u->faction) {
                cmistake(u, ord, 5, MSG_PRODUCE);
                return 0;
            }
        }
        built = b->size;
    }

    if (btype->maxsize > 0) {
        int remain = btype->maxsize - built;
        if (remain < n) {
            n = remain;
        }
    }
    built = build_stages(u, btype, built, n, basesk, &skills);

    if (built < 0) {
        return build_failure(u, ord, btype, want, built);
    }

    if (b) {
        b->size += built;
    } else {
        /* build a new building */
        b = new_building(btype, r, lang, built);
        b->type = btype;

        /* Die Einheit befindet sich automatisch im Inneren der neuen Burg. */
        if (u->number && leave(u, false)) {
            u_set_building(u, b);
        }
    }

    btname = LOC(lang, btype->_name);

    if (want <= built) {
        /* gebaeude fertig */
        new_order = default_order(lang);
    }
    else if (want != INT_MAX && btname) {
        /* reduzierte restgroesse */
        const char *hasspace = strchr(btname, ' ');
        if (hasspace) {
            new_order =
                create_order(K_MAKE, lang, "%d \"%s\" %i", n - built, btname, b->no);
        }
        else {
            new_order =
                create_order(K_MAKE, lang, "%d %s %i", n - built, btname, b->no);
        }
    }
    else if (btname) {
        /* Neues Haus, Befehl mit Gebaeudename */
        const char *hasspace = strchr(btname, ' ');
        if (hasspace) {
            new_order = create_order(K_MAKE, lang, "\"%s\" %i", btname, b->no);
        }
        else {
            new_order = create_order(K_MAKE, lang, "%s %i", btname, b->no);
        }
    }

    if (new_order) {
        replace_order(&u->orders, ord, new_order);
        free_order(new_order);
    }

    ADDMSG(&u->faction->msgs, msg_message("buildbuilding",
        "building unit size", b, u, built));

    if (b->type->maxsize > 0 && b->size > b->type->maxsize) {
        log_error("build: %s has size=%d, maxsize=%d", buildingname(b), b->size, b->type->maxsize);
    }
    fset(b, BLD_EXPANDED);

    return built;
}

void construction_init(struct construction *con, int minskill, skill_t sk, int reqsize, int maxsize) {
    con->minskill = minskill;
    con->skill = sk;
    con->reqsize = reqsize;
    con->maxsize = maxsize;
}

void free_construction(struct construction *cons)
{
    free(cons->materials);
    free(cons);
}
