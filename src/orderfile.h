#ifndef H_ORDERFILE
#define H_ORDERFILE

#include <skill.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct stream;

    void read_orderfile(const char *filename);
    void read_orders(struct stream *strm);

#ifdef __cplusplus
}
#endif
#endif
