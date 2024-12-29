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

static struct critbit_tree cb_keys = CRITBIT_TREE();

static void add_key(const char *str, key_t key)
{
    char buffer[32];
    size_t len = strlen(str);
    len = cb_new_kv(str, len, &key, sizeof(key_t), buffer);
    cb_insert(&cb_keys, buffer, len);
}

static key_t get_key(const char *name)
{
    key_t result = KEY_UNKNOWN;
    void *match = NULL;
    int err = cb_find_prefix(&cb_keys, name, strlen(name) + 1, &match, 1, 0);
    if (CB_EXISTS == err) {
        cb_get_kv(match, &result, sizeof(key_t));
    }
    return result;
}


static void init_keys()
{
    add_key("id", KEY_ID);
    add_key("Typ", KEY_TYPE);
    add_key("type", KEY_TYPE);
    add_key("Name", KEY_NAME);
    add_key("Anzahl", KEY_NUMBER);
    add_key("number", KEY_NUMBER);
    add_key("Partei", KEY_FACTION);
    add_key("Terrain", KEY_TERRAIN);
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
            if (forest_terrain(value)) {
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

static enum CR_Error handle_item(context *ctx, int number, const char *name)
{
    item **itm, **items = NULL;
    const struct item_type *itype = finditemtype(name, default_locale);
    if (!itype) {
        log_error("unknown item: %s", name);
        return CR_ERROR_NONE;
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
    case BLOCK_ITEMS:
        err = handle_item(ctx, atoi(value), name);
        if (err != CR_ERROR_NONE) {
            log_warning("parse error in item. %s = %s",
                name, value);
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

    if (!cb_keys.root) init_keys();

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
