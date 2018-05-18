#ifdef _MSC_VER
#include <platform.h>
#endif

#include "kernel/types.h"

#include "jsonconf.h"

#include "kernel/config.h"
#include "kernel/building.h"
#include "kernel/item.h"
#include "kernel/race.h"
#include "kernel/ship.h"
#include "kernel/spell.h"
#include "kernel/order.h"
#include "kernel/terrain.h"

#include "util/language.h"

#include "kernel/calendar.h"
#include "direction.h"
#include "keyword.h"
#include "move.h"
#include "prefix.h"

#include <CuTest.h>
#include <cJSON.h>
#include <tests.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static const struct race * race_with_flag(const char * name) {
    char data[1024];
    cJSON *json;
    sprintf(data, "{\"races\" : { \"orc\": { \"speed\" : 1, \"flags\" : [ \"%s\"] }}}", name);

    json = cJSON_Parse(data);
    free_races();
    json_config(json);
    cJSON_Delete(json);
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
    test_setup();
    check_flag(tc, "player", RCF_PLAYABLE);
    check_flag(tc, "scarepeasants", RCF_SCAREPEASANTS);
    check_flag(tc, "nosteal", RCF_NOSTEAL);
    check_flag(tc, "noheal", RCF_NOHEAL);
    check_flag(tc, "undead", RCF_UNDEAD);
    check_flag(tc, "dragon", RCF_DRAGON);
    check_flag(tc, "fly", RCF_FLY);
    check_ec_flag(tc, "getitem", ECF_GETITEM);
    check_ec_flag(tc, "giveperson", ECF_GIVEPERSON);
    check_ec_flag(tc, "giveunit", ECF_GIVEUNIT);
    test_teardown();
}

static void test_settings(CuTest * tc)
{
    const char * data = "{\"settings\": { "
        "\"string\" : \"1d4\","
        "\"integer\" : 14,"
        "\"true\": true,"
        "\"game.id\": 4,"
        "\"game.name\": \"E3\","
        "\"false\": false,"
        "\"float\" : 1.5 }}";
    cJSON *json = cJSON_Parse(data);

    test_setup();
    config_set("game.id", "42"); /* should not be replaced */
    config_set("game.name", "Eressea"); /* should not be replaced */
    json_config(json);
    CuAssertStrEquals(tc, "42", config_get("game.id"));
    CuAssertStrEquals(tc, "Eressea", config_get("game.name"));
    CuAssertStrEquals(tc, "1", config_get("true"));
    CuAssertStrEquals(tc, "0", config_get("false"));
    CuAssertStrEquals(tc, "1d4", config_get("string"));
    CuAssertIntEquals(tc, 14, config_get_int("integer", 0));
    CuAssertDblEquals(tc, 1.5f, config_get_flt("float", 0), 0.01);
    cJSON_Delete(json);
    test_teardown();
}

static void test_prefixes(CuTest * tc)
{
    const char * data = "{\"prefixes\": [ "
        "\"snow\","
        "\"sea\","
        "\"dark\""
        "]}";
    cJSON *json = cJSON_Parse(data);

    test_setup();
    json_config(json);
    CuAssertPtrNotNull(tc, race_prefixes);
    CuAssertStrEquals(tc, "snow", race_prefixes[0]);
    CuAssertStrEquals(tc, "dark", race_prefixes[2]);
    CuAssertPtrEquals(tc, NULL, race_prefixes[3]);
    cJSON_Delete(json);
    test_teardown();
}

static void test_disable(CuTest * tc)
{
    const char * data = "{\"disabled\": [ "
        "\"alchemy\","
        "\"pay\","
        "\"besiege\","
        "\"module\""
        "]}";
    cJSON *json = cJSON_Parse(data);

    test_setup();
    CuAssertTrue(tc, skill_enabled(SK_ALCHEMY));
    CuAssertTrue(tc, !keyword_disabled(K_BANNER));
    CuAssertTrue(tc, !keyword_disabled(K_PAY));
    CuAssertTrue(tc, !keyword_disabled(K_BESIEGE));
    CuAssertIntEquals(tc, 1, config_get_int("module.enabled", 1));
    json_config(json);
    CuAssertTrue(tc, !skill_enabled(SK_ALCHEMY));
    CuAssertTrue(tc, !keyword_disabled(K_BANNER));
    CuAssertTrue(tc, keyword_disabled(K_PAY));
    CuAssertTrue(tc, keyword_disabled(K_BESIEGE));
    CuAssertIntEquals(tc, 0, config_get_int("module.enabled", 1));
    cJSON_Delete(json);
    test_teardown();
}

static void test_calendar(CuTest * tc)
{
    const char * data = "{\"calendar\": { "
        "\"weeks\" : [ \"one\", \"two\", \"three\" ],"
        "\"months\" : ["
        "{ \"storm\" : 99, \"season\" : 1 },"
        "{ \"storm\" : 22, \"season\" : 2 }"
        "]"
        "}}";
    cJSON *json = cJSON_Parse(data);

    test_setup();
    json_config(json);
    CuAssertPtrNotNull(tc, storms);
    CuAssertIntEquals(tc, 2, months_per_year);
    CuAssertIntEquals(tc, 3, weeks_per_month);
    CuAssertIntEquals(tc, 99, storms[0]);
    CuAssertIntEquals(tc, 22, storms[1]);
    CuAssertPtrNotNull(tc, month_season);
    CuAssertIntEquals(tc, 1, month_season[0]);
    CuAssertIntEquals(tc, 2, month_season[1]);
    cJSON_Delete(json);
    test_teardown();
}

static void test_races(CuTest * tc)
{
    const char * data = "{\"races\": { \"orc\" : { "
        "\"damage\" : \"1d4\","
        "\"magres\" : 100,"
        "\"maxaura\" : 200,"
        "\"regaura\" : 3.0,"
        "\"speed\" : 4.0,"
        "\"recruitcost\" : 1,"
        "\"maintenance\" : 2,"
        "\"weight\" : 3,"
        "\"capacity\" : 4,"
        "\"income\" : 30,"
        "\"hp\" : 5,"
        "\"ac\" : 6,"
        "\"flags\" : [ \"player\", \"walk\", \"undead\" ]"
        "}}}";
    cJSON *json = cJSON_Parse(data);
    const struct race *rc;

    test_setup();

    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, NULL, races);
    json_config(json);

    CuAssertPtrNotNull(tc, races);
    rc = rc_find("orc");
    CuAssertPtrNotNull(tc, rc);
    CuAssertIntEquals(tc, RCF_PLAYABLE | RCF_WALK | RCF_UNDEAD, rc->flags);
    CuAssertStrEquals(tc, "1d4", rc->def_damage);
    CuAssertTrue(tc, frac_equal(frac_one, rc->magres));
    CuAssertIntEquals(tc, 200, rc->maxaura);
    CuAssertDblEquals(tc, 2.0, rc_maxaura(rc), 0.0);
    CuAssertDblEquals(tc, 3.0, rc->regaura, 0.0);
    CuAssertDblEquals(tc, 4.0, rc->speed, 0.0);
    CuAssertIntEquals(tc, 1, rc->recruitcost);
    CuAssertIntEquals(tc, 2, rc->maintenance);
    CuAssertIntEquals(tc, 3, rc->weight);
    CuAssertIntEquals(tc, 4, rc->capacity);
    CuAssertIntEquals(tc, 30, rc->income);
    CuAssertIntEquals(tc, 5, rc->hitpoints);
    CuAssertIntEquals(tc, 6, rc->armor);
    cJSON_Delete(json);
    test_teardown();
}

static void test_findrace(CuTest *tc) {
    const char * data = "{\"races\": { \"dwarf\": {} }, \"strings\": { \"de\" : { \"race::dwarf\" : \"Zwerg\" } } }";
    cJSON *json = cJSON_Parse(data);
    struct locale *lang;
    const race *rc;

    CuAssertPtrNotNull(tc, json);
    test_setup();
    lang = get_or_create_locale("de");
    CuAssertPtrEquals(tc, NULL, (void *)findrace("Zwerg", lang));

    json_config(json);
    init_locale(lang);
    rc = findrace("Zwerg", lang);
    CuAssertPtrNotNull(tc, rc);
    CuAssertStrEquals(tc, "dwarf", rc->_name);
    cJSON_Delete(json);
    test_teardown();
}

static void test_items(CuTest * tc)
{
    const char * data = "{\"items\": { "
        "\"axe\" : { \"weight\" : 2},"
        "\"horse\" : { \"flags\" : [ \"animal\", \"big\" ], \"capacity\" : 20 }"
        "}}";
    cJSON *json = cJSON_Parse(data);
    const item_type * itype;

    test_setup();

    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, NULL, it_find("axe"));
    CuAssertPtrEquals(tc, NULL, rt_find("axe"));
    CuAssertPtrEquals(tc, NULL, (void *)get_resourcetype(R_HORSE));

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
    cJSON_Delete(json);
    test_teardown();
}

static void test_ships(CuTest * tc)
{
    const char * data = "{\"ships\": { \"boat\" : { "
        "\"construction\" : { \"maxsize\" : 20, \"reqsize\" : 10, \"minskill\" : 1 },"
        "\"coasts\" : [ \"plain\" ],"
        "\"range\" : 8,"
        "\"maxrange\" : 16"
        "}}}";

    cJSON *json = cJSON_Parse(data);
    const ship_type *st;
    const terrain_type *ter;

    test_setup();

    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, NULL, shiptypes);
    json_config(json);

    CuAssertPtrNotNull(tc, shiptypes);
    st = st_find("boat");
    CuAssertPtrNotNull(tc, st);
    CuAssertPtrNotNull(tc, st->construction);
    CuAssertIntEquals(tc, 10, st->construction->reqsize);
    CuAssertIntEquals(tc, 20, st->construction->maxsize);
    CuAssertIntEquals(tc, 1, st->construction->minskill);
    CuAssertIntEquals(tc, 8, st->range);
    CuAssertIntEquals(tc, 16, st->range_max);

    ter = get_terrain("plain");
    CuAssertPtrNotNull(tc, ter);

    CuAssertPtrNotNull(tc, st->coasts);
    CuAssertPtrEquals(tc, (void *)ter, (void *)st->coasts[0]);
    CuAssertPtrEquals(tc, NULL, (void *)st->coasts[1]);

    cJSON_Delete(json);
    test_teardown();
}

static void test_castles(CuTest *tc) {
    const char * data = "{\"buildings\": { \"castle\" : { "
        "\"stages\" : ["
        "{ \"construction\": { \"maxsize\" : 2 }, \"name\": \"site\" },"
        "{ \"construction\": { \"maxsize\" : 8 } },"
        "{ \"construction\": { \"maxsize\" : -1 } }"
        "]}}}";

    cJSON *json = cJSON_Parse(data);
    const building_type *bt;
    const building_stage *stage;
    const construction *con;

    test_setup();

    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, NULL, buildingtypes);
    json_config(json);

    CuAssertPtrNotNull(tc, buildingtypes);
    bt = bt_find("castle");
    CuAssertPtrNotNull(tc, bt);
    CuAssertPtrNotNull(tc, stage = bt->stages);
    CuAssertStrEquals(tc, "site", stage->name);
    CuAssertPtrNotNull(tc, con = stage->construction);
    CuAssertIntEquals(tc, 2, con->maxsize);

    CuAssertPtrNotNull(tc, stage = stage->next);
    CuAssertPtrEquals(tc, NULL, stage->name);
    CuAssertPtrNotNull(tc, con = stage->construction);
    CuAssertIntEquals(tc, 6, con->maxsize);

    CuAssertPtrNotNull(tc, stage = stage->next);
    CuAssertPtrNotNull(tc, con = stage->construction);
    CuAssertIntEquals(tc, -1, con->maxsize);

    CuAssertPtrEquals(tc, NULL, stage->next);

    cJSON_Delete(json);
    test_teardown();
}

static void test_spells(CuTest * tc)
{
    const char * data = "{\"spells\": { \"fireball\" : { \"syntax\" : \"u+\" } } }";

    cJSON *json = cJSON_Parse(data);
    const spell *sp;

    test_setup();
    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, NULL, find_spell("fireball"));

    json_config(json);
    sp = find_spell("fireball");
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "u+", sp->syntax);

    cJSON_Delete(json);
    test_teardown();
    CuAssertPtrEquals(tc, NULL, find_spell("fireball"));
}

static const char * building_data = "{\"buildings\": { "
"\"house\" : { "
"\"maintenance\" : "
"{ \"type\" : \"iron\", \"amount\" : 1, \"flags\" : [ \"variable\" ] }"
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
    const construction *con;

    test_setup();

    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, NULL, buildingtypes);
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
    CuAssertIntEquals(tc, MTF_VARIABLE, bt->maintenance[0].flags);
    CuAssertIntEquals(tc, 0, bt->maintenance[1].number);

    CuAssertPtrNotNull(tc, bt->stages);
    CuAssertPtrEquals(tc, NULL, bt->stages->next);
    CuAssertPtrNotNull(tc, bt->stages->construction);
    CuAssertPtrNotNull(tc, con = bt->stages->construction);
    CuAssertPtrNotNull(tc, con->materials);
    CuAssertIntEquals(tc, 2, con->materials[0].number);
    CuAssertPtrEquals(tc, (void *)get_resourcetype(R_STONE), (void *)con->materials[0].rtype);
    CuAssertIntEquals(tc, 1, con->materials[1].number);
    CuAssertPtrEquals(tc, (void *)get_resourcetype(R_IRON), (void *)con->materials[1].rtype);
    CuAssertIntEquals(tc, 0, con->materials[2].number);
    CuAssertIntEquals(tc, 10, con->reqsize);
    CuAssertIntEquals(tc, 20, con->maxsize);
    CuAssertIntEquals(tc, 1, con->minskill);
    cJSON_Delete(json);
    test_teardown();
}

static const char * building_defaults_data = "{\"buildings\": { "
"\"house\" : { }"
"}}";

static void test_buildings_default(CuTest * tc)
{
    cJSON *json = cJSON_Parse(building_defaults_data);
    const building_type *bt;
    building_type clone;

    test_setup();

    bt = bt_get_or_create("house");
    clone = *bt;

    CuAssertIntEquals(tc, 0, memcmp(bt, &clone, sizeof(building_type)));
    CuAssertPtrNotNull(tc, json);
    json_config(json);

    CuAssertPtrEquals(tc, (void *)bt, (void *)bt_find("house"));
    CuAssertIntEquals(tc, 0, memcmp(bt, &clone, sizeof(building_type)));
    cJSON_Delete(json);
    test_teardown();
}

static const char * ship_defaults_data = "{\"ships\": { "
"\"hodor\" : { }"
"}}";

static void test_ships_default(CuTest * tc)
{
    cJSON *json = cJSON_Parse(ship_defaults_data);
    const ship_type *st;
    ship_type clone;

    test_setup();

    st = st_get_or_create("hodor");
    clone = *st;

    CuAssertIntEquals(tc, 0, memcmp(st, &clone, sizeof(ship_type)));
    CuAssertPtrNotNull(tc, json);
    json_config(json);

    CuAssertPtrEquals(tc, (void *)st, (void *)st_find("hodor"));
    CuAssertIntEquals(tc, 0, memcmp(st, &clone, sizeof(ship_type)));
    cJSON_Delete(json);
    test_teardown();
}

static void test_configs(CuTest * tc)
{
    const char * data = "{\"include\": [ \"test.json\" ] }";
    FILE *F;
    cJSON *json = cJSON_Parse(data);

    test_setup();

    F = fopen("test.json", "w");
    fwrite(building_data, 1, strlen(building_data), F);
    fclose(F);
    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, NULL, buildingtypes);
    json_config(json);
    CuAssertPtrNotNull(tc, buildingtypes);
    if (remove("test.json")!=0 && errno==ENOENT) {
        errno = 0;
    }
    cJSON_Delete(json);
    test_teardown();
}

static void test_terrains(CuTest * tc)
{
    const char * data = "{\"terrains\": { \"plain\" : { "
        "\"herbs\": [ \"h0\", \"h1\" ], "
        "\"production\": { \"stone\": { \"chance\": 0.1, \"base\": \"1d4\", \"div\": \"1d5\", \"level\": \"1d6\" }, \"iron\": {} }, "
        "\"size\": 4000, "
        "\"road\": 50, "
        "\"seed\": 3, "
        "\"flags\" : [ \"forbidden\", \"arctic\", \"cavalry\", \"sea\", \"forest\", \"land\", \"fly\", \"swim\", \"walk\" ] } }}";
    const terrain_type *ter;

    cJSON *json = cJSON_Parse(data);

    test_setup();
    CuAssertPtrNotNull(tc, json);
    CuAssertPtrEquals(tc, NULL, (void *)get_terrain("plain"));

    json_config(json);
    ter = get_terrain("plain");
    CuAssertPtrNotNull(tc, ter);
    CuAssertIntEquals(tc, ARCTIC_REGION | LAND_REGION | SEA_REGION | FOREST_REGION | CAVALRY_REGION | FORBIDDEN_REGION | FLY_INTO | WALK_INTO | SWIM_INTO , ter->flags);
    CuAssertIntEquals(tc, 4000, ter->size);
    CuAssertIntEquals(tc, 50, ter->max_road);
    CuAssertIntEquals(tc, 3, ter->distribution);
    CuAssertPtrNotNull(tc, ter->herbs);
    CuAssertPtrEquals(tc, rt_get_or_create("h0"), ter->herbs[0]->rtype);
    CuAssertPtrEquals(tc, rt_get_or_create("h1"), ter->herbs[1]->rtype);
    CuAssertPtrEquals(tc, NULL, (void *)ter->herbs[2]);
    CuAssertPtrNotNull(tc, ter->name); /* anything named "plain" uses plain_name() */
    CuAssertPtrNotNull(tc, ter->production);
    CuAssertPtrEquals(tc, rt_get_or_create("stone"), (resource_type *)ter->production[0].type);
    CuAssertDblEquals(tc, 0.1, ter->production[0].chance, 0.01);
    CuAssertStrEquals(tc, "1d4", ter->production[0].base);
    CuAssertStrEquals(tc, "1d5", ter->production[0].divisor);
    CuAssertStrEquals(tc, "1d6", ter->production[0].startlevel);
    CuAssertPtrEquals(tc, rt_get_or_create("iron"), (resource_type *)ter->production[1].type);
    CuAssertPtrEquals(tc, NULL, (void *)ter->production[2].type);

    cJSON_Delete(json);
    test_teardown();
}

static void test_directions(CuTest * tc)
{
    const char * data = "{\"directions\": { \"de\" : { \"east\" : \"osten\", \"northwest\" : [ \"nw\", \"nordwest\" ], \"pause\" : \"pause\" }}}";
    const struct locale * lang;

    cJSON *json = cJSON_Parse(data);

    test_setup();
    lang = get_or_create_locale("de");
    CuAssertPtrNotNull(tc, json);
    CuAssertIntEquals(tc, NODIRECTION, get_direction("ost", lang));

    json_config(json);
    CuAssertIntEquals(tc, D_EAST, get_direction("ost", lang));
    CuAssertIntEquals(tc, D_NORTHWEST, get_direction("nw", lang));
    CuAssertIntEquals(tc, D_NORTHWEST, get_direction("nordwest", lang));
    CuAssertIntEquals(tc, D_PAUSE, get_direction("pause", lang));

    cJSON_Delete(json);
    test_teardown();
}

static void test_skills(CuTest * tc)
{
    const char * data = "{\"skills\": { \"de\" : { \"alchemy\" : \"ALCHEMIE\", \"crossbow\" : [ \"ARMBRUST\", \"KREUZBOGEN\" ] }}}";
    const struct locale * lang;

    cJSON *json = cJSON_Parse(data);

    test_setup();
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

    cJSON_Delete(json);
    test_teardown();
}

static void test_keywords(CuTest * tc)
{
    const char * data = "{\"keywords\": { \"de\" : { \"move\" : \"NACH\", \"study\" : [ \"LERNEN\", \"STUDIEREN\" ] }}}";
    const struct locale * lang;

    cJSON *json = cJSON_Parse(data);

    test_setup();
    lang = get_or_create_locale("de");
    CuAssertPtrNotNull(tc, json);
    CuAssertIntEquals(tc, NOKEYWORD, get_keyword("potato", lang));

    json_config(json);
    CuAssertIntEquals(tc, K_STUDY, get_keyword("studiere", lang));
    CuAssertIntEquals(tc, K_STUDY, get_keyword("lerne", lang));
    CuAssertIntEquals(tc, K_MOVE, get_keyword("nach", lang));

    CuAssertStrEquals(tc, "LERNEN", LOC(lang, "keyword::study"));
    CuAssertStrEquals(tc, "NACH", LOC(lang, "keyword::move"));

    cJSON_Delete(json);
    test_teardown();
}

static void test_strings(CuTest * tc)
{
    const char * data = "{\"strings\": { \"de\" : { \"move\" : \"NACH\", \"study\" : \"LERNEN\" }}}";
    const struct locale * lang;

    cJSON *json = cJSON_Parse(data);
    CuAssertPtrNotNull(tc, json);

    test_setup();
    lang = get_or_create_locale("de");
    CuAssertPtrNotNull(tc, lang);
    CuAssertPtrEquals(tc, NULL, (void *)LOC(lang, "move"));
    json_config(json);
    CuAssertStrEquals(tc, "NACH", LOC(lang, "move"));
    CuAssertStrEquals(tc, "LERNEN", LOC(lang, "study"));
    cJSON_Delete(json);
    test_teardown();
}

static void test_infinitive_from_config(CuTest *tc) {
    char buffer[32];
    struct locale *lang;
    struct order *ord;
    const char * data = "{\"keywords\": { \"de\" : { \"study\" : [ \"LERNE\", \"LERNEN\" ] }}}";

    cJSON *json = cJSON_Parse(data);
    CuAssertPtrNotNull(tc, json);
    test_setup();
    lang = get_or_create_locale("de");
    json_config(json);

    CuAssertIntEquals(tc, K_STUDY, get_keyword("LERN", lang));
    CuAssertIntEquals(tc, K_STUDY, get_keyword("LERNE", lang));
    CuAssertIntEquals(tc, K_STUDY, get_keyword("LERNEN", lang));

    ord = create_order(K_STUDY, lang, "");
    CuAssertStrEquals(tc, "LERNE", get_command(ord, lang, buffer, sizeof(buffer)));
    free_order(ord);
    cJSON_Delete(json);
    test_teardown();
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
    SUITE_ADD_TEST(suite, test_settings);
    SUITE_ADD_TEST(suite, test_prefixes);
    SUITE_ADD_TEST(suite, test_disable);
    SUITE_ADD_TEST(suite, test_infinitive_from_config);
    SUITE_ADD_TEST(suite, test_calendar);
    return suite;
}

