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

#ifndef H_KRNL_MAGIC
#define H_KRNL_MAGIC
#ifdef __cplusplus
extern "C" {
#endif

#include "curse.h"
struct fighter;
struct building;

/* ------------------------------------------------------------- */

#define MAXCOMBATSPELLS 3    /* PRECOMBAT COMBAT POSTCOMBAT */
#define MAX_SPELLRANK 9      /* Standard-Rank 5 */
#define MAXINGREDIENT	5      /* bis zu 5 Komponenten pro Zauber */
#define CHAOSPATZERCHANCE 10 /* +10% Chance zu Patzern */

/* ------------------------------------------------------------- */

#define IRONGOLEM_CRUMBLE   15  /* monatlich Chance zu zerfallen */
#define STONEGOLEM_CRUMBLE  10  /* monatlich Chance zu zerfallen */

/* ------------------------------------------------------------- */
typedef struct spell_ptr spell_ptr;
typedef struct castorder castorder;

/* ------------------------------------------------------------- */
/* Spruchparameter
 * Wir suchen beim Parsen des Befehls erstmal nach lokalen Objekten,
 * erst in verify_targets wird dann global gesucht, da in den meisten
 * Fällen das Zielobjekt lokal sein dürfte */

/* siehe auch typ_t in objtypes.h */
typedef enum {
	SPP_UNIT,          /* "u" : getunit() -> *unit */
	SPP_REGION,        /* "r" : findregion(x,y) -> *region */
	SPP_BUILDING,      /* "b" : findbuilding() -> *building */
	SPP_SHIP,          /* "s" : findship() -> *ship */
	SPP_UNIT_ID,       /*  -  : atoi36() -> int */
	SPP_BUILDING_ID,   /*  -  : atoi() -> int */
	SPP_SHIP_ID,       /*  -  : atoi() -> int */
	SPP_STRING,        /* "c" */
	SPP_INT,           /* "i" : atoi() -> int */
	SPP_TUNIT_ID       /*  -  : temp einheit */
} sppobj_t;

typedef struct spllprm{
	sppobj_t typ;
	int flag;
	union{
		struct region *r;
		struct unit *u;
		struct building *b;
		struct ship *sh;
		char *s;
		int i;
	} data;
} spllprm;

typedef struct spellparameter{
	int length;     /* Anzahl der Elemente */
	struct spllprm **param;
} spellparameter;

typedef struct strarray {
	int length;     /* Anzahl der Elemente */
	char **strings;
} strarray;

#define TARGET_RESISTS (1<<0)
#define TARGET_NOTFOUND (1<<1)

/* ------------------------------------------------------------- */
/* Magierichtungen */

/* typedef unsigned char magic_t; */
enum {
	M_GRAU,       /* none */
	M_TRAUM,      /* Illaun */
	M_ASTRAL,     /* Tybied */
	M_BARDE,      /* Cerddor */
	M_DRUIDE,     /* Gwyrrd */
	M_CHAOS,      /* Draig */
	MAXMAGIETYP,
	M_NONE = (magic_t) -1
};
extern const char *magietypen[MAXMAGIETYP];

/* ------------------------------------------------------------- */
/* Magier:
 * - Magierichtung
 * - Magiepunkte derzeit
 * - Malus (neg. Wert)/ Bonus (pos. Wert) auf maximale Magiepunkte
 *   (können sich durch Questen absolut verändern und durch Gegenstände
 *   temporär). Auch für Artefakt benötigt man permanente MP
 * - Anzahl bereits gezauberte Sprüche diese Runde
 * - Kampfzauber (3) (vor/während/nach)
 * - Spruchliste
 */

typedef struct sc_mage {
	magic_t magietyp;
	int spellpoints;
	int spchange;
	int spellcount;
	spellid_t combatspell[MAXCOMBATSPELLS];
	int     combatspelllevel[MAXCOMBATSPELLS];
	int     precombataura;		/* Merker, wieviel Aura in den Präcombatzauber
															 gegangen ist. Nicht speichern. */
	spell_ptr *spellptr;
} sc_mage;

struct spell_ptr {
	spell_ptr *next;
	spellid_t spellid;
};

/* ------------------------------------------------------------- */
/* Spruchstukturdefinition:
 * id:
 *  SPL_NOSPELL muss der letzte Spruch in der Liste spelldaten[] sein,
 *  denn nicht auf die Reihenfolge in der Liste sondern auf die id wird
 *  geprüft
 * rank:
 *  gibt die Priorität und damit die Reihenfolge an, in der der Spruch
 *  gezaubert wird.
 * sptyp:
 *  besondere Spruchtypen (Artefakt, Regionszauber, Kampfzauber ..)
 * Komponenten[Anzahl mögl. Items][Art:Anzahl:Faktor]
 *
 */

/* typedef struct fighter fighter; */

typedef struct spell {
	spellid_t id;
	const char *sname;
	const char *info;
	const char *syntax;
	const char *parameter;
	magic_t magietyp;
	int sptyp;
	char rank;  /* Reihenfolge der Zauber */
	int level;  /* Stufe des Zaubers */
	resource_t komponenten[MAXINGREDIENT][3];
	void (*sp_function) (void*);
	void (*patzer) (castorder*);
} spell;


/* ------------------------------------------------------------- */
/* Zauberliste */


struct castorder {
	castorder *next;
	void *magician;       /* Magier (kann vom Typ struct unit oder fighter sein) */
	struct unit *familiar;       /* Vertrauter, gesetzt, wenn der Spruch durch
													 den Vertrauten gezaubert wird */
	struct spell *sp;            /* Spruch */
	int level;            /* gewünschte Stufe oder Stufe des Magiers */
	int force;            /* Stärke des Zaubers */
	struct region *rt;           /* Zielregion des Spruchs */
	int distance;         /* Entfernung zur Zielregion */
	char *order;          /* Befehl */
	struct spellparameter *par;  /* für weitere Parameter */
};

/* ------------------------------------------------------------- */

/* irgendwelche zauber: */
typedef void (*spell_f) (void*);
/* normale zauber: */
typedef int (*nspell_f)(castorder*);
/* kampfzauber: */
typedef int (*cspell_f) (struct fighter*, int, int, struct spell * sp);
/* zauber-patzer: */
typedef void (*pspell_f) (castorder *);

/* besondere Spruchtypen */
#define FARCASTING      (1<<0)	/* ZAUBER [struct region x y] */
#define SPELLLEVEL      (1<<1)	/* ZAUBER [STUFE x] */

/* ID's können zu drei unterschiedlichen Entitäten gehören: Einheiten,
 * Gebäuden und Schiffen. */
#define UNITSPELL       (1<<2)	/* ZAUBER .. <Einheit-Nr> [<Einheit-Nr> ..] */
#define SHIPSPELL       (1<<3)	/* ZAUBER .. <Schiff-Nr> [<Schiff-Nr> ..] */
#define BUILDINGSPELL   (1<<4)	/* ZAUBER .. <Gebäude-Nr> [<Gebäude-Nr> ..] */
#define REGIONSPELL     (1<<5)  /* wirkt auf struct region */
#define ONETARGET       (1<<6)	/* ZAUBER .. <Ziel-Nr> */

#define PRECOMBATSPELL	(1<<7)	/* PRÄKAMPFZAUBER .. */
#define COMBATSPELL     (1<<8)	/* KAMPFZAUBER .. */
#define POSTCOMBATSPELL	(1<<9)	/* POSTKAMPFZAUBER .. */
#define ISCOMBATSPELL   (PRECOMBATSPELL|COMBATSPELL|POSTCOMBATSPELL)

#define OCEANCASTABLE   (1<<10)	/* Können auch nicht-Meermenschen auf
																	 hoher See zaubern */
#define ONSHIPCAST      (1<<11) /* kann auch auf von Land ablegenden
																	 Schiffen stehend gezaubert werden */
/*  */
#define NOTFAMILIARCAST (1<<12)
#define TESTRESISTANCE  (1<<13) /* alle Zielobjekte (u, s, b, r) auf
																	 Magieresistenz prüfen */
#define SEARCHGLOBAL    (1<<14) /* Ziel global anstatt nur in target_region
																	 suchen */
#define TESTCANSEE      (1<<15) /* alle Zielunits auf cansee prüfen */

/* Flag Spruchkostenberechnung: */
enum{
	SPC_FIX,      /* Fixkosten */
	SPC_LEVEL,    /* Komponenten pro Level */
	SPC_LINEAR    /* Komponenten pro Level und müssen vorhanden sein */
};

enum {
	RS_DUMMY,
	RS_FARVISION,
	MAX_REGIONSPELLS
};

/* ------------------------------------------------------------- */
/* Prototypen */

void magic(void);

void regeneration_magiepunkte(void);

extern attrib_type at_deathcloud;
extern attrib_type at_seenspell;
extern attrib_type at_mage;
extern attrib_type at_familiarmage;
extern attrib_type at_familiar;
extern attrib_type at_clonemage;
extern attrib_type at_clone;
extern attrib_type at_reportspell;
extern attrib_type at_icastle;

typedef struct icastle_data {
	const struct building_type * type;
	struct building * building; /* reverse pointer to dissolve the object */
	int time;
} icastle_data;


/* ------------------------------------------------------------- */
/* Kommentare:
 *
 * Spruchzauberrei und Gegenstandszauberrei werden getrennt behandelt.
 * Das macht u.a. bestimmte Fehlermeldungen einfacher, das
 * identifizieren der Komponennten über den Missversuch ist nicht
 * möglich
 * Spruchzauberrei: 'ZAUBER [struct region x y] [STUFE a] "Spruchname" [Ziel]'
 * Gegenstandszauberrei: 'BENUTZE "Gegenstand" [Ziel]'
 *
 * Die Funktionen:
 */

/* Magier */
sc_mage * create_mage(struct unit *u, magic_t mtyp);
	/*	macht die struct unit zu einem neuen Magier: legt die struct u->mage an
	 *	und	initialisiert den Magiertypus mit mtyp.  */
sc_mage * get_mage(const struct unit *u);
	/*	gibt u->mage zurück, bei nicht-Magiern *NULL */
magic_t find_magetype(const struct unit *u);
	/*	gibt den Magietyp der struct unit zurück, bei nicht-Magiern 0 */
boolean is_mage(const struct unit *u);
	/*	gibt true, wenn u->mage gesetzt.  */
boolean is_familiar(const struct unit *u);
	/*	gibt true, wenn eine Familiar-Relation besteht.  */

/* Sprüche */
spell *find_spellbyname(struct unit *u, const char *s, const struct locale * lang);
	/*	versucht einen Spruch über den Namen zu identifizieren, gibt
	 *	ansonsten NULL zurück */
spell *find_spellbyid(spellid_t i);
	/*	versucht einen Spruch über seine Id zu identifizieren, gibt
	 *	ansonsten NULL zurück */
int get_combatspelllevel(const struct unit *u, int nr);
	/*  versucht, eine eingestellte maximale Kampfzauberstufe
	 *  zurückzugeben. 0 = Maximum, -1 u ist kein Magier. */
spell *get_combatspell(const struct unit *u, int nr);
	/*	gibt den Kampfzauber nr [pre/kampf/post] oder NULL zurück */
void set_combatspell(struct unit *u, spell *sp, const char * cmd, int level);
	/* 	setzt Kampfzauber */
void unset_combatspell(struct unit *u, spell *sp);
	/* 	löscht Kampfzauber */
void addspell(struct unit *u, spellid_t spellid);
	/* fügt den Spruch mit der Id spellid der Spruchliste der Einheit hinzu. */
boolean getspell(const struct unit *u, spellid_t spellid);
	/* prüft, ob der Spruch in der Spruchliste der Einheit steht. */
void updatespelllist(struct unit *u);
	/* fügt alle Zauber des Magiegebietes der Einheit, deren Stufe kleiner
	 * als das aktuelle Magietalent ist, in die Spruchliste der Einheit
	 * ein */
boolean knowsspell(const struct region * r, const struct unit * u, const spell * sp);
	/* prüft, ob die Einheit diesen Spruch gerade beherrscht, dh
	 * mindestens die erforderliche Stufe hat. Hier können auch Abfragen
	 * auf spezielle Antimagiezauber auf Regionen oder Einheiten eingefügt
	 * werden
	 */


/* Magiepunkte */
int get_spellpoints(const struct unit *u);
	/*	Gibt die aktuelle Anzahl der Magiepunkte der Einheit zurück */
void set_spellpoints(struct unit * u, int sp);
	/* setzt die Magiepunkte auf sp */
int change_spellpoints(struct unit *u, int mp);
	/*	verändert die Anzahl der Magiepunkte der Einheit um +mp */
int max_spellpoints(const struct region *r, const struct unit *u);
	/*	gibt die aktuell maximal möglichen Magiepunkte der Einheit zurück */
int change_maxspellpoints(struct unit * u, int csp);
   /* verändert die maximalen Magiepunkte einer Einheit */

/* Zaubern */
int spellpower(struct region *r, struct unit *u, spell *spruch, int cast_level);
	/*	ermittelt die Stärke eines Spruchs */
boolean fumble (struct region *r, struct unit *u, spell *spruch, int cast_level);
	/*	true, wenn der Zauber misslingt, bei false gelingt der Zauber */

/* */
castorder *new_castorder(void *u, struct unit *familiar, spell *sp, struct region *r,
		int lev, int force, int distance, char *cmd, spellparameter *p);
	/* Zwischenspreicher für Zauberbefehle, notwendig für Prioritäten */
void add_castorder(castorder **cll, castorder *co);
	/* Hänge c-order co an die letze c-order von cll an */
void free_castorders(castorder *co);
	/* Speicher wieder freigeben */

/* Prüfroutinen für Zaubern */
int countspells(struct unit *u, int step);
	/*	erhöht den Counter für Zaubersprüche um 'step' und gibt die neue
	 *	Anzahl der gezauberten Sprüche zurück. */
int spellcost(struct unit *u, spell *spruch);
	/*	gibt die für diesen Spruch derzeit notwendigen Magiepunkte auf der
	 *	geringstmöglichen Stufe zurück, schon um den Faktor der bereits
	 *	zuvor gezauberten Sprüche erhöht */
boolean cancast (struct unit *u, spell *spruch, int eff_stufe, int distance, char *cmd);
	/*	true, wenn Einheit alle Komponenten des Zaubers (incl. MP) für die
	 *	geringstmögliche Stufe hat und den Spruch beherrscht */
void pay_spell(struct unit *u, spell *spruch, int eff_stufe, int distance);
	/*	zieht die Komponenten des Zaubers aus dem Inventory der Einheit
	 *	ab. Die effektive Stufe des gezauberten Spruchs ist wichtig für
	 *	die korrekte Bestimmung der Magiepunktkosten */
int eff_spelllevel(struct unit *u, spell * sp, int cast_level, int distance);
	/*	ermittelt die effektive Stufe des Zaubers. Dabei ist cast_level
	 *	die gewünschte maximale Stufe (im Normalfall Stufe des Magiers,
	 *	bei Farcasting Stufe*2^Entfernung) */
boolean is_magic_resistant(struct unit *magician, struct unit *target, int
	resist_bonus);
	/*	Mapperfunktion für target_resists_magic() vom Typ struct unit. */
int magic_resistance(struct unit *target);
	/*	gibt die Chance an, mit der einem Zauber widerstanden wird. Je
	 *	größer, desto resistenter ist da Opfer */
boolean target_resists_magic(struct unit *magician, void *obj, int objtyp,
		int resist_bonus);
	/*	gibt false zurück, wenn der Zauber gelingt, true, wenn das Ziel
	 *	widersteht */


/* Sprüche in der struct region */
   /* (sind in curse)*/
extern struct unit * get_familiar(const struct unit *u);
extern struct unit * get_familiar_mage(const struct unit *u);
extern struct unit * get_clone(const struct unit *u);
extern struct unit * get_clone_mage(const struct unit *u);
extern struct attrib_type at_familiar;
extern struct attrib_type at_familiarmage;
extern void remove_familiar(struct unit * mage);
extern void create_newfamiliar(struct unit * mage, struct unit * familiar);
extern void create_newclone(struct unit * mage, struct unit * familiar);
extern struct unit * has_clone(struct unit * mage);
extern struct attrib *create_special_direction(struct region *r, int x, int y, int duration,
		const char *desc, const char *keyword);

extern struct plane * astral_plane;

extern const char * spell_info(const struct spell * sp, const struct locale * lang);
extern const char * spell_name(const struct spell * sp, const struct locale * lang);

#ifdef __cplusplus
}
#endif
#endif
