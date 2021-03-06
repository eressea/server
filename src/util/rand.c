#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include "rand.h"
#include "mtrand.h"
#include "rng.h"

#include <stddef.h>
#include <math.h>

int lovar(double xpct_x2)
{
    int n = (int)(xpct_x2 * 500) + 1;
    if (n == 0)
        return 0;
    return (rng_int() % n + rng_int() % n) / 1000;
}

/* gaussian distribution
 * taken from http://c-faq.com/lib/gaussian.html
 */

double normalvariate(double mu, double sigma)
{
    static double U, V;
    static int phase = 0;
    double Z;

    if (phase == 0) {
        U = (rng_int() + 1.) / (RNG_RAND_MAX + 2.);
        V = rng_int() / (RNG_RAND_MAX + 1.);
        Z = sqrt(-2 * log(U)) * sin(2 * M_PI * V);
    }
    else {
        Z = sqrt(-2 * log(U)) * cos(2 * M_PI * V);
    }
    phase = 1 - phase;

    return mu + Z *sigma;
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
    return (1-rng_double()) < x;
}

typedef struct random_source {
    double (*double_source) (void);
    int (*int32_source) (void);
} random_source;

random_source *r_source = 0;

double rng_injectable_double(void) {
    if (r_source && r_source->double_source)
        return r_source->double_source();
    return genrand_real2();
}

int rng_injectable_int(void) {
    if (r_source && r_source->int32_source)
        return r_source->int32_source();
    return (int)genrand_int31();
}

static double constant_value_double;
static int constant_value_int32;

static double constant_double(void) {
    return constant_value_double;
}

static int constant_int32(void) {
    return constant_value_int32;
}

struct random_source constant_provider = {
    constant_double, NULL
};

void random_source_inject_constant(double dval) {
    constant_value_double = dval;
    r_source = &constant_provider;
}

struct random_source constants_provider = {
    constant_double, constant_int32
};

void random_source_inject_constants(double dval, int ival) {
    constant_value_double = dval;
    constant_value_int32 = ival;
    r_source = &constants_provider;
}

void random_source_reset(void) {
    r_source = NULL;
}
