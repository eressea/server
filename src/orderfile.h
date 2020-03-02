#ifndef H_ORDERFILE
#define H_ORDERFILE

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct input {
        const char *(*getbuf)(void *data);
        void *data;
    } input;

    int parseorders(FILE *F);

#ifdef __cplusplus
}
#endif
#endif
