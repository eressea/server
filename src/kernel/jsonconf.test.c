#include <platform.h>
#include "types.h"
#include "jsonconf.h"

#include "building.h"
#include "direction.h"
#include "keyword.h"
#include "race.h"
#include "ship.h"
#include "terrain.h"
#include "util/language.h"
#include <CuTest.h>
#include <cJSON.h>
#include <tests.h>
#include <stdio.h>

static void check_flag(CuTest *tc, const char *name, int flag) {
    char data[1024];
    const struct race *rc;
    cJSON *json;
    sprintf(data, "{\"races\" : { \"orc\": { \"%s\" : true }}}", name);

    json = cJSON_Parse(data);
    free_races();
    json_config(json);
    rc = rc_find("orc");
    CuAssertPtrNotNull(tc, rc);
    CuAssertIntEquals(tc, flag, rc->flags);
}

static void test_flags(CuTest *tc) {
    check_flag(tc, "playerrace", RCF_PLAYERRACE);
    check_flag(tc, "scarepeasants", RCF_SCAREPEASANTS);
    check_flag(tc, "cansteal", RCF_CANSTEAL);
    check_flag(tc, "noheal", RCF_NOHEAL);
    check_flag(tc, "undead", RCF_UNDEAD);
    check_flag(tc, "dragon", RCF_DRAGON);
    check_flag(tc, "fly", RCF_FLY);
    test_cleanup();
}

static void test_races(CuTest * tc)
{
    const char * data = "{\"races\": { \"orc\" : { "
        "\"damage\" : \"1d4\","
        "\"magres\" : 1.0,"
        "\"maxaura\" : 2.0,"
        "\"regaura\" : 3.0,"
        "\"speed\" : 4.0,"
        "\"recruitcost\" : 1,"
        "\"maintenance\" : 2,"
        "\"weight\" : 3,"
        "\"capacity\" : 4,"
        "\"hp\" : 5,"
        "\"ac\" : 6"
        "}}}";
    cJSON *json = cJSON_Parse(data);
    const struct race *rc;

    test_cleanup();

    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, 0, races);
    json_config(json);

    CuAssertPtrNotNull(tc, races);
    rc = rc_find("orc");
    CuAssertPtrNotNull(tc, rc);
    CuAssertStrEquals(tc, "1d4", rc->def_damage);
    CuAssertDblEquals(tc, 1.0, rc->magres, 0.0);
    CuAssertDblEquals(tc, 2.0, rc->maxaura, 0.0);
    CuAssertDblEquals(tc, 3.0, rc->regaura, 0.0);
    CuAssertDblEquals(tc, 4.0, rc->speed, 0.0);
    CuAssertIntEquals(tc, 1, rc->recruitcost);
    CuAssertIntEquals(tc, 2, rc->maintenance);
    CuAssertIntEquals(tc, 3, rc->weight);
    CuAssertIntEquals(tc, 4, rc->capacity);
    CuAssertIntEquals(tc, 5, rc->hitpoints);
    CuAssertIntEquals(tc, 6, rc->armor);
    test_cleanup();
}

static void test_ships(CuTest * tc)
{
    const char * data = "{\"ships\": { \"boat\" : { "
        "\"construction\" : { \"maxsize\" : 20, \"reqsize\" : 10, \"minskill\" : 1 }"
        "}}}";

    cJSON *json = cJSON_Parse(data);
    const struct ship_type *st;

    test_cleanup();

    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, 0, shiptypes);
    json_config(json);

    CuAssertPtrNotNull(tc, shiptypes);
    st = st_find("boat");
    CuAssertPtrNotNull(tc, st);
    CuAssertPtrNotNull(tc, st->construction);
    CuAssertIntEquals(tc, 10, st->construction->reqsize);
    CuAssertIntEquals(tc, 20, st->construction->maxsize);
    CuAssertIntEquals(tc, 1, st->construction->minskill);
    test_cleanup();
}

static void test_buildings(CuTest * tc)
{
    const char * data = "{\"buildings\": { \"house\" : { "
        "\"construction\" : { \"maxsize\" : 20, \"reqsize\" : 10, \"minskill\" : 1 }"
        "}}}";

    cJSON *json = cJSON_Parse(data);
    const building_type *bt;

    test_cleanup();

    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, 0, buildingtypes);
    json_config(json);

    CuAssertPtrNotNull(tc, buildingtypes);
    bt = bt_find("house");
    CuAssertPtrNotNull(tc, bt);
    CuAssertPtrNotNull(tc, bt->construction);
    CuAssertIntEquals(tc, 10, bt->construction->reqsize);
    CuAssertIntEquals(tc, 20, bt->construction->maxsize);
    CuAssertIntEquals(tc, 1, bt->construction->minskill);
    test_cleanup();
}

static void test_terrains(CuTest * tc)
{
    const char * data = "{\"terrains\": { \"plain\" : { \"flags\" : [ \"land\", \"fly\", \"walk\" ] } }}";
    const terrain_type *ter;

    cJSON *json = cJSON_Parse(data);

    test_cleanup();
    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, 0, (void *)get_terrain("plain"));

    json_config(json);
    ter = get_terrain("plain");
    CuAssertPtrNotNull(tc, ter);
    CuAssertIntEquals(tc, ter->flags, LAND_REGION|FLY_INTO|WALK_INTO);

    test_cleanup();
}

static void test_directions(CuTest * tc)
{
    const char * data = "{\"directions\": { \"de\" : { \"east\" : \"osten\", \"northwest\" : [ \"nw\", \"nordwest\" ], \"pause\" : \"pause\" }}}";
    const struct locale * lang;

    cJSON *json = cJSON_Parse(data);

    test_cleanup();
    lang = get_or_create_locale("de");
    CuAssertPtrNotNull(tc, json);
    CuAssertIntEquals(tc, NODIRECTION, get_direction("ost", lang));

    json_config(json);
    CuAssertIntEquals(tc, D_EAST, get_direction("ost", lang));
    CuAssertIntEquals(tc, D_NORTHWEST, get_direction("nw", lang));
    CuAssertIntEquals(tc, D_NORTHWEST, get_direction("nordwest", lang));
    CuAssertIntEquals(tc, D_PAUSE, get_direction("pause", lang));

    test_cleanup();
}

static void test_keywords(CuTest * tc)
{
    const char * data = "{\"keywords\": { \"de\" : { \"move\" : \"NACH\", \"study\" : [ \"LERNEN\", \"STUDIEREN\" ] }}}";
    const struct locale * lang;

    cJSON *json = cJSON_Parse(data);

    test_cleanup();
    lang = get_or_create_locale("de");
    CuAssertPtrNotNull(tc, json);
    CuAssertIntEquals(tc, NOKEYWORD, get_keyword("potato", lang));

    json_config(json);
    CuAssertIntEquals(tc, K_STUDY, get_keyword("studiere", lang));
    CuAssertIntEquals(tc, K_STUDY, get_keyword("lerne", lang));
    CuAssertIntEquals(tc, K_MOVE, get_keyword("nach", lang));

    CuAssertStrEquals(tc, "LERNEN", locale_string(lang, "keyword::study"));

    test_cleanup();
}

CuSuite *get_jsonconf_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_keywords);
    SUITE_ADD_TEST(suite, test_directions);
    SUITE_ADD_TEST(suite, test_ships);
    SUITE_ADD_TEST(suite, test_buildings);
    SUITE_ADD_TEST(suite, test_terrains);
    SUITE_ADD_TEST(suite, test_races);
    SUITE_ADD_TEST(suite, test_flags);
    return suite;
}

