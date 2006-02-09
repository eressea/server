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
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "rand.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

#define drand() ((rand()%RAND_MAX)/(double)RAND_MAX)
#define M_PIl   3.1415926535897932384626433832795029L  /* pi */

static double nv_next;
static char valid_next = 0;

/* NormalRand aus python, random.py geklaut, dort ist Referenz auf
* den Algorithmus. mu = Mittelwert, sigma = Standardabweichung. */
double
normalvariate(double mu, double sigma)
{
  static double NV_MAGICCONST = 1.7155277699214135;
  double z;
  for (;;) {
    double u1 = drand();
    double u2 = 1.0 - drand();
    z = NV_MAGICCONST*(u1-0.5)/u2;
    if (z*z/4.0 <= -log(u2)) {
      break;
    }
  }
  return mu+z*sigma;
}

int
ntimespprob(int n, double p, double mod)
{
  int count = 0;
  int i;

  for(i=0; i<n && p>0; i++) {
    if(drand() < p) {
      count++;
      p += mod;
    }
  }
  return count;
}

boolean
chance(double x)
{
  if (x>=1.0) return true;
  return (boolean) (rand() % RAND_MAX < RAND_MAX * x);
}

