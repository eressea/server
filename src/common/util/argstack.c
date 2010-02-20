#include "argstack.h"
#include <assert.h>

#define ARGSTKSIZE 16
typedef struct arguments {
  int n;
  struct value {
    int type;
    union {
      double number;
      const char * str;
      void * data;
    } value;
  } stack[ARGSTKSIZE];
} arguments;

void arg_init(struct arguments * arg) {
  arg->n = 0;
}

void arg_done(struct arguments * arg) {
  arg = arg;
}

void arg_pushuserdata(struct arguments * arg, int type, void * data) {
  assert(arg->n<ARGSTKSIZE);
  arg->stack[arg->n].type = type;
  arg->stack[arg->n].value.data = data;
  ++arg->n;
}

void arg_pushstring(struct arguments * arg, const char * str) {
  assert(arg->n<ARGSTKSIZE);
  arg->stack[arg->n].type = ARG_TSTRING;
  arg->stack[arg->n].value.str = str;
  ++arg->n;
}

void arg_pushnumber(struct arguments * arg, double number) {
  assert(arg->n<ARGSTKSIZE);
  arg->stack[arg->n].type = ARG_TSTRING;
  arg->stack[arg->n].value.number = number;
  ++arg->n;
}

int arg_size(struct arguments * arg) {
  return arg->n;
}

void * arg_touserdata(struct arguments * arg, int idx, int type) {
  assert(arg->n>idx && idx>=0);
  assert(arg->stack[arg->n].type==type);
  return arg->stack[arg->n].value.data;
}

double arg_tonumber(struct arguments * arg, int idx) {
  assert(arg->n>idx && idx>=0);
  assert(arg->stack[arg->n].type==ARG_TNUMBER);
  return arg->stack[arg->n].value.number;
}

const char * arg_tostring(struct arguments * arg, int idx) {
  assert(arg->n>idx && idx>=0);
  assert(arg->stack[arg->n].type==ARG_TSTRING);
  return arg->stack[arg->n].value.str;
}
