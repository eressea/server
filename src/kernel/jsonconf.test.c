#include <platform.h>
#include "types.h"
#include "jsonconf.h"

#include "config.h"
#include "building.h"
#include "direction.h"
#include "item.h"
#include "keyword.h"
#include "race.h"
#include "ship.h"
#include "spell.h"
#include "order.h"
#include "terrain.h"
#include "util/language.h"
#include <CuTest.h>
#include <cJSON.h>
#include <tests.h>
#include <string.h>
#include <stdio.h>

static const struct race * race_with_flag(const char * name) {
    char data[1024];
    cJSON *json;
    sprintf(data, "{\"races\" : { \"orc\": { \"speed\" : 1, \"flags\" : [ \"%s\"] }}}", name);

    json = cJSON_Parse(data);
    free_races();
    json_config(json);
    return rc_find("orc");
}

static void check_ec_flag(CuTest *tc, const char *name, int flag) {
    const struct race *rc = race_with_flag(name);
    CuAssertPtrNotNull(tc, rc);
    CuAssertIntEquals(tc, flag, rc->ec_flags);
}

static void check_flag(CuTest *tc, const char *name, int flag) {
    const struct race *rc = race_with_flag(name);
    CuAssertPtrNotNull(tc, rc);
    CuAssertIntEquals(tc, flag, rc->flags);
}

static void test_flags(CuTest *tc) {
    check_flag(tc, "npc", RCF_NPC);
    check_flag(tc, "scarepeasants", RCF_SCAREPEASANTS);
    check_flag(tc, "nosteal", RCF_NOSTEAL);
    check_flag(tc, "noheal", RCF_NOHEAL);
    check_flag(tc, "undead", RCF_UNDEAD);
    check_flag(tc, "dragon", RCF_DRAGON);
    check_flag(tc, "fly", RCF_FLY);
    check_ec_flag(tc, "getitem", GETITEM);
    check_ec_flag(tc, "giveitem", GIVEITEM);
    check_ec_flag(tc, "giveperson", GIVEPERSON);
    check_ec_flag(tc, "giveunit", GIVEUNIT);
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
        "\"ac\" : 6,"
        "\"flags\" : [ \"npc\", \"walk\", \"undead\" ]"
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
    CuAssertIntEquals(tc, RCF_NPC | RCF_WALK | RCF_UNDEAD, rc->flags);
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

static void test_findrace(CuTest *tc) {
    const char * data = "{\"races\": { \"dwarf\": {} }, \"strings\": { \"de\" : { \"race::dwarf\" : \"Zwerg\" } } }";
    cJSON *json = cJSON_Parse(data);
    struct locale *lang;
    const race *rc;

    CuAssertPtrNotNull(tc, json);
    test_cleanup();
    lang = get_or_create_locale("de");
    CuAssertPtrEquals(tc, 0, (void *)findrace("Zwerg", lang));

    json_config(json);
    init_locale(lang);
    rc = findrace("Zwerg", lang);
    CuAssertPtrNotNull(tc, rc);
    CuAssertStrEquals(tc, "dwarf", rc->_name);
    test_cleanup();
}

static void test_items(CuTest * tc)
{
    const char * data = "{\"items\": { "
        "\"axe\" : { \"weight\" : 2},"
        "\"horse\" : { \"flags\" : [ \"animal\", \"big\" ], \"capacity\" : 20 }"
        "}}";
    cJSON *json = cJSON_Parse(data);
    const item_type * itype;

    test_cleanup();

    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, 0, it_find("axe"));
    CuAssertPtrEquals(tc, 0, rt_find("axe"));
    CuAssertPtrEquals(tc, 0, (void *)get_resourcetype(R_HORSE));

    json_config(json);

    itype = it_find("axe");
    CuAssertPtrNotNull(tc, itype);
    CuAssertIntEquals(tc, 2, itype->weight);
    CuAssertIntEquals(tc, 0, itype->flags);

    itype = it_find("horse");
    CuAssertPtrNotNull(tc, itype);
    CuAssertIntEquals(tc, 20, itype->capacity);
    CuAssertIntEquals(tc, ITF_ANIMAL | ITF_BIG, itype->flags);

    CuAssertPtrNotNull(tc, rt_find("axe"));
    CuAssertPtrNotNull(tc, (void *)get_resourcetype(R_HORSE));
}

static void test_ships(CuTest * tc)
{
    const char * data = "{\"ships\": { \"boat\" : { "
        "\"construction\" : { \"maxsize\" : 20, \"reqsize\" : 10, \"minskill\" : 1 },"
        "\"coasts\" : [ \"plain\" ]"
        "}}}";

    cJSON *json = cJSON_Parse(data);
    const ship_type *st;
    const terrain_type *ter;

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

    ter = get_terrain("plain");
    CuAssertPtrNotNull(tc, ter);

    CuAssertPtrNotNull(tc, st->coasts);
    CuAssertPtrEquals(tc, (void *)ter, (void *)st->coasts[0]);
    CuAssertPtrEquals(tc, 0, (void *)st->coasts[1]);

    test_cleanup();
}

static void test_castles(CuTest *tc) {
    const char * data = "{\"buildings\": { \"castle\" : { "
        "\"construction\" : ["
        "{ \"maxsize\" : 2 },"
        "{ \"maxsize\" : 8 }"
        "]}}}";

    cJSON *json = cJSON_Parse(data);
    const building_type *bt;

    test_cleanup();

    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, 0, buildingtypes);
    json_config(json);

    CuAssertPtrNotNull(tc, buildingtypes);
    bt = bt_find("castle");
    CuAssertPtrNotNull(tc, bt);
    CuAssertPtrNotNull(tc, bt->construction);
    CuAssertIntEquals(tc, 2, bt->construction->maxsize);
    CuAssertPtrNotNull(tc, bt->construction->improvement);
    CuAssertIntEquals(tc, 6, bt->construction->improvement->maxsize);
    CuAssertPtrEquals(tc, 0, bt->construction->improvement->improvement);
}

static void test_spells(CuTest * tc)
{
    const char * data = "{\"spells\": { \"fireball\" : { \"syntax\" : \"u+\" } } }";

    cJSON *json = cJSON_Parse(data);
    const spell *sp;

    test_cleanup();
    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, 0, find_spell("fireball"));

    json_config(json);
    sp = find_spell("fireball");
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "u+", sp->syntax);

    test_cleanup();
    CuAssertPtrEquals(tc, 0, find_spell("fireball"));
}

static const char * building_data = "{\"buildings\": { "
"\"house\" : { "
"\"maintenance\" : "
"{ \"type\" : \"iron\", \"amount\" : 1, \"flags\" : [ \"required\", \"variable\" ] }"
","
"\"construction\" : {"
"\"maxsize\" : 20,"
"\"reqsize\" : 10,"
"\"minskill\" : 1,"
"\"materials\" : {"
"\"stone\" : 2,"
"\"iron\" : 1"
"}}},"
"\"shed\" : {"
"\"maintenance\" : ["
"{ \"type\" : \"iron\", \"amount\" : 1 },"
"{ \"type\" : \"stone\", \"amount\" : 2 }"
"]}"
"}}";

static void test_buildings(CuTest * tc)
{
    cJSON *json = cJSON_Parse(building_data);
    const building_type *bt;

    test_cleanup();

    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, 0, buildingtypes);
    json_config(json);

    CuAssertPtrNotNull(tc, buildingtypes);
    bt = bt_find("shed");
    CuAssertPtrNotNull(tc, bt);
    CuAssertPtrNotNull(tc, bt->maintenance);
    CuAssertPtrEquals(tc, (void *)get_resourcetype(R_IRON), (void *)bt->maintenance[0].rtype);
    CuAssertPtrEquals(tc, (void *)get_resourcetype(R_STONE), (void *)bt->maintenance[1].rtype);
    CuAssertIntEquals(tc, 1, bt->maintenance[0].number);
    CuAssertIntEquals(tc, 2, bt->maintenance[1].number);
    CuAssertIntEquals(tc, 0, bt->maintenance[2].number);

    bt = bt_find("house");
    CuAssertPtrNotNull(tc, bt);

    CuAssertPtrNotNull(tc, bt->maintenance);
    CuAssertIntEquals(tc, 1, bt->maintenance[0].number);
    CuAssertPtrEquals(tc, (void *)get_resourcetype(R_IRON), (void *)bt->maintenance[0].rtype);
    CuAssertIntEquals(tc, MTF_VARIABLE | MTF_VITAL, bt->maintenance[0].flags);
    CuAssertIntEquals(tc, 0, bt->maintenance[1].number);

    CuAssertPtrNotNull(tc, bt->construction);
    CuAssertPtrNotNull(tc, bt->construction->materials);
    CuAssertIntEquals(tc, 2, bt->construction->materials[0].number);
    CuAssertPtrEquals(tc, (void *)get_resourcetype(R_STONE), (void *)bt->construction->materials[0].rtype);
    CuAssertIntEquals(tc, 1, bt->construction->materials[1].number);
    CuAssertPtrEquals(tc, (void *)get_resourcetype(R_IRON), (void *)bt->construction->materials[1].rtype);
    CuAssertIntEquals(tc, 0, bt->construction->materials[2].number);
    CuAssertIntEquals(tc, 10, bt->construction->reqsize);
    CuAssertIntEquals(tc, 20, bt->construction->maxsize);
    CuAssertIntEquals(tc, 1, bt->construction->minskill);
    CuAssertPtrEquals(tc, 0, bt->construction->improvement);
    test_cleanup();
}

static const char * building_defaults_data = "{\"buildings\": { "
"\"house\" : { }"
"}}";

static void test_buildings_default(CuTest * tc)
{
    cJSON *json = cJSON_Parse(building_defaults_data);
    const building_type *bt;
    building_type clone;

    test_cleanup();

    bt = bt_get_or_create("house");
    clone = *bt;

    CuAssertIntEquals(tc, 0, memcmp(bt, &clone, sizeof(building_type)));
    CuAssertPtrNotNull(tc, json);
    json_config(json);

    CuAssertPtrEquals(tc, (void *)bt, (void *)bt_find("house"));
    CuAssertIntEquals(tc, 0, memcmp(bt, &clone, sizeof(building_type)));
    test_cleanup();
}

static const char * ship_defaults_data = "{\"ships\": { "
"\"hodor\" : { }"
"}}";

static void test_ships_default(CuTest * tc)
{
    cJSON *json = cJSON_Parse(ship_defaults_data);
    const ship_type *st;
    ship_type clone;

    test_cleanup();

    st = st_get_or_create("hodor");
    clone = *st;

    CuAssertIntEquals(tc, 0, memcmp(st, &clone, sizeof(ship_type)));
    CuAssertPtrNotNull(tc, json);
    json_config(json);

    CuAssertPtrEquals(tc, (void *)st, (void *)st_find("hodor"));
    CuAssertIntEquals(tc, 0, memcmp(st, &clone, sizeof(ship_type)));
    test_cleanup();
}

static void test_configs(CuTest * tc)
{
    const char * data = "{\"include\": [ \"test.json\" ] }";
    FILE *F;
    cJSON *json = cJSON_Parse(data);

    test_cleanup();

    F = fopen("test.json", "wt");
    fwrite(building_data, 1, strlen(building_data), F);
    fclose(F);
    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, 0, buildingtypes);
    json_config(json);
    CuAssertPtrNotNull(tc, buildingtypes);
    unlink("test.json");
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
    CuAssertIntEquals(tc, ter->flags, LAND_REGION | FLY_INTO | WALK_INTO);

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

static void test_skills(CuTest * tc)
{
    const char * data = "{\"skills\": { \"de\" : { \"alchemy\" : \"ALCHEMIE\", \"crossbow\" : [ \"ARMBRUST\", \"KREUZBOGEN\" ] }}}";
    const struct locale * lang;

    cJSON *json = cJSON_Parse(data);

    test_cleanup();
    lang = get_or_create_locale("de");
    CuAssertPtrNotNull(tc, json);
    CuAssertIntEquals(tc, NOSKILL, get_skill("potato", lang));

    json_config(json);
    CuAssertIntEquals(tc, NOSKILL, get_skill("potato", lang));
    CuAssertIntEquals(tc, SK_CROSSBOW, get_skill("armbrust", lang));
    CuAssertIntEquals(tc, SK_CROSSBOW, get_skill("kreuz", lang));
    CuAssertIntEquals(tc, SK_ALCHEMY, get_skill("alchemie", lang));

    CuAssertStrEquals(tc, "ALCHEMIE", LOC(lang, "skill::alchemy"));
    CuAssertStrEquals(tc, "ARMBRUST", LOC(lang, "skill::crossbow"));

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

    CuAssertStrEquals(tc, "LERNEN", LOC(lang, "keyword::study"));
    CuAssertStrEquals(tc, "NACH", LOC(lang, "keyword::move"));

    test_cleanup();
}

static void test_strings(CuTest * tc)
{
    const char * data = "{\"strings\": { \"de\" : { \"move\" : \"NACH\", \"study\" : \"LERNEN\" }}}";
    const struct locale * lang;

    cJSON *json = cJSON_Parse(data);
    CuAssertPtrNotNull(tc, json);

    test_cleanup();
    lang = get_or_create_locale("de");
    CuAssertPtrNotNull(tc, lang);
    CuAssertPtrEquals(tc, NULL, (void *)LOC(lang, "move"));
    json_config(json);
    CuAssertStrEquals(tc, "NACH", LOC(lang, "move"));
    CuAssertStrEquals(tc, "LERNEN", LOC(lang, "study"));
}

static void test_infinitive_from_config(CuTest *tc) {
    char buffer[32];
    struct locale *lang;
    struct order *ord;
    const char * data = "{\"keywords\": { \"de\" : { \"study\" : [ \"LERNE\", \"LERNEN\" ] }}}";

    cJSON *json = cJSON_Parse(data);
    CuAssertPtrNotNull(tc, json);
    json_config(json);

    lang = get_or_create_locale("de");
    CuAssertIntEquals(tc, K_STUDY, get_keyword("LERN", lang));
    CuAssertIntEquals(tc, K_STUDY, get_keyword("LERNE", lang));
    CuAssertIntEquals(tc, K_STUDY, get_keyword("LERNEN", lang));

    ord = create_order(K_STUDY, lang, "");
    CuAssertStrEquals(tc, "LERNE", get_command(ord, buffer, sizeof(buffer)));
    test_cleanup();
}

CuSuite *get_jsonconf_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_keywords);
    SUITE_ADD_TEST(suite, test_skills);
    SUITE_ADD_TEST(suite, test_directions);
    SUITE_ADD_TEST(suite, test_items);
    SUITE_ADD_TEST(suite, test_ships_default);
    SUITE_ADD_TEST(suite, test_ships);
    SUITE_ADD_TEST(suite, test_buildings);
    SUITE_ADD_TEST(suite, test_buildings_default);
    SUITE_ADD_TEST(suite, test_configs);
    SUITE_ADD_TEST(suite, test_castles);
    SUITE_ADD_TEST(suite, test_terrains);
    SUITE_ADD_TEST(suite, test_races);
    SUITE_ADD_TEST(suite, test_findrace);
    SUITE_ADD_TEST(suite, test_strings);
    SUITE_ADD_TEST(suite, test_spells);
    SUITE_ADD_TEST(suite, test_flags);
    SUITE_ADD_TEST(suite, test_infinitive_from_config);
    return suite;
}

