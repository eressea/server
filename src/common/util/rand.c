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
#include <ctype.h>

#define drand() (((double)rand())/(double)RAND_MAX)
#define M_PIl   3.1415926535897932384626433832795029L  /* pi */

static double nv_next;
static char valid_next = 0;

/* NormalRand aus python, random.py geklaut, dort ist Referenz auf
* den Algorithmus. mu = Mittelwert, sigma = Standardabweichung. */
double
normalvariate(double mu, double sigma)
{
  double x2pi, g2rad, z;
  double t1, t2;
  double fac=1;
  static double mu_alt, sigma_alt;

  if(mu < 10) {
    fac=0.01;
    mu*=100;
    sigma*=100;
  }

  if(mu_alt!=mu || sigma_alt!= sigma)
    valid_next=0;

  mu_alt=mu;
  sigma_alt=sigma;

  if (valid_next == 0) {
    x2pi = drand() * 2.0L * M_PIl;
    t1   = drand();
    t1   = 1.0 - t1;
    t2   = log(t1);
    g2rad = sqrt(-2.0 * t2);
    z = cos(x2pi) * g2rad;
    nv_next = sin(x2pi) * g2rad;
    valid_next = 1;
  } else {
    z = nv_next;
    valid_next = 0;
  }
  return (fac*(mu + z*sigma)); /* mu thorin */
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

