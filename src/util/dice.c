/*
Copyright (c) 2010-2015, Enno Rehling <enno@eressea.de>,
Christian Schlittchen <corwin@amber.kn-bremen.de>,
Katja Zedel <katze@felidae.kn-bremen.de>,
Henning Peters <faroul@beyond.kn-bremen.de>

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

#include <platform.h>
#include "rng.h"

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

/** rolls a number of n-sided dice.
 * Usage: 3d6-3d4+5 = dice(3,6)-dice(3,4)+5 */
int dice(int count, int value)
{
    int d = 0, c;

    if (value <= 0)
        return 0;                   /* (enno) %0 geht nicht. echt wahr. */
    if (count >= 0)
        for (c = count; c > 0; c--)
            d += rng_int() % value + 1;
    else
        for (c = count; c < 0; c++)
            d -= rng_int() % value + 1;

    return d;
}

static int term_eval(const char **sptr)
{
    const char *c = *sptr;
    int m = 0, d = 0, k = 0, term = 1, multi = 1;
    int state = 1;

    for (;;) {
        if (isdigit(*(unsigned char *)c)) {
            k = k * 10 + (*c - '0');
        }
        else if (*c == '+' || *c == '-' || *c == 0 || *c == '*' || *c == ')'
            || *c == '(') {
            if (state == 1)           /* konstante k addieren */
                m += k * multi;
            else if (state == 2) {    /* dDk */
                int i;
                if (k == 0)
                    k = 6;                /* 3d == 3d6 */
                for (i = 0; i != d; ++i)
                    m += (1 + rng_int() % k) * multi;
            }
            else
                assert(!"dice_rand: illegal token");
            if (*c == '*') {
                term *= m;
                m = 0;
            }
            k = d = 0;
            state = 1;
            multi = (*c == '-') ? -1 : 1;

            if (*c == '(') {
                ++c;
                k = term_eval(&c);
            }
            else if (*c == 0 || *c == ')') {
                break;
            }
        }
        else if (*c == 'd' || *c == 'D') {
            if (k == 0)
                k = 1;                  /* d9 == 1d9 */
            assert(state == 1 || !"dice_rand: illegal token");
            d = k;
            k = 0;
            state = 2;
        }
        c++;
    }
    *sptr = c;
    return m * term;
}

int dice_rand(const char *s)
{
    return term_eval(&s);
}
