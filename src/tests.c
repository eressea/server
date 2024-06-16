#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "tests.h"

#include "creport.h"
#include "eressea.h"
#include "magic.h"             // for spell_component, create_castorder, ...
#include "report.h"
#include "reports.h"

#include "kernel/alliance.h"
#include "kernel/build.h"
#include "kernel/building.h"
#include "kernel/config.h"
#include "kernel/calendar.h"
#include "kernel/callbacks.h"
#include "kernel/direction.h" 
#include "kernel/faction.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/plane.h"
#include "kernel/region.h"
#include "kernel/skill.h"      // for enable_skill, skillnames, MAXSKILLS
#include "kernel/status.h"     // for ST_FLEE
#include "kernel/terrain.h"
#include "kernel/types.h"      // for MAXMAGIETYP
#include "kernel/unit.h"
#include "kernel/race.h"
#include "kernel/ship.h"
#include "kernel/skills.h"
#include "kernel/spell.h"

#include "util/keyword.h"
#include "util/language.h"
#include "util/log.h"
#include "util/message.h"
#include "util/param.h"
#include "util/rand.h"
#include "util/stats.h"
#include "util/variant.h"      // for variant, VAR_VOIDPTR, VAR_INT

#include <strings.h>

#include <stb_ds.h>
#include <CuTest.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>           // for true
#include <stdio.h>             // for fprintf, stderr
#include <stdlib.h>
#include <string.h>

struct skill *test_set_skill(unit *u, enum skill_t sk, int level, int weeks)
{
    skill *sv;
    set_level(u, sk, level);
    sv = unit_skill(u, sk);
    if (weeks > 0) {
        sv->weeks = weeks;
    }
    return sv;
}

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
    rc->recruitcost = 100;
    rc->recruit_multi = 1;
    rc->flags |= (RCF_WALK|RCF_PLAYABLE);
    rc->ec_flags |= ECF_GETITEM|ECF_GIVEUNIT;
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
        locale_setstring(loc, "factiondefault", param_name(P_FACTION, NULL));
        locale_setstring(loc, "unitdefault", param_name(P_UNIT, NULL));
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
            const char* p = param_name(i, NULL);
            locale_setstring(loc, p, p);
            test_translate_param(loc, i, p);
        }
        for (i = 0; i != MAXMAGIETYP; ++i) {
            locale_setstring(loc, mkname("school", magic_school[i]), magic_school[i]);
        }
        init_locale(loc);
    }
    return loc;
}

struct faction *test_create_faction_ex(const struct race *rc, const struct locale *loc)
{
    faction* f;
    if (loc == NULL) {
        loc = test_create_locale();
    }
    f = addfaction("nobody@eressea.de", NULL, rc ? rc : test_create_race("human"), loc);
    test_clear_messages(f);
    return f;
}

struct faction* test_create_faction(void) {
    return test_create_faction_ex(NULL, NULL);
}

struct unit *test_create_unit(struct faction *f, struct region *r)
{
    const struct race * rc = f ? f->race : NULL;
    if (!rc) rc = rc_get_or_create("human");
    if (!f) f = test_create_faction_ex(rc, NULL);
    if (!r) r = test_create_plain(0, 0);
    return create_unit(r, f, 1, rc ? rc : rc_get_or_create("human"), 0, 0, 0);
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

static season_t test_months[4] = { SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN };

void test_create_calendar(void) {
    months_per_year = 4;
    weeks_per_month = 2;
    month_season = test_months;
}

/*
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
*/

void test_reset(void)
{
    if (month_season == test_months) {
        month_season = NULL;
    }
    free_gamedata();
}

static void test_reset_full(void) {
    int i;
    turn = 1;
    default_locale = NULL;

    if (errno) {
        int error = errno;
        errno = 0;
        log_error("errno: %d (%s)", error, strerror(error));
    }
    memset(&callbacks, 0, sizeof(callbacks));

    test_reset();
    free_configuration();
    calendar_cleanup();
    creport_cleanup();
    report_cleanup();
    close_orders();
    log_close();
    stats_close();
    free_locales();
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

void test_use_astral(void)
{
    config_set_int("modules.astralspace", 1);
    test_create_terrain("fog", LAND_REGION);
    test_create_terrain("thickfog", FORBIDDEN_REGION);
}

void test_setup_test(CuTest *tc, const char *file, int line) {
    test_log_stderr(LOG_CPERROR);
    test_reset_full();
    config_set("hunger.damage", "0");
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
    test_reset_full();
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
        btype = test_create_buildingtype("test");
    }
    b = new_building(btype, r, default_locale, (btype->maxsize > 0) ? btype->maxsize : 1);
    return b;
}

ship * test_create_ship(region * r, const ship_type * stype)
{
    ship * s = new_ship(stype ? stype : test_create_shiptype("test"), r, default_locale);
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
        construction *con = stype->construction = calloc(1, sizeof(construction));
        assert(stype->construction);
        stype->construction->maxsize = 5;
        stype->construction->minskill = 1;
        stype->construction->reqsize = 1;
        stype->construction->skill = SK_SHIPBUILDING;
        con->materials = malloc(2 * sizeof(requirement));
        if (con->materials) {
            con->materials[0].number = 1;
            con->materials[0].rtype = rt_get_or_create("log");
            con->materials[1].number = 0;
        }
    }

    arrsetlen(stype->coasts, 1);
    if (!stype->coasts) abort();
    stype->coasts[0] = test_create_terrain("plain",
        LAND_REGION | FOREST_REGION | WALK_INTO | CAVALRY_REGION | FLY_INTO);
    if (default_locale) {
        const char* str = locale_getstring(default_locale, name);
        if (!str || strcmp(name, str) != 0) {
            locale_setstring(default_locale, name, name);
        }
    }
    return stype;
}

building_type * test_create_buildingtype(const char * name)
{
    construction *con = NULL;
    building_type *btype = bt_get_or_create(name);
    if (btype->a_stages) {
        con = &btype->a_stages[0].construction;
    } else {
        building_stage *stage;
        arrsetlen(btype->a_stages, 1);
        stage = btype->a_stages;
        if (stage) {
            stage->name = NULL;
            con = &stage->construction;
            con->materials = NULL;
            construction_init(con, 1, SK_BUILDING, 1, -1);
        }
    }
    if (con && !con->materials) {
        con->materials = malloc(2 * sizeof(requirement));
        if (con->materials) {
            con->materials[0].number = 1;
            con->materials[0].rtype = get_resourcetype(R_STONE);
            con->materials[1].number = 0;
        }
    }
    if (default_locale) {
        if (locale_getstring(default_locale, name) == NULL) {
            locale_setstring(default_locale, name, name);
        }
    }
    return btype;
}

static void init_stage(building_stage * stage, int minskill, int maxsize,
    const char *name, const resource_type *rtype)
{
    stage->name = str_strdup(name);
    construction_init(&stage->construction, minskill, SK_BUILDING, 1, maxsize);
    stage->construction.materials = malloc(2 * sizeof(requirement));
    if (stage->construction.materials) {
        stage->construction.materials[0].number = 1;
        stage->construction.materials[0].rtype = rtype;
        stage->construction.materials[1].number = 0;
    }
}

building_type *test_create_castle(void) {
    building_type *btype = bt_get_or_create("castle");
    const resource_type *rtype = get_resourcetype(R_STONE);
    if (!rtype) {
        rtype = test_create_itemtype("stone")->rtype;
    }

    if (!btype->a_stages) {
        building_stage *stages = arrsetlen(btype->a_stages, 7);
        btype->flags |= BTF_FORTIFICATION;
        init_stage(stages + 0, 1, 2, "site", rtype);
        init_stage(stages + 1, 1, 8, "tradepost", rtype);
        init_stage(stages + 2, 2, 40, "fortification", rtype);
        init_stage(stages + 3, 3, 200, "tower", rtype);
        init_stage(stages + 4, 4, 1000, "castle", rtype);
        init_stage(stages + 5, 5, 5000, "fortress", rtype);
        init_stage(stages + 6, 6, -1, "citadel", rtype);
    }
    return btype;
}


item_type * test_create_itemtype(const char * name) {
    resource_type * rtype;
    item_type * itype;

    rtype = rt_get_or_create(name);
    rtype->flags = RTF_POOLED|RTF_ITEM;
    itype = it_get_or_create(rtype);

    return itype;
}

void test_create_castorder(castorder *co, unit *u, int level, float force, int range, spellparameter *par) {
    create_castorder(co, u, NULL, NULL, u->region, level, force, range, NULL, par);
}

spell * test_create_spell(void)
{
    spell *sp;
    sp = create_spell("testspell");

    sp->components = (spell_component *)calloc(4, sizeof(spell_component));
    assert(sp->components);
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

void test_translate_param(const struct locale *lang, enum param_t param, const char *text) {
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

item_type *test_create_cart(void) {
    item_type * itype;
    itype = test_create_itemtype("cart");
    itype->flags |= ITF_BIG | ITF_VEHICLE;
    itype->weight = 4000;
    itype->capacity = 14000;
    return itype;
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
    return get_messagetype_name(msg);
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

struct message *test_find_messagetype(struct message_list *msgs, const char *name)
{
    return test_find_messagetype_ex(msgs, name, NULL);
}

struct message *test_find_faction_message(const faction *f, const char *name)
{
    return test_find_messagetype_ex(f->msgs, name, NULL);
}

struct message *test_find_region_message(const region *r, const char *name, const faction *f)
{
    if (f) {
        const struct individual_message *imsg;
        for (imsg = r->individual_messages; imsg; imsg = imsg->next) {
            if (imsg->viewer == f) {
                return test_find_messagetype(imsg->msgs, name);
            }
        }
        return NULL;
    }
    return test_find_messagetype(r->msgs, name);
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
        test_clear_messagelist(&f->msgs);
    }
}

void test_clear_region_messages(struct region *r)
{
    if (r->msgs) {
        test_clear_messagelist(&r->msgs);
    }
    if (r->individual_messages) {
        struct individual_message *imsg = r->individual_messages;
        while (imsg)
        {
            struct individual_message *inext = imsg->next;
            test_clear_messagelist(&imsg->msgs);
            free(imsg);
            imsg = inext;
        }
        r->individual_messages = NULL;
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
