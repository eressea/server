#ifndef H_MAGIC
#define H_MAGIC

#include "kernel/types.h"
#include "util/variant.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAXCOMBATSPELLS 3       /* PRECOMBAT COMBAT POSTCOMBAT */
#define MAX_SPELLRANK 9         /* Standard-Rank 5 */
#define MAXINGREDIENT	5       /* bis zu 5 Komponenten pro Zauber */
#define CHAOSPATZERCHANCE 10    /* +10% Chance zu Patzern */
#define IRONGOLEM_CRUMBLE   15  /* monatlich Chance zu zerfallen */
#define STONEGOLEM_CRUMBLE  10  /* monatlich Chance zu zerfallen */

    struct sc_mage;
    struct unit;
    struct resource;

    extern const char *magic_school[MAXMAGIETYP];
    extern struct attrib_type at_familiar;
    extern struct attrib_type at_familiarmage;

    extern const struct curse_type ct_magicresistance;

    /* ------------------------------------------------------------- */
    /* Spruchparameter
     * Wir suchen beim Parsen des Befehls erstmal nach lokalen Objekten,
     * erst in verify_targets wird dann global gesucht, da in den meisten
     * Faellen das Zielobjekt lokal sein duerfte */

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
     *   (koennen sich durch Questen absolut veraendern und durch Gegenstaende
     *   temporaer). Auch fuer Artefakt benoetigt man permanente MP
     * - Anzahl bereits gezauberte Sprueche diese Runde
     * - Kampfzauber (3) (vor/waehrend/nach)
     * - Spruchliste
     */

    typedef struct spell_names {
        struct spell_names *next;
        const struct locale *lang;
        void * tokens;
    } spell_names;

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
        int level;                  /* gewuenschte Stufe oder Stufe des Magiers */
        double force;               /* Staerke des Zaubers */
        struct region *_rtarget;     /* Zielregion des Spruchs */
        int distance;               /* Entfernung zur Zielregion */
        struct order *order;        /* Befehl */
        struct spellparameter *par; /* fuer weitere Parameter */
    } castorder;

    struct unit * co_get_caster(const struct castorder * co);
    struct unit * co_get_magician(const struct castorder * co);
    struct region * co_get_region(const struct castorder * co);

    /* Flag Spruchkostenberechnung: */
    enum spellcost_t {
        SPC_FIX,                    /* Fixkosten */
        SPC_LEVEL,                  /* Komponenten pro Level */
        SPC_LINEAR                  /* Komponenten pro Level und muessen vorhanden sein */
    };

    typedef struct spell_component {
        const struct resource_type *type;
        int amount;
        enum spellcost_t cost;
    } spell_component;

    /* ------------------------------------------------------------- */

    /* besondere Spruchtypen */
#define FARCASTING      (1<<0)  /* ZAUBER [struct region x y] */
#define SPELLLEVEL      (1<<1)  /* ZAUBER [STUFE x] */

#define OCEANCASTABLE   (1<<2) /* Koennen auch nicht-Meermenschen auf
     hoher See zaubern */
#define ONSHIPCAST      (1<<3) /* kann auch auf von Land ablegenden
     Schiffen stehend gezaubert werden */
#define TESTCANSEE      (1<<4) /* alle Zielunits auf cansee pruefen */

     /* ID's koennen zu drei unterschiedlichen Entitaeten gehoeren: Einheiten,
     * Gebaeuden und Schiffen. */
#define UNITSPELL       (1<<5)  /* ZAUBER .. <Einheit-Nr> [<Einheit-Nr> ..] */
#define SHIPSPELL       (1<<6)  /* ZAUBER .. <Schiff-Nr> [<Schiff-Nr> ..] */
#define BUILDINGSPELL   (1<<7)  /* ZAUBER .. <Gebaeude-Nr> [<Gebaeude-Nr> ..] */
#define REGIONSPELL     (1<<8)  /* wirkt auf struct region */
#define GLOBALTARGET    (1<<9) /* Ziel kann ausserhalb der region sein */
#define NORESISTANCE    (1<<10) /* Zielobjekte nicht auf Magieresistenz pruefen. */

#define PRECOMBATSPELL	(1<<11)  /* PRAEKAMPFZAUBER .. */
#define COMBATSPELL     (1<<12)  /* KAMPFZAUBER .. */
#define POSTCOMBATSPELL	(1<<13)  /* POSTKAMPFZAUBER .. */
#define ISCOMBATSPELL   (PRECOMBATSPELL|COMBATSPELL|POSTCOMBATSPELL)


#define NOTFAMILIARCAST (1<<14) /* not used by XML? */
#define ANYTARGET       (UNITSPELL|REGIONSPELL|BUILDINGSPELL|SHIPSPELL) /* wirkt auf alle objekttypen (unit, ship, building, region) */

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
     * identifizieren der Komponennten ueber den Missversuch ist nicht
     * moeglich
     * Spruchzauberrei: 'ZAUBER [struct region x y] [STUFE a] "Spruchname" [Ziel]'
     * Gegenstandszauberrei: 'BENUTZE "Gegenstand" [Ziel]'
     *
     * Die Funktionen:
     */

    /* Magier */
    struct sc_mage *create_mage(struct unit *u, magic_t mtyp);
    /*      macht die struct unit zu einem neuen Magier: legt die struct u->mage an
     *      und     initialisiert den Magiertypus mit mtyp.  */
    struct sc_mage *get_mage(const struct unit *u);

    enum magic_t mage_get_type(const struct sc_mage *mage);
    const struct spell *mage_get_combatspell(const struct sc_mage *mage, int nr, int *level);
    struct spellbook * mage_get_spellbook(const struct sc_mage * mage);
    int mage_get_spellpoints(const struct sc_mage *m);
    void mage_set_spellpoints(struct sc_mage *m, int aura);
    int mage_change_spellpoints(struct sc_mage *m, int delta);

    enum magic_t unit_get_magic(const struct unit *u);
    void unit_set_magic(struct unit *u, enum magic_t mtype);
    struct spellbook * unit_get_spellbook(const struct unit * u);
    void unit_add_spell(struct unit * u, struct spell * sp, int level);
    int unit_spell_level(const struct unit *u, const struct spell *sp);

    bool is_mage(const struct unit *u);
    /*      gibt true, wenn u->mage gesetzt.  */
    bool is_familiar(const struct unit *u);
    /*      gibt true, wenn eine Familiar-Relation besteht.  */

    /* Sprueche */
    int get_combatspelllevel(const struct unit *u, int nr);
    /*  versucht, eine eingestellte maximale Kampfzauberstufe
     *  zurueckzugeben. 0 = Maximum, -1 u ist kein Magier. */
    const struct spell *get_combatspell(const struct unit *u, int nr);
    /*      gibt den Kampfzauber nr [pre/kampf/post] oder NULL zurueck */
    void set_combatspell(struct unit *u, struct spell * sp, struct order *ord,
        int level);
    /*      setzt Kampfzauber */
    void unset_combatspell(struct unit *u, struct spell * sp);
    /*      loescht Kampfzauber */
    /* fuegt den Spruch mit der Id spellid der Spruchliste der Einheit hinzu. */
    bool u_hasspell(const struct unit *u, const struct spell *sp);
    /* prueft, ob der Spruch in der Spruchliste der Einheit steht. */
    void pick_random_spells(struct faction *f, int level, struct spellbook * book, int num_spells);
    bool knowsspell(
        const struct region *r,
        const struct unit *u,
        const struct spell * sp);
    /* prueft, ob die Einheit diesen Spruch gerade beherrscht, dh
     * mindestens die erforderliche Stufe hat. Hier koennen auch Abfragen
     * auf spezielle Antimagiezauber auf Regionen oder Einheiten eingefuegt
     * werden
     */

    /* Magiepunkte */
    int get_spellpoints(const struct unit *u);
    /*      Gibt die aktuelle Anzahl der Magiepunkte der Einheit zurueck */
    void set_spellpoints(struct unit *u, int sp);
    /* setzt die Magiepunkte auf sp */
    int change_spellpoints(struct unit *u, int mp);
    /*      veraendert die Anzahl der Magiepunkte der Einheit um +mp */
    int max_spellpoints(const struct unit *u, const struct region *r);
    /*      gibt die aktuell maximal moeglichen Magiepunkte der Einheit zurueck */
    int change_maxspellpoints(struct unit *u, int csp);
    /* veraendert die maximalen Magiepunkte einer Einheit */

    /* Zaubern */
    double spellpower(struct region *r, struct unit *u, const struct spell * sp,
        int cast_level);
    /*      ermittelt die Staerke eines Spruchs */
    bool fumble(struct region *r, struct unit *u, const struct spell * sp,
        int cast_level);
    /*      true, wenn der Zauber misslingt, bei false gelingt der Zauber */

    typedef struct spellrank {
        struct castorder *begin;
        struct castorder **handle_end;
    } spellrank;

    struct castorder *create_castorder(struct castorder * co, struct unit *caster,
    struct unit * familiar, const struct spell * sp, struct region * r,
        int lev, double force, int range, struct order * ord, struct spellparameter * p);
    void free_castorder(struct castorder *co);
    /* Zwischenspreicher fuer Zauberbefehle, notwendig fuer Prioritaeten */
    void add_castorder(struct spellrank *cll, struct castorder *co);
    /* Haenge c-order co an die letze c-order von cll an */
    void free_castorders(struct castorder *co);
    /* Speicher wieder freigeben */
    int cast_spell(struct castorder *co);

    /* Pruefroutinen fuer Zaubern */
    int countspells(struct unit *u, int step);
    int spellcount(const struct unit *u);
    /*      erhoeht den Counter fuer Zaubersprueche um 'step' und gibt die neue
     *      Anzahl der gezauberten Sprueche zurueck. */
    int auracost(const struct unit *caster, const struct spell *sp);
    int spellcost(const struct unit *caster, const struct spell_component *spc);
    /*      gibt die fuer diesen Spruch derzeit notwendigen Magiepunkte auf der
     *      geringstmoeglichen Stufe zurueck, schon um den Faktor der bereits
     *      zuvor gezauberten Sprueche erhoeht */
    bool cancast(struct unit *u, const struct spell * spruch, int eff_stufe,
        int distance, struct order *ord);
    /*      true, wenn Einheit alle Komponenten des Zaubers (incl. MP) fuer die
     *      geringstmoegliche Stufe hat und den Spruch beherrscht */
    void pay_spell(struct unit *mage, const struct unit *caster, const struct spell * sp, int eff_stufe, int distance);
    /*      zieht die Komponenten des Zaubers aus dem Inventory der Einheit
     *      ab. Die effektive Stufe des gezauberten Spruchs ist wichtig fuer
     *      die korrekte Bestimmung der Magiepunktkosten */
    int max_spell_level(struct unit *mage, struct unit *caster,
        const struct spell * sp, int cast_level, int distance, 
        struct resource **reslist_p);
    /*      ermittelt die effektive Stufe des Zaubers. Dabei ist cast_level
     *      die gewuenschte maximale Stufe (im Normalfall Stufe des Magiers,
     *      bei Farcasting Stufe*2^Entfernung) */
    bool is_magic_resistant(const struct unit *magician, const struct unit *target,
        int resist_bonus);
    /*      Mapperfunktion fuer target_resists_magic() vom Typ struct unit. */
    variant magic_resistance(const struct unit *target);
    /*      gibt die Chance an, mit der einem Zauber widerstanden wird. Je
     *      groesser, desto resistenter ist da Opfer */
    bool target_resists_magic(const struct unit *magician, const void *obj, int objtyp,
        int bonus_percent);
    variant resist_chance(const struct unit *magician, const void *obj, int objtyp,
        int bonus_percent);
    /*      gibt false zurueck, wenn der Zauber gelingt, true, wenn das Ziel
     *      widersteht */
    struct spell * unit_getspell(struct unit *u, const char *s,
        const struct locale *lang);
    const char *magic_name(magic_t mtype, const struct locale *lang);

    /* Sprueche in der struct region */
    /* (sind in curse) */
    void set_familiar(struct unit * mage, struct unit * familiar);
    struct unit *get_familiar(const struct unit *u);
    struct unit *get_familiar_mage(const struct unit *u);
    struct unit *get_clone(const struct unit *u);
    void remove_familiar(struct unit *mage);
    void create_newfamiliar(struct unit *mage, struct unit *familiar);
    void create_newclone(struct unit *mage, struct unit *familiar);

    void fix_fam_spells(struct unit *u);
    void fix_fam_migrant(struct unit *u);

    const char *mkname_spell(const struct spell *sp);
    const char *spell_name(const char *spname, const struct locale *lang);
    const char *spell_info(const struct spell *sp,
        const struct locale *lang);
    const char *curse_name(const struct curse_type *ctype,
        const struct locale *lang);

    struct message *msg_unitnotfound(const struct unit *mage,
    struct order *ord, const struct spllprm *spobj);
    bool FactionSpells(void);

    struct spellbook * get_spellbook(const char * name);
    void free_spellbooks(void);
    void free_spellbook(struct spellbook *sb);

    void register_magicresistance(void);
#ifdef __cplusplus
}
#endif
#endif
