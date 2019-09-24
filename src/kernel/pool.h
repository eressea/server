#ifndef H_KRNL_POOL_H
#define H_KRNL_POOL_H
#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct resource_type;
    struct item_type;
    struct region;

    /* bitfield values for get/use/change operations */
#define GET_SLACK      0x01
#define GET_RESERVE    0x02

#define GET_POOLED_SLACK    0x08
#define GET_POOLED_RESERVE  0x10
#define GET_POOLED_FORCE    0x20        /* ignore f->options pools */
#define GET_ALLIED_SLACK    0x40
#define GET_ALLIED_RESERVE  0x80

    /* for convenience: */
#define GET_DEFAULT (GET_RESERVE|GET_SLACK|GET_POOLED_SLACK)
#define GET_ALL (GET_SLACK|GET_RESERVE|GET_POOLED_SLACK|GET_POOLED_RESERVE|GET_POOLED_FORCE)

    int get_pooled(const struct unit *u, const struct resource_type *res,
        int mode, int count);
    int use_pooled(struct unit *u, const struct resource_type *res,
        int mode, int count);
    /** use_pooled
     * verbraucht 'count' Objekte der resource 'itm'
     * unter zuhilfenahme des Pools der struct region und Aufbrauch des
     * von der Einheit reservierten Resourcen
     */

    int get_resource(const struct unit *u, const struct resource_type *res);
    int change_resource(struct unit *u, const struct resource_type *res,
        int change);

    int get_reservation(const struct unit *u, const struct item_type *res);
    int change_reservation(struct unit *u, const struct item_type *res,
        int value);

    int set_resvalue(struct unit * u, const struct item_type * rtype, int value);

#ifdef __cplusplus
}
#endif
#endif
