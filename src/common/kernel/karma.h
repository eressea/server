/* vi: set ts=2:
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
#ifndef H_KRNL_KARMA
#define H_KRNL_KARMA
#ifdef __cplusplus
extern "C" {
#endif

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
	FS_UNDEAD,
	FS_QUICK,
	FS_LUCKY,
	FS_LYCANTROPE,
	FS_ELITE,
	MAXFACTIONSPECIALS
} fspecial_t;

#ifdef KARMA_MODULE
typedef enum {
	PR_AID,
	PR_MERCY,
	MAXPRAYEREFFECTS
} prayereffect_t;

typedef struct fspecialdata {
	const char  *name;
	const char  *description;
	const short maxlevel;
} fspecialdata;

extern struct attrib_type at_faction_special;
extern struct attrib_type at_prayer_timeout;
extern struct attrib_type at_prayer_effect;
extern struct attrib_type at_wyrm;
extern struct attrib_type at_fshidden;
extern struct attrib_type at_jihad;

extern struct fspecialdata fspecials[];

extern int fspecial(const struct faction *f, fspecial_t special);
extern void karma(void);
extern int jihad(struct faction *, const struct race *);
extern void jihad_attacks(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
