#ifndef H_CALLBACK_H
#define H_CALLBACK_H

#include <stdarg.h>

typedef struct {
    void (*cbv)(va_list va);
} CALLBACK;

CALLBACK register_callback(const char *name, void (*cbv)(va_list va));
CALLBACK create_callback(void (*cbv)(va_list va));
int find_callback(const char *name, CALLBACK *result);
int call_callback(CALLBACK cb, const char *name, ...);
void reset_callbacks(void);

#endif
