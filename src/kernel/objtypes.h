#ifndef H_KRNL_OBJTYPES
#define H_KRNL_OBJTYPES
#ifdef __cplusplus
extern "C" {
#endif

  typedef enum {
    TYP_UNIT,
    TYP_REGION,
    TYP_BUILDING,
    TYP_SHIP,
    TYP_FACTION,
    TYP_ACTION,
    TYP_TRIGGER,
    TYP_TIMEOUT
  } objtype_t;

#ifdef __cplusplus
}
#endif
#endif
