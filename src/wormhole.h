#ifndef H_MOD_WORMHOLE
#define H_MOD_WORMHOLE
#ifdef __cplusplus
extern "C" {
#endif

    struct region;
    struct building;

    void wormholes_update(void);
    void wormholes_register(void);
    void wormhole_transfer(struct building *b, struct region *exit);

#ifdef __cplusplus
}
#endif
#endif
