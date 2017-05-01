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

#ifndef H_KRNL_SPELL
#define H_KRNL_SPELL
#ifdef __cplusplus
extern "C" {
#endif

    struct castorder;
    struct unit;
    struct region;
    struct spell;
    struct spell_component;
    struct selist;
    struct attrib_type;

    typedef int(*spell_f)(struct castorder * co);
    typedef void(*fumble_f)(const struct castorder * co);

    typedef struct spell {
        char *sname;
        char *syntax;
        char *parameter;
        int sptyp;
        int rank;                   /* Reihenfolge der Zauber */
        struct spell_component *components;
        spell_f cast_fun;
    } spell;

    typedef struct spellref {
        char * name;
        struct spell *sp;
    } spellref;

    void add_fumble(const char *sname, fumble_f fun);
    fumble_f get_fumble(const char *sname);

    struct spellref *spellref_create(struct spell *sp, const char *name);
    void spellref_free(struct spellref *spref);
    struct spell *spellref_get(struct spellref *spref);

    int sp_antimagiczone(struct castorder *co);

    struct spell * create_spell(const char * name);
    struct spell * find_spell(const char *name);
    void add_spell(struct selist **slistp, spell * sp);
    void free_spells(void);

    /** globals **/
    extern struct attrib_type at_unitdissolve;
    extern struct attrib_type at_wdwpyramid;
    extern struct selist * spells;
#ifdef __cplusplus
}
#endif
#endif
/* ------------------------------------------------------------- *//* Erl�uterungen zu den Spruchdefinitionen
 *
 * Spruchstukturdefinition:
 * spell{
 *  id, name,
 *  beschreibung,
 *  syntax,
 *  parameter,
 *  magietyp,
 *  sptyp,
 *  rank,level,
 *  costtyp, aura,
 *  komponenten[5][2][faktorart],
 *  &funktion, patzer}
 *
 * id:
 * SPL_NOSPELL muss der letzte Spruch in der Liste spelldaten sein,
 * denn nicht auf die Reihenfolge in der Liste sondern auf die id wird
 * gepr�ft
 *
 * sptyp:
 * besondere Spruchtypen und Flags
 *    (Regionszauber, Kampfzauber, Farcastbar, Stufe variable, ..)
 *
 * rank:
 * gibt die Priorit�t und damit die Reihenfolge an, in der der Spruch
 * gezaubert wird.
 * 1: Aura �bertragen
 * 2: Antimagie
 * 3: Magierver�ndernde Spr�che (Magic Boost, ..)
 * 4: Monster erschaffen
 * 5: Standartlevel
 * 7: Teleport
 *
 * Komponenten[Anzahl m�gl. Items][Art:Anzahl:Kostentyp]
 *
 * R_AURA:
 * Grundkosten f�r einen Zauber. Soviel Mp m�ssen mindestens investiert
 * werden, um den Spruch zu wirken. Zus�tzliche Mp k�nnen unterschiedliche
 * Auswirkungen haben, die in der Spruchfunktionsroutine definiert werden.
 *
 * R_PERMAURA:
 * Kosten an permantenter Aura
 *
 * Komponenten Kostentyp:
 * SPC_LEVEL == Spruch mit Levelabh�ngigen Magiekosten. Die angegeben
 * Kosten m�ssen f�r Stufe 1 berechnet sein.
 * SPC_FIX   == Feste Kosten
 *
 * Wenn keine spezielle Syntax angegeben ist, wird die
 * Syntaxbeschreibung aus sptyp  generiert:
 * FARCASTING: ZAUBER [REGION x y]
 * SPELLLEVEL: ZAUBER [STUFE n]
 * UNITSPELL : ZAUBER <spruchname> <Einheit-Nr> [<Einheit-Nr> ..]
 * SHIPSPELL : ZAUBER <spruchname> <Schiff-Nr> [<Schiff-Nr> ..]
 * BUILDINGSPELL: ZAUBER <spruchname> <Gebaeude-Nr> [<Gebaeude-Nr> ..]
 * PRECOMBATSPELL : KAMPFZAUBER [STUFE n] <spruchname>
 * COMBATSPELL    : KAMPFZAUBER [STUFE n] <spruchname>
 * POSTCOMBATSPELL: KAMPFZAUBER [STUFE n] <spruchname>
 *
 * Das Parsing
 *
 * Der String spell->parameter gibt die Syntax an, nach der die
 * Parameter des Spruches in add_spellparameter() geparst werden sollen.
 *
 * u : eine Einheitennummer
 * r : hier kommen zwei Regionskoordinaten x y
 * b : Geb�ude- oder Burgnummer
 * s : Schiffsnummer
 * c : String, wird ohne Weiterverarbeitung �bergeben
 * i : Zahl (int), wird ohne Weiterverarbeitung �bergeben
 * k : Keywort - dieser String gibt den Paramter an, der folgt. Der
 *     Parameter wird mit findparam() identifiziert.
 *     k muss immer von einem c als Platzhalter f�r das Objekt gefolgt
 *     werden.
 *     Ein gutes Beispiel sind hierf�r die Spr�che zur Magieanalyse.
 * + : gibt an, das der vorherige Parameter mehrfach vorkommen kann. Da
 *     ein Ende nicht definiert werden kann, muss dies immer am Schluss
 *     kommen.
 *
 * Flags f�r das Parsing:
 * TESTRESISTANCE : alle Zielobjekte, also alle Parameter vom Typ Unit,
 *		  Burg, Schiff oder Region, werden auf ihre
 *		  Magieresistenz �berpr�ft
 * TESTCANSEE     : jedes Objekt vom Typ Einheit wird auf seine
 *		  Sichtbarkeit �berpr�ft
 * SEARCHLOCAL   : die Zielobjekte werden nur regional gesucht
 * REGIONSPELL    : Ziel ist die Region, auch wenn kein Zielobjekt
 *		  angegeben wird. Ist TESTRESISTANCE gesetzt, so wird
 *		  die Magieresistenz der Region �berpr�ft
 *
 * Bei fehlendem Ziel oder wenn dieses dem Zauber widersteht, wird die
 * Spruchfunktion nicht aufgerufen.
 * Sind zu wenig Parameter vorhanden, wird der Zauber ebenfalls nicht
 * ausgef�hrt.
 * Ist eins von mehreren Zielobjekten resistent, so wird das Flag
 * pa->param[n]->flag == TARGET_RESISTS
 * Ist eins von mehreren Zielobjekten nicht gefunden worden, so ist
 * pa->param[n]->flag == TARGET_NOTFOUND
 *
 *//* ------------------------------------------------------------- */
