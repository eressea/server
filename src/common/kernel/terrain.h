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
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#ifndef TERRAIN_H
#define TERRAIN_H
#ifdef __cplusplus
extern "C" {
#endif

/* diverse Flags */
/* Strassen können gebaut werden, wenn max_road > 0 */
#define LAND_REGION      (1<<0)		/* Standard-Land-struct region */
#define SEA_REGION       (1<<1)		/* hier braucht man ein Boot */
#define FOREST_REGION    (1<<2)		/* Elfen- und Kampfvorteil durch Bäume */
#define ARCTIC_REGION    (1<<3)   /* Gletscher & co = Keine Insekten! */
#define CAVALRY_REGION   (1<<4)   /* Gletscher & co = Keine Insekten! */
/* Achtung: SEA_REGION ist nicht das Gegenteil von LAND_REGION. Die zwei schliessen sich nichtmal aus! */
#define FORBIDDEN_REGION (1<<5)		/* unpassierbare Blockade-struct region */
#define SAIL_INTO		     (1<<6)		/* man darf hierhin segeln */
#define FLY_INTO		     (1<<7)		/* man darf hierhin fliegen */
#define SWIM_INTO		     (1<<8)		/* man darf hierhin schwimmen */
#define WALK_INTO		     (1<<9)		/* man darf hierhin laufen */
#define LARGE_SHIPS		   (1<<10)		/* grosse Schiffe dürfen hinfahren */

#define NORMAL_TERRAIN	(WALK_INTO|SAIL_INTO|FLY_INTO)

typedef struct production_rule {
	char * name;
	const struct resource_type * rtype;

	void (*terraform) (struct production_rule *, const struct region *);
	void (*update) (struct production_rule *, const struct region *);
	void (*use) (struct production_rule *, const struct region *, int amount);
	int (*visible) (const struct production_rule *, int skilllevel);

	/* no initialization required */
	struct production_rule * next;
} production_rule;

typedef struct terrain_production {
  const struct resource_type * type;
	const char *startlevel;
	const char *base;
	const char *divisor;
	float chance;
} terrain_production;

typedef struct terrain_type {
  char * _name;
  int size;         /* how many peasants can work? */
  unsigned int flags;
  short max_road;   /* this many stones make a full road */
  struct terrain_production * production;
  const struct item_type ** herbs; /* zero-terminated array of herbs */
  const char * (*name)(const struct region * r);
  const struct terrain_type * next;
} terrain_type;

extern const terrain_type * terrains(void);
extern void register_terrain(struct terrain_type * terrain);
extern const struct terrain_type * get_terrain(const char * name);
extern const char * terrain_name(const struct region * r);

extern void init_terrains(void);

#ifdef __cplusplus
}
#endif
#endif /* TERRAIN_H */
