/* vi: set ts=2:
 *
 *	$Id: karma.h,v 1.2 2001/01/26 16:19:39 enno Exp $
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
#ifndef KARMA_H
#define KARMA_H

struct faction;

typedef enum {
	FS_REGENERATION,
	FS_URBAN,
	FS_WARRIOR,
	FS_WYRM,
	FS_MILITIA,
	FS_FAERIE,
	FS_ADMINISTRATOR,
	FS_TELEPATHY,
	FS_AMPHIBIAN,
	FS_MAGOCRACY,
	FS_SAPPER,
	FS_JUMPER,
	FS_HIDDEN,
	FS_EARTHELEMENTALIST,
	FS_MAGICIMMUNE,
	FS_VARIEDMAGIC,
	FS_JIHAD,
	MAXFACTIONSPECIALS
} fspecial_t;

typedef enum {
	PR_AID,
	PR_MERCY,
	MAXPRAYEREFFECTS
} prayereffect_t;

typedef struct fspecialdata fspecialdata;
struct fspecialdata {
	const char *name;
	const char *description;
	boolean levels;
};

extern struct attrib_type at_faction_special;
extern struct attrib_type at_prayer_timeout;
extern struct attrib_type at_prayer_effect;
extern struct attrib_type at_wyrm;
extern struct attrib_type at_fshidden;
extern struct attrib_type at_jihad;

extern struct fspecialdata fspecials[];

int fspecial(const struct faction *f, fspecial_t special);
void set_jihad(void);
int jihad(struct faction *f, race_t race);
void jihad_attacks(void);

#endif
