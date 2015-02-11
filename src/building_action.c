/* 
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <platform.h>
#include <kernel/config.h>
#include <kernel/building.h>
#include <kernel/version.h>
#include <util/attrib.h>
#include <util/log.h>
#include <util/resolve.h>

#include <storage.h>

#include <tolua.h>
#include <lua.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct building_action {
    struct building *b;
    char *fname;
    char *param;
} building_action;

static int lc_age(struct attrib *a)
{
    building_action *data = (building_action *)a->data.v;
    const char *fname = data->fname;
    const char *fparam = data->param;
    building *b = data->b;
    int result = -1;

    assert(b != NULL);
    if (fname != NULL) {
        lua_State *L = (lua_State *)global.vm_state;

        lua_getglobal(L, fname);
        if (lua_isfunction(L, -1)) {
            tolua_pushusertype(L, (void *)b, TOLUA_CAST "building");
            if (fparam) {
                tolua_pushstring(L, fparam);
            }

            if (lua_pcall(L, fparam ? 2 : 1, 1, 0) != 0) {
                const char *error = lua_tostring(L, -1);
                log_error("lc_age(%s) calling '%s': %s.\n", buildingname(b), fname, error);
                lua_pop(L, 1);
            }
            else {
                result = (int)lua_tonumber(L, -1);
                lua_pop(L, 1);
            }
        }
        else {
            log_error("lc_age(%s) calling '%s': not a function.\n", buildingname(b), fname);
            lua_pop(L, 1);
        }
    }
    return (result != 0) ? AT_AGE_KEEP : AT_AGE_REMOVE;
}

static const char *NULLSTRING = "(null)";

static void lc_init(struct attrib *a)
{
    a->data.v = calloc(1, sizeof(building_action));
}

static void lc_done(struct attrib *a)
{
    building_action *data = (building_action *)a->data.v;
    if (data->fname)
        free(data->fname);
    if (data->param)
        free(data->param);
    free(data);
}

static void
lc_write(const struct attrib *a, const void *owner, struct storage *store)
{
    building_action *data = (building_action *)a->data.v;
    const char *fname = data->fname;
    const char *fparam = data->param;
    building *b = data->b;

    write_building_reference(b, store);
    WRITE_TOK(store, fname);
    WRITE_TOK(store, fparam ? fparam : NULLSTRING);
}

static int lc_read(struct attrib *a, void *owner, struct storage *store)
{
    char name[NAMESIZE];
    building_action *data = (building_action *)a->data.v;
    int result =
        read_reference(&data->b, store, read_building_reference, resolve_building);
    READ_TOK(store, name, sizeof(name));
    if (strcmp(name, "tunnel_action") == 0) {
        /* E2: Weltentor has a new module, doesn't need this any longer */
        result = 0;
        data->b = 0;
    }
    else {
        data->fname = _strdup(name);
    }
    READ_TOK(store, name, sizeof(name));
    if (strcmp(name, "tnnL") == 0) {
        /* tunnel_action was the old Weltentore, their code has changed. ignore this object */
        result = 0;
        data->b = 0;
    }
    if (strcmp(name, NULLSTRING) == 0)
        data->param = 0;
    else {
        data->param = _strdup(name);
    }
    if (result == 0 && !data->b) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

attrib_type at_building_action = {
    "lcbuilding",
    lc_init, lc_done,
    lc_age,
    lc_write, lc_read
};

void building_addaction(building * b, const char *fname, const char *param)
{
    attrib *a = a_add(&b->attribs, a_new(&at_building_action));
    building_action *data = (building_action *)a->data.v;
    data->b = b;
    data->fname = _strdup(fname);
    if (param) {
        data->param = _strdup(param);
    }
}
