#include <platform.h>
#include "rand.h"
#include "mtrand.h"
#include "rng.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

/* do not use M_PI, use one of these instead: */
#define PI_F 3.1415926535897932384626433832795F
#define PI_D 3.1415926535897932384626433832795
#define PI_L 3.1415926535897932384626433832795L

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
        Z = sqrt(-2 * log(U)) * sin(2 * PI_D * V);
    }
    else {
        Z = sqrt(-2 * log(U)) * cos(2 * PI_D * V);
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
    return rng_double() < x;
}

typedef struct random_source {
    double (*double_source) (void);
} random_source;

random_source *r_source = 0;

double rng_injectable_double(void) {
    if (r_source)
        return r_source->double_source();
    return genrand_real2();
}

static double constant_value;

static double constant_source (void) {
    return constant_value;
}

struct random_source constant_provider = {
    constant_source
};

void random_source_inject_constant(double value) {
    constant_value = value;
    r_source = &constant_provider;
}

static double *values;
static int value_size;
static int value_index;

static double array_source (void) {
    assert(value_index<value_size);
    return values[value_index++];
}

struct random_source array_provider = {
    array_source
};

void random_source_inject_array(double inject[], int size) {
    int i;
    assert(size > 0);
    value_size = size;
    if (values)
        free(values);
    values = malloc(sizeof(double) * size);
    if (!values) abort();
    for (i=0; i < size; ++i) {
        values[i] = inject[i];
    }
    value_index = 0;
    r_source = &array_provider;
}

void random_source_reset(void) {
    if (values)
        free(values);
    values = NULL;
    r_source = NULL;
}
