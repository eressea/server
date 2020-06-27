#ifndef CURSE_H
#define CURSE_H

#include <util/variant.h>
#include "objtypes.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct curse;
    struct curse_type;
    struct gamedata;
    struct storage;
    struct attrib;
    struct faction;

    /* Sprueche in der struct region und auf Einheiten, Schiffen oder Burgen
     * (struct attribute)
     */

    /* Brainstorming Ueberarbeitung curse
     *
     * Ziel: Keine Enum-Liste, flexible, leicht erweiterbare Curse-Objekte
     *
     * Was wird gebraucht?
     * - Eindeutige Kennung fuer globale Suche
     * - eine Wirkung, die sich einfach 'anwenden' laesst, dabei flexibel ist,
     *   Raum laesst fuer variable Boni, Anzahl betroffener Personen,
     *   spezielle Effekte oder anderes
     * - einfacher Zugriff auf allgemeine Funktionen wie zb Alterung, aber
     *   auch Antimagieverhalten
     * - Ausgabe von Beschreibungen in verschiedenen Sprachen
     * - definiertes gekapseltes Verhalten zb bei Zusammenlegung von
     *   Einheiten, Uebergabe von Personen, Mehrfachverzauberung
     * - (Rueck-)Referenzen auf Objekt, Verursacher (Magier), ?
     *
     * Vieleicht waere ein Wirkungsklassensystem sinnvoll, so das im uebrigen
     * source einfach alle curse-attribs abgefragt werden koennen und bei
     * gewuenschter Wirkungsklasse angewendet, also nicht fuer jeden curse
     * spezielle Aenderungen im uebrigen source notwendig sind.
     *
     * Die (Wirkungs-)Typen sollten die wichtigen Funktionen speziell
     * belegen koennen, zb Alterung, Ausgabetexte, Merge-Verhalten
     *
     * Was sind wichtige individuelle Eigenschaften?
     * - Referenzen auf Objekt und Verursacher
     * - Referenzen fuer globale Liste
     * > die Wirkung:
     * - Dauer
     * - Widerstandskraft, zb gegen Antimagie
     * - Seiteneffekte zb Flag ONLYONE, Unvertraeglichkeiten
     * - Alterungsverhalten zb Flag NOAGE
     * - Effektverhalten, zb Bonus (variabel)
     * - bei Einheitenzaubern die Zahl der betroffenen Personen
     *
     * Dabei sind nur die beiden letzten Punkte wirklich reine individuelle
     * Wirkung, die anderen sind eher allgemeine Kennzeichen eines jeden
     * Curse, die individuell belegt sind.
     * ONLYONE und NOAGE koennten auch Eigenschaften des Typs sein, nicht des
     * individuellen curse. Allgemein ist Alterung wohl eher eine
     * Typeigenschaft als die eines individuellen curse.
     * Dagegen ist der Widerstand gegen Antimagie sowohl abhaengig von der
     * Guete des Verursachers, also des Magiers zum Zeitpunkt des Zaubers,
     * als auch vom Typ, der gegen bestimmte Arten des 'Fluchbrechens' immun
     * sein koennte.
     *
     * Was sind wichtige Typeigenschaften?
     * - Verhalten bei Personenuebergaben
     * - allgemeine Wirkung
     * - Beschreibungstexte
     * - Verhalten bei Antimagie
     * - Alterung
     * - Speicherung des C-Objekts
     * - Laden des C-Objekts
     * - Erzeugen des C-Objekts
     * - Loeschen und Aufraeumen des C-Objekts
     * - Funktionen zur Aenderung der Werte
     *
     * */

    extern struct attrib_type at_curse;

    /* ------------------------------------------------------------- */
    /* Zauberwirkungen */
    /* nicht vergessen curse_type zu aktualisieren und Reihenfolge beachten!
     */

    enum {
        C_FOGTRAP,
        C_ANTIMAGICZONE,
        C_FARVISION,
        C_GBDREAM,
        C_AURA,
        C_MAELSTROM,
        C_BLESSEDHARVEST,
        C_DROUGHT,
        C_BADLEARN,
        C_SHIP_SPEEDUP,             /*  9 - Sturmwind-Zauber */
        C_SHIP_FLYING,              /* 10 - Luftschiff-Zauber */
        C_SHIP_NODRIFT,             /* 11 - GuenstigeWinde-Zauber */
        C_DEPRESSION,
        C_MAGICWALLS,               /* 13 - Heimstein */
        C_STRONGWALL,               /* 14 - Feste Mauer - Precombat */
        C_ASTRALBLOCK,              /* 15 - Astralblock */
        C_GENEROUS,                 /* 16 - Unterhaltung vermehren */
        C_PEACE,                    /* 17 - Regionsweit Attacken verhindern */
        C_MAGICSTREET,              /* 19 - magisches Strassennetz */
        C_RESIST_MAGIC,             /* 20 - veraendert Magieresistenz von Objekten */
        C_SONG_BADMR,               /* 21 - veraendert Magieresistenz */
        C_SONG_GOODMR,              /* 22 - veraendert Magieresistenz */
        C_SLAVE,                    /* 23 - dient fremder Partei */
        C_CALM,                     /* 25 - Beinflussung */
        C_OLDRACE,
        C_FUMBLE,
        C_RIOT,                     /*region in Aufruhr */
        C_CURSED_BY_THE_GODS,
        C_SPEED,                    /* Beschleunigt */
        C_ORC,
        C_MBOOST,
        C_KAELTESCHUTZ,
        MAXCURSE                    /* OBS: when removing curses, remember to update read_ccompat() */
    };

    /* ------------------------------------------------------------- */
    /* Flags */

    /* Verhalten von Zaubern auf Units beim Uebergeben von Personen */
    typedef enum {
        CURSE_ISNEW = 0x01,         /* wirkt in der zauberrunde nicht (default) */
        CURSE_NOAGE = 0x02,         /* wirkt ewig */
        CURSE_IMMUNE = 0x04,        /* ignoriert Antimagie */
        CURSE_ONLYONE = 0x08,       /* Verhindert, das ein weiterer Zauber dieser Art auf das Objekt gezaubert wird */

        /* the following are mutually exclusive */
        CURSE_SPREADNEVER = 0x00,   /* wird nie mit uebertragen */
        CURSE_SPREADALWAYS = 0x10,  /* wird immer mit uebertragen */
        CURSE_SPREADMODULO = 0x20,  /* personenweise weitergabe */
        CURSE_SPREADCHANCE = 0x30   /* Ansteckungschance je nach Mengenverhaeltnis */
    } curseflags;

#define CURSE_FLAGSMASK 0x0F
#define CURSE_SPREADMASK 0x30

    /* typ von struct */
    enum {
        CURSETYP_NORM,
        CURSETYP_UNIT,
        CURSETYP_REGION,            /* stores the region in c->data.v */
        MAXCURSETYP
    };

    /* Verhalten beim Zusammenfassen mit einem schon bestehenden Zauber
     * gleichen Typs */

#define NO_MERGE      0         /* erzeugt jedesmal einen neuen Zauber */
#define M_DURATION    1         /* Die Zauberdauer ist die maximale Dauer beider */
#define M_SUMDURATION 2         /* die Dauer des Zaubers wird summiert */
#define M_MAXEFFECT   4         /* der Effekt ist der maximale Effekt beider */
#define M_SUMEFFECT   8         /* der Effekt summiert sich */
#define M_MEN        16         /* die Anzahl der betroffenen Personen summiert
     sich */
#define M_VIGOUR     32         /* das Maximum der beiden Staerken wird die
     Staerke des neuen Zaubers */
#define M_VIGOUR_ADD 64         /* Vigour wird addiert */

    /* ------------------------------------------------------------- */
    /* Allgemeine Zauberwirkungen */

#define c_flags(c) ((c)->type->flags ^ (c)->mask)

    /* ------------------------------------------------------------- */

    typedef struct curse_type {
        const char *cname;          /* Name der Zauberwirkung, Identifizierung des curse */
        int typ;
        int flags;
        int mergeflags;
        struct message *(*curseinfo) (const void *, objtype_t,
            const struct curse *, int);
        void(*change_vigour) (struct curse *, double);
        int(*read) (struct gamedata *data, struct curse *, void *target);
        int(*write) (struct storage *store, const struct curse *,
            const void *target);
        int(*cansee) (const struct faction *, const void *, objtype_t,
            const struct curse *, int);
        int(*age) (struct curse *);
    } curse_type;

    typedef struct curse {
        variant data;               /* pointer auf spezielle curse-unterstructs */
        struct curse *nexthash;
        const curse_type *type;     /* Zeiger auf ein curse_type-struct */
        struct unit *magician;      /* Pointer auf den Magier, der den Spruch gewirkt hat */
        double vigour;              /* Staerke der Verzauberung, Widerstand gegen Antimagie */
        double effect;
        int no;                     /* 'Einheitennummer' dieses Curse */
        int mask;                   /* This is XORed with type->flags, see c_flags()! */
        int duration;               /* Dauer der Verzauberung. Wird jede Runde vermindert */
    } curse;

    void curses_done(void); /* de-register all curse-types */

    void curse_write(const union variant *v, const void *owner,
        struct storage *store);
    int curse_read(union variant *v, void *owner, struct gamedata *store);

    /* ------------------------------------------------------------- */
    /* Kommentare:
     * Bei einigen Typen von Verzauberungen (z.B.Skillmodif.) muss neben
     * der curse-id noch ein weiterer Identifizierer angegeben werden (id2).
     *
     * Wenn der Typ korrekt definiert wurde, erfolgt die Verzweigung zum
     * korrekten Typus automatisch, und die unterschiedlichen struct-typen
     * sind nach aussen unsichtbar.
     *
     * allgemeine Funktionen *
     */

    curse *create_curse(struct unit *magician, struct attrib **ap,
        const curse_type * ctype, double vigour, int duration, double ceffect,
        int men);
    /* Verzweigt automatisch zum passenden struct-typ. Sollte es schon
     * einen Zauber dieses Typs geben, so wird der neue dazuaddiert. Die
     * Zahl der verzauberten Personen sollte beim Aufruf der Funktion
     * nochmal gesondert auf min(get_cursedmen, u->number) gesetzt werden.
     */

    void destroy_curse(curse * c);

    /* ignoriert CURSE_ISNEW */
    bool is_cursed_internal(struct attrib *ap, const curse_type * ctype);

    /* loescht einen konkreten Spruch auf einem Objekt.  */
    bool remove_curse(struct attrib **ap, const struct curse *c);

    /* gibt die Auswirkungen der Verzauberungen zurueck. zB bei
     * Skillmodifiziernden Verzauberungen ist hier der Modifizierer
     * gespeichert. Wird automatisch beim Anlegen eines neuen curse
     * gesetzt. Gibt immer den ersten Treffer von ap aus zurueck.
     */
    int curse_geteffect_int(const struct curse *c);
    double curse_geteffect(const struct curse *c);

    /* veraendert die Staerke der Verzauberung um i */
    double curse_changevigour(struct attrib **ap, curse * c, double delta);

    /* gibt bei Personenbeschraenkten Verzauberungen die Anzahl der
     * betroffenen Personen zurueck. Ansonsten wird 0 zurueckgegeben. */
    int get_cursedmen(const struct unit *u, const struct curse *c);

    /* setzt/loescht Spezialflag einer Verzauberung (zB 'dauert ewig') */
    void c_setflag(curse * c, unsigned int flag);
    void c_clearflag(curse * c, unsigned int flags);

    /* sorgt dafuer, das bei der Uebergabe von Personen die curse-attribute
     * korrekt gehandhabt werden. Je nach internen Flag kann dies
     * unterschiedlich gewuenscht sein
     * */
    void transfer_curse(const struct unit *u, struct unit *u2, int n);

    /* gibt pointer auf die erste curse-struct zurueck, deren Typ ctype ist,
     * oder einen NULL-pointer
     * */
    struct curse *get_curse(struct attrib *ap, const curse_type * ctype);

    const curse_type *ct_find(const char *c);
    void ct_register(const curse_type *);
    void ct_remove(const char *c);
    void ct_checknames(void);

    curse *findcurse(int curseid);

    void curse_init(union variant *a);
    void curse_done(union variant *a);
    int curse_age(struct attrib *a, void *owner);

    double destr_curse(struct curse *c, int cast_level, double force);

    bool is_cursed_with(const struct attrib *ap, const struct curse *c);

    /* gibt true, wenn der Curse nicht NULL oder inaktiv ist */
    bool curse_active(const struct curse *c);

    /*** COMPATIBILITY MACROS. DO NOT USE FOR NEW CODE, REPLACE IN OLD CODE: */
    struct message *cinfo_simple(const void *obj, objtype_t typ,
        const struct curse *c, int self);
    int curse_cansee(const struct curse *c, const struct faction *viewer, objtype_t typ, const void *obj, int self);
#define is_cursed(a, ctype) \
  (a && curse_active(get_curse(a, ctype)))
#define get_curseeffect(a, ctype) \
  curse_geteffect(get_curse(a, ctype))

#ifdef __cplusplus
}
#endif
#endif
