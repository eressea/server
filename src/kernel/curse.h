/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef CURSE_H
#define CURSE_H

#include <util/variant.h>
#include "objtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* Sprueche in der struct region und auf Einheiten, Schiffen oder Burgen
     * (struct attribute)
     */

    /* Brainstorming �berarbeitung curse
     *
     * Ziel: Keine Enum-Liste, flexible, leicht erweiterbare Curse-Objekte
     *
     * Was wird gebraucht?
     * - Eindeutige Kennung f�r globale Suche
     * - eine Wirkung, die sich einfach 'anwenden' l��t, dabei flexibel ist,
     *   Raum l��t f�r variable Boni, Anzahl betroffener Personen,
     *   spezielle Effekte oder anderes
     * - einfacher Zugriff auf allgemeine Funktionen wie zb Alterung, aber
     *   auch Antimagieverhalten
     * - Ausgabe von Beschreibungen in verschiedenen Sprachen
     * - definiertes gekapseltes Verhalten zb bei Zusammenlegung von
     *   Einheiten, �bergabe von Personen, Mehrfachverzauberung
     * - (R�ck-)Referenzen auf Objekt, Verursacher (Magier), ?
     *
     * Vieleicht w�re ein Wirkungsklassensystem sinnvoll, so das im �brigen
     * source einfach alle curse-attribs abgefragt werden k�nnen und bei
     * gew�nschter Wirkungsklasse angewendet, also nicht f�r jeden curse
     * spezielle �nderungen im �brigen source notwendig sind.
     *
     * Die (Wirkungs-)Typen sollten die wichtigen Funktionen speziell
     * belegen k�nnen, zb Alterung, Ausgabetexte, Merge-Verhalten
     *
     * Was sind wichtige individuelle Eigenschaften?
     * - Referenzen auf Objekt und Verursacher
     * - Referenzen f�r globale Liste
     * > die Wirkung:
     * - Dauer
     * - Widerstandskraft, zb gegen Antimagie
     * - Seiteneffekte zb Flag ONLYONE, Unvertr�glichkeiten
     * - Alterungsverhalten zb Flag NOAGE
     * - Effektverhalten, zb Bonus (variabel)
     * - bei Einheitenzaubern die Zahl der betroffenen Personen
     *
     * Dabei sind nur die beiden letzten Punkte wirklich reine individuelle
     * Wirkung, die anderen sind eher allgemeine Kennzeichen eines jeden
     * Curse, die individuell belegt sind.
     * ONLYONE und NOAGE k�nnten auch Eigenschaften des Typs sein, nicht des
     * individuellen curse. Allgemein ist Alterung wohl eher eine
     * Typeigenschaft als die eines individuellen curse.
     * Dagegen ist der Widerstand gegen Antimagie sowohl abh�ngig von der
     * G�te des Verursachers, also des Magiers zum Zeitpunkt des Zaubers,
     * als auch vom Typ, der gegen bestimmte Arten des 'Fluchbrechens' immun
     * sein k�nnte.
     *
     * Was sind wichtige Typeigenschaften?
     * - Verhalten bei Personen�bergaben
     * - allgemeine Wirkung
     * - Beschreibungstexte
     * - Verhalten bei Antimagie
     * - Alterung
     * - Speicherung des C-Objekts
     * - Laden des C-Objekts
     * - Erzeugen des C-Objekts
     * - L�schen und Aufr�umen des C-Objekts
     * - Funktionen zur �nderung der Werte
     *
     * */

#include <util/variant.h>

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
        C_SHIP_SPEEDUP,             /*  9 - Sturmwind-Zauber */
        C_SHIP_FLYING,              /* 10 - Luftschiff-Zauber */
        C_SHIP_NODRIFT,             /* 11 - G�nstigeWinde-Zauber */
        C_DEPRESSION,
        C_MAGICWALLS,               /* 13 - Heimstein */
        C_STRONGWALL,               /* 14 - Feste Mauer - Precombat */
        C_ASTRALBLOCK,              /* 15 - Astralblock */
        C_GENEROUS,                 /* 16 - Unterhaltung vermehren */
        C_PEACE,                    /* 17 - Regionsweit Attacken verhindern */
        C_MAGICSTREET,              /* 19 - magisches Stra�ennetz */
        C_RESIST_MAGIC,             /* 20 - ver�ndert Magieresistenz von Objekten */
        C_SONG_BADMR,               /* 21 - ver�ndert Magieresistenz */
        C_SONG_GOODMR,              /* 22 - ver�ndert Magieresistenz */
        C_SLAVE,                    /* 23 - dient fremder Partei */
        C_CALM,                     /* 25 - Beinflussung */
        C_OLDRACE,
        C_FUMBLE,
        C_RIOT,                     /*region in Aufruhr */
        C_NOCOST,
        C_CURSED_BY_THE_GODS,
        /* struct's vom untertyp curse_unit: */
        C_SPEED,                    /* Beschleunigt */
        C_ORC,
        C_MBOOST,
        C_KAELTESCHUTZ,
        C_STRENGTH,
        C_ALLSKILLS,
        C_MAGICRESISTANCE,          /* 44 - ver�ndert Magieresistenz */
        C_ITEMCLOAK,
        C_SPARKLE,
        /* struct's vom untertyp curse_skill: */
        C_SKILL,
        MAXCURSE                    /* OBS: when removing curses, remember to update read_ccompat() */
    };

    /* ------------------------------------------------------------- */
    /* Flags */

    /* Verhalten von Zaubern auf Units beim �bergeben von Personen */
    typedef enum {
        CURSE_ISNEW = 0x01,         /* wirkt in der zauberrunde nicht (default) */
        CURSE_NOAGE = 0x02,         /* wirkt ewig */
        CURSE_IMMUNE = 0x04,        /* ignoriert Antimagie */
        CURSE_ONLYONE = 0x08,       /* Verhindert, das ein weiterer Zauber dieser Art auf das Objekt gezaubert wird */

        /* the following are mutually exclusive */
        CURSE_SPREADNEVER = 0x00,   /* wird nie mit �bertragen */
        CURSE_SPREADALWAYS = 0x10,  /* wird immer mit �bertragen */
        CURSE_SPREADMODULO = 0x20,  /* personenweise weitergabe */
        CURSE_SPREADCHANCE = 0x30   /* Ansteckungschance je nach Mengenverh�ltnis */
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
#define M_VIGOUR     32         /* das Maximum der beiden St�rken wird die
     St�rke des neuen Zaubers */
#define M_VIGOUR_ADD 64         /* Vigour wird addiert */

    /* ------------------------------------------------------------- */
    /* Allgemeine Zauberwirkungen */

    typedef struct curse {
        struct curse *nexthash;
        int no;                     /* 'Einheitennummer' dieses Curse */
        const struct curse_type *type;      /* Zeiger auf ein curse_type-struct */
        int flags;         /* WARNING: these are XORed with type->flags! */
        int duration;               /* Dauer der Verzauberung. Wird jede Runde vermindert */
        float vigour;              /* St�rke der Verzauberung, Widerstand gegen Antimagie */
        struct unit *magician;      /* Pointer auf den Magier, der den Spruch gewirkt hat */
        float effect;
        variant data;               /* pointer auf spezielle curse-unterstructs */
    } curse;

#define c_flags(c) ((c)->type->flags ^ (c)->flags)

    /* ------------------------------------------------------------- */

    typedef struct curse_type {
        const char *cname;          /* Name der Zauberwirkung, Identifizierung des curse */
        int typ;
        int flags;
        int mergeflags;
        struct message *(*curseinfo) (const void *, objtype_t, const struct curse *,
            int);
        void(*change_vigour) (curse *, float);
        int(*read) (struct storage * store, curse * c, void *target);
        int(*write) (struct storage * store, const struct curse * c,
            const void *target);
        int(*cansee) (const struct faction *, const void *, objtype_t,
            const struct curse *, int);
        int(*age) (curse *);
    } curse_type;

    extern struct attrib_type at_curse;
    void curse_write(const struct attrib *a, const void *owner,
    struct storage *store);
    int curse_read(struct attrib *a, void *owner, struct storage *store);

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
        const curse_type * ctype, float vigour, int duration, float ceffect,
        int men);
    /* Verzweigt automatisch zum passenden struct-typ. Sollte es schon
     * einen Zauber dieses Typs geben, so wird der neue dazuaddiert. Die
     * Zahl der verzauberten Personen sollte beim Aufruf der Funktion
     * nochmal gesondert auf min(get_cursedmen, u->number) gesetzt werden.
     */

    void destroy_curse(curse * c);

    bool is_cursed_internal(struct attrib *ap, const curse_type * ctype);
    /* ignoriert CURSE_ISNEW */

    bool remove_curse(struct attrib **ap, const struct curse *c);
    /* l�scht einen konkreten Spruch auf einem Objekt.
     */

    int curse_geteffect_int(const struct curse *c);
    float curse_geteffect(const struct curse *c);
    /* gibt die Auswirkungen der Verzauberungen zur�ck. zB bei
     * Skillmodifiziernden Verzauberungen ist hier der Modifizierer
     * gespeichert. Wird automatisch beim Anlegen eines neuen curse
     * gesetzt. Gibt immer den ersten Treffer von ap aus zur�ck.
     */

    float curse_changevigour(struct attrib **ap, curse * c, float i);
    /* ver�ndert die St�rke der Verzauberung um i */

    int get_cursedmen(struct unit *u, const struct curse *c);
    /* gibt bei Personenbeschr�nkten Verzauberungen die Anzahl der
     * betroffenen Personen zur�ck. Ansonsten wird 0 zur�ckgegeben. */

    void c_setflag(curse * c, unsigned int flag);
    void c_clearflag(curse * c, unsigned int flags);
    /* setzt/loescht Spezialflag einer Verzauberung (zB 'dauert ewig') */

    void transfer_curse(struct unit *u, struct unit *u2, int n);
    /* sorgt daf�r, das bei der �bergabe von Personen die curse-attribute
     * korrekt gehandhabt werden. Je nach internen Flag kann dies
     * unterschiedlich gew�nscht sein
     * */

    struct curse *get_curse(struct attrib *ap, const curse_type * ctype);
    /* gibt pointer auf die erste curse-struct zur�ck, deren Typ ctype ist,
     * oder einen NULL-pointer
     * */

    int find_cursebyname(const char *c);
    const curse_type *ct_find(const char *c);
    void ct_register(const curse_type *);
    /* Regionszauber */

    curse *cfindhash(int i);

    curse *findcurse(int curseid);

    void curse_init(struct attrib *a);
    void curse_done(struct attrib *a);
    int curse_age(struct attrib *a);

    float destr_curse(struct curse *c, int cast_level, float force);

    int resolve_curse(variant data, void *address);
    bool is_cursed_with(const struct attrib *ap, const struct curse *c);

    bool curse_active(const struct curse *c);
    /* gibt true, wenn der Curse nicht NULL oder inaktiv ist */

    /*** COMPATIBILITY MACROS. DO NOT USE FOR NEW CODE, REPLACE IN OLD CODE: */
    const char *oldcursename(int id);
    struct message *cinfo_simple(const void *obj, objtype_t typ,
        const struct curse *c, int self);

#define is_cursed(a, id, id2) \
  curse_active(get_curse(a, ct_find(oldcursename(id))))
#define get_curseeffect(a, id, id2) \
  curse_geteffect(get_curse(a, ct_find(oldcursename(id))))

    /* eressea-defined attribute-type flags */
#define ATF_CURSE  ATF_USER_DEFINED

#ifdef __cplusplus
}
#endif
#endif
