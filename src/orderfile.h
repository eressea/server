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

    int read_orders(struct input *in);
    int readorders(FILE *F);
    int parseorders(FILE *F);

#ifdef __cplusplus
}
#endif
#endif
