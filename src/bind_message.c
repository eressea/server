#include <platform.h>
#include <kernel/config.h>

#include "spells.h"

/* kernel includes */
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/region.h>
#include <kernel/unit.h>

/* util includes */
#include <util/language.h>
#include <util/message.h>
#include <util/nrmessage.h>

/* lua includes */
#include <tolua.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define E_OK 0
#define E_INVALID_MESSAGE 1
#define E_INVALID_PARAMETER_NAME 2
#define E_INVALID_PARAMETER_TYPE 3
#define E_INVALID_PARAMETER_VALUE 4

typedef struct lua_message {
    const message_type *mtype;
    message *msg;
    variant *args;
} lua_message;

int mtype_get_param(const message_type * mtype, const char *param)
{
    int i;
    for (i = 0; i != mtype->nparameters; ++i) {
        if (strcmp(mtype->pnames[i], param) == 0) {
            return i;
        }
    }
    return mtype->nparameters;
}

static lua_message *msg_create_message(const char *type)
{
    lua_message *lmsg = malloc(sizeof(lua_message));
    lmsg->msg = 0;
    lmsg->args = 0;
    lmsg->mtype = mt_find(type);
    if (lmsg->mtype) {
        lmsg->args = (variant *)calloc(lmsg->mtype->nparameters, sizeof(variant));
    }
    return lmsg;
}

static int msg_set_resource(lua_message * msg, const char *param, const char *resname)
{
    if (msg->mtype) {
        int i = mtype_get_param(msg->mtype, param);
        const resource_type * rtype;
        if (i == msg->mtype->nparameters) {
            return E_INVALID_PARAMETER_NAME;
        }
        if (strcmp(msg->mtype->types[i]->name, "resource") != 0) {
            return E_INVALID_PARAMETER_TYPE;
        }

        rtype = rt_find(resname);
        if (rtype) {
            msg->args[i].v = (void *)rtype;
        }
        else {
            return E_INVALID_PARAMETER_VALUE;
        }
        return E_OK;
    }
    return E_INVALID_MESSAGE;
}

static int msg_set_order(lua_message * msg, const char *param, struct order *ord)
{
    if (msg->mtype) {
        int i = mtype_get_param(msg->mtype, param);
        if (i == msg->mtype->nparameters) {
            return E_INVALID_PARAMETER_NAME;
        }
        if (strcmp(msg->mtype->types[i]->name, "order") != 0) {
            return E_INVALID_PARAMETER_TYPE;
        }

        msg->args[i].v = (void *)ord;
        return E_OK;
    }
    return E_INVALID_MESSAGE;
}

static int msg_set_unit(lua_message * msg, const char *param, const unit * u)
{
    if (msg->mtype) {
        int i = mtype_get_param(msg->mtype, param);

        if (i == msg->mtype->nparameters) {
            return E_INVALID_PARAMETER_NAME;
        }
        if (strcmp(msg->mtype->types[i]->name, "unit") != 0) {
            return E_INVALID_PARAMETER_TYPE;
        }

        msg->args[i].v = (void *)u;

        return E_OK;
    }
    return E_INVALID_MESSAGE;
}

static int msg_set_region(lua_message * msg, const char *param, const region * r)
{
    if (msg->mtype) {
        int i = mtype_get_param(msg->mtype, param);

        if (i == msg->mtype->nparameters) {
            return E_INVALID_PARAMETER_NAME;
        }
        if (strcmp(msg->mtype->types[i]->name, "region") != 0) {
            return E_INVALID_PARAMETER_TYPE;
        }

        msg->args[i].v = (void *)r;

        return E_OK;
    }
    return E_INVALID_MESSAGE;
}

static int msg_set_string(lua_message * msg, const char *param, const char *value)
{
    if (msg->mtype) {
        int i = mtype_get_param(msg->mtype, param);
        variant var;

        if (i == msg->mtype->nparameters) {
            return E_INVALID_PARAMETER_NAME;
        }
        if (strcmp(msg->mtype->types[i]->name, "string") != 0) {
            return E_INVALID_PARAMETER_TYPE;
        }

        var.v = (void *)value;
        msg->args[i] = msg->mtype->types[i]->copy(var);

        return E_OK;
    }
    return E_INVALID_MESSAGE;
}

static int msg_set_int(lua_message * msg, const char *param, int value)
{
    if (msg->mtype) {
        int i = mtype_get_param(msg->mtype, param);
        if (i == msg->mtype->nparameters) {
            return E_INVALID_PARAMETER_NAME;
        }
        if (strcmp(msg->mtype->types[i]->name, "int") != 0) {
            return E_INVALID_PARAMETER_TYPE;
        }

        msg->args[i].i = value;

        return E_OK;
    }
    return E_INVALID_MESSAGE;
}

static int msg_send_faction(lua_message * msg, faction * f)
{
    assert(f);
    assert(msg);

    if (msg->mtype) {
        if (msg->msg == NULL) {
            msg->msg = msg_create(msg->mtype, msg->args);
        }
        add_message(&f->msgs, msg->msg);
        return E_OK;
    }
    return E_INVALID_MESSAGE;
}

static int msg_send_region(lua_message * lmsg, region * r)
{
    if (lmsg->mtype) {
        if (lmsg->msg == NULL) {
            lmsg->msg = msg_create(lmsg->mtype, lmsg->args);
        }
        add_message(&r->msgs, lmsg->msg);
        return E_OK;
    }
    return E_INVALID_MESSAGE;
}

static int tolua_msg_create(lua_State * L)
{
    const char *type = tolua_tostring(L, 1, 0);
    lua_message *lmsg = msg_create_message(type);
    tolua_pushusertype(L, (void *)lmsg, TOLUA_CAST "message");
    return 1;
}

static int tolua_msg_set_string(lua_State * L)
{
    lua_message *lmsg = (lua_message *)tolua_tousertype(L, 1, 0);
    const char *param = tolua_tostring(L, 2, 0);
    const char *value = tolua_tostring(L, 3, 0);
    int result = msg_set_string(lmsg, param, value);
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_msg_set_int(lua_State * L)
{
    lua_message *lmsg = (lua_message *)tolua_tousertype(L, 1, 0);
    const char *param = tolua_tostring(L, 2, 0);
    int value = (int)tolua_tonumber(L, 3, 0);
    int result = msg_set_int(lmsg, param, value);
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_msg_set_resource(lua_State * L)
{
    lua_message *lmsg = (lua_message *)tolua_tousertype(L, 1, 0);
    const char *param = tolua_tostring(L, 2, 0);
    const char *value = tolua_tostring(L, 3, 0);
    int result = msg_set_resource(lmsg, param, value);
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_msg_set_order(lua_State * L)
{
    lua_message *lmsg = (lua_message *)tolua_tousertype(L, 1, 0);
    const char *param = tolua_tostring(L, 2, 0);
    struct order *value = (struct order *)tolua_tousertype(L, 3, 0);
    int result = msg_set_order(lmsg, param, value);
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_msg_set_unit(lua_State * L)
{
    lua_message *lmsg = (lua_message *)tolua_tousertype(L, 1, 0);
    const char *param = tolua_tostring(L, 2, 0);
    unit *value = (unit *)tolua_tousertype(L, 3, 0);
    int result = msg_set_unit(lmsg, param, value);
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_msg_set_region(lua_State * L)
{
    lua_message *lmsg = (lua_message *)tolua_tousertype(L, 1, 0);
    const char *param = tolua_tostring(L, 2, 0);
    region *value = (region *)tolua_tousertype(L, 3, 0);
    int result = msg_set_region(lmsg, param, value);
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_msg_set(lua_State * L)
{
    tolua_Error err;
    if (tolua_isnumber(L, 3, 0, &err)) {
        return tolua_msg_set_int(L);
    }
    else if (tolua_isusertype(L, 3, TOLUA_CAST "region", 0, &err)) {
        return tolua_msg_set_region(L);
    }
    else if (tolua_isusertype(L, 3, TOLUA_CAST "unit", 0, &err)) {
        return tolua_msg_set_unit(L);
    }
    lua_pushinteger(L, -1);
    return 1;
}

static int tolua_msg_send_region(lua_State * L)
{
    lua_message *lmsg = (lua_message *)tolua_tousertype(L, 1, 0);
    region *r = (region *)tolua_tousertype(L, 2, 0);
    int result = msg_send_region(lmsg, r);
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_msg_report_action(lua_State * L)
{
    lua_message *lmsg = (lua_message *)tolua_tousertype(L, 1, 0);
    region *r = (region *)tolua_tousertype(L, 2, 0);
    unit *u = (unit *)tolua_tousertype(L, 3, 0);
    int result, flags = (int)tolua_tonumber(L, 4, 0);
    if (lmsg->msg == NULL) {
        lmsg->msg = msg_create(lmsg->mtype, lmsg->args);
    }
    result = report_action(r, u, lmsg->msg, flags);
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_msg_send_faction(lua_State * L)
{
    lua_message *lmsg = (lua_message *)tolua_tousertype(L, 1, 0);
    faction *f = (faction *)tolua_tousertype(L, 2, 0);
    if (f && lmsg) {
        int result = msg_send_faction(lmsg, f);
        lua_pushinteger(L, result);
        return 1;
    }
    return 0;
}

static int tolua_msg_get_type(lua_State * L)
{
    lua_message *lmsg = (lua_message *)tolua_tousertype(L, 1, 0);
    lua_pushstring(L, lmsg->msg->type->name);
    return 1;
}

static int tolua_msg_render(lua_State * L)
{
    lua_message *lmsg = (lua_message *)tolua_tousertype(L, 1, 0);
    const char * lname = tolua_tostring(L, 2, 0);
    const struct locale * lang = lname ? get_locale(lname) : default_locale;
    char name[64];

    if (lmsg->msg == NULL) {
        lmsg->msg = msg_create(lmsg->mtype, lmsg->args);
    }
    nr_render(lmsg->msg, lang, name, sizeof(name), NULL);
    lua_pushstring(L, name);
    return 1;
}

void tolua_message_open(lua_State * L)
{
    /* register user types */
    tolua_usertype(L, TOLUA_CAST "message");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_function(L, TOLUA_CAST "message", tolua_msg_create);

        tolua_cclass(L, TOLUA_CAST "message", TOLUA_CAST "message", TOLUA_CAST "",
            NULL);
        tolua_beginmodule(L, TOLUA_CAST "message");
        {
            tolua_function(L, TOLUA_CAST "render", tolua_msg_render);
            tolua_variable(L, TOLUA_CAST "type", tolua_msg_get_type, 0);
            tolua_function(L, TOLUA_CAST "set", tolua_msg_set);
            tolua_function(L, TOLUA_CAST "set_unit", tolua_msg_set_unit);
            tolua_function(L, TOLUA_CAST "set_order", tolua_msg_set_order);
            tolua_function(L, TOLUA_CAST "set_region", tolua_msg_set_region);
            tolua_function(L, TOLUA_CAST "set_resource", tolua_msg_set_resource);
            tolua_function(L, TOLUA_CAST "set_int", tolua_msg_set_int);
            tolua_function(L, TOLUA_CAST "set_string", tolua_msg_set_string);
            tolua_function(L, TOLUA_CAST "send_faction", tolua_msg_send_faction);
            tolua_function(L, TOLUA_CAST "send_region", tolua_msg_send_region);
            tolua_function(L, TOLUA_CAST "report_action", tolua_msg_report_action);

            tolua_function(L, TOLUA_CAST "create", tolua_msg_create);
        }
        tolua_endmodule(L);
    }
    tolua_endmodule(L);
}
