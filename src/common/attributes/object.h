/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
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
  TUNIT = 10, TFACTION = 11, TREGION = 12, TBUILDING = 13, TSHIP = 14, 
} object_type;

extern attrib_type at_object;

extern struct attrib * object_create(const char * name, object_type type, variant value);
extern void object_get(const struct attrib * a, variant * value, object_type * type);
extern const char * object_name(const struct attrib * a);

#ifdef __cplusplus
}
#endif
#endif

