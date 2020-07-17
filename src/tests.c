#include <platform.h>
#include "tests.h"
#include "prefix.h"
#include "creport.h"
#include "report.h"
#include "reports.h"
#include "vortex.h"

#include "kernel/calendar.h"
#include "kernel/callbacks.h"
#include "kernel/config.h"
#include "kernel/alliance.h"
#include <kernel/equipment.h>
#include <kernel/messages.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/item.h>
#include <kernel/unit.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/faction.h>
#include <kernel/building.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/terrain.h>

#include <util/functions.h>
#include "util/keyword.h"
#include <util/language.h>
#include <util/lists.h>
#include <util/message.h>
#include <util/log.h>
#include "util/param.h"
#include <util/rand.h>
#include <util/assert.h>

#include <CuTest.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

int test_set_item(unit * u, const item_type *itype, int value)
{
    item *i;

    assert(itype);
    i = *i_find(&u->items, itype);
    if (!i) {
        i = i_add(&u->items, i_new(itype, value));
    }
    else {
        i->number = value;
        assert(i->number >= 0);
    }
    return value;
}

struct race *test_create_race(const char *name)
{
    race *rc = rc_get_or_create(name ? name : "smurf");
    rc->maintenance = 10;
    rc->hitpoints = 20;
    rc->maxaura = 100;
    rc->flags |= (RCF_WALK|RCF_PLAYABLE);
    rc->ec_flags |= ECF_GETITEM;
    rc->battle_flags = BF_EQUIPMENT;
    return rc;
}

struct region *test_create_region(int x, int y, const terrain_type *terrain)
{
    region *r = findregion(x, y);
    if (!r) {
        r = new_region(x, y, findplane(x, y), 0);
    }
    if (!terrain) {
        terrain_type *t = test_create_terrain("plain", LAND_REGION | CAVALRY_REGION | FOREST_REGION | FLY_INTO | WALK_INTO);
        terraform_region(r, t);
    }
    else {
        terraform_region(r, terrain);
    }
    r->flags &= ~RF_MALLORN;
    rsettrees(r, 0, 0);
    rsettrees(r, 1, 0);
    rsettrees(r, 2, 0);
    rsethorses(r, 0);
    rsetpeasants(r, r->terrain->size);
    return r;
}

region *test_create_ocean(int x, int y)
{
    terrain_type *ter = test_create_terrain("ocean", SEA_REGION | FLY_INTO | SWIM_INTO);
    return test_create_region(x, y, ter);
}

region *test_create_plain(int x, int y) {
    return test_create_region(x, y, NULL);
}

struct locale * test_create_locale(void) {
    struct locale *loc = get_locale("test");
    if (!loc) {
        int i;
        loc = get_or_create_locale("test");
        locale_setstring(loc, "factiondefault", parameters[P_FACTION]);
        locale_setstring(loc, "unitdefault", parameters[P_UNIT]);
        locale_setstring(loc, "money", "Silber");
        locale_setstring(loc, "money_p", "Silber");
        locale_setstring(loc, "cart", "Wagen");
        locale_setstring(loc, "cart_p", "Wagen");
        locale_setstring(loc, "horse", "Pferd");
        locale_setstring(loc, "horse_p", "Pferde");
        locale_setstring(loc, "iron", "Eisen");
        locale_setstring(loc, "iron_p", "Eisen");
        locale_setstring(loc, "stone", "Stein");
        locale_setstring(loc, "stone_p", "Steine");
        locale_setstring(loc, "plain", "Ebene");
        locale_setstring(loc, "ocean", "Ozean");
        for (i = 0; i < MAXRACES; ++i) {
            if (racenames[i]) {
                char name[64];
                rc_key(racenames[i], NAME_PLURAL, name, sizeof(name));
                if (!locale_getstring(loc, name)) {
                    locale_setstring(loc, name, name + 6);
                }
                rc_key(racenames[i], NAME_SINGULAR, name, sizeof(name));
                if (!locale_getstring(loc, name)) {
                    locale_setstring(loc, name, name + 6);
                }
            }
        }
        for (i = 0; i < MAXSKILLS; ++i) {
            if (!locale_getstring(loc, mkname("skill", skillnames[i])))
                locale_setstring(loc, mkname("skill", skillnames[i]), skillnames[i]);
        }
        for (i = 0; i != ALLIANCE_MAX; ++i) {
            locale_setstring(loc, alliance_kwd[i], alliance_kwd[i]);
        }
        for (i = 0; i != MAXDIRECTIONS; ++i) {
            locale_setstring(loc, shortdirections[i], shortdirections[i] + 4);
            locale_setstring(loc, directions[i], directions[i]);
            init_direction(loc, i, directions[i]);
            init_direction(loc, i, coasts[i] + 7);
        }
        for (i = 0; i <= ST_FLEE; ++i) {
            locale_setstring(loc, combatstatus[i], combatstatus[i] + 7);
        }
        for (i = 0; i != MAXKEYWORDS; ++i) {
            if (keywords[i]) {
                locale_setstring(loc, mkname("keyword", keywords[i]), keywords[i]);
            }
        }
        for (i = 0; i != MAXPARAMS; ++i) {
            locale_setstring(loc, parameters[i], parameters[i]);
            test_translate_param(loc, i, parameters[i]);
        }
        for (i = 0; i != MAXMAGIETYP; ++i) {
            locale_setstring(loc, mkname("school", magic_school[i]), magic_school[i]);
        }
        init_locale(loc);
    }
    return loc;
}

struct faction *test_create_faction(const struct race *rc)
{
    struct locale * loc = test_create_locale();
    faction *f = addfaction("nobody@eressea.de", NULL, rc ? rc : test_create_race("human"), loc);
    test_clear_messages(f);
    return f;
}

struct unit *test_create_unit(struct faction *f, struct region *r)
{
    const struct race * rc = f ? f->race : 0;
    assert(f && r);
    if (!rc) rc = rc_get_or_create("human");
    return create_unit(r, f, 1, rc ? rc : rc_get_or_create("human"), 0, 0, 0);
}

static void log_list(void *udata, int flags, const char *module, const char *format, va_list args) {
    strlist **slp = (strlist **)udata;
    addstrlist(slp, format);
}

struct log_t * test_log_start(int flags, strlist **slist) {
    return log_create(flags, slist, log_list);
}

void test_log_stop(struct log_t *log, struct strlist *slist) {
    freestrlist(slist);
    log_destroy(log);
}

void test_log_stderr(int flags) {
    static struct log_t *stderrlog;
    if (flags) {
        if (stderrlog) {
            log_error("stderr logging is still active. did you call test_teardown?");
            log_destroy(stderrlog);
        }
        stderrlog = log_to_file(flags, stderr);
    }
    else {
        if (stderrlog) {
            log_destroy(stderrlog);
        }
        else {
            log_warning("stderr logging is inactive. did you call test_teardown twice?");
        }
        stderrlog = 0;
    }

}

static void test_reset(void) {
    int i;
    turn = 1;
    default_locale = 0;

    if (errno) {
        int error = errno;
        errno = 0;
        log_error("errno: %d (%s)", error, strerror(error));
    }
    memset(&callbacks, 0, sizeof(callbacks));
    free_gamedata();
    free_terrains();
    free_resources();
    free_functions();
    free_config();
    default_locale = 0;
    calendar_cleanup();
    creport_cleanup();
    report_cleanup();
    close_orders();
    log_close();
    stats_close();
    free_special_directions();
    free_locales();
    free_spells();
    free_buildingtypes();
    free_shiptypes();
    free_races();
    free_spellbooks();
    free_prefixes();
    mt_clear();

    for (i = 0; i != MAXSKILLS; ++i) {
        enable_skill(i, true);
    }
    for (i = 0; i != MAXKEYWORDS; ++i) {
        enable_keyword(i, true);
    }
    random_source_reset();

    mt_create_va(mt_new("changepasswd", NULL),
        "value:string", MT_NEW_END);
    mt_create_va(mt_new("starvation", NULL),
        "unit:unit", "region:region", "dead:int", "live:int", MT_NEW_END);
    mt_create_va(mt_new("malnourish", NULL),
        "unit:unit", "region:region", MT_NEW_END);

    if (errno) {
        int error = errno;
        errno = 0;
        log_error("errno: %d (%s)", error, strerror(error));
    }
}

void test_create_calendar(void) {
    config_set_int("game.start", 184);
    months_per_year = 9;
    month_season = malloc(sizeof(season_t) * months_per_year);
    if (!month_season) abort();
    month_season[0] = SEASON_SUMMER;
    month_season[1] = SEASON_AUTUMN;
    month_season[2] = SEASON_AUTUMN;
    month_season[3] = SEASON_WINTER;
    month_season[4] = SEASON_WINTER;
    month_season[5] = SEASON_WINTER;
    month_season[6] = SEASON_SPRING;
    month_season[7] = SEASON_SPRING;
    month_season[8] = SEASON_SUMMER;
}

void test_setup_test(CuTest *tc, const char *file, int line) {
    test_log_stderr(LOG_CPERROR);
    test_reset();
    message_handle_missing(MESSAGE_MISSING_REPLACE);
    if (tc) {
        log_debug("start test: %s", tc->name);
    }
    else {
        log_debug("start test in %s:%d", file, line);
    }
    errno = 0;
}

void test_teardown(void)
{
    message_handle_missing(MESSAGE_MISSING_IGNORE);
    test_reset();
    test_log_stderr(0);
}

terrain_type *
test_create_terrain(const char * name, int flags)
{
    terrain_type * t = get_or_create_terrain(name);

    if (flags < 0) {
        /* sensible defaults for most terrains */
        flags = LAND_REGION | WALK_INTO | FLY_INTO;
    }
    if (flags & LAND_REGION) {
        t->size = 1000;
    }
    t->flags = flags;
    return t;
}

building * test_create_building(region * r, const building_type * btype)
{
    building * b;
    assert(r);
    if (!btype) {
        building_type *bt_castle = test_create_buildingtype("castle");
        bt_castle->flags |= BTF_FORTIFICATION;
        btype = bt_castle;
    }
    b = new_building(btype, r, default_locale, (btype->maxsize > 0) ? btype->maxsize : 1);
    return b;
}

ship * test_create_ship(region * r, const ship_type * stype)
{
    ship * s = new_ship(stype ? stype : test_create_shiptype("boat"), r, default_locale);
    s->size = s->type->construction ? s->type->construction->maxsize : 1;
    return s;
}

ship_type * test_create_shiptype(const char * name)
{
    ship_type * stype = st_get_or_create(name);
    stype->cptskill = 1;
    stype->sumskill = 1;
    stype->minskill = 1;
    stype->range = 2;
    stype->cargo = 1000;
    stype->damage = 1;
    if (!stype->construction) {
        stype->construction = calloc(1, sizeof(construction));
        assert_alloc(stype->construction);
        stype->construction->maxsize = 5;
        stype->construction->minskill = 1;
        stype->construction->reqsize = 1;
        stype->construction->skill = SK_SHIPBUILDING;
    }

    if (stype->coasts) {
        free(stype->coasts);
    }
    stype->coasts =
        (terrain_type **)malloc(sizeof(terrain_type *) * 3);
    stype->coasts[0] = test_create_terrain("plain", LAND_REGION | FOREST_REGION | WALK_INTO | CAVALRY_REGION | FLY_INTO);
    stype->coasts[1] = test_create_terrain("ocean", SEA_REGION | SWIM_INTO | FLY_INTO);
    stype->coasts[2] = NULL;
    if (default_locale) {
        locale_setstring(default_locale, name, name);
    }
    return stype;
}

building_type * test_create_buildingtype(const char * name)
{
    construction *con;
    building_type *btype = bt_get_or_create(name);
    if (btype->stages) {
        con = btype->stages->construction;
    } else {
        btype->stages = calloc(1, sizeof(building_stage));
        con = (construction *)calloc(1, sizeof(construction));
        if (con) {
            con->skill = SK_BUILDING;
            con->maxsize = -1;
            con->minskill = 1;
            con->reqsize = 1;
            btype->stages->construction = con;
        }
    }
    if (con && !con->materials) {
        con->materials = (requirement *)calloc(2, sizeof(requirement));
        con->materials[1].number = 0;
        con->materials[0].number = 1;
        con->materials[0].rtype = get_resourcetype(R_STONE);
    }
    if (default_locale) {
        if (locale_getstring(default_locale, name) == NULL) {
            locale_setstring(default_locale, name, name);
        }
    }
    return btype;
}

item_type * test_create_itemtype(const char * name) {
    resource_type * rtype;
    item_type * itype;

    rtype = rt_get_or_create(name);
    itype = it_get_or_create(rtype);

    return itype;
}

void test_create_castorder(castorder *co, unit *u, int level, float force, int range, spellparameter *par) {
    struct locale * lang;
    order *ord;

    lang = test_create_locale();
    create_castorder(co, u, NULL, NULL, u->region, level, force, range, ord = create_order(K_CAST, lang, ""), par);
    free_order(ord);
}

spell * test_create_spell(void)
{
    spell *sp;
    sp = create_spell("testspell");

    sp->components = (spell_component *)calloc(4, sizeof(spell_component));
    assert_alloc(sp->components);
    sp->components[0].amount = 1;
    sp->components[0].type = get_resourcetype(R_SILVER);
    sp->components[0].cost = SPC_FIX;
    sp->components[1].amount = 1;
    sp->components[1].type = get_resourcetype(R_AURA);
    sp->components[1].cost = SPC_LEVEL;
    sp->components[2].amount = 1;
    sp->components[2].type = get_resourcetype(R_HORSE);
    sp->components[2].cost = SPC_LINEAR;
    sp->syntax = 0;
    sp->parameter = 0;
    return sp;
}

void test_translate_param(const struct locale *lang, param_t param, const char *text) {
    struct critbit_tree **cb;

    assert(lang && text);
    cb = (struct critbit_tree **)get_translations(lang, UT_PARAMS);
    add_translation(cb, text, param);
}

item_type *test_create_silver(void) {
    item_type * itype;
    itype = test_create_itemtype("money");
    itype->weight = 1;
    return itype;
}

item_type *test_create_horse(void) {
    item_type * itype;
    itype = test_create_itemtype("horse");
    itype->flags |= ITF_BIG | ITF_ANIMAL;
    itype->weight = 5000;
    itype->capacity = 7000;
    return itype;
}

/** creates a small world and some stuff in it.
 * two terrains: 'plain' and 'ocean'
 * one race: 'human'
 * one ship_type: 'boat'
 * one building_type: 'castle'
 * in 0.0 and 1.0 is an island of two plains, around it is ocean.
 */
void test_create_world(void)
{
    terrain_type *t_plain, *t_ocean;
    region *island[2];
    int i;
    item_type * itype;
    struct locale * loc;

    loc = test_create_locale();

    locale_setstring(loc, parameters[P_SHIP], "SCHIFF");
    locale_setstring(loc, parameters[P_ANY], "ALLE");
    init_parameters(loc);

    locale_setstring(loc, "status_aggressive", "aggressiv");
    locale_setstring(loc, keyword(K_RESERVE), "RESERVIEREN");
    locale_setstring(loc, "money", "Silber");
    locale_setstring(loc, "money_p", "Silber");
    locale_setstring(loc, "cart", "Wagen");
    locale_setstring(loc, "cart_p", "Wagen");
    locale_setstring(loc, "horse", "Pferd");
    locale_setstring(loc, "horse_p", "Pferde");
    locale_setstring(loc, "iron", "Eisen");
    locale_setstring(loc, "iron_p", "Eisen");
    locale_setstring(loc, "stone", "Stein");
    locale_setstring(loc, "stone_p", "Steine");
    locale_setstring(loc, "plain", "Ebene");
    locale_setstring(loc, "ocean", "Ozean");
    init_resources();
    get_resourcetype(R_SILVER)->itype->weight = 1;

    test_create_horse();

    itype = test_create_itemtype("cart");
    itype->flags |= ITF_BIG | ITF_VEHICLE;
    itype->weight = 4000;
    itype->capacity = 14000;

    test_create_itemtype("iron");
    test_create_itemtype("stone");

    t_plain = test_create_terrain("plain", LAND_REGION | FOREST_REGION | WALK_INTO | CAVALRY_REGION | FLY_INTO);
    t_plain->size = 1000;
    t_plain->max_road = 100;
    t_ocean = test_create_terrain("ocean", SEA_REGION | SWIM_INTO | FLY_INTO);
    t_ocean->size = 0;

    island[0] = test_create_region(0, 0, t_plain);
    island[1] = test_create_region(1, 0, t_plain);
    for (i = 0; i != 2; ++i) {
        int j;
        region *r = island[i];
        for (j = 0; j != MAXDIRECTIONS; ++j) {
            region *rn = r_connect(r, (direction_t)j);
            if (!rn) {
                rn = test_create_region(r->x + delta_x[j], r->y + delta_y[j], t_ocean);
            }
        }
    }

    test_create_race("human");

    test_create_buildingtype("castle");
    test_create_shiptype("boat");
}

message * test_get_last_message(message_list *msgs) {
    if (msgs) {
        struct mlist *iter = msgs->begin;
        while (iter->next) {
            iter = iter->next;
        }
        return iter->msg;
    }
    return 0;
}

const char * test_get_messagetype(const message *msg) {
    const char * name;
    assert(msg);
    name = msg->type->name;
    if (strcmp(name, "missing_message") == 0) {
        name = (const char *)msg->parameters[0].v;
    }
    else if (strcmp(name, "missing_feedback") == 0) {
        name = (const char *)msg->parameters[3].v;
    }
    return name;
}

struct message * test_find_messagetype_ex(struct message_list *msgs, const char *name, struct message *prev)
{
    struct mlist *ml;
    if (!msgs) return 0;
    for (ml = msgs->begin; ml; ml = ml->next) {
        if (prev && ml->msg == prev) {
            prev = NULL;
        }
        else if (strcmp(name, test_get_messagetype(ml->msg)) == 0) {
            return ml->msg;
        }
    }
    return NULL;
}

struct message * test_find_messagetype(struct message_list *msgs, const char *name)
{
    return test_find_messagetype_ex(msgs, name, NULL);
}

int test_count_messagetype(struct message_list *msgs, const char *name)
{
    int count = 0;
    struct mlist *ml;
    if (!msgs) return 0;
    for (ml = msgs->begin; ml; ml = ml->next) {
        if (strcmp(name, test_get_messagetype(ml->msg)) == 0) {
            ++count;
        }
    }
    return count;
}

void test_clear_messagelist(message_list **msgs) {
    if (*msgs) {
        free_messagelist((*msgs)->begin);
        free(*msgs);
        *msgs = NULL;
    }
}

void test_clear_messages(faction *f) {
    if (f->msgs) {
        free_messagelist(f->msgs->begin);
        free(f->msgs);
        f->msgs = NULL;
    }
}

void assert_message(CuTest * tc, message *msg, char *name, int numpar) {
    const message_type *mtype = msg->type;
    assert(mtype);

    CuAssertStrEquals(tc, name, mtype->name);
    CuAssertIntEquals(tc, numpar, mtype->nparameters);
}

void assert_pointer_parameter(CuTest * tc, message *msg, int index, void *arg) {
    const message_type *mtype = (msg)->type;
    CuAssertIntEquals((tc), VAR_VOIDPTR, mtype->types[(index)]->vtype); CuAssertPtrEquals((tc), (arg), msg->parameters[(index)].v);
}

void assert_int_parameter(CuTest * tc, message *msg, int index, int arg) {
    const message_type *mtype = (msg)->type;
    CuAssertIntEquals((tc), VAR_INT, mtype->types[(index)]->vtype); CuAssertIntEquals((tc), (arg), msg->parameters[(index)].i);
}

void assert_string_parameter(CuTest * tc, message *msg, int index, const char *arg) {
    const message_type *mtype = (msg)->type;
    CuAssertIntEquals((tc), VAR_VOIDPTR, mtype->types[(index)]->vtype); CuAssertStrEquals((tc), (arg), msg->parameters[(index)].v);
}

void disabled_test(void *suite, void(*test)(CuTest *), const char *name) {
    (void)test;
    fprintf(stderr, "%s: SKIP\n", name);
}
