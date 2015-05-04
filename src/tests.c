#include <platform.h>
#include "tests.h"
#include "keyword.h"

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/item.h>
#include <kernel/unit.h>
#include <kernel/race.h>
#include <kernel/faction.h>
#include <kernel/building.h>
#include <kernel/order.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/terrain.h>
#include <kernel/messages.h>
#include <util/bsdstring.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/message.h>
#include <util/log.h>

#include <CuTest.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct race *test_create_race(const char *name)
{
    race *rc = rc_get_or_create(name);
    rc->maintenance = 10;
    rc->ec_flags |= GETITEM;
    return rc;
}

struct region *test_create_region(int x, int y, const terrain_type *terrain)
{
    region *r = new_region(x, y, NULL, 0);
    if (!terrain) {
        terrain_type *t = get_or_create_terrain("plain");
        t->size = 1000;
        fset(t, LAND_REGION);
        terraform_region(r, t);
    } else
        terraform_region(r, terrain);
    rsettrees(r, 0, 0);
    rsettrees(r, 1, 0);
    rsettrees(r, 2, 0);
    rsethorses(r, 0);
    rsetpeasants(r, r->terrain->size);
    return r;
}

struct faction *test_create_faction(const struct race *rc)
{
    faction *f = addfaction("nobody@eressea.de", NULL, rc ? rc : rc_get_or_create("human"), default_locale, 0);
    return f;
}

struct unit *test_create_unit(struct faction *f, struct region *r)
{
    const struct race * rc = f ? f->race : 0;
    assert(f || !r);
    return create_unit(r, f, 1, rc ? rc : rc_get_or_create("human"), 0, 0, 0);
}

void test_cleanup(void)
{
    free_terrains();
    free_resources();
    global.functions.maintenance = NULL;
    global.functions.wage = NULL;
    default_locale = 0;
    free_locales();
    free_spells();
    free_buildingtypes();
    free_shiptypes();
    free_races();
    free_spellbooks();
    free_gamedata();
}

terrain_type *
test_create_terrain(const char * name, unsigned int flags)
{
    terrain_type * t = get_or_create_terrain(name);
    t->flags = flags;
    return t;
}

building * test_create_building(region * r, const building_type * btype)
{
    building * b = new_building(btype ? btype : bt_get_or_create("castle"), r, default_locale);
    b->size = b->type->maxsize > 0 ? b->type->maxsize : 1;
    return b;
}

ship * test_create_ship(region * r, const ship_type * stype)
{
    ship * s = new_ship(stype ? stype : st_get_or_create("boat"), r, default_locale);
    s->size = s->type->construction ? s->type->construction->maxsize : 1;
    return s;
}

ship_type * test_create_shiptype(const char * name)
{
    ship_type * stype = st_get_or_create(name);
    locale_setstring(default_locale, name, name);
    return stype;
}

building_type * test_create_buildingtype(const char * name)
{
    building_type *btype = (building_type *)calloc(sizeof(building_type), 1);
    btype->flags = BTF_NAMECHANGE;
    btype->_name = _strdup(name);
    btype->construction = (construction *)calloc(sizeof(construction), 1);
    btype->construction->skill = SK_BUILDING;
    btype->construction->maxsize = -1;
    btype->construction->minskill = 1;
    btype->construction->reqsize = 1;
    btype->construction->materials = (requirement *)calloc(sizeof(requirement), 2);
    btype->construction->materials[1].number = 0;
    btype->construction->materials[0].number = 1;
    btype->construction->materials[0].rtype = get_resourcetype(R_STONE);
    if (default_locale) {
        locale_setstring(default_locale, name, name);
    }
    bt_register(btype);
    return btype;
}

item_type * test_create_itemtype(const char * name) {
    resource_type * rtype;
    item_type * itype;

    rtype = rt_get_or_create(name);
    itype = it_get_or_create(rtype);

    return itype;
}

void test_translate_param(const struct locale *lang, param_t param, const char *text) {
    struct critbit_tree **cb;

    assert(lang && text);
    cb = (struct critbit_tree **)get_translations(lang, UT_PARAMS);
    add_translation(cb, text, param);
}

world_fixture *test_create_essential(void) {
    struct world_fixture *fix = calloc(1, sizeof(world_fixture));

    fix->loc = get_or_create_locale("de");
    locale_setstring(fix->loc, keyword(K_RESERVE), "RESERVIEREN");
    locale_setstring(fix->loc, "money", "SILBER");
    init_resources();

    fix->horse_type = test_create_itemtype("horse");
    fix->horse_type->flags |= ITF_BIG | ITF_ANIMAL;
    fix->horse_type->weight = 5000;
    fix->horse_type->capacity = 7000;

    fix->cart_type = test_create_itemtype("cart");
    fix->cart_type->flags |= ITF_BIG | ITF_VEHICLE;
    fix->cart_type->weight = 4000;
    fix->cart_type->capacity = 14000;

    test_create_itemtype("iron");
    test_create_itemtype("stone");

    fix->t_plain = test_create_terrain("plain", LAND_REGION | FOREST_REGION | WALK_INTO | CAVALRY_REGION | FLY_INTO);
    fix->t_plain->size = 1000;
    fix->t_plain->max_road = 100;
    fix->t_ocean = test_create_terrain("ocean", SEA_REGION | SAIL_INTO | SWIM_INTO  | FLY_INTO);
    fix->t_ocean->size = 0;


    test_create_race("human");

    test_create_buildingtype("castle");
    test_create_shiptype("boat");

    return fix;
}

/** creates a small world and some stuff in it.
 * two terrains: 'plain' and 'ocean'
 * one race: 'human'
 * one ship_type: 'boat'
 * one building_type: 'castle'
 * in 0.0 and 1.0 is an island of two plains, around it is ocean.
 */
world_fixture *test_create_world(void)
{
    struct world_fixture *fix = test_create_essential();
    int i;
    
    fix->island[0] = test_create_region(0, 0, fix->t_plain);
    fix->island[1] = test_create_region(1, 0, fix->t_plain);
    for (i = 0; i != 2; ++i) {
        int j;
        region *r = fix->island[i];
        for (j = 0; j != MAXDIRECTIONS; ++j) {
            region *rn = r_connect(r, (direction_t)j);
            if (!rn) {
                rn = test_create_region(r->x + delta_x[j], r->y + delta_y[j], fix->t_ocean);
            }
        }
    }

    return fix;
}

message * test_get_last_message(message_list *msgs) {
    struct mlist *iter = msgs->begin;
    while (iter->next) {
        iter = iter->next;
    }
    return iter->msg;
}

const char * test_get_messagetype(const message *msg) {
    const char * name = msg->type->name;
    if (strcmp(name, "missing_message") == 0) {
        name = (const char *)msg->parameters[0].v;
    }
    else if (strcmp(name, "missing_feedback") == 0) {
        name = (const char *)msg->parameters[3].v;
    }
    return name;
}

const message_type *register_msg(const char *type, int n_param, ...) {
    char **argv;
    va_list args;
    int i;

    va_start(args, n_param);

    argv = malloc(sizeof(char *) * (n_param + 1));
    for (i = 0; i < n_param; ++i) {
        argv[i] = va_arg(args, char *);
    }
    argv[n_param] = 0;
    va_end(args);
    return mt_register(mt_new(type, (const char **)argv));
}

void assert_messages(struct CuTest * tc, struct mlist *msglist, const message_type **types,
    int num_msgs, bool exact_match, ...) {
    char buf[100];
    va_list args;
    int found = 0, argc = -1;
    struct message *msg;
    bool match = true;

    va_start(args, exact_match);

    while (msglist) {
        msg = msglist->msg;
        if (found >= num_msgs) {
            if (exact_match) {
                slprintf(buf, sizeof(buf), "too many messages: %s", msg->type->name);
                CuFail(tc, buf);
            } else {
                break;
            }
        }
        if (exact_match || match)
            argc = va_arg(args, int);

        match = strcmp(types[argc]->name, msg->type->name) == 0;
        if (match)
            ++found;
        else if (exact_match)
            CuAssertStrEquals(tc, types[argc]->name, msg->type->name);

        msglist = msglist->next;
    }

    CuAssertIntEquals_Msg(tc, "not enough messages", num_msgs, found);

    va_end(args);
}


void assert_order(CuTest *tc, const char *expected, const unit *unit, bool has)
{
    char cmd[32];
    order *order;
    for (order = unit->orders; order; order = order->next) {
        if (strcmp(expected, get_command(order, cmd, sizeof(cmd))) == 0) {
            if (has)
                return;
            CuAssertStrEquals(tc, "", expected);
        }
    }
    if (has)
        CuAssertStrEquals(tc, expected, "");
}

void disabled_test(void *suite, void (*test)(CuTest *), const char *name) {
    (void)test;
    fprintf(stderr, "%s: SKIP\n", name);
}
