#include <platform.h>
#include "config.h"
#include "types.h"
#include "ally.h"
#include "group.h"
#include "faction.h"
#include "unit.h"
#include "region.h"
#include "save.h"
#include "version.h"

#include <util/gamedata.h>
#include <util/attrib.h>
#include <attributes/key.h>

#include <stream.h>
#include <filestream.h>
#include <storage.h>
#include <memstream.h>
#include <binarystore.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>

static void test_group_readwrite_dead_faction(CuTest *tc) {
    gamedata data;
    storage store;
    faction *f, *f2;
    unit * u;
    group *g;
    ally *al;
    int fno;

    test_cleanup();
    f = test_create_faction(0);
    fno = f->no;
    CuAssertPtrEquals(tc, f, factions);
    CuAssertPtrEquals(tc, 0, f->next);
    f2 = test_create_faction(0);
    CuAssertPtrEquals(tc, f2, factions->next);
    u = test_create_unit(f2, test_create_region(0, 0, 0));
    CuAssertPtrNotNull(tc, u);
    g = join_group(u, "group");
    CuAssertPtrNotNull(tc, g);
    al = ally_add(&g->allies, f);
    CuAssertPtrNotNull(tc, al);

    CuAssertPtrEquals(tc, f, factions);
    destroyfaction(&factions);
    CuAssertTrue(tc, !f->_alive);
    CuAssertPtrEquals(tc, f2, factions);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_game(&data);
    free_gamedata();
    f = f2 = NULL;
    data.strm.api->rewind(data.strm.handle);
    read_game(&data);
    mstream_done(&data.strm);
    gamedata_done(&data);
    CuAssertPtrEquals(tc, 0, findfaction(fno));
    f2 = factions;
    CuAssertPtrNotNull(tc, f2);
    u = f2->units;
    CuAssertPtrNotNull(tc, u);
    g = get_group(u);
    CuAssertPtrNotNull(tc, g);
    CuAssertPtrEquals(tc, 0, g->allies);
    test_cleanup();
}

static void test_group_readwrite(CuTest * tc)
{
    faction * f;
    group *g;
    ally *al;
    int i;
    gamedata data;
    storage store;

    test_cleanup();
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    f = test_create_faction(0);
    g = new_group(f, "NW", 42);
    g = new_group(f, "Egoisten", 43);
    key_set(&g->attribs, 44);
    al = ally_add(&g->allies, f);
    al->status = HELP_GIVE;
    write_groups(&store, f);
    WRITE_INT(&store, 47);

    free_group(f->groups);
    free_group(g);
    f->groups = 0;
    data.strm.api->rewind(data.strm.handle);
    read_groups(&data, f);
    READ_INT(&store, &i);
    mstream_done(&data.strm);
    gamedata_done(&data);

    CuAssertIntEquals(tc, 47, i);
    CuAssertPtrNotNull(tc, f->groups);
    CuAssertIntEquals(tc, 42, f->groups->gid);
    CuAssertStrEquals(tc, "NW", f->groups->name);
    CuAssertPtrNotNull(tc, f->groups->next);
    CuAssertIntEquals(tc, 43, f->groups->next->gid);
    CuAssertStrEquals(tc, "Egoisten", f->groups->next->name);
    CuAssertPtrEquals(tc, 0, f->groups->allies);
    g = f->groups->next;
    CuAssertTrue(tc, key_get(g->attribs, 44));
    CuAssertPtrNotNull(tc, g->allies);
    CuAssertPtrEquals(tc, 0, g->allies->next);
    CuAssertPtrEquals(tc, f, g->allies->faction);
    CuAssertIntEquals(tc, HELP_GIVE, g->allies->status);
    test_cleanup();
}

static void test_group(CuTest * tc)
{
    unit *u;
    region *r;
    faction *f;
    group *g;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    assert(r && f);
    u = test_create_unit(f, r);
    assert(u);
    CuAssertPtrNotNull(tc, (g = join_group(u, "hodor")));
    CuAssertPtrEquals(tc, g, get_group(u));
    CuAssertStrEquals(tc, "hodor", g->name);
    CuAssertIntEquals(tc, 1, g->members);
    set_group(u, 0);
    CuAssertIntEquals(tc, 0, g->members);
    CuAssertPtrEquals(tc, 0, get_group(u));
    set_group(u, g);
    CuAssertIntEquals(tc, 1, g->members);
    CuAssertPtrEquals(tc, g, get_group(u));
    test_cleanup();
}

CuSuite *get_group_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_group);
    SUITE_ADD_TEST(suite, test_group_readwrite);
    SUITE_ADD_TEST(suite, test_group_readwrite_dead_faction);
    return suite;
}

