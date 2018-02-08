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

#ifndef H_KRNL_MAGIC
#define H_KRNL_MAGIC

#include <kernel/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    /* ------------------------------------------------------------- */

#define MAXCOMBATSPELLS 3       /* PRECOMBAT COMBAT POSTCOMBAT */
#define MAX_SPELLRANK 9         /* Standard-Rank 5 */
#define MAXINGREDIENT	5       /* bis zu 5 Komponenten pro Zauber */
#define CHAOSPATZERCHANCE 10    /* +10% Chance zu Patzern */

    /* ------------------------------------------------------------- */

#define IRONGOLEM_CRUMBLE   15  /* monatlich Chance zu zerfallen */
#define STONEGOLEM_CRUMBLE  10  /* monatlich Chance zu zerfallen */

    extern const char *magic_school[MAXMAGIETYP];
    extern struct attrib_type at_familiar;
    extern struct attrib_type at_familiarmage;

    /* ------------------------------------------------------------- */
    /* Spruchparameter
     * Wir suchen beim Parsen des Befehls erstmal nach lokalen Objekten,
     * erst in verify_targets wird dann global gesucht, da in den meisten
     * Fällen das Zielobjekt lokal sein dürfte */

    /* siehe auch objtype_t in objtypes.h */
    typedef enum {
        SPP_REGION,                 /* "r" : findregion(x,y) -> *region */
        SPP_UNIT,                   /*  -  : atoi36() -> int */
        SPP_TEMP,                   /*  -  : temp einheit */
        SPP_BUILDING,               /*  -  : atoi() -> int */
        SPP_SHIP,                   /*  -  : atoi() -> int */
        SPP_STRING,                 /* "c" */
        SPP_INT                     /* "i" : atoi() -> int */
    } sppobj_t;

    typedef struct spllprm {
        sppobj_t typ;
        int flag;
        union {
            struct region *r;
            struct unit *u;
            struct building *b;
            struct ship *sh;
            char *s;
            char *xs;
            int i;
        } data;
    } spllprm;

    typedef struct spellparameter {
        int length;                 /* Anzahl der Elemente */
        struct spllprm **param;
    } spellparameter;

    typedef struct strarray {
        int length;                 /* Anzahl der Elemente */
        char **strings;
    } strarray;

#define TARGET_RESISTS (1<<0)
#define TARGET_NOTFOUND (1<<1)

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

    typedef struct combatspell {
        int level;
        const struct spell *sp;
    } combatspell;

    typedef struct spell_names {
        struct spell_names *next;
        const struct locale *lang;
        void * tokens;
    } spell_names;

    typedef struct sc_mage {
        magic_t magietyp;
        int spellpoints;
        int spchange;
        int spellcount;
        combatspell combatspells[MAXCOMBATSPELLS];
        struct spellbook *spellbook;
    } sc_mage;

    /* ------------------------------------------------------------- */
    /* Zauberliste */

    typedef struct castorder {
        struct castorder *next;
        union {
            struct unit *u;
            struct fighter *fig;
        } magician;                 /* Magier (kann vom Typ struct unit oder fighter sein) */
        struct unit *_familiar;     /* Vertrauter, gesetzt, wenn der Spruch durch
                                       den Vertrauten gezaubert wird */
        const struct spell *sp;     /* Spruch */
        int level;                  /* gewünschte Stufe oder Stufe des Magiers */
        double force;               /* Stärke des Zaubers */
        struct region *_rtarget;     /* Zielregion des Spruchs */
        int distance;               /* Entfernung zur Zielregion */
        struct order *order;        /* Befehl */
        struct spellparameter *par; /* für weitere Parameter */
    } castorder;

    struct unit * co_get_caster(const struct castorder * co);
    struct region * co_get_region(const struct castorder * co);

    typedef struct spell_component {
        const struct resource_type *type;
        int amount;
        int cost;
    } spell_component;

    /* ------------------------------------------------------------- */

    /* besondere Spruchtypen */
#define FARCASTING      (1<<0)  /* ZAUBER [struct region x y] */
#define SPELLLEVEL      (1<<1)  /* ZAUBER [STUFE x] */

    /* ID's können zu drei unterschiedlichen Entitäten gehören: Einheiten,
     * Gebäuden und Schiffen. */
#define UNITSPELL       (1<<2)  /* ZAUBER .. <Einheit-Nr> [<Einheit-Nr> ..] */
#define SHIPSPELL       (1<<3)  /* ZAUBER .. <Schiff-Nr> [<Schiff-Nr> ..] */
#define BUILDINGSPELL   (1<<4)  /* ZAUBER .. <Gebaeude-Nr> [<Gebaeude-Nr> ..] */
#define REGIONSPELL     (1<<5)  /* wirkt auf struct region */

#define PRECOMBATSPELL	(1<<7)  /* PRÄKAMPFZAUBER .. */
#define COMBATSPELL     (1<<8)  /* KAMPFZAUBER .. */
#define POSTCOMBATSPELL	(1<<9)  /* POSTKAMPFZAUBER .. */
#define ISCOMBATSPELL   (PRECOMBATSPELL|COMBATSPELL|POSTCOMBATSPELL)

#define OCEANCASTABLE   (1<<10) /* Können auch nicht-Meermenschen auf
     hoher See zaubern */
#define ONSHIPCAST      (1<<11) /* kann auch auf von Land ablegenden
     Schiffen stehend gezaubert werden */
    /*  */
#define NOTFAMILIARCAST (1<<12)
#define TESTRESISTANCE  (1<<13) /* alle Zielobjekte (u, s, b, r) auf
                                       Magieresistenz prüfen */
#define SEARCHLOCAL     (1<<14) /* Ziel muss in der target_region sein */
#define TESTCANSEE      (1<<15) /* alle Zielunits auf cansee prüfen */
#define ANYTARGET       (UNITSPELL|REGIONSPELL|BUILDINGSPELL|SHIPSPELL) /* wirkt auf alle objekttypen (unit, ship, building, region) */

    /* Flag Spruchkostenberechnung: */
    enum {
        SPC_FIX,                    /* Fixkosten */
        SPC_LEVEL,                  /* Komponenten pro Level */
        SPC_LINEAR                  /* Komponenten pro Level und müssen vorhanden sein */
    };

    /* ------------------------------------------------------------- */
    /* Prototypen */

    void magic(void);

    void regenerate_aura(void);

    extern struct attrib_type at_mage;
    extern struct attrib_type at_familiarmage;
    extern struct attrib_type at_familiar;
    extern struct attrib_type at_clonemage;
    extern struct attrib_type at_clone;
    extern struct attrib_type at_icastle;

    void make_icastle(struct building *b, const struct building_type *btype, int timeout);
    const struct building_type *icastle_type(const struct attrib *a);

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
    sc_mage *create_mage(struct unit *u, magic_t mtyp);
    /*      macht die struct unit zu einem neuen Magier: legt die struct u->mage an
     *      und     initialisiert den Magiertypus mit mtyp.  */
    sc_mage *get_mage(const struct unit *u);
    sc_mage *get_mage_depr(const struct unit *u);
    /*      gibt u->mage zurück, bei nicht-Magiern *NULL */
    bool is_mage(const struct unit *u);
    /*      gibt true, wenn u->mage gesetzt.  */
    bool is_familiar(const struct unit *u);
    /*      gibt true, wenn eine Familiar-Relation besteht.  */

    /* Sprüche */
    int get_combatspelllevel(const struct unit *u, int nr);
    /*  versucht, eine eingestellte maximale Kampfzauberstufe
     *  zurückzugeben. 0 = Maximum, -1 u ist kein Magier. */
    const struct spell *get_combatspell(const struct unit *u, int nr);
    /*      gibt den Kampfzauber nr [pre/kampf/post] oder NULL zurück */
    void set_combatspell(struct unit *u, struct spell * sp, struct order *ord,
        int level);
    /*      setzt Kampfzauber */
    void unset_combatspell(struct unit *u, struct spell * sp);
    /*      löscht Kampfzauber */
    /* fügt den Spruch mit der Id spellid der Spruchliste der Einheit hinzu. */
    int u_hasspell(const struct unit *u, const struct spell *sp);
    /* prüft, ob der Spruch in der Spruchliste der Einheit steht. */
    void pick_random_spells(struct faction *f, int level, struct spellbook * book, int num_spells);
    void show_new_spells(struct faction * f, int level, const struct spellbook *book);
    bool knowsspell(const struct region *r, const struct unit *u,
        const struct spell * sp);
    /* prüft, ob die Einheit diesen Spruch gerade beherrscht, dh
     * mindestens die erforderliche Stufe hat. Hier können auch Abfragen
     * auf spezielle Antimagiezauber auf Regionen oder Einheiten eingefügt
     * werden
     */

    /* Magiepunkte */
    int get_spellpoints(const struct unit *u);
    /*      Gibt die aktuelle Anzahl der Magiepunkte der Einheit zurück */
    void set_spellpoints(struct unit *u, int sp);
    /* setzt die Magiepunkte auf sp */
    int change_spellpoints(struct unit *u, int mp);
    /*      verändert die Anzahl der Magiepunkte der Einheit um +mp */
    int max_spellpoints(const struct region *r, const struct unit *u);
    /*      gibt die aktuell maximal möglichen Magiepunkte der Einheit zurück */
    int change_maxspellpoints(struct unit *u, int csp);
    /* verändert die maximalen Magiepunkte einer Einheit */

    /* Zaubern */
    extern double spellpower(struct region *r, struct unit *u, const struct spell * sp,
        int cast_level, struct order *ord);
    /*      ermittelt die Stärke eines Spruchs */
    bool fumble(struct region *r, struct unit *u, const struct spell * sp,
        int cast_level);
    /*      true, wenn der Zauber misslingt, bei false gelingt der Zauber */

    typedef struct spellrank {
        struct castorder *begin;
        struct castorder **end;
    } spellrank;

    struct castorder *create_castorder(struct castorder * co, struct unit *caster,
    struct unit * familiar, const struct spell * sp, struct region * r,
        int lev, double force, int range, struct order * ord, struct spellparameter * p);
    void free_castorder(struct castorder *co);
    /* Zwischenspreicher für Zauberbefehle, notwendig für Prioritäten */
    void add_castorder(struct spellrank *cll, struct castorder *co);
    /* Hänge c-order co an die letze c-order von cll an */
    void free_castorders(struct castorder *co);
    /* Speicher wieder freigeben */
    int cast_spell(struct castorder *co);

    /* Prüfroutinen für Zaubern */
    int countspells(struct unit *u, int step);
    /*      erhöht den Counter für Zaubersprüche um 'step' und gibt die neue
     *      Anzahl der gezauberten Sprüche zurück. */
    int spellcost(struct unit *u, const struct spell * sp);
    /*      gibt die für diesen Spruch derzeit notwendigen Magiepunkte auf der
     *      geringstmöglichen Stufe zurück, schon um den Faktor der bereits
     *      zuvor gezauberten Sprüche erhöht */
    bool cancast(struct unit *u, const struct spell * spruch, int eff_stufe,
        int distance, struct order *ord);
    /*      true, wenn Einheit alle Komponenten des Zaubers (incl. MP) für die
     *      geringstmögliche Stufe hat und den Spruch beherrscht */
    void pay_spell(struct unit *u, const struct spell * sp, int eff_stufe, int distance);
    /*      zieht die Komponenten des Zaubers aus dem Inventory der Einheit
     *      ab. Die effektive Stufe des gezauberten Spruchs ist wichtig für
     *      die korrekte Bestimmung der Magiepunktkosten */
    int eff_spelllevel(struct unit *u, const struct spell * sp, int cast_level,
        int distance);
    /*      ermittelt die effektive Stufe des Zaubers. Dabei ist cast_level
     *      die gewünschte maximale Stufe (im Normalfall Stufe des Magiers,
     *      bei Farcasting Stufe*2^Entfernung) */
    bool is_magic_resistant(struct unit *magician, struct unit *target, int
        resist_bonus);
    /*      Mapperfunktion für target_resists_magic() vom Typ struct unit. */
    variant magic_resistance(struct unit *target);
    /*      gibt die Chance an, mit der einem Zauber widerstanden wird. Je
     *      größer, desto resistenter ist da Opfer */
    bool target_resists_magic(struct unit *magician, void *obj, int objtyp,
        int resist_bonus);
    /*      gibt false zurück, wenn der Zauber gelingt, true, wenn das Ziel
     *      widersteht */
    extern struct spell * unit_getspell(struct unit *u, const char *s,
        const struct locale *lang);

    /* Sprüche in der struct region */
    /* (sind in curse) */
    void set_familiar(struct unit * mage, struct unit * familiar);
    struct unit *get_familiar(const struct unit *u);
    struct unit *get_familiar_mage(const struct unit *u);
    struct unit *get_clone(const struct unit *u);
    struct unit *get_clone_mage(const struct unit *u);
    void remove_familiar(struct unit *mage);
    void create_newfamiliar(struct unit *mage, struct unit *familiar);
    void create_newclone(struct unit *mage, struct unit *familiar);
    struct unit *has_clone(struct unit *mage);

    const char *spell_info(const struct spell *sp,
        const struct locale *lang);
    const char *spell_name(const struct spell *sp,
        const struct locale *lang);
    const char *curse_name(const struct curse_type *ctype,
        const struct locale *lang);

    struct message *msg_unitnotfound(const struct unit *mage,
    struct order *ord, const struct spllprm *spobj);
    bool FactionSpells(void);

    struct spellbook * get_spellbook(const char * name);
    void free_spellbooks(void);
    void free_spellbook(struct spellbook *sb);
#ifdef __cplusplus
}
#endif
#endif
