/* vi: set ts=2:
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

#ifndef CURSE_H
#define CURSE_H

/* ------------------------------------------------------------- */

/* Sprueche in der struct region und auf Einheiten, Schiffen oder Burgen
 * (struct attribute)
 */

/* Brainstorming Überarbeitung curse
 * 
 * Ziel: Keine Enum-Liste, flexible, leicht erweiterbare Curse-Objekte
 *
 * Was wird gebraucht?
 * - Eindeutige Kennung für globale Suche
 * - eine Wirkung, die sich einfach 'anwenden' läßt, dabei flexibel ist,
 *   Raum läßt für variable Boni, Anzahl betroffener Personen,
 *   spezielle Effekte oder anderes
 * - einfacher Zugriff auf allgemeine Funktionen wie zb Alterung, aber
 *   auch Antimagieverhalten
 * - Ausgabe von Beschreibungen in verschiedenen Sprachen
 * - definiertes gekapseltes Verhalten zb bei Zusammenlegung von
 *   Einheiten, Übergabe von Personen, Mehrfachverzauberung
 * - (Rück-)Referenzen auf Objekt, Verursacher (Magier), ?
 *
 * Vieleicht wäre ein Wirkungsklassensystem sinnvoll, so das im übrigen
 * source einfach alle curse-attribs abgefragt werden können und bei
 * gewünschter Wirkungsklasse angewendet, also nicht für jeden curse
 * spezielle Änderungen im übrigen source notwendig sind.
 *
 * Die (Wirkungs-)Typen sollten die wichtigen Funktionen speziell
 * belegen können, zb Alterung, Ausgabetexte, Merge-Verhalten
 *
 * Was sind wichtige individuelle Eigenschaften?
 * - Referenzen auf Objekt und Verursacher
 * - Referenzen für globale Liste
 * > die Wirkung:
 * - Dauer
 * - Widerstandskraft, zb gegen Antimagie
 * - Seiteneffekte zb Flag ONLYONE, Unverträglichkeiten
 * - Alterungsverhalten zb Flag NOAGE
 * - Effektverhalten, zb Bonus (variabel)
 * - bei Einheitenzaubern die Zahl der betroffenen Personen
 * 
 * Dabei sind nur die beiden letzten Punkte wirklich reine individuelle
 * Wirkung, die anderen sind eher allgemeine Kennzeichen eines jeden
 * Curse, die individuell belegt sind.
 * ONLYONE und NOAGE könnten auch Eigenschaften des Typs sein, nicht des
 * individuellen curse. Allgemein ist Alterung wohl eher eine
 * Typeigenschaft als die eines individuellen curse. 
 * Dagegen ist der Widerstand gegen Antimagie sowohl abhängig von der
 * Güte des Verursachers, also des Magiers zum Zeitpunkt des Zaubers,
 * als auch vom Typ, der gegen bestimmte Arten des 'Fluchbrechens' immun
 * sein könnte.
 *
 * Was sind wichtige Typeigenschaften?
 * - Verhalten bei Personenübergaben
 * - allgemeine Wirkung
 * - Beschreibungstexte
 * - Verhalten bei Antimagie
 * - Alterung
 * - Speicherung des C-Objekts
 * - Laden des C-Objekts
 * - Erzeugen des C-Objekts
 * - Löschen und Aufräumen des C-Objekts
 * - Funktionen zur Änderung der Werte
 *
 * */

/* ------------------------------------------------------------- */
/* Zauberwirkungen */
/* nicht vergessen curse_type zu aktualisieren und Reihenfolge beachten!
 */

enum {
/* struct's vom typ curse: */
	C_FOGTRAP,
	C_ANTIMAGICZONE,
	C_FARVISION,
	C_GBDREAM,
	C_AURA,
	C_MAELSTROM,
	C_BLESSEDHARVEST,
	C_DROUGHT,
	C_BADLEARN,
	C_SHIP_SPEEDUP,		/*  9 - Sturmwind-Zauber */
	C_SHIP_FLYING,		/* 10 - Luftschiff-Zauber */
	C_SHIP_NODRIFT,		/* 11 - GünstigeWinde-Zauber */
	C_DEPRESSION,
	C_MAGICSTONE,     /* 13 - Heimstein */
	C_STRONGWALL,	    /* 14 - Feste Mauer - Precombat*/
	C_ASTRALBLOCK,		/* 15 - Astralblock */
	C_GENEROUS,	      /* 16 - Unterhaltung vermehren */
	C_PEACE,          /* 17 - Regionsweit Attacken verhindern */
	C_REGCONF,        /* 18 - Erschwert Bewegungen */
	C_MAGICSTREET,    /* 19 - magisches Straßennetz */
	C_RESIST_MAGIC,   /* 20 - verändert Magieresistenz von Objekten */
	C_SONG_BADMR,     /* 21 - verändert Magieresistenz */
	C_SONG_GOODMR,    /* 22 - verändert Magieresistenz */
	C_SLAVE,          /* 23 - dient fremder Partei */
	C_DISORIENTATION, /* 24 - Spezielle Auswirkung auf Schiffe */
	C_CALM,           /* 25 - Beinflussung */
	C_OLDRACE,
	C_FUMBLE,
	C_RIOT,           /*region in Aufruhr */
	C_NOCOST,
	C_HOLYGROUND,
	C_CURSED_BY_THE_GODS,
	C_FREE_14,
	C_FREE_15,
	C_FREE_16,
	C_FREE_17,
	C_FREE_18,
	C_FREE_19,
/* struct's vom untertyp curse_unit: */
	C_SPEED,					/* Beschleunigt */
	C_ORC,
	C_MBOOST,
	C_KAELTESCHUTZ,
	C_STRENGTH,
	C_ALLSKILLS,
	C_MAGICRESISTANCE,    /* 44 - verändert Magieresistenz */
	C_ITEMCLOAK,
	C_SPARKLE,
	C_FREE_22,
	C_FREE_23,
	C_FREE_24,
/* struct's vom untertyp curse_skill: */
	C_SKILL,
	C_DUMMY,
	MAXCURSE
};

/* ------------------------------------------------------------- */
/* Flags */

#define CURSE_ISNEW 1 /* wirkt in der zauberrunde nicht (default)*/
#define CURSE_NOAGE 2 /* wirkt ewig */
#define CURSE_IMMUN 4 /* ignoriert Antimagie */
#define CURSE_ONLYONE 8 /* Verhindert, das ein weiterer Zauber dieser Art
													 auf das Objekt gezaubert wird */

/* Verhalten von Zaubern auf Units beim Übergeben von Personen */
enum{
	CURSE_SPREADNEVER,  /* wird nie mit übertragen */
	CURSE_SPREADALWAYS, /* wird immer mit übertragen */
	CURSE_SPREADMODULO, /* personenweise weitergabe */
	CURSE_SPREADCHANCE  /* Ansteckungschance je nach Mengenverhältnis*/
};

/* typ von struct */
enum {
	CURSETYP_NORM,
	CURSETYP_UNIT,
	CURSETYP_SKILL,
	MAXCURSETYP
};

/* Verhalten beim Zusammenfassen mit einem schon bestehenden Zauber
 * gleichen Typs */

#define NO_MERGE      0 /* erzeugt jedesmal einen neuen Zauber */
#define M_DURATION    1 /* Die Zauberdauer ist die maximale Dauer beider */
#define M_SUMDURATION 2 /* die Dauer des Zaubers wird summiert */
#define M_MAXEFFECT   4 /* der Effekt ist der maximale Effekt beider */
#define M_SUMEFFECT   8 /* der Effekt summiert sich */
#define M_MEN        16 /* die Anzahl der betroffenen Personen summiert
													 sich */
#define M_VIGOUR     32 /* das Maximum der beiden Stärken wird die
													 Stärke des neuen Zaubers */
#define M_VIGOUR_ADD 64	/* Vigour wird addiert */

/* ------------------------------------------------------------- */
/* Allgemeine Zauberwirkungen */

typedef struct curse {
	struct curse *nexthash;
	int no;            /* 'Einheitennummer' dieses Curse */
	struct curse_type * type; /* Zeiger auf ein curse_type-struct */
	int flag;          /* generelle Flags wie zb CURSE_ISNEW oder CURSE_NOAGE */
	int duration;      /* Dauer der Verzauberung. Wird jede Runde vermindert */
	int vigour;        /* Stärke der Verzauberung, Widerstand gegen Antimagie */
	struct unit *magician;    /* Pointer auf den Magier, der den Spruch gewirkt hat */
	int effect;
	void *data;        /* pointer auf spezielle curse-unterstructs*/
} curse;

/* Die Unterattribute curse->data: */
/* Einheitenzauber:
 * auf Einzelpersonen in einer Einheit bezogene Zauber. Für Zauber, die
 * nicht immer auf ganze Einheiten wirken brauchen
 */
typedef struct curse_unit {
	int cursedmen;        /* verzauberte Personen in der Einheit */
} curse_unit;

/* Einheitenzauber:
 * Spezialtyp, der Talentmodifizierer. Bei Handwerkstalenten oder
 * Kampftalenten können Personenbezogenen Boni oder Mali berücksichtigt
 * werden, bei anderen Talenten muss sich der Implementiere genauere
 * Gedanken zu den Auswirkungen bei nicht ganz voll verzauberten
 * Einheiten machen.
 */
typedef struct curse_skill {
	int cursedmen;        /* verzauberte Personen in der Einheit */
	skill_t skill;        /* Talent auf das der Spruch wirkt (id2) */
} curse_skill;

typedef int (*cdesc_fun)(const void*, int, curse*, int);
/* Parameter: Objekt, auf dem curse liegt, Typ des Objekts, curse,
 * Besitzerpartei?1:0 */

/* ------------------------------------------------------------- */

typedef struct curse_type {
	curse_t cspellid;  /* Id des Cursezaubers */
	const char *name; /* Name der Zauberwirkung, Identifizierung des curse */
	int typ;
	int givemenacting;
	int mergeflags;
	const char *info;  /* Wirkung des curse, wird bei einer gelungenen
								 Zauberanalyse angezeigt */
	int (*curseinfo)(const void*, int, curse*, int);
	void (*change_vigour)(curse*, int);
} curse_type;

extern struct curse_type cursedaten[];

extern attrib_type at_curse;
extern void curse_write(const attrib * a,FILE * f);
extern int curse_read(struct attrib * a,FILE * f);

/* ------------------------------------------------------------- */
/* Kommentare:
 * Bei einigen Typen von Verzauberungen (z.B.Skillmodif.) muss neben
 * der curse-id noch ein weiterer Identifizierer angegeben werden (id2).
 * Das ist bei curse_skill der Skill, bei curse_secondid irgendwas frei
 * definierbares. In allen anderen Fällen ist id2 egal.
 *
 * Wenn der Typ korrekt definiert wurde, erfolgt die Verzweigung zum
 * korrekten Typus automatisch, und die unterschiedlichen struct-typen
 * sind nach aussen unsichtbar.
 *
* allgemeine Funktionen *
*/

curse * create_curse(struct unit *magician, struct attrib**ap, curse_t id, int id2,
		int vigour, int duration, int effect, int men);
	/* Verzweigt automatisch zum passenden struct-typ. Sollte es schon
	 * einen Zauber dieses Typs geben, so wird der neue dazuaddiert. Die
	 * Zahl der verzauberten Personen sollte beim Aufruf der Funktion
	 * nochmal gesondert auf min(get_cursedmen, u->number) gesetzt werden.
	 */

boolean is_cursed(struct attrib *ap, curse_t id, int id2);
	/* gibt true, wenn bereits ein Zauber dieses Typs vorhanden ist */
boolean is_cursed_internal(struct attrib *ap, curse_t id, int id2);
	/* ignoriert CURSE_ISNEW */

void remove_curse(struct attrib **ap, curse_t id, int id2);
	/* löscht einen Spruch auf einem Objekt. Bei einigen Typen von
	 * Verzauberungen (z.B. Skillmodif.) muss neben der curse-id noch
	 * ein weiterer Identifizierer angegeben werden, zb der Skill. In
	 * allen anderen Fällen ist id2 egal.
	 */
void remove_allcurse(struct attrib **ap, const void * data, boolean(*compare)(const attrib *, const void *));
	/* löscht alle Curse dieses Typs */
void remove_cursec(attrib **ap, curse *c);
	/* löscht den curse c, wenn dieser in ap steht */

int get_curseeffect(struct attrib *ap, curse_t id, int id2);
	/* gibt die Auswirkungen der Verzauberungen zurück. zB bei
	 * Skillmodifiziernden Verzauberungen ist hier der Modifizierer
	 * gespeichert. Wird automatisch beim Anlegen eines neuen curse
	 * gesetzt. Gibt immer den ersten Treffer von ap aus zurück.
	 */


int get_cursevigour(struct attrib *ap, curse_t id, int id2);
	/* gibt die allgemeine Stärke der Verzauberung zurück. id2 wird wie
	 * oben benutzt. Dies ist nicht die Wirkung, sondern die Kraft und
	 * damit der gegen Antimagie wirkende Widerstand einer Verzauberung */
void set_cursevigour(struct attrib *ap, curse_t id, int id2, int i);
	/* setzt die Stärke der Verzauberung auf i */
int change_cursevigour(struct attrib **ap, curse_t id, int id2, int i);
	/* verändert die Stärke der Verzauberung um i */

int get_cursedmen(struct unit *u, struct curse *c);
	/* gibt bei Personenbeschränkten Verzauberungen die Anzahl der
	 * betroffenen Personen zurück. Ansonsten wird 0 zurückgegeben. */
int change_cursedmen(struct attrib **ap, curse_t id, int id2, int cursedmen);
	/* verändert die Anzahl der betroffenen Personen um cursedmen */
void set_cursedmen(struct attrib *ap, curse_t id, int id2, int cursedmen);
	/* setzt die Anzahl der betroffenen Personen auf cursedmen */

void set_curseflag(struct attrib *ap, curse_t id, int id2, int flag);
	/* setzt Spezialflag einer Verzauberung (zB 'dauert ewig') */
void remove_curseflag(struct attrib *ap, curse_t id, int id2, int flag);
	/* löscht Spezialflag einer Verzauberung (zB 'ist neu') */

void transfer_curse(struct unit * u, struct unit * u2, int n);
	/* sorgt dafür, das bei der Übergabe von Personen die curse-attribute
	 * korrekt gehandhabt werden. Je nach internen Flag kann dies
	 * unterschiedlich gewünscht sein
	 * */
void remove_cursemagepointer(struct unit *magician, attrib *ap_target);
	/* wird von remove_empty_units verwendet um alle Verweise auf
	 * gestorbene Magier zu löschen.
	 * */
curse * get_curse(struct attrib *ap, curse_t id, int id2);
	/* gibt pointer auf die passende curse-struct zurück, oder einen
	 * NULL-pointer
	 * */
struct unit * get_tracingunit(struct attrib **ap, curse_t id);
struct unit * get_cursingmagician(struct attrib *ap, curse_t id, int id2);
	/* gibt struct unit-pointer auf Magier zurück, oder einen Nullpointer
	 * */
int find_cursebyname(const char *c);
const curse_type * ct_find(const char *c);
void ct_register(const curse_type *);
/* Regionszauber */

curse * cfindhash(int i);
void chash(curse *c);
void cunhash(curse *c);

curse * findcurse(int curseid);

extern void curse_init(struct attrib * a);
extern void curse_done(struct attrib * a);
extern int curse_age(struct attrib * a);

extern boolean cmp_curse(const attrib * a, const void * data);
/* compatibility mode für katjas curses: */
extern boolean cmp_oldcurse(const attrib * a, const void * data);
extern struct twoids * packids(int id, int id2);

extern void * resolve_curse(void * data);

extern boolean is_spell_active(const struct region * r, curse_t id);
	/* prüft, ob ein bestimmter Zauber auf einer struct region liegt */

extern boolean is_cursed_with(attrib *ap, curse *c);
const curse_type * find_cursetype(curse_t id);
	
	
#endif
