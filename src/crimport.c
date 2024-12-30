#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "crimport.h"

#include <kernel/calendar.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/skill.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/base36.h>
#include <util/language.h>
#include <util/log.h>

#include <crpat.h>
#include <critbit.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum block_t {
    BLOCK_OTHER,
    BLOCK_VERSION,
    BLOCK_OPTIONS,
    BLOCK_ALLIANCE,
    BLOCK_GROUP,
    BLOCK_REGION,
    BLOCK_PRICES,
    BLOCK_BORDER,
    BLOCK_RESOURCE,
    BLOCK_FACTION,
    BLOCK_UNIT,
    BLOCK_COMMANDS,
    BLOCK_SPELLS,
    BLOCK_COMBATSPELLS,
    BLOCK_SKILLS,
    BLOCK_ITEMS,
    BLOCK_SHIP,
    BLOCK_BUILDING,
    BLOCK_EFFECTS
} block_t;

typedef enum tag_t {
    TAG_UNKNOWN,
    TAG_ID,
    TAG_TYPE,
    TAG_NUMBER,
    TAG_NAME,
    TAG_FACTION,
    TAG_OTHER_FACTION,
    TAG_STATUS,
    TAG_LOCALE,
    TAG_AGE,
    TAG_OPTIONS,
    TAG_SCHOOL,
    TAG_NMR,
    TAG_EMAIL,
    TAG_PREFIX,
    TAG_DESCRIPTION,
    TAG_MEMO,
    TAG_BUILDING,
    TAG_SHIP,
    TAG_TERRAIN,
} tag_t;

typedef enum special_t {
    SPECIAL_UNKNOWN,
    SPECIAL_FOREST,
    SPECIAL_VIAL,
    SPECIAL_HERBS,
    SPECIAL_MONEYBAG,
    SPECIAL_MONEYCHEST,
    SPECIAL_DRAGONHOARD
} special_t;

static struct critbit_tree cb_keys = CRITBIT_TREE();
static struct critbit_tree cb_special = CRITBIT_TREE();

static void cb_add(critbit_tree *cb, const char *str, int key)
{
    char buffer[32];
    size_t len = strlen(str);
    len = cb_new_kv(str, len, &key, sizeof(key), buffer);
    cb_insert(cb, buffer, len);
}

static int cb_get(critbit_tree *cb, const char *str, int def)
{
    void *match = NULL;
    int count = cb_find_prefix(cb, str, strlen(str) + 1, &match, 1, 0);
    if (count > 0) {
        int result;
        cb_get_kv(match, &result, sizeof(result));
        return result;
    }
    return def;
}

static void add_key(const char *str, tag_t key)
{
    cb_add(&cb_keys, str, (int)key);
}

static void init_keys(void)
{
    add_key("id", TAG_ID);
    add_key("Typ", TAG_TYPE);
    add_key("type", TAG_TYPE);
    add_key("Name", TAG_NAME);
    add_key("Parteiname", TAG_NAME);
    add_key("Anzahl", TAG_NUMBER);
    add_key("number", TAG_NUMBER);
    add_key("Partei", TAG_FACTION);
    add_key("locale", TAG_LOCALE);
    add_key("age", TAG_AGE);
    add_key("Optionen", TAG_OPTIONS);
    add_key("Magiegebiet", TAG_SCHOOL);
    add_key("nmr", TAG_NMR);
    add_key("email", TAG_EMAIL);
    add_key("typprefix", TAG_PREFIX);
    add_key("Anderepartei", TAG_OTHER_FACTION);
    add_key("banner", TAG_DESCRIPTION);
    add_key("Beschr", TAG_DESCRIPTION);
    add_key("Terrain", TAG_TERRAIN);
    add_key("Kampfstatus", TAG_STATUS);
    add_key("Status", TAG_STATUS);
    add_key("privat", TAG_MEMO);
    add_key("Burg", TAG_BUILDING);
    add_key("Schiff", TAG_SHIP);
}

static tag_t get_key(const char *name)
{
    if (!cb_keys.root) init_keys();
    return (tag_t)cb_get(&cb_keys, name, TAG_UNKNOWN);
}

static void add_special(const char *str, special_t key)
{
    cb_add(&cb_special, str, (int)key);
}

static void init_specials(void)
{
    add_special("Wald", SPECIAL_FOREST);
    add_special("Phiole", SPECIAL_VIAL);
    // FIXME: NON-ASCII characters in the source are ugly af.
    add_special("Kräuterbeutel", SPECIAL_HERBS);
    add_special("Silberbeutel", SPECIAL_MONEYBAG);
    add_special("Silberkassette", SPECIAL_MONEYCHEST);
    add_special("Drachenhort", SPECIAL_DRAGONHOARD);
}

static special_t get_special(const char *name)
{
    if (!cb_special.root) init_specials();
    return (special_t)cb_get(&cb_special, name, SPECIAL_UNKNOWN);
}

typedef struct context {
    block_t block;
    struct region *region;
    struct faction *faction;
    struct unit *unit;
    struct ship *ship;
    struct building *building;
    const char *stack;
} context;

static enum CR_Error handle_element(void *udata, const char *name,
    unsigned int keyc, int keyv[]) {
    context *ctx = (context *)udata;

    ctx->block = BLOCK_OTHER;
    if (keyc >= 2) {
        if (0 == strcmp("SCHEMEN", name) || 0 == strcmp("REGION", name)) {
            int x = keyv[0];
            int y = keyv[1];
            plane *pl = (keyc < 3) ? NULL : getplanebyid(keyv[2]);
            memset(ctx, 0, sizeof(context));
            ctx->region = new_region(x, y, pl, 0);
            ctx->block = BLOCK_REGION;
        }
        else {
            log_info("unsupported block %s", name);
        }
    }
    else if (keyc == 1) {
        if (0 == strcmp("EINHEIT", name)) {
            unit * u = ctx->unit = unit_create(keyv[0]);
            ctx->ship = NULL;
            ctx->building = NULL;
            move_unit(u, ctx->region, NULL);
            ctx->block = BLOCK_UNIT;
        }
        else if (0 == strcmp("PARTEI", name)) {
            memset(ctx, 0, sizeof(context));
            ctx->faction = faction_create(keyv[0]);
            ctx->block = BLOCK_FACTION;
        }
        else if (0 == strcmp("RESOURCE", name)) {
            ctx->block = BLOCK_RESOURCE;
            ctx->stack = NULL;
        }
        else if (0 == strcmp("BURG", name)) {
            ctx->block = BLOCK_BUILDING;
        }
        else if (0 == strcmp("SCHIFF", name)) {
            ctx->block = BLOCK_SHIP;
        }
        else if (0 == strcmp("ALLIANZ", name)) {
            ctx->block = BLOCK_ALLIANCE;
        }
        else if (0 == strcmp("VERSION", name)) {
            ctx->block = BLOCK_VERSION;
        }
        else if (0 == strcmp("GRUPPE", name)) {
            ctx->block = BLOCK_GROUP;
        }
        else if (0 == strcmp("GRENZE", name)) {
            ctx->block = BLOCK_BORDER;
        }
        else if (0 == strcmp("KAMPFZAUBER", name)) {
            ctx->block = BLOCK_COMBATSPELLS;
        }
        else {
            log_info("unsupported block %s", name);
        }
    }
    else if (keyc == 0) {
        if (0 == strcmp("GEGENSTAENDE", name)) {
            ctx->block = BLOCK_ITEMS;
        }
        else if (0 == strcmp("TALENTE", name)) {
            ctx->block = BLOCK_SKILLS;
        }
        else if (0 == strcmp("OPTIONEN", name)) {
            ctx->block = BLOCK_OPTIONS;
        }
        else if (0 == strcmp("PREISE", name)) {
            ctx->block = BLOCK_PRICES;
        }
        else if (0 == strcmp("EFFECTS", name)) {
            ctx->block = BLOCK_EFFECTS;
        }
        else if (0 == strcmp("COMMANDS", name)) {
            ctx->block = BLOCK_COMMANDS;
        }
        else if (0 == strcmp("SPRUECHE", name)) {
            ctx->block = BLOCK_SPELLS;
        }
        else {
            log_info("unsupported block %s", name);
        }
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_unit(context *ctx, tag_t key, const char *value)
{
    unit *u = ctx->unit;
    switch (key) {
    case TAG_BUILDING:
    case TAG_SHIP:
    case TAG_OTHER_FACTION:
    case TAG_PREFIX:
        break;
    case TAG_TYPE:
        u_setrace(u, findrace(value, default_locale));
        break;
    case TAG_STATUS:
        unit_setstatus(u, atoi(value));
        break;
    case TAG_NAME:
        unit_setname(u, value);
        break;
    case TAG_NUMBER:
        scale_number(u, atoi(value));
        break;
    case TAG_FACTION:
        u_setfaction(u, findfaction(atoi(value)));
        break;
    case TAG_DESCRIPTION:
        unit_setinfo(u, value);
        break;
    case TAG_MEMO:
        usetprivate(u, value);
        break;
    default:
        break;
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_resource(context *ctx, tag_t key, const char *value)
{
    region *r = ctx->region;
    if (!r) {
        return CR_ERROR_GRAMMAR;
    }
    switch (key) {
    case TAG_NUMBER:
        if (ctx->stack) {
            const resource_type *rtype = rt_find(ctx->stack);
            region_setresource(r, rtype, atoi(value));
        }
        else {
            ctx->stack = value;
        }
        break;
    case TAG_TYPE:
        if (ctx->stack) {
            const resource_type *rtype = rt_find(value);
            region_setresource(r, rtype, atoi(ctx->stack));
        }
        else {
            ctx->stack = value;
        }
    default:
        break;
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_faction(context *ctx, tag_t key, const char *value)
{
    faction *f = ctx->faction;
    if (!f) {
        return CR_ERROR_GRAMMAR;
    }
    switch (key) {
    case TAG_AGE:
        faction_set_age(f, atoi(value));
        break;
    case TAG_OPTIONS:
    case TAG_SCHOOL:
        break;
    case TAG_NMR:
        f->lastorders= turn - atoi(value);
        break;
    case TAG_LOCALE:
        f->locale = get_locale(value);
        break;
    case TAG_NAME:
        faction_setname(f, value);
        break;
    case TAG_DESCRIPTION:
        faction_setbanner(f, value);
        break;
    case TAG_EMAIL:
        faction_setemail(f, value);
        break;
    default:
        break;
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_region(context *ctx, tag_t key, const char *value)
{
    region *r = ctx->region;
    if (!r) {
        return CR_ERROR_GRAMMAR;
    }
    switch (key) {
    case TAG_NAME:
        region_setname(r, value);
        break;
    case TAG_ID: {
        int uid = atoi(value);
        if (uid != r->uid) {
            runhash(r);
            r->uid = uid;
            rhash(r);
        }
        break;
    }
    case TAG_TERRAIN: {
        const struct terrain_type *ter = findterrain(value, default_locale);
        if (!ter) {
            if (get_special(value) == SPECIAL_FOREST) {
                ter = get_terrain("plain");
            }
        }
        if (ter) {
            terraform_region(r, ter);
        }
        else {
            log_error("import_cr: unknown terrain %s", value);
        }
        break;
    }
    default:
        break;
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_skill(context *ctx, const char *value, const char *name)
{
    unit *u = ctx->unit;
    skill_t sk;
    if (!u) {
        return CR_ERROR_GRAMMAR;
    }
    sk = findskill(name, default_locale);
    if (sk == NOSKILL) {
        log_error("import_cr: unknown skill %s", name);
    }
    else {
        char *val = strchr(value, ' ');
        int level = atoi(val + 1);
        set_level(u, sk, level);
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_item(context *ctx, int number, const char *name)
{
    item **itm, **items = NULL;
    const struct item_type *itype = finditemtype(name, default_locale);
    if (!itype) {
        special_t spec = get_special(name);
        if (spec == SPECIAL_DRAGONHOARD) {
            itype = it_find("money");
            number = 50001;
        }
        else if (spec == SPECIAL_MONEYCHEST) {
            itype = it_find("money");
            number = 5001;
        }
        else if (spec == SPECIAL_MONEYBAG) {
            itype = it_find("money");
            number = 501;
        }
        else if (spec == SPECIAL_UNKNOWN) {
            log_error("unknown item: %s", name);
            return CR_ERROR_NONE;
        }
        else {
            log_warning("unhandled special item: %s", name);
            return CR_ERROR_NONE;
        }
    }
    if (ctx->unit) {
        items = &ctx->unit->items;
    }
    else if (ctx->faction) {
        items = &ctx->faction->items;
    }
    else {
        return CR_ERROR_GRAMMAR;
    }
    itm = i_find(items, itype);
    if (*itm) {
        (*itm)->number = number;
    }
    else {
        i_change(items, itype, number);
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_property(void *udata, const char *name, const char *value) {
    context *ctx = (context *)udata;
    enum CR_Error err = CR_ERROR_NONE;
    switch (ctx->block) {
    case BLOCK_EFFECTS:
    case BLOCK_OPTIONS:
    case BLOCK_ALLIANCE:
    case BLOCK_GROUP:
    case BLOCK_SHIP:
    case BLOCK_BUILDING:
    case BLOCK_SPELLS:
    case BLOCK_COMBATSPELLS:
    case BLOCK_BORDER:
    case BLOCK_PRICES:
        break;
    case BLOCK_SKILLS:
        err = handle_skill(ctx, value, name);
        if (err != CR_ERROR_NONE) {
            log_warning("parse error in skill for unit %u. %s = %s",
                itoa36(ctx->unit->no), name, value);
        }
        break;
    case BLOCK_ITEMS:
        err = handle_item(ctx, atoi(value), name);
        if (err != CR_ERROR_NONE) {
            log_warning("parse error in item. %s = %s",
                name, value);
        }
        break;
    case BLOCK_FACTION:
        err = handle_faction(ctx, get_key(name), value);
        if (err != CR_ERROR_NONE) {
            log_warning("parse error in region %d,%d. %s = %s",
                ctx->region->x, ctx->region->y, name, value);
        }
        break;
    case BLOCK_REGION:
        err = handle_region(ctx, get_key(name), value);
        if (err != CR_ERROR_NONE) {
            log_warning("parse error in region %d,%d. %s = %s",
                ctx->region->x, ctx->region->y, name, value);
        }
        break;
    case BLOCK_RESOURCE:
        err = handle_resource(ctx, get_key(name), value);
        if (err != CR_ERROR_NONE) {
            log_warning("parse error in resource for region %d,%d. %s = %s",
                ctx->region->x, ctx->region->y, name, value);
        }
        break;
    case BLOCK_UNIT:
        err = handle_unit(ctx, get_key(name), value);
        if (err != CR_ERROR_NONE) {
            log_warning("parse error in unit %s. %s = %s",
                itoa36(ctx->unit->no), name, value);
        }
        break;
    default:
        break;
    }
    return err;
}

static enum CR_Error handle_text(void *udata, const char *value) {
    context *ctx = (context *)udata;
    enum CR_Error err = CR_ERROR_NONE;
    switch (ctx->block) {
    case BLOCK_COMMANDS: {
        unit *u = ctx->unit;
        if (!u) {
            return CR_ERROR_GRAMMAR;
        }
        unit_addorder(u, parse_order(value, u->faction->locale));
        break;
    }
    default:
        break;
    }
    return err;
}

int crimport(const char *filename)
{
    CR_Parser cp;
    context ctx = { BLOCK_OTHER, NULL };
    int done = 0;
    int err = 0;
    char buf[2048];

    FILE *F = fopen(filename, "rt");
    if (!F) {
        return errno;
    }
    cp = CR_ParserCreate();
    CR_SetUserData(cp, (void *)&ctx);
    CR_SetElementHandler(cp, handle_element);
    CR_SetPropertyHandler(cp, handle_property);
    CR_SetTextHandler(cp, handle_text);

    while (!done) {
        size_t len = (int)fread(buf, 1, sizeof(buf), F);
        if (ferror(F)) {
            fprintf(stderr,
                "read error at line %d of %s: %s\n",
                CR_GetCurrentLineNumber(cp),
                filename, strerror(errno));
            err = errno;
            break;
        }
        done = feof(F);
        if (CR_Parse(cp, buf, len, done) == CR_STATUS_ERROR) {
            int code = CR_GetErrorCode(cp);
            log_error(
                "parse error at line %d of %s: %s\n",
                CR_GetCurrentLineNumber(cp),
                filename, CR_ErrorString(code));
            err = -1;
            break;
        }
    }
    CR_ParserFree(cp);
    fclose(F);
    return err;
}
