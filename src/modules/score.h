#ifndef SCORE_H
#define SCORE_H

#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

    struct item_type;
    typedef long long score_t;

    void score(void);
    score_t average_score_of_age(int age, int a);
    int default_score(const struct item_type *itype);
    void write_score(char *buffer, size_t size, score_t score);

#ifdef __cplusplus
}
#endif
#endif
