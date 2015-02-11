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

#include <platform.h>
#include "rand.h"
#include "rng.h"

#include <assert.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

#define M_PIl   3.1415926535897932384626433832795029L   /* pi */

/* NormalRand aus python, random.py geklaut, dort ist Referenz auf
* den Algorithmus. mu = Mittelwert, sigma = Standardabweichung.
* http://de.wikipedia.org/wiki/Standardabweichung#Diskrete_Gleichverteilung.2C_W.C3.BCrfel
*/
double normalvariate(double mu, double sigma)
{
    static const double NV_MAGICCONST = 1.7155277699214135;       /* STATIC_CONST: a constant */
    double z;
    for (;;) {
        double u1 = rng_double();
        double u2 = 1.0 - rng_double();
        z = NV_MAGICCONST * (u1 - 0.5) / u2;
        if (z * z / 4.0 <= -log(u2)) {
            break;
        }
    }
    return mu + z * sigma;
}

int ntimespprob(int n, double p, double mod)
{
    int count = 0;
    int i;

    for (i = 0; i < n && p > 0; i++) {
        if (rng_double() < p) {
            count++;
            p += mod;
        }
    }
    return count;
}

bool chance(double x)
{
    if (x >= 1.0)
        return true;
    return rng_double() < x;
}
