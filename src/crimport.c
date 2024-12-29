#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "crimport.h"

#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
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
    BLOCK_REGION,
    BLOCK_RESOURCE,
    BLOCK_FACTION,
    BLOCK_ITEMS,
    BLOCK_SKILLS,
    BLOCK_UNIT,
    BLOCK_SHIP,
    BLOCK_BUILDING
} block_t;

typedef enum key_t {
    KEY_UNKNOWN,
    KEY_ID,
    KEY_TYPE,
    KEY_NUMBER,
    KEY_NAME,
    KEY_FACTION,
    KEY_TERRAIN,
} key_t;

typedef enum special_t {
    SPECIAL_UNKNOWN,
    SPECIAL_FOREST,
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

static void add_key(const char *str, key_t key)
{
    cb_add(&cb_keys, str, (int)key);
}

static void init_keys()
{
    add_key("id", KEY_ID);
    add_key("Typ", KEY_TYPE);
    add_key("type", KEY_TYPE);
    add_key("Name", KEY_NAME);
    add_key("Parteiname", KEY_NAME);
    add_key("Anzahl", KEY_NUMBER);
    add_key("number", KEY_NUMBER);
    add_key("Partei", KEY_FACTION);
    add_key("Terrain", KEY_TERRAIN);
}

static key_t get_key(const char *name)
{
    if (!cb_keys.root) init_keys();
    return (key_t)cb_get(&cb_keys, name, KEY_UNKNOWN);
}

static void add_special(const char *str, special_t key)
{
    cb_add(&cb_special, str, (int)key);
}

static void init_specials()
{
    add_special("Wald", SPECIAL_FOREST);
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
        if (0 == strcmp("REGION", name)) {
            int x = keyv[0];
            int y = keyv[1];
            plane *pl = (keyc < 3) ? NULL : getplanebyid(keyv[2]);
            memset(ctx, 0, sizeof(context));
            ctx->region = new_region(x, y, pl, 0);
            ctx->block = BLOCK_REGION;
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
    }
    else if (keyc == 0) {
        if (0 == strcmp("GEGENSTAENDE", name)) {
            ctx->block = BLOCK_ITEMS;
        }
        else if (0 == strcmp("TALENTE", name)) {
            ctx->block = BLOCK_SKILLS;
        }
    }
    return CR_ERROR_NONE;
}

static bool forest_terrain(const char * value)
{
    static const char *forest_name = NULL;
    if (!forest_name) forest_name = LOC(default_locale, "forest");
    return (0 == strcmp(value, forest_name));
}

static enum CR_Error handle_unit(context *ctx, key_t key, const char *value)
{
    unit *u = ctx->unit;
    switch (key) {
    case KEY_TYPE:
        u_setrace(u, findrace(value, default_locale));
        break;
    case KEY_NAME:
        unit_setname(u, value);
        break;
    case KEY_NUMBER:
        scale_number(u, atoi(value));
        break;
    case KEY_FACTION:
        u_setfaction(u, findfaction(atoi(value)));
        break;
    default:
        break;
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_resource(context *ctx, key_t key, const char *value)
{
    region *r = ctx->region;
    if (!r) {
        return CR_ERROR_GRAMMAR;
    }
    switch (key) {
    case KEY_NUMBER:
        if (ctx->stack) {
            const resource_type *rtype = rt_find(ctx->stack);
            region_setresource(r, rtype, atoi(value));
        }
        else {
            ctx->stack = value;
        }
        break;
    case KEY_TYPE:
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

static enum CR_Error handle_faction(context *ctx, key_t key, const char *value)
{
    faction *f = ctx->faction;
    if (!f) {
        return CR_ERROR_GRAMMAR;
    }
    switch (key) {
    case KEY_NAME:
        faction_setname(f, value);
        break;
    default:
        break;
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_region(context *ctx, key_t key, const char *value)
{
    region *r = ctx->region;
    if (!r) {
        return CR_ERROR_GRAMMAR;
    }
    switch (key) {
    case KEY_NAME:
        region_setname(r, value);
        break;
    case KEY_ID: {
        int uid = atoi(value);
        if (uid != r->uid) {
            runhash(r);
            r->uid = uid;
            rhash(r);
        }
        break;
    }
    case KEY_TERRAIN: {
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
        else {
            log_error("unknown item: %s", name);
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

int crimport(const char *filename)
{
    CR_Parser cp;
    context ctx = { BLOCK_OTHER, NULL };
    bool done = false;
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
    //CR_SetTextHandler(cp, handle_text);

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
