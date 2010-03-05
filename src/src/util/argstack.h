#ifndef UTIL_ARG_H
#define UTIL_ARG_H
#ifdef __cplusplus
extern "C" {
#endif

struct arguments;

#define ARG_TNUMBER -1
#define ARG_TSTRING -2

void arg_init(struct arguments * arg);
void arg_done(struct arguments * arg);

void arg_push(struct arguments * arg, int type, void * data);
void arg_pushstring(struct arguments * arg, const char * str);
void arg_pushnumber(struct arguments * arg, double number);

int arg_size(struct arguments * arg);
void * arg_touserdata(struct arguments * arg, int idx, int type);
double arg_tonumber(struct arguments * arg, int idx);
const char * arg_tostring(struct arguments * arg, int idx);

#ifdef __cplusplus
}
#endif
#endif
