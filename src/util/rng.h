#ifndef UTIL_RNG_H
#define UTIL_RNG_H

#define RNG_MT

#ifdef __cplusplus
extern "C" {
#endif

    /* generates a random number on [0,1)-real-interval */
    double rng_injectable_double(void);

#ifdef RNG_MT
# include "mtrand.h"
# define rng_init(seed) init_genrand(seed)
# define rng_int (int)genrand_int31
# define rng_uint (unsigned int)genrand_int32
# define rng_double rng_injectable_double
# define RNG_RAND_MAX 0x7fffffff
#else
# include <stdlib.h>
# define rng_init(seed) srand(seed)
# define rng_int rand()
# define rng_double ((rand()%RAND_MAX)/(double)RAND_MAX)
# define RNG_RAND_MAX RAND_MAX
#endif
#define RAND_ROUND(fractional) ((rng_double() < fractional-(int)fractional)?((int)fractional+1):((int)fractional))
#ifdef __cplusplus
}
#endif
#endif
