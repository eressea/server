/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef RAND_H
#define RAND_H
#ifdef __cplusplus
extern "C" {
#endif

extern int dice_rand(const char *str);
extern int dice(int count, int value);
extern double normalvariate(double mu, double sigma);
extern int ntimespprob(int n, double p, double mod);
extern boolean chance(double x);

#ifdef __cplusplus
}
#endif
#endif
