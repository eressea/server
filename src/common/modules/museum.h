/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

extern attrib_type at_warden;
extern attrib_type at_museumexit;
extern attrib_type at_museumgivebackcookie;
extern attrib_type at_museumgiveback;

typedef struct {
	int       warden_no;
	int       cookie;
} museumgivebackcookie;

typedef struct {
	int          cookie;
	struct item *items;
} museumgiveback;

void warden_add_give(struct unit *src, struct unit *u, const struct item_type *itype, int n);
void init_museum(void);
void create_museum(void);
int use_museumticket(struct unit *u, const struct item_type *itype, const char *cmd);
int use_museumexitticket(struct unit *u, const struct item_type *itype, const char * cmd);

