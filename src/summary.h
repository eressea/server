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

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct summary;

    void report_summary(const struct summary *sum, bool full);
    struct summary *make_summary(void);
    void free_summary(struct summary *sum);
    int update_nmrs(void);
    extern int* nmrs;


#ifdef __cplusplus
}
#endif
#endif
