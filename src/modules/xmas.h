#ifndef H_MOD_XMAS
#define H_MOD_XMAS

struct building;
struct trigger;

struct trigger *trigger_xmasgate(struct building *b);

void register_xmas(void);

#endif
