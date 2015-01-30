#ifndef H_CALLBACK_H
#define H_CALLBACK_H

#include <stdarg.h>

typedef struct {
    void(*cbv)(va_list va);
} HCALLBACK;

HCALLBACK register_callback(const char *name, void(*cbv)(va_list va));
HCALLBACK create_callback(void(*cbv)(va_list va));
int find_callback(const char *name, HCALLBACK *result);
int call_callback(HCALLBACK cb, const char *name, ...);
void reset_callbacks(void);

#endif
