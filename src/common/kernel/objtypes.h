/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */


#ifndef H_KRNL_OBJTYPES
#define H_KRNL_OBJTYPES
#ifdef __cplusplus
extern "C" {
#endif

typedef struct obj_ID {
	int a, b;
} obj_ID;

enum {
	TYP_UNIT,
	TYP_REGION,
	TYP_BUILDING,
	TYP_SHIP,
	TYP_FACTION,
	TYP_ACTION,
	TYP_TRIGGER,
	TYP_TIMEOUT
};

extern obj_ID get_ID(void *obj, typ_t typ);
extern void write_ID(FILE *f, obj_ID id);
extern obj_ID read_ID(FILE *f);

extern void add_ID_resolve(obj_ID id, void *objPP, typ_t typ);
extern void resolve_IDs(void);


typedef obj_ID (*ID_fun)(void *obj);
typedef void *(*find_fun)(obj_ID id);
typedef char *(*desc_fun)(void *obj);
typedef attrib **(*attrib_fun)(void *obj);
typedef void (*destroy_fun)(void *obj);
typedef void *(*deref_fun)(void *obj);
typedef void (*set_fun)(void *ptrptr, void *obj);	/* 	*ptrptr = obj  */


typedef struct {
	ID_fun		getID;		/* liefert obj_ID zu struct unit* */
	find_fun	find;		/* liefert struct unit* zu obj_ID  */
	desc_fun	getname;	/* unitname() */
	attrib_fun	getattribs;	/* liefert &u->attribs */
	destroy_fun destroy;	/* destroy_unit() */
	deref_fun	ppget;		/* liefert struct unit* aus struct unit**  */
	set_fun		ppset;		/* setzt *(struct unit **) zu struct unit*  */
} typdata_t;

extern typdata_t typdata[];

#ifdef __cplusplus
}
#endif
#endif

