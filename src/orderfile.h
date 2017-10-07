#ifndef H_ORDERFILE
#define H_ORDERFILE

#include <skill.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct input {
        const char *(*getbuf)(void *data);
        void *data;
    } input;

    int read_orders(struct input *in);
    int readorders(const char *filename);

#ifdef __cplusplus
}
#endif
#endif
