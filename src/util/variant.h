#ifndef STRUCT_VARIANT_H
#define STRUCT_VARIANT_H
#ifdef __cplusplus
extern "C" {
#endif

  typedef union variant {
    void *v; //TODO: V117 http://www.viva64.com/en/V117 Memsize type is used in the union.
    int i;
    char c;
    short s;
    short sa[2];
    char ca[4]; //TODO: V112 http://www.viva64.com/en/V112 Dangerous magic number 4 used: char ca[4];.
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
