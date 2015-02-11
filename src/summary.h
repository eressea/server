/* 
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2007   |
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *
 */

#ifndef H_GC_SUMMARY
#define H_GC_SUMMARY

#ifdef __cplusplus
extern "C" {
#endif

    struct summary;

    void report_summary(struct summary *n, struct summary *o, bool full);
    struct summary *make_summary(void);

    int update_nmrs(void);
    extern int* nmrs;


#ifdef __cplusplus
}
#endif
#endif
