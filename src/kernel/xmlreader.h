/* vi: set ts=2:
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2007   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#ifndef H_KRNL_XMLREADER_H
#define H_KRNL_XMLREADER_H
#ifdef __cplusplus
extern "C" {
#endif
    extern void register_xmlreader(void);

    /* game-specific callbacks */
    extern void(*set_spelldata_cb) (struct spell * sp);
#ifdef __cplusplus
}
#endif
#endif
