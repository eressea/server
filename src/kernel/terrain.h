#ifndef TERRAIN_H
#define TERRAIN_H

#ifdef __cplusplus
extern "C" {
#endif

    /* diverse Flags */
    /* Strassen und Gebaeude koennen gebaut werden, wenn max_road > 0 */
#define LAND_REGION      (1<<0) /* Standard-Land-struct region */
#define SEA_REGION       (1<<1) /* hier braucht man ein Boot */
#define FOREST_REGION    (1<<2) /* Elfen- und Kampfvorteil durch Baeume */
#define ARCTIC_REGION    (1<<3) /* Gletscher & co = Keine Insekten! */
#define CAVALRY_REGION   (1<<4) /* riding in combat is possible */
    /* Achtung: SEA_REGION ist nicht das Gegenteil von LAND_REGION. Die zwei schliessen sich nichtmal aus! */
#define FORBIDDEN_REGION (1<<5) /* unpassierbare Blockade-struct region */
#define FLY_INTO		     (1<<7)     /* man darf hierhin fliegen */
#define SWIM_INTO		     (1<<8)     /* man darf hierhin schwimmen */
#define WALK_INTO		     (1<<9)     /* man darf hierhin laufen */

    extern const char *terrainnames[];

    typedef struct production_rule {
        char *name;
        const struct resource_type *rtype;

        void(*terraform) (struct production_rule *, const struct region *);
        void(*update) (struct production_rule *, const struct region *);
        void(*use) (struct production_rule *, const struct region *, int amount);
        int(*visible) (const struct production_rule *, int skilllevel);

        /* no initialization required */
        struct production_rule *next;
    } production_rule;

    typedef struct terrain_production {
        const struct resource_type *type;
        char *startlevel;
        char *base;
        char *divisor;
        float chance;
    } terrain_production;

    typedef struct terrain_type {
        char *_name;
        int size;                   /* how many peasants can work? */
        int flags;
        short max_road;             /* this many stones make a full road */
        short distribution;         /* multiplier used for seeding */
        struct terrain_production *production;
        struct item_type **herbs;     /* zero-terminated array of herbs */
        const char *(*name) (const struct region * r);
        struct terrain_type *next;
    } terrain_type;

    terrain_type *get_or_create_terrain(const char *name);
    const terrain_type *terrains(void);
    const terrain_type *get_terrain(const char *name);
    const char *terrain_name(const struct region *r);
    bool terrain_changed(int *cache);

    void init_terrains(void);
    void free_terrains(void);

#ifdef __cplusplus
}
#endif
#endif                          /* TERRAIN_H */
