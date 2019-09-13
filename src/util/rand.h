#pragma once
#ifndef RAND_H
#define RAND_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    /* in dice.c: */
    int dice_rand(const char *str);
    int dice(int count, int value);

    /* in rand.c: */
    int lovar(double xpct_x2);
    double normalvariate(double mu, double sigma);
    int ntimespprob(int n, double p, double mod);
    bool chance(double x);

    /* a random source that generates numbers in [0, 1).
       By calling the random_source_inject... functions you can set a special random source,
       which is handy for testing */
    double rng_injectable_double(void);

    void random_source_inject_constant(double value);
    void random_source_inject_array(double inject[], int size);
    void random_source_reset(void);

#ifdef __cplusplus
}
#endif
#endif
