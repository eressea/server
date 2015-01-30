/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#ifndef H_ATTRIBUTE_OBJECT
#define H_ATTRIBUTE_OBJECT

#include <util/variant.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        TNONE = 0, TINTEGER = 1, TREAL = 2, TSTRING = 3,
        TUNIT = 10, TFACTION = 11, TREGION = 12, TBUILDING = 13, TSHIP = 14
    } dict_type;

    extern struct attrib_type at_dict;

    struct attrib *dict_create(const char *name, dict_type type,
        variant value);
    void dict_get(const struct attrib *a, dict_type * type,
        variant * value);
    void dict_set(struct attrib *a, dict_type type, variant value);
    const char *dict_name(const struct attrib *a);

#ifdef __cplusplus
}
#endif
#endif
