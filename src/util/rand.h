#pragma once
/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

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
