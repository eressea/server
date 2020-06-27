#ifndef H_KRNL_PLANES
#define H_KRNL_PLANES
#ifdef __cplusplus
extern "C" {
#endif

    struct faction;
    struct region;
    struct faction;
    struct plane;
    struct storage;

#define PFL_NOCOORDS        1   /* not implemented */
#define PFL_NORECRUITS      2   /* cannot recruit */
#define PFL_NOALLIANCES     4   /* not implemented */
#define PFL_LOWSTEALING     8   /* not implemented */
#define PFL_NOGIVE         16   /* Uebergaben sind unmoeglich */
#define PFL_NOATTACK       32   /* Angriffe und Diebstaehle sind unmoeglich */
#define PFL_NOTERRAIN      64   /* Terraintyp wird nicht angezeigt TODO? */
#define PFL_NOMAGIC       128   /* Zaubern ist unmoeglich */
#define PFL_NOSTEALTH     256   /* Tarnung ausser Betrieb */
#define PFL_NOTEACH       512   /* Lehre ausser Betrieb */
#define PFL_NOBUILD      1024   /* Bauen ausser Betrieb */
#define PFL_NOFEED       2048   /* Kein Unterhalt noetig */
#define PFL_FRIENDLY     4096   /* not implemented */
#define PFL_NOORCGROWTH  8192   /* orcs don't grow */
#define PFL_NOMONSTERS  16384   /* no monster randenc */

    typedef struct plane {
        struct plane *next;
        int id;
        char *name;
        int minx, maxx, miny, maxy;
        int flags;
        struct attrib *attribs;
    } plane;

#define plane_id(pl) ( (pl) ? (pl)->id : 0 )

    extern struct plane *planes;

    struct plane *getplane(const struct region *r);
    struct plane *findplane(int x, int y);
    int getplaneid(const struct region *r);
    struct plane *getplanebyid(int id);
    int plane_center_x(const struct plane *pl);
    int plane_center_y(const struct plane *pl);
    struct plane *create_new_plane(int id, const char *name, int minx, int maxx,
        int miny, int maxy, int flags);
    struct plane *getplanebyname(const char *);
    struct plane *get_homeplane(void);
    int rel_to_abs(const struct plane *pl, const struct faction *f,
        int rel, unsigned char index);
    int plane_width(const plane * pl);
    int plane_height(const plane * pl);
    void adjust_coordinates(const struct faction *f, int *x, int *y, const struct plane *pl);

    void free_plane(struct plane *pl);
    void remove_plane(struct plane *pl);
#ifdef __cplusplus
}
#endif
#endif
