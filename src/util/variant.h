#ifndef STRUCT_VARIANT_H
#define STRUCT_VARIANT_H
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

#ifdef __cplusplus
}
#endif
#endif
