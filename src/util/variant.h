#pragma once

#ifndef STRUCT_VARIANT_H
#define STRUCT_VARIANT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

  typedef union variant {
    void *v;
    int i;
    char c;
    short s;
    short sa[2];
    char ca[4];
    float f;
  } variant;

  typedef enum variant_type {
    VAR_NONE, VAR_INT, VAR_VOIDPTR, VAR_CHAR, VAR_SHORT, VAR_SHORTA, VAR_CHARA,
      VAR_FLOAT
  } variant_type;

  extern const variant frac_zero;
  extern const variant frac_one;

  variant frac_make(int num, int den);
  variant frac_add(variant a, variant b);
  variant frac_sub(variant a, variant b);
  variant frac_mul(variant a, variant b);
  variant frac_div(variant a, variant b);
  int frac_sign(variant a);
  bool frac_equal(variant a, variant b);

#ifdef __cplusplus
}
#endif
#endif
