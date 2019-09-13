#ifndef H_MOD_XMAS
#define H_MOD_XMAS
#ifdef __cplusplus
extern "C" {
#endif

  struct region;
  struct unit;

  extern struct trigger *trigger_xmasgate(struct building *b);

  extern void register_xmas(void);

#ifdef __cplusplus
}
#endif
#endif
