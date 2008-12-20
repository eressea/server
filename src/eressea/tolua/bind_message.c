#include <config.h>
#include <kernel/eressea.h>

// kernel includes
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/message.h>
#include <kernel/region.h>
#include <kernel/unit.h>

// util includes
#include <util/message.h>

// lua includes
#include <lua.h>
#include <tolua++.h>

#define E_OK 0
#define E_INVALID_MESSAGE 1
#define E_INVALID_PARAMETER_NAME 2
#define E_INVALID_PARAMETER_TYPE 3
#define E_INVALID_PARAMETER_VALUE 4

typedef struct lua_message {
  const message_type * mtype;
  message * msg;
  variant * args;
} lua_message;

int 
mtype_get_param(const message_type * mtype, const char * param)
{
  int i;
  for (i=0;i!=mtype->nparameters;++i) {
    if (strcmp(mtype->pnames[i], param)==0) {
      return i;
    }
  }
  return mtype->nparameters;
}

static lua_message *
msg_create_message(const char *type)
{
  lua_message * lmsg = malloc(sizeof(lua_message));
  lmsg->msg = 0;
  lmsg->args = 0;
  lmsg->mtype = mt_find(type);
  if (lmsg->mtype) {
    lmsg->args = (variant*)calloc(lmsg->mtype->nparameters, sizeof(variant));
  }
  return lmsg;
}

/*
 static void
msg_destroy_message(lua_message * msg)
{
  if (msg->msg) msg_release(msg->msg);
  if (msg->mtype) {
    int i;
    for (i=0;i!=msg->mtype->nparameters;++i) {
      if (msg->mtype->types[i]->release) {
        msg->mtype->types[i]->release(msg->args[i]);
      }
    }
  }
}
*/
int
msg_set_resource(lua_message * msg, const char * param, const char * resname) 
{
  if (msg->mtype) {
    int i = mtype_get_param(msg->mtype, param);
    if (i==msg->mtype->nparameters) {
      return E_INVALID_PARAMETER_NAME;
    }
    if (strcmp(msg->mtype->types[i]->name, "resource")!=0) {
      return E_INVALID_PARAMETER_TYPE;
    }

    msg->args[i].v = (void*)rt_find(resname);

    return E_OK;
  }
  return E_INVALID_MESSAGE;
}

int
msg_set_unit(lua_message * msg, const char * param, const unit * u)
{
  int i = mtype_get_param(msg->mtype, param);

  if (msg->mtype) {

    if (i==msg->mtype->nparameters) {
      return E_INVALID_PARAMETER_NAME;
    }
    if (strcmp(msg->mtype->types[i]->name, "unit")!=0) {
      return E_INVALID_PARAMETER_TYPE;
    }

    msg->args[i].v = (void*)u;

    return E_OK;
  }
  return E_INVALID_MESSAGE;
}

int
msg_set_region(lua_message * msg, const char * param, const region * r)
{
  int i = mtype_get_param(msg->mtype, param);
  if (msg->mtype) {

    if (i==msg->mtype->nparameters) {
      return E_INVALID_PARAMETER_NAME;
    }
    if (strcmp(msg->mtype->types[i]->name, "region")!=0) {
      return E_INVALID_PARAMETER_TYPE;
    }

    msg->args[i].v = (void*)r;

    return E_OK;
  }
  return E_INVALID_MESSAGE;
}

int
msg_set_string(lua_message * msg, const char * param, const char * value)
{
  int i = mtype_get_param(msg->mtype, param);
  if (msg->mtype) {
    variant var;
    if (i==msg->mtype->nparameters) {
      return E_INVALID_PARAMETER_NAME;
    }
    if (strcmp(msg->mtype->types[i]->name, "string")!=0) {
      return E_INVALID_PARAMETER_TYPE;
    }

    var.v = (void*)value;
    msg->args[i] = msg->mtype->types[i]->copy(var);

    return E_OK;
  }
  return E_INVALID_MESSAGE;
}

int
msg_set_int(lua_message * msg, const char * param, int value)
{
  if (msg->mtype) {
    int i = mtype_get_param(msg->mtype, param);
    if (i==msg->mtype->nparameters) {
      return E_INVALID_PARAMETER_NAME;
    }
    if (strcmp(msg->mtype->types[i]->name, "int")!=0) {
      return E_INVALID_PARAMETER_TYPE;
    }

    msg->args[i].i = value;

    return E_OK;
  }
  return E_INVALID_MESSAGE;
}

int 
msg_send_faction(lua_message * msg, faction * f)
{
  if (msg->mtype) {
    if (msg->msg==NULL) {
      msg->msg = msg_create(msg->mtype, msg->args);
    }
    add_message(&f->msgs, msg->msg);
    return E_OK;
  }
  return E_INVALID_MESSAGE;
}

int 
msg_send_region(lua_message * lmsg, region * r)
{
  if (lmsg->mtype) {
    if (lmsg->msg==NULL) {
      lmsg->msg = msg_create(lmsg->mtype, lmsg->args);
    }
    add_message(&r->msgs, lmsg->msg);
    return E_OK;
  }
  return E_INVALID_MESSAGE;
}


static int
tolua_msg_create(lua_State * tolua_S)
{
  const char * type = tolua_tostring(tolua_S, 1, 0);
  lua_message * lmsg = msg_create_message(type);
  tolua_pushusertype(tolua_S, (void*)lmsg, "message");
  return 1;
}
static int
tolua_msg_set_string(lua_State * tolua_S)
{
  lua_message * lmsg = (lua_message *)tolua_tousertype(tolua_S, 1, 0);
  const char * param = tolua_tostring(tolua_S, 2, 0);
  const char * value = tolua_tostring(tolua_S, 3, 0);
  int result = msg_set_string(lmsg, param, value);
  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_msg_set_int(lua_State * tolua_S)
{
  lua_message * lmsg = (lua_message *)tolua_tousertype(tolua_S, 1, 0);
  const char * param = tolua_tostring(tolua_S, 2, 0);
  int value = (int)tolua_tonumber(tolua_S, 3, 0);
  int result = msg_set_int(lmsg, param, value);
  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_msg_set_resource(lua_State * tolua_S)
{
  lua_message * lmsg = (lua_message *)tolua_tousertype(tolua_S, 1, 0);
  const char * param = tolua_tostring(tolua_S, 2, 0);
  const char * value = tolua_tostring(tolua_S, 3, 0);
  int result = msg_set_resource(lmsg, param, value);
  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_msg_set_unit(lua_State * tolua_S)
{
  lua_message * lmsg = (lua_message *)tolua_tousertype(tolua_S, 1, 0);
  const char * param = tolua_tostring(tolua_S, 2, 0);
  unit * value = (unit *)tolua_tousertype(tolua_S, 3, 0);
  int result = msg_set_unit(lmsg, param, value);
  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_msg_set_region(lua_State * tolua_S)
{
  lua_message * lmsg = (lua_message *)tolua_tousertype(tolua_S, 1, 0);
  const char * param = tolua_tostring(tolua_S, 2, 0);
  region * value = (region *)tolua_tousertype(tolua_S, 3, 0);
  int result = msg_set_region(lmsg, param, value);
  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_msg_set(lua_State * tolua_S)
{
  tolua_Error err;
  if (tolua_isnumber(tolua_S, 3, 0, &err)) {
    return tolua_msg_set_int(tolua_S);
  } else if (tolua_isusertype(tolua_S, 3, "region", 0, &err)) {
    return tolua_msg_set_region(tolua_S);
  } else if (tolua_isusertype(tolua_S, 3, "unit", 0, &err)) {
    return tolua_msg_set_unit(tolua_S);
  }
  tolua_pushnumber(tolua_S, (lua_Number)-1);
  return 1;
}

static int
tolua_msg_send_region(lua_State * tolua_S)
{
  lua_message * lmsg = (lua_message *)tolua_tousertype(tolua_S, 1, 0);
  region * r = (region *)tolua_tousertype(tolua_S, 2, 0);
  int result = msg_send_region(lmsg, r);
  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_msg_send_faction(lua_State * tolua_S)
{
  lua_message * lmsg = (lua_message *)tolua_tousertype(tolua_S, 1, 0);
  faction * f = (faction *)tolua_tousertype(tolua_S, 2, 0);
  int result = msg_send_faction(lmsg, f);
  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

void
tolua_message_open(lua_State* tolua_S)
{
  /* register user types */
  tolua_usertype(tolua_S, "message");

  tolua_module(tolua_S, NULL, 0);
  tolua_beginmodule(tolua_S, NULL);
  {
    tolua_function(tolua_S, "message", tolua_msg_create);

    tolua_cclass(tolua_S, "message", "message", "", NULL);
    tolua_beginmodule(tolua_S, "message");
    {
      tolua_function(tolua_S, "set", tolua_msg_set);
      tolua_function(tolua_S, "set_unit", tolua_msg_set_unit);
      tolua_function(tolua_S, "set_region", tolua_msg_set_region);
      tolua_function(tolua_S, "set_resource", tolua_msg_set_resource);
      tolua_function(tolua_S, "set_int", tolua_msg_set_int);
      tolua_function(tolua_S, "set_string", tolua_msg_set_string);
      tolua_function(tolua_S, "send_faction", tolua_msg_send_faction);
      tolua_function(tolua_S, "send_region", tolua_msg_send_region);

      tolua_function(tolua_S, "create", tolua_msg_create);
    }
    tolua_endmodule(tolua_S);
  }
  tolua_endmodule(tolua_S);
}
