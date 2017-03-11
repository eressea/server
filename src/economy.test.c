#include <platform.h>
#include <kernel/config.h>
#include "economy.h"

#include <util/message.h>
#include <kernel/building.h>
#include <kernel/item.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/attrib.h>
#include <util/language.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>

static void test_give_control_building(CuTest * tc)
{
    unit *u1, *u2;
    building *b;
    struct faction *f;
    region *r;

    test_cleanup();
    f = test_create_faction(0);
    r = test_create_region(0, 0, 0);
    b = test_create_building(r, 0);
    u1 = test_create_unit(f, r);
    u_set_building(u1, b);
    u2 = test_create_unit(f, r);
    u_set_building(u2, b);
    CuAssertPtrEquals(tc, u1, building_owner(b));
    give_control(u1, u2);
    CuAssertPtrEquals(tc, u2, building_owner(b));
    test_cleanup();
}

static void test_give_control_ship(CuTest * tc)
{
    unit *u1, *u2;
    ship *sh;
    struct faction *f;
    region *r;

    test_cleanup();
    f = test_create_faction(0);
    r = test_create_region(0, 0, 0);
    sh = test_create_ship(r, 0);
    u1 = test_create_unit(f, r);
    u_set_ship(u1, sh);
    u2 = test_create_unit(f, r);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u1, ship_owner(sh));
    give_control(u1, u2);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    test_cleanup();
}

struct steal {
    struct unit *u;
    struct region *r;
    struct faction *f;
};

static void setup_steal(struct steal *env, struct terrain_type *ter, struct race *rc) {
    env->r = test_create_region(0, 0, ter);
    env->f = test_create_faction(rc);
    env->u = test_create_unit(env->f, env->r);
}

static void test_steal_okay(CuTest * tc) {
    struct steal env;
    race *rc;
    struct terrain_type *ter;

    test_cleanup();
    ter = test_create_terrain("plain", LAND_REGION);
    rc = test_create_race("human");
    rc->flags = 0;
    setup_steal(&env, ter, rc);
    CuAssertPtrEquals(tc, 0, check_steal(env.u, 0));
    test_cleanup();
}

static void test_steal_nosteal(CuTest * tc) {
    struct steal env;
    race *rc;
    terrain_type *ter;
    message *msg;

    test_cleanup();
    ter = test_create_terrain("plain", LAND_REGION);
    rc = test_create_race("human");
    rc->flags = RCF_NOSTEAL;
    setup_steal(&env, ter, rc);
    CuAssertPtrNotNull(tc, msg = check_steal(env.u, 0));
    msg_release(msg);
    test_cleanup();
}

static void test_steal_ocean(CuTest * tc) {
    struct steal env;
    race *rc;
    terrain_type *ter;
    message *msg;

    test_cleanup();
    ter = test_create_terrain("ocean", SEA_REGION);
    rc = test_create_race("human");
    setup_steal(&env, ter, rc);
    CuAssertPtrNotNull(tc, msg = check_steal(env.u, 0));
    msg_release(msg);
    test_cleanup();
}

static struct unit *create_recruiter(void) {
    region *r;
    faction *f;
    unit *u;
    const resource_type* rtype;

    r=test_create_region(0, 0, NULL);
    rsetpeasants(r, 999);
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    rtype = get_resourcetype(R_SILVER);
    change_resource(u, rtype, 1000);
    return u;
}

static void test_heroes_dont_recruit(CuTest * tc) {
    unit *u;

    test_setup();
    init_resources();
    u = create_recruiter();

    fset(u, UFL_HERO);
    unit_addorder(u, create_order(K_RECRUIT, default_locale, "1"));

    economics(u->region);

    CuAssertIntEquals(tc, 1, u->number);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error_herorecruit"));

    test_cleanup();
}

static void test_normals_recruit(CuTest * tc) {
    unit *u;

    test_setup();
    init_resources();
    u = create_recruiter();
    unit_addorder(u, create_order(K_RECRUIT, default_locale, "1"));

    economics(u->region);

    CuAssertIntEquals(tc, 2, u->number);

    test_cleanup();
}

typedef struct request {
    struct request *next;
    struct unit *unit;
    struct order *ord;
    int qty;
    int no;
    union {
        bool goblin;             /* stealing */
        const struct luxury_type *ltype;    /* trading */
    } type;
} request;

static void test_tax_cmd(CuTest *tc) {
    order *ord;
    faction *f;
    region *r;
    unit *u;
    item_type *sword, *silver;
    request *taxorders = 0;


    test_setup();
    init_resources();
    config_set("taxing.perlevel", "20");
    f = test_create_faction(NULL);
    r = test_create_region(0, 0, NULL);
    assert(r && f);
    u = test_create_unit(f, r);

    ord = create_order(K_TAX, f->locale, "");
    assert(ord);

    tax_cmd(u, ord, &taxorders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error48"));
    test_clear_messages(u->faction);

    silver = get_resourcetype(R_SILVER)->itype;

    sword = test_create_itemtype("sword");
    new_weapontype(sword, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE, 1);
    i_change(&u->items, sword, 1);
    set_level(u, SK_MELEE, 1);

    tax_cmd(u, ord, &taxorders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error_no_tax_skill"));
    test_clear_messages(u->faction);

    set_level(u, SK_TAXING, 1);
    tax_cmd(u, ord, &taxorders);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(u->faction->msgs, "error_no_tax_skill"));
    CuAssertPtrNotNull(tc, taxorders);

    rsetmoney(r, 11);
    expandtax(r, taxorders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "income"));
    /* taxing is in multiples of 10 */
    CuAssertIntEquals(tc, 10, i_get(u->items, silver));
    test_clear_messages(u->faction);
    i_change(&u->items, silver, -i_get(u->items, silver));

    rsetmoney(r, 1000);
    taxorders = 0;
    tax_cmd(u, ord, &taxorders);
    expandtax(r, taxorders);
    CuAssertIntEquals(tc, 20, i_get(u->items, silver));
    test_clear_messages(u->faction);

    free_order(ord);
    test_cleanup();
}

/** 
 * see https://bugs.eressea.de/view.php?id=2234
 */
static void test_maintain_buildings(CuTest *tc) {
    region *r;
    building *b;
    building_type *btype;
    unit *u;
    faction *f;
    maintenance *req;
    item_type *itype;

    test_cleanup();
    btype = test_create_buildingtype("Hort");
    btype->maxsize = 10;
    r = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    u = test_create_unit(f, r);
    b = test_create_building(r, btype);
    itype = test_create_itemtype("money");
    b->size = btype->maxsize;
    u_set_building(u, b);

    /* this building has no upkeep, it just works: */
    b->flags = 0;
    maintain_buildings(r);
    CuAssertIntEquals(tc, BLD_MAINTAINED, fval(b, BLD_MAINTAINED));
    CuAssertPtrEquals(tc, 0, f->msgs);
    CuAssertPtrEquals(tc, 0, r->msgs);

    req = calloc(2, sizeof(maintenance));
    req[0].number = 100;
    req[0].rtype = itype->rtype;
    btype->maintenance = req;

    /* we cannot afford to pay: */
    b->flags = 0;
    maintain_buildings(r);
    CuAssertIntEquals(tc, 0, fval(b, BLD_MAINTAINED));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "maintenancefail"));
    CuAssertPtrNotNull(tc, test_find_messagetype(r->msgs, "maintenance_nowork"));
    test_clear_messagelist(&f->msgs);
    test_clear_messagelist(&r->msgs);
    
    /* we can afford to pay: */
    i_change(&u->items, itype, 100);
    b->flags = 0;
    maintain_buildings(r);
    CuAssertIntEquals(tc, BLD_MAINTAINED, fval(b, BLD_MAINTAINED));
    CuAssertIntEquals(tc, 0, i_get(u->items, itype));
    CuAssertPtrEquals(tc, 0, r->msgs);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "maintenance_nowork"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "maintenance"));
    test_clear_messagelist(&f->msgs);

    /* this building has no owner, it doesn't work: */
    u_set_building(u, NULL);
    b->flags = 0;
    maintain_buildings(r);
    CuAssertIntEquals(tc, 0, fval(b, BLD_MAINTAINED));
    CuAssertPtrEquals(tc, 0, f->msgs);
    CuAssertPtrNotNull(tc, test_find_messagetype(r->msgs, "maintenance_noowner"));
    test_clear_messagelist(&r->msgs);

    test_cleanup();
}

static void test_recruit(CuTest *tc) {
    unit *u;
    faction *f;

    test_setup();
    f = test_create_faction(0);
    u = test_create_unit(f, test_create_region(0, 0, 0));
    CuAssertIntEquals(tc, 1, u->number);
    add_recruits(u, 1, 1);
    CuAssertIntEquals(tc, 2, u->number);
    CuAssertPtrEquals(tc, u, f->units);
    CuAssertPtrEquals(tc, NULL, u->nextF);
    CuAssertPtrEquals(tc, NULL, u->prevF);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "recruit"));
    add_recruits(u, 1, 2);
    CuAssertIntEquals(tc, 3, u->number);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "recruit"));
    test_cleanup();
}

static void test_income(CuTest *tc)
{
    race *rc;
    unit *u;
    test_setup();
    rc = test_create_race("nerd");
    u = test_create_unit(test_create_faction(rc), test_create_region(0, 0, 0));
    CuAssertIntEquals(tc, 20, income(u));
    u->number = 5;
    CuAssertIntEquals(tc, 100, income(u));
    test_cleanup();
}

static void test_make_item(CuTest *tc) {
    unit *u;
    struct item_type *itype;
    const struct resource_type *rt_silver;
    resource_type *rtype;
    resource_limit *rdata;
    double d = 0.6;

    test_setup();
    init_resources();

    /* make items from other items (turn silver to stone) */
    rt_silver = get_resourcetype(R_SILVER);
    itype = test_create_itemtype("stone");
    rtype = itype->rtype;
    u = test_create_unit(test_create_faction(0), test_create_region(0,0,0));
    make_item(u, itype, 1);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error_cannotmake"));
    CuAssertIntEquals(tc, 0, get_item(u, itype));
    test_clear_messages(u->faction);
    itype->construction = calloc(1, sizeof(construction));
    itype->construction->skill = SK_ALCHEMY;
    itype->construction->minskill = 1;
    itype->construction->maxsize = 1;
    itype->construction->reqsize = 1;
    itype->construction->materials = calloc(2, sizeof(requirement));
    itype->construction->materials[0].rtype = rt_silver;
    itype->construction->materials[0].number = 1;
    set_level(u, SK_ALCHEMY, 1);
    set_item(u, rt_silver->itype, 1);
    make_item(u, itype, 1);
    CuAssertIntEquals(tc, 1, get_item(u, itype));
    CuAssertIntEquals(tc, 0, get_item(u, rt_silver->itype));

    /* make level-based raw materials, no materials used in construction */
    free(itype->construction->materials);
    itype->construction->materials = 0;
    rtype->flags |= RTF_LIMITED;
    rmt_create(rtype);
    rdata = rtype->limit = calloc(1, sizeof(resource_limit));
    add_resource(u->region, 1, 300, 150, rtype);
    u->region->resources->amount = 300; /* there are 300 stones at level 1 */
    set_level(u, SK_ALCHEMY, 10);

    make_item(u, itype, 10);
    split_allocations(u->region);
    CuAssertIntEquals(tc, 11, get_item(u, itype));
    CuAssertIntEquals(tc, 290, u->region->resources->amount); /* used 10 stones to make 10 stones */

    rdata->modifiers = calloc(2, sizeof(resource_mod));
    rdata->modifiers[0].flags = RMF_SAVEMATERIAL;
    rdata->modifiers[0].race = u->_race;
    rdata->modifiers[0].value.sa[0] = (short)(0.5+100*d);
    rdata->modifiers[0].value.sa[1] = 100;
    make_item(u, itype, 10);
    split_allocations(u->region);
    CuAssertIntEquals(tc, 21, get_item(u, itype));
    CuAssertIntEquals(tc, 284, u->region->resources->amount); /* 60% saving = 6 stones make 10 stones */

    make_item(u, itype, 1);
    split_allocations(u->region);
    CuAssertIntEquals(tc, 22, get_item(u, itype));
    CuAssertIntEquals(tc, 283, u->region->resources->amount); /* no free lunches */

    rdata->modifiers[0].value = frac_make(1, 2);
    make_item(u, itype, 6);
    split_allocations(u->region);
    CuAssertIntEquals(tc, 28, get_item(u, itype));
    CuAssertIntEquals(tc, 280, u->region->resources->amount); /* 50% saving = 3 stones make 6 stones */

    rdata->modifiers[0].flags = RMF_REQUIREDBUILDING;
    rdata->modifiers[0].race = NULL;
    rdata->modifiers[0].btype = bt_get_or_create("mine");
    make_item(u, itype, 10);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error104"));

    test_cleanup();
}

CuSuite *get_economy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_give_control_building);
    SUITE_ADD_TEST(suite, test_give_control_ship);
    SUITE_ADD_TEST(suite, test_income);
    SUITE_ADD_TEST(suite, test_make_item);
    SUITE_ADD_TEST(suite, test_steal_okay);
    SUITE_ADD_TEST(suite, test_steal_ocean);
    SUITE_ADD_TEST(suite, test_steal_nosteal);
    SUITE_ADD_TEST(suite, test_normals_recruit);
    SUITE_ADD_TEST(suite, test_heroes_dont_recruit);
    SUITE_ADD_TEST(suite, test_tax_cmd);
    SUITE_ADD_TEST(suite, test_maintain_buildings);
    SUITE_ADD_TEST(suite, test_recruit);
    return suite;
}
