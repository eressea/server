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

struct attrib_type;
struct attrib;

#ifdef __cplusplus
extern "C" {
#endif

    extern struct attrib_type at_dict; // DEPRECATED: at_dict has been replaced with at_keys

    void dict_set(struct attrib * a, const char * name, int value);

#ifdef __cplusplus
}
#endif
#endif
