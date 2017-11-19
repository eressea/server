/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

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

#define PFL_NOCOORDS        1   /* not in use */
#define PFL_NORECRUITS      2
#define PFL_NOALLIANCES     4
#define PFL_LOWSTEALING     8
#define PFL_NOGIVE         16   /* �bergaben sind unm�glich */
#define PFL_NOATTACK       32   /* Angriffe und Diebst�hle sind unm�glich */
#define PFL_NOTERRAIN      64   /* Terraintyp wird nicht angezeigt TODO? */
#define PFL_NOMAGIC       128   /* Zaubern ist unm�glich */
#define PFL_NOSTEALTH     256   /* Tarnung au�er Betrieb */
#define PFL_NOTEACH       512   /* Lehre au�er Betrieb */
#define PFL_NOBUILD      1024   /* Bauen au�er Betrieb */
#define PFL_NOFEED       2048   /* Kein Unterhalt n�tig */
#define PFL_FRIENDLY     4096   /* everyone is your ally */
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
