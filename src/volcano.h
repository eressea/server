#ifndef H_GC_VOLCANO
#define H_GC_VOLCANO

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct region;
    struct unit;
    
    int volcano_damage(struct unit* u, const char* dice);
    void volcano_outbreak(struct region * r, struct region *rn);
    void volcano_update(void);
    bool volcano_module(void);
    
#ifdef __cplusplus
}
#endif
#endif
