/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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
#include "cvector.h"
#include "rng.h"

#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <memory.h>

void
cv_init(cvector * cv)
{
  cv->begin = 0;
  cv->end = 0;
  cv->space = 0;
}

cvector *
cv_kill(cvector * cv)
{
  if (cv->begin) free(cv->begin);
  cv_init(cv);
  return cv;
}

size_t
cv_size(cvector * cv)
{
  return cv->end - cv->begin;
}

void
cv_reserve(cvector * cv, size_t size)
{
  size_t count = cv->end - cv->begin;
  cv->begin = realloc(cv->begin, size * sizeof(void *));

  cv->space = size;
  cv->end = cv->begin + count;
}

void
cv_pushback(cvector * cv, void *u)
{
  if (cv->space == cv_size(cv))
    cv_reserve(cv, cv->space ? cv->space * 2 : 2);
  *(cv->end++) = u;
}

int
__cv_scramblecmp(const void *p1, const void *p2)
{
  return *((long *) p1) - *((long *) p2);
}

#define addptr(p,i)         ((void *)(((char *)p) + i))

/** randomly shuffle an array
 * for correctness, see Donald E. Knuth, The Art of Computer Programming
 */
static void
__cv_scramble(void *v1, size_t n, size_t width)
{
  size_t i;
  void * temp = malloc(width);
  for (i=0;i!=n;++i) {
    size_t j = i + (rng_int() % (n-i));
    memcpy(temp, addptr(v1, i*width), width);
    memcpy(addptr(v1, i*width), addptr(v1, j*width), width);
    memcpy(addptr(v1, j*width), temp, width);
  }
  free(temp);
}

void
v_scramble(void **begin, void **end)
{
  __cv_scramble(begin, end - begin, sizeof(void *));
}
