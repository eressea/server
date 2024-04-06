#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "magic.h"

#include "contact.h"
#include "helpers.h"
#include "laws.h"
#include "spells.h"
#include "study.h"

#include <triggers/timeout.h>
#include <triggers/shock.h>
#include <triggers/killunit.h>
#include <triggers/giveitem.h>
#include <triggers/changerace.h>
#include <triggers/clonedied.h>

#include <spells/regioncurse.h>
#include <spells/buildingcurse.h>
#include <spells/unitcurse.h>

#include <kernel/ally.h>
#include <kernel/attrib.h>
#include <kernel/building.h>
#include <kernel/callbacks.h>
#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/equipment.h>
#include <kernel/event.h>
#include <kernel/faction.h>
#include <kernel/gamedata.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/objtypes.h>
#include <kernel/order.h>
#include <kernel/pathfinder.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include "kernel/skills.h"
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

/* util includes */
#include <util/aliases.h>
#include <util/base36.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/message.h>
#include <util/param.h>
#include <util/parser.h>
#include <util/rand.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/umlaut.h>

#include <critbit.h>
#include <selist.h>
#include <storage.h>
#include <strings.h>

#include <stb_ds.h>

/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

const char *magic_school[MAXMAGIETYP] = {
    "gray",
    "illaun",
    "tybied",
    "cerddor",
    "gwyrrd",
    "draig",
    "common"
};

struct combatspell {
    int level;
    const struct spell *sp;
};

typedef struct sc_mage {
    magic_t magietyp;
    int spellpoints;
    int spchange;
    int spellcount;
    struct combatspell combatspells[MAXCOMBATSPELLS];
    struct spellbook *spellbook;
} sc_mage;

void mage_set_spellpoints(sc_mage *m, int aura)
{
    m->spellpoints = aura;
}

int mage_get_spellpoints(const sc_mage *m)
{
    return m ? m->spellpoints : 0;
}

int mage_change_spellpoints(sc_mage *m, int delta)
{
    if (m) {
        int val = m->spellpoints + delta;
        return m->spellpoints = (val >= 0) ? val : m->spellpoints;
    }
    return 0;
}

magic_t mage_get_type(const sc_mage *m)
{
    return m ? m->magietyp : M_GRAY;
}

const spell *mage_get_combatspell(const sc_mage *mage, int nr, int *level)
{
    assert(nr < MAXCOMBATSPELLS);
    if (mage) {
        if (level) {
            *level = mage->combatspells[nr].level;
        }
        return mage->combatspells[nr].sp;
    }
    return NULL;
}

void unit_set_magic(struct unit *u, enum magic_t mtype)
{
    sc_mage *mage = get_mage(u);
    if (mage) {
        mage->magietyp = mtype;
    }
}

magic_t unit_get_magic(const unit *u)
{
    return mage_get_type(get_mage(u));
}

void unit_add_spell(unit * u, struct spell * sp, int level)
{
    sc_mage *mage = get_mage(u);

    if (!mage) {
        log_error("adding new spell %s to a previously non-magical unit %s\n", sp->sname, unitname(u));
        mage = create_mage(u, M_GRAY);
    }
    if (!mage->spellbook) {
        mage->spellbook = create_spellbook(0);
    }
    spellbook_add(mage->spellbook, sp, level);
}

/**
 ** at_icastle
 ** TODO: separate castle-appearance from illusion-effects
 **/

static double MagicRegeneration(void)
{
    return config_get_flt("magic.regeneration", 1.0);
}

static double MagicPower(double force)
{
    if (force > 0) {
        const char *str = config_get("magic.power");
        double value = str ? atof(str) : 1.0;
        return fmax(value * force, 1.0f);
    }
    return 0;
}

typedef struct icastle_data {
    const struct building_type *type;
    int time;
} icastle_data;

static int a_readicastle(variant *var, void *owner, struct gamedata *data)
{
    storage *store = data->store;
    icastle_data *idata = (icastle_data *)var->v;
    char token[32];

    UNUSED_ARG(owner);
    READ_TOK(store, token, sizeof(token));
    if (data->version < ATTRIBOWNER_VERSION) {
        READ_INT(store, NULL);
    }
    READ_INT(store, &idata->time);
    idata->type = bt_find(token);
    return AT_READ_OK;
}

static void
a_writeicastle(const variant *var, const void *owner, struct storage *store)
{
    icastle_data *data = (icastle_data *)var->v;
    UNUSED_ARG(owner);
    WRITE_TOK(store, data->type->_name);
    WRITE_INT(store, data->time);
}

static int a_ageicastle(struct attrib *a, void *owner)
{
    icastle_data *data = (icastle_data *)a->data.v;
    if (data->time <= 0) {
        building *b = (building *)owner;
        region *r = b->region;
        assert(owner == b);
        ADDMSG(&r->msgs, msg_message("icastle_dissolve", "building", b));
        /* remove_building lets units leave the building */
        remove_building(&r->buildings, b);
        return AT_AGE_REMOVE;
    }
    else
        data->time--;
    return AT_AGE_KEEP;
}

static void a_initicastle(variant *var)
{
    var->v = calloc(1, sizeof(icastle_data));
}

attrib_type at_icastle = {
    "zauber_icastle",
    a_initicastle,
    a_free_voidptr,
    a_ageicastle,
    a_writeicastle,
    a_readicastle
};

void make_icastle(building *b, const building_type *btype, int timeout) {
    attrib *a = a_add(&b->attribs, a_new(&at_icastle));
    icastle_data *data = (icastle_data *)a->data.v;
    data->type = btype;
    data->time = timeout;
}

const building_type *icastle_type(const struct attrib *a) {
    icastle_data *icastle = (icastle_data *)a->data.v;
    return icastle->type;
}
/* ------------------------------------------------------------- */

extern int dice(int count, int value);

bool FactionSpells(void)
{
    static int config, rule;
    if (config_changed(&config)) {
        rule = config_get_int("rules.magic.factionlist", 0);
    }
    return rule != 0;
}

int get_spell_level_mage(const spell * sp, void * cbdata)
{
    sc_mage *mage = (sc_mage *)cbdata;
    spellbook *book = mage_get_spellbook(mage);
    spellbook_entry *sbe = spellbook_get(book, sp);
    return sbe ? sbe->level : 0;
}

/* ------------------------------------------------------------- */
/* aus dem alten System uebriggebliegene Funktionen, die bei der
 * Umwandlung von alt nach neu gebraucht werden */
 /* ------------------------------------------------------------- */

static void init_mage(variant *var)
{
    var->v = calloc(1, sizeof(sc_mage));
}

static void free_mage(variant *var)
{
    sc_mage *mage = (sc_mage *)var->v;
    if (mage->spellbook) {
        spellbook_clear(mage->spellbook);
        free(mage->spellbook);
    }
    free(mage);
}

static int read_mage(variant *var, void *owner, struct gamedata *data)
{
    storage *store = data->store;
    int i, mtype;
    sc_mage *mage = (sc_mage *)var->v;
    char spname[64];

    UNUSED_ARG(owner);
    READ_INT(store, &mtype);
    mage->magietyp = (magic_t)mtype;
    READ_INT(store, &mage->spellpoints);
    READ_INT(store, &mage->spchange);
    for (i = 0; i != MAXCOMBATSPELLS; ++i) {
        spell *sp = NULL;
        int level = 0;
        READ_TOK(store, spname, sizeof(spname));
        READ_INT(store, &level);

        if (strcmp("none", spname) != 0) {
            sp = find_spell(spname);
            if (!sp) {
                log_error("read_mage: could not find combat spell '%s' in school '%s'\n", spname, magic_school[mage->magietyp]);
            }
        }
        if (sp && level >= 0) {
            int slot = -1;
            if (sp->sptyp & PRECOMBATSPELL)
                slot = 0;
            else if (sp->sptyp & COMBATSPELL)
                slot = 1;
            else if (sp->sptyp & POSTCOMBATSPELL)
                slot = 2;
            if (slot >= 0) {
                mage->combatspells[slot].level = level;
                mage->combatspells[slot].sp = sp;
            }
        }
    }
    if (mage->magietyp == M_GRAY) {
        read_spellbook(&mage->spellbook, data, get_spell_level_mage, mage);
    }
    else {
        read_spellbook(NULL, data, NULL, mage);
    }
    return AT_READ_OK;
}

static void
write_mage(const variant *var, const void *owner, struct storage *store)
{
    int i;
    sc_mage *mage = (sc_mage *)var->v;

    UNUSED_ARG(owner);
    WRITE_INT(store, mage->magietyp);
    WRITE_INT(store, mage->spellpoints);
    WRITE_INT(store, mage->spchange);
    for (i = 0; i != MAXCOMBATSPELLS; ++i) {
        WRITE_TOK(store,
            mage->combatspells[i].sp ? mage->combatspells[i].sp->sname : "none");
        WRITE_INT(store, mage->combatspells[i].level);
    }
    write_spellbook(mage->spellbook, store);
}

static int reset_mage(attrib *a, void *owner) {
    sc_mage *mage = (sc_mage *)a->data.v;
    UNUSED_ARG(owner);
    mage->spellcount = 0;
    return 1;
}

attrib_type at_mage = {
    "mage",
    init_mage,
    free_mage,
    reset_mage,
    write_mage,
    read_mage,
    NULL,
    ATF_UNIQUE
};

bool is_mage(const struct unit * u)
{
    sc_mage *m = get_mage(u);
    return (m && m->magietyp != M_GRAY);
}

sc_mage *get_mage(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_mage);
    if (a) {
        return (sc_mage *)a->data.v;
    }
    return NULL;
}

struct spellbook * mage_get_spellbook(const struct sc_mage * mage) {
    if (mage) {
        return mage->spellbook;
    }
    return NULL;
}

struct spellbook * unit_get_spellbook(const struct unit * u)
{
    sc_mage * mage = get_mage(u);
    if (mage) {
        if (mage->spellbook) {
            return mage->spellbook;
        }
        if (mage->magietyp != M_GRAY) {
            return faction_get_spellbook(u->faction);
        }
    }
    return NULL;
}

#define MAXSPELLS 256

/** update the spellbook with a new level
* Written for E3
*/
void pick_random_spells(faction * f, int level, spellbook * book, int num_spells)
{
    spellbook_entry *commonspells[MAXSPELLS];
    int qi, numspells = 0;
    selist *ql;

    if (level <= f->max_spelllevel) {
        return;
    }

    for (qi = 0, ql = book->spells; ql; selist_advance(&ql, &qi, 1)) {
        spellbook_entry * sbe = (spellbook_entry *)selist_get(ql, qi);
        if (sbe && sbe->level <= level) {
            commonspells[numspells++] = sbe;
        }
    }

    while (numspells > 0 && level > f->max_spelllevel) {
        int i;

        ++f->max_spelllevel;
        for (i = 0; i < num_spells; ++i) {
            int maxspell = numspells;
            int spellno = -1;
            spellbook_entry *sbe = NULL;
            while (!sbe && maxspell > 0) {
                spellno = rng_int() % maxspell;
                sbe = commonspells[spellno];
                if (sbe->level > f->max_spelllevel) {
                    /* not going to pick it in this round, move it to the end for later */
                    commonspells[spellno] = commonspells[--maxspell];
                    commonspells[maxspell] = sbe;
                    sbe = 0;
                }
                else {
                    if (f->spellbook) {
                        const spell *sp = spellref_get(&sbe->spref);
                        if (sp && spellbook_get(f->spellbook, sp)) {
                            /* already have this spell, remove it from the list of candidates */
                            commonspells[spellno] = commonspells[--numspells];
                            if (maxspell > numspells) {
                                maxspell = numspells;
                            }
                            sbe = 0;
                        }
                    }
                }
            }

            if (sbe && spellno < maxspell) {
                if (!f->spellbook) {
                    f->spellbook = create_spellbook(0);
                }
                spellbook_add(f->spellbook, spellref_get(&sbe->spref), sbe->level);
                commonspells[spellno] = commonspells[--numspells];
            }
        }
    }
}

/* ------------------------------------------------------------- */
/* Erzeugen eines neuen Magiers */
sc_mage *create_mage(unit * u, magic_t mtyp)
{
    sc_mage *mage;
    attrib *a;

    a = a_find(u->attribs, &at_mage);
    if (a == NULL) {
        a = a_add(&u->attribs, a_new(&at_mage));
    }
    mage = (sc_mage *)a->data.v;
    mage->magietyp = mtyp;
    return mage;
}

/* ------------------------------------------------------------- */
/* Funktionen fuer die Bearbeitung der List-of-known-spells */

int unit_spell_level(const unit *u, const struct spell *sp)
{
    spellbook *book = unit_get_spellbook(u);
    spellbook_entry *sbe = book ? spellbook_get(book, sp) : NULL;
    if (sbe) {
        return sbe->level;
    }
    return 0;
}

bool u_hasspell(const unit *u, const struct spell *sp)
{
    int level = unit_spell_level(u, sp);
    if (level > 0) {
        return level <= effskill(u, SK_MAGIC, NULL);
    }
    return false;
}

/* ------------------------------------------------------------- */
/* Eingestellte Kampfzauberstufe ermitteln */

int get_combatspelllevel(const unit * u, int nr)
{
    int level;
    if (mage_get_combatspell(get_mage(u), nr, &level) != NULL) {
        if (level > 0) {
            int maxlevel = effskill(u, SK_MAGIC, NULL);
            if (level > maxlevel) {
                return maxlevel;
            }
        }
        return level;
    }
    return 0;
}

/* ------------------------------------------------------------- */
/* Kampfzauber ermitteln, setzen oder loeschen */

const spell *get_combatspell(const unit * u, int nr)
{
    return mage_get_combatspell(get_mage(u), nr, NULL);
}

void set_combatspell(unit * u, spell * sp, struct order *ord, int level)
{
    sc_mage *mage = get_mage(u);
    int i = -1;

    assert(mage || !"trying to set a combat spell for non-mage");
    if (sp->sptyp & PRECOMBATSPELL)
        i = 0;
    else if (sp->sptyp & COMBATSPELL)
        i = 1;
    else if (sp->sptyp & POSTCOMBATSPELL)
        i = 2;
    assert(i >= 0);
    mage->combatspells[i].sp = sp;
    mage->combatspells[i].level = level;
    return;
}

void unset_combatspell(unit * u, spell * sp)
{
    int nr = 0;
    sc_mage *m = get_mage(u);

    if (!m) {
        return;
    }
    if (!sp) {
        int i;
        for (i = 0; i < MAXCOMBATSPELLS; i++) {
            m->combatspells[i].sp = NULL;
        }
    }
    else {
        if (sp->sptyp & PRECOMBATSPELL) nr = 0;
        else if (sp->sptyp & COMBATSPELL) nr = 1;
        else if (sp->sptyp & POSTCOMBATSPELL) nr = 2;
        if (sp != mage_get_combatspell(m, nr, NULL)) {
            return;
        }
    }
    m->combatspells[nr].sp = NULL;
    m->combatspells[nr].level = 0;
    return;
}

/**
 * Gibt die aktuelle Anzahl der Magiepunkte der Einheit zurueck
 */
int get_spellpoints(const unit * u)
{
    return mage_get_spellpoints(get_mage(u));
}

void set_spellpoints(unit * u, int sp)
{
    sc_mage *m = get_mage(u);
    if (m) {
        mage_set_spellpoints(m, sp);
    }
}

/**
 * Veraendert die Anzahl der Magiepunkte der Einheit um +mp
 */
int change_spellpoints(unit * u, int mp)
{
    return mage_change_spellpoints(get_mage(u), mp);
}

/* ein Magier kann normalerweise maximal Stufe^2.1/1.2+1 Magiepunkte
 * haben.
 * Manche Rassen haben einen zusaetzlichen Multiplikator
 * Durch Talentverlust (zB Insekten im Berg) koennen negative Werte
 * entstehen
 */

 /* Artefakt der Staerke
  * Ermoeglicht dem Magier mehr Magiepunkte zu 'speichern'
  */
  /** TODO: at_skillmod daraus machen */
static int use_item_aura(const region * r, const unit * u)
{
    int sk, n;

    sk = effskill(u, SK_MAGIC, r);
    n = (int)(sk * sk * rc_maxaura(u_race(u)) / 4);

    return n;
}

int max_spellpoints(const struct unit *u, const region * r)
{
    int sk;
    double n, msp = 0;
    double potenz = 2.1;
    double divisor = 1.2;
    const struct resource_type *rtype;
    const sc_mage *m;

    assert(u);
    m = get_mage(u);
    if (!m) return 0;
    if (!r) r = u->region;

    sk = effskill(u, SK_MAGIC, r);
    msp = rc_maxaura(u_race(u)) * (pow(sk, potenz) / divisor + 1);
    msp += m->spchange;

    rtype = rt_find("aurafocus");
    if (rtype && i_get(u->items, rtype->itype) > 0) {
        msp += use_item_aura(r, u);
    }
    n = get_curseeffect(u->attribs, &ct_auraboost);
    if (n > 0) {
        msp = (msp * n) / 100;
    }
    return (msp > 0) ? (int)msp : 0;
}

int change_maxspellpoints(unit * u, int csp)
{
    sc_mage *m = get_mage(u);
    if (!m) {
        return 0;
    }
    m->spchange += csp;
    return max_spellpoints(u, u->region);
}

/* ------------------------------------------------------------- */
/* Counter fuer die bereits gezauberte Anzahl Sprueche pro Runde.
 */
int countspells(unit * u, int step)
{
    sc_mage * m = get_mage(u);
    if (m) {
        int count;
        if (step == 0) {
            return m->spellcount;
        }
        count = m->spellcount + step;
        m->spellcount = (count > 0) ? count : 0;
        return m->spellcount;
    }
    return 0;
}

int spellcount(const unit *u) {
    sc_mage *m = get_mage(u);
    return m ? m->spellcount : 0;
}

/**
 * Die Grundkosten pro Stufe werden um 2^count erhoeht. countspells(u)
 * ist dabei die Anzahl der bereits gezauberten Sprueche
 */
int aura_multiplier(const unit * u) {
    int count = spellcount(u);
    return (1 << count);
}

int spellcost(const unit * caster, const struct spell_component *spc)
{
    const resource_type *r_aura = get_resourcetype(R_AURA);

    if (spc->type == r_aura) {
        return spc->amount * aura_multiplier(caster);
    }
    return spc->amount;
}

/**
 * Die fuer den Spruch benoetigte Aura pro Stufe.
 */
int auracost(const unit *caster, const spell *sp) {
    const resource_type *r_aura = get_resourcetype(R_AURA);
    if (sp->components) {
        int k;
        for (k = 0; sp->components[k].type; ++k) {
            const struct spell_component *spc = sp->components + k;
            if (r_aura == spc->type) {
                return spc->amount * aura_multiplier(caster);
            }
        }
    }
    return 0;
}

static void add_missing_component(resource **reslist_p, const struct spell_component *spc, int n) {
    resource *res = malloc(sizeof(resource));
    if (res) {
        res->number = n;
        res->type = spc->type;
        res->next = *reslist_p;
        *reslist_p = res;
    }
}

/**
 * Durch Komponenten und cast_level begrenzter maximal moeglicher Level.
 */
int max_spell_level(unit * u, unit *caster, const spell * sp, int cast_level, int range, resource **reslist_p)
{
    const struct spell_component *spc;
    int maxlevel = cast_level;

    if (!sp->components) {
        return cast_level;
    }

    for (spc = sp->components; spc->type; ++spc) {
        if (spc->amount > 0) {
            int need = 0, have, level_cost = spellcost(u, spc) * range;
            if (level_cost <= 0) continue;

            have = get_pooled(u, spc->type, GET_DEFAULT,
                level_cost * cast_level);

            /* sind die Kosten fix, so muss die Komponente nur einmal vorhanden
             * sein und der cast_level aendert sich nicht,
             * ansonsten wird das Minimum aus maximal moeglicher Stufe und der
             * gewuenschten gebildet */
            if (spc->cost == SPC_FIX) {
                if (have < level_cost) {
                    maxlevel = 0;
                    need = level_cost - have;
                }
            }
            else {
                need = level_cost * cast_level;
                if (have < need) {
                    /* bei Typ Linear muessen die Kosten in Hoehe der Stufe vorhanden
                     * sein, ansonsten schlaegt der Spruch fehl */
                    if (spc->cost == SPC_LINEAR) {
                        maxlevel = 0;
                    }
                    else {
                        maxlevel = have / level_cost;
                    }
                }
                need -= have;
            }
            if (reslist_p && need > 0) {
                add_missing_component(reslist_p, spc, need);
            }
        }
    }
    return maxlevel;
}

/* ------------------------------------------------------------- */
/* Die Spruchgrundkosten werden mit der Entfernung (Farcasting)
 * multipliziert, wobei die Aurakosten ein Sonderfall sind, da sie sich
 * auch durch die Menge der bereits gezauberten Sprueche erhoeht.
 * Je nach Kostenart werden dann die Komponenten noch mit cast_level
 * multipliziert.
 */
void pay_spell(unit * mage, const unit *caster, const spell * sp, int cast_level, int range)
{
    int k;

    assert(cast_level > 0);
    for (k = 0; sp->components && sp->components[k].type; k++) {
        const struct spell_component *spc = sp->components + k;
        int resuse = spellcost(caster ? caster : mage, spc) * range;

        if (spc->cost == SPC_LINEAR || spc->cost == SPC_LEVEL) {
            resuse *= cast_level;
        }

        use_pooled(mage, spc->type, GET_DEFAULT, resuse);
    }
}

/* ------------------------------------------------------------- */
/* Ein Magier kennt den Spruch und kann sich die Beschreibung anzeigen
 * lassen, wenn diese in seiner Spruchliste steht. Zaubern muss er ihn
 * aber dann immer noch nicht koennen, vieleicht ist seine Stufe derzeit
 * nicht ausreichend oder die Komponenten fehlen.
 */
bool knowsspell(const region * r, const unit * u, const spell * sp)
{
    int level = unit_spell_level(u, sp);
    /* steht der Spruch in der Spruchliste? */
    return level > 0;
}

/* Um einen Spruch zu beherrschen, muss der Magier die Stufe des
 * Spruchs besitzen, nicht nur wissen, das es ihn gibt (also den Spruch
 * in seiner Spruchliste haben).
 * Kosten fuer einen Spruch koennen Magiepunkte, Silber, Kraeuter
 * und sonstige Gegenstaende sein.
 */

static void
report_missing_components(unit *u, order *ord, resource *reslist) {
    assert(reslist);
    assert(u);
    assert(u->faction);
    assert(ord);
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "missing_components_list",
        "list", reslist));
}

static void free_components(resource *reslist) {
    while (reslist) {
        resource *res = reslist->next;
        free(reslist);
        reslist = res;
    }
}

bool
cancast(unit * u, const spell * sp, int level, int range, struct order * ord)
{
    if (u_hasspell(u, sp)) {
        /* Diesen Zauber kennt die Einheit nicht */
        cmistake(u, ord, 173, MSG_MAGIC);
        return false;
    }
    /* reicht die Stufe aus? */
    if (effskill(u, SK_MAGIC, NULL) < level) {
        /* die Einheit ist nicht erfahren genug fuer diesen Zauber */
        cmistake(u, ord, 169, MSG_MAGIC);
        return false;
    }

    return true;
}

/* ------------------------------------------------------------- */
/* generische Spruchstaerke,
 *
 * setzt sich derzeit aus der Stufe des Magiers, Magieturmeffekt,
 * Spruchitems und Antimagiefeldern zusammen. Es koennen noch die
 * Stufe des Spruchs und Magiekosten mit einfliessen.
 *
 * Die effektive Spruchstaerke und ihre Auswirkungen werden in der
 * Spruchfunktionsroutine ermittelt.
 */

double
spellpower(region * r, unit * u, const spell * sp, int cast_level)
{
    double force = cast_level;
    static int elf_power, config;
    const struct resource_type *rtype;

    if (sp == NULL) {
        return 0;
    }
    else {
        /* Bonus durch Magieturm und gesegneten Steinkreis */
        struct building *b = inside_building(u);
        const struct building_type *btype = building_is_active(b) ? b->type : NULL;
        if (btype && btype->flags & BTF_MAGIC) ++force;
    }

    if (config_changed(&config)) {
        elf_power = config_get_int("rules.magic.elfpower", 0);
    }
    if (elf_power) {
        static int rc_cache;
        static const race *rc_elf;
        if (rc_changed(&rc_cache)) {
            rc_elf = get_race(RC_ELF);
        }
        if (u_race(u) == rc_elf && r_isforest(r)) {
            ++force;
        }
    }
    rtype = rt_find("rop");
    if (rtype && i_get(u->items, rtype->itype) > 0) {
        ++force;
    }

    if (r->attribs) {
        curse *c;

        /* Antimagie in der Zielregion */
        c = get_curse(r->attribs, &ct_antimagiczone);
        if (curse_active(c)) {
            unit *mage = c->magician;
            force -= curse_geteffect(c);
            curse_changevigour(&r->attribs, c, -cast_level);
            if (mage != NULL && mage->faction != NULL) {
                if (force > 0) {
                    ADDMSG(&mage->faction->msgs, msg_message("reduce_spell",
                        "self mage region", mage, u, r));
                }
                else {
                    ADDMSG(&mage->faction->msgs, msg_message("block_spell",
                        "self mage region", mage, u, r));
                }
            }
        }

        /* Patzerfluch-Effekt: */
        c = get_curse(r->attribs, &ct_fumble);
        if (curse_active(c)) {
            unit *mage = c->magician;
            force -= curse_geteffect(c);
            curse_changevigour(&u->attribs, c, -1);
            if (mage != NULL && mage->faction != NULL) {
                if (force > 0) {
                    ADDMSG(&mage->faction->msgs, msg_message("reduce_spell",
                        "self mage region", mage, u, r));
                }
                else {
                    ADDMSG(&mage->faction->msgs, msg_message("block_spell",
                        "self mage region", mage, u, r));
                }
            }
        }
    }
    return (force > 0) ? (int)force : 0;
}

/* ------------------------------------------------------------- */
/* farcasting() == 1 -> gleiche Region, da man mit Null nicht vernuenfigt
 * rechnen kann */
static int farcasting(unit * magician, region * r)
{
    int dist;
    int mult;

    if (!r) {
        return INT_MAX;
    }

    dist = koor_distance(r->x, r->y, magician->region->x, magician->region->y);

    if (dist > 24)
        return INT_MAX;

    mult = 1 << dist;
    if (dist > 1) {
        if (!path_exists(magician->region, r, dist * 2, allowed_fly))
            mult = INT_MAX;
    }

    return mult;
}

/* ------------------------------------------------------------- */
/* Antimagie - Magieresistenz */
/* ------------------------------------------------------------- */

/* allgemeine Magieresistenz einer Einheit,
 * reduziert magischen Schaden */
variant magic_resistance(const unit * target)
{
    attrib *a;
    curse *c;
    const resource_type *rtype;
    const race *rc = u_race(target);
    variant v, prob = rc_magres(rc);
    const plane *pl = rplane(target->region);
    bool good_resist = true;
    bool bad_resist = true;

    if (rc == get_race(RC_HIRNTOETER) && !pl) {
        prob = frac_mul(prob, frac_make(1, 2));
    }
    assert(target->number > 0);
    /* Magier haben einen Resistenzbonus vom Magietalent * 5% */
    prob = frac_add(prob, frac_make(effskill(target, SK_MAGIC, NULL), 20));

    /* Auswirkungen von Zaubern auf der Einheit */
    c = get_curse(target->attribs, &ct_magicresistance);
    if (c) {
        /* TODO: legacy. magicresistance-effect is an integer-percentage stored in a double */
        variant effect = frac_make(curse_geteffect_int(c), 100);
        effect = frac_mul(effect, frac_make(get_cursedmen(target, c), target->number));
        prob = frac_add(prob, effect);
    }

    /* Unicorn +10 */
    rtype = get_resourcetype(R_UNICORN);
    if (rtype) {
        int n = i_get(target->items, rtype->itype);
        if (n) {
            prob = frac_add(prob, frac_make(n, target->number * 10));
        }
    }

    /* Auswirkungen von Zaubern auf der Region */
    a = a_find(target->region->attribs, &at_curse);
    while (a && a->type == &at_curse) {
        unit *mage;

        c = (curse *)a->data.v;
        mage = c->magician;

        if (mage != NULL && mage->number > 0) {
            if (good_resist && c->type == &ct_goodmagicresistancezone) {
                if (alliedunit(mage, target->faction, HELP_GUARD)) {
                    /* TODO: legacy. magicresistance-effect is an integer-percentage stored in a double */
                    prob = frac_add(prob, frac_make(curse_geteffect_int(c), 100));
                    good_resist = false; /* only one effect per region */
                }
            }
            else if (bad_resist && c->type == &ct_badmagicresistancezone) {
                if (!alliedunit(mage, target->faction, HELP_GUARD)) {
                    /* TODO: legacy. magicresistance-effect is an integer-percentage stored in a double */
                    prob = frac_sub(prob, frac_make(curse_geteffect_int(c), 100));
                    bad_resist = false; /* only one effect per region */
                }
            }
        }
        a = a->next;
    }
    /* Bonus durch Artefakte */
    /* TODO (noch gibs keine) */

    /* Bonus durch Gebaeude */
    {
        struct building *b = inside_building(target);

        /* gesegneter Steinkreis gibt 30% dazu */
        if (b && b->type->magresbonus && building_is_active(b)) {
            /* TODO: legacy. building-bonus is an integer-percentage */
            prob = frac_add(prob, frac_make(b->type->magresbonus, 100));
        }
    }

    /* resistance must never be more than 90% */
    v = frac_make(9, 10);
    if (frac_sign(frac_sub(prob, v)) > 0) { /* prob < 90% */
        return v; /* at most 90% */
    }
    return prob;
}

variant resist_chance(const unit *magician, const void *obj, int objtyp, int bonus_percent) {
    variant v02p, v98p, prob = frac_make(bonus_percent, 100);
    const attrib *a = NULL;

    switch (objtyp) {
    case TYP_UNIT:
    {
        int at, pa = 0;
        size_t s, len;
        const unit *u = (const unit *)obj;

        if (ucontact(u, magician)) {
            return frac_zero;
        }
        at = effskill(magician, SK_MAGIC, NULL);

        for (len = arrlen(u->skills), s = 0; s != len; ++s) {
            const skill* sv = u->skills + s;
            int sk = eff_skill(u, sv, NULL);
            if (pa < sk) {
                pa = sk;
            }
        }

        /* Contest, probability = 0.05 * (10 + pa - at); */
        prob = frac_add(prob, frac_make(10 + pa - at, 20));
        prob = frac_add(prob, magic_resistance(u));
        break;
    }

    case TYP_REGION:
        a = ((const region *)obj)->attribs;
        break;

    case TYP_BUILDING:
        a = ((const building *)obj)->attribs;
        /* Bonus durch Typ */
        prob = frac_add(prob, ((const building *)obj)->type->magres);
        break;

    case TYP_SHIP:
        /* Bonus durch Zauber */
        a = ((const ship *)obj)->attribs;
        break;
    }

    if (a) {
        const curse *c = get_curse(a, &ct_magicrunes);
        int effect = curse_geteffect_int(c);
        prob = frac_add(prob, frac_make(effect, 100));
    }
    /* ignore results < 2% and > 98% */
    v02p = frac_make(1, 50);
    v98p = frac_make(49, 50);
    if (frac_sign(frac_sub(prob, v02p)) < 0) {
        return v02p;
    }
    else if (frac_sign(frac_sub(prob, v98p)) > 0) {
        return v98p;
    }
    return prob;
}

/* ------------------------------------------------------------- */
/* Prueft, ob das Objekt dem Zauber widerstehen kann.
 * Objekte koennen Regionen, Units, Gebaeude oder Schiffe sein.
 * TYP_UNIT:
 * Das hoechste Talent des Ziels ist sein 'Magieresistenz-Talent', Magier
 * bekommen einen Bonus. Grundchance ist 50%, fuer jede Stufe
 * Unterschied gibt es 5%, minimalchance ist 5% fuer jeden (5-95%)
 * Scheitert der Spruch an der Magieresistenz, so gibt die Funktion
 * true zurueck
 */

bool
target_resists_magic(const unit * magician, const void *obj, int objtyp, int t_bonus)
{
    variant prob;
    static bool never_resist;
    static int config;
    if (config_changed(&config)) {
        never_resist = config_get_int("magic.resist.enable", 1) == 0;
    }
    if (never_resist) {
        return false;
    }

    if (magician == NULL || obj == NULL) {
        return true;
    }

    prob = resist_chance(magician, obj, objtyp, t_bonus);

    /* gibt true, wenn die Zufallszahl kleiner als die chance ist und
     * false, wenn sie gleich oder groesser ist, dh je groesser die
     * Magieresistenz (chance) desto eher gibt die Funktion true zurueck */
    if (prob.sa[0] <= 0) {
        return false;
    }
    return rng_int() % prob.sa[1] < prob.sa[0];
}

/* ------------------------------------------------------------- */

bool is_magic_resistant(const unit * magician, const unit * target, int resist_bonus)
{
    return target_resists_magic(magician, target, TYP_UNIT,
        resist_bonus);
}

/* ------------------------------------------------------------- */
/* Patzer */
/* ------------------------------------------------------------- */
/* allgemeine Zauberpatzer:
 * Treten auf, wenn die Stufe des Magieres zu niedrig ist.
 * Abhaengig von Staerke des Spruchs.
 *
 * Die Warscheinlichkeit reicht von 20% (beherrscht den Spruch gerade
 * eben) bis zu etwa 1% bei doppelt so gut wie notwendig
 */

bool fumble(region * r, unit * u, const spell * sp, int cast_grade)
{
    /* X ergibt Zahl zwischen 1 und 0, je kleiner, desto besser der Magier.
     * 0,5*40-20=0, dh wenn der Magier doppelt so gut ist, wie der Spruch
     * benoetigt, gelingt er immer, ist er gleich gut, gelingt der Spruch mit
     * 20% Warscheinlichkeit nicht
     * */

    int fumble_chance, rnd = 0;
    int effsk = effskill(u, SK_MAGIC, r);
    struct building *b = inside_building(u);
    const struct building_type *btype = building_is_active(b) ? b->type : NULL;
    int fumble_enabled = config_get_int("magic.fumble.enable", 1);
    sc_mage * mage;

    UNUSED_ARG(sp);
    if (effsk <= 0 || !fumble_enabled) {
        return false;
    }
    fumble_chance = (int)((cast_grade * 40.0 / (double)effsk) - 20.0);
    if (btype) {
        fumble_chance -= btype->fumblebonus;
    }

    /* CHAOSPATZERCHANCE 10 : +10% Chance zu Patzern */
    mage = get_mage(u);
    if (mage && mage->magietyp == M_DRAIG) {
        fumble_chance += CHAOSPATZERCHANCE;
    }
    if (is_cursed(u->attribs, &ct_magicboost)) {
        fumble_chance += CHAOSPATZERCHANCE;
    }
    if (is_cursed(u->attribs, &ct_fumble)) {
        fumble_chance += CHAOSPATZERCHANCE;
    }

    /* wenn die Chance kleiner als 0 ist, koennen wir gleich false
     * zurueckgeben */
    if (fumble_chance <= 0) {
        return false;
    }
    rnd = rng_int() % 100;

    return (rnd <= fumble_chance);
}

/* ------------------------------------------------------------- */
/* Dummy-Zauberpatzer, Platzhalter fuer speziell auf die Sprueche
* zugeschnittene Patzer */
static void fumble_default(castorder * co)
{
    unit *mage = co_get_magician(co);

    cmistake(mage, co->order, 180, MSG_MAGIC);

    return;
}

/** fumble: turns into a toad.
 * 
 * when this ends, one or two things will happen: 
 * the toad changes its race back,
 * and may or may not get toadslime.
 * The list of things to happen are attached to a timeout
 * trigger and that's added to the triggerlist of the mage gone toad.
 */
void fumble_toad(unit* mage, const spell *sp, int level)
{
    static int rc_cache;
    static const race* rc_toad = NULL;
    region* r = mage->region;
    int duration;
    trigger* trestore;

    if (rc_changed(&rc_cache)) {
        rc_toad = get_race(RC_TOAD);
    }
    duration = 2 + (rng_int() % level) / 2;
    trestore = change_race(mage, duration, rc_toad, NULL);
    if (trestore) {
        if (chance(0.7)) {
            const resource_type* rtype = rt_find("toadslime");
            if (rtype) {
                t_add(&trestore, trigger_giveitem(mage, rtype->itype, 1));
            }
        }
        ADDMSG(&r->msgs, msg_message("patzer6", "unit region spell", mage, r, sp));
    }
}

/* Die normalen Spruchkosten muessen immer bezahlt werden, hier noch
 * alle weiteren Folgen eines Patzers
 */

static void do_fumble(castorder * co)
{
    curse *c;
    region *r = co_get_region(co);
    unit *mage = co_get_magician(co);
    unit *caster = co_get_caster(co);
    const spell *sp = co->sp;
    int level = co->level;
    double effect;
    fumble_f fun;
    int duration;

    ADDMSG(&caster->faction->msgs,
        msg_message("patzer", "unit region spell", caster, r, sp));
    switch (rng_int() % 10) {
    case 0:
        /* wenn vorhanden spezieller Patzer, ansonsten nix */
        fun = get_fumble(sp->sname);
        if (fun) {
            fun(co);
        }
        else {
            fumble_default(co);
        }
        break;

    case 1:
        fumble_toad(mage, sp, level);
        break;

    case 2:
        /* temporary skill loss */
        duration = rng_int() % level / 2;
        if (duration < 2) duration = 2;
        effect = level / -2.0;
        c = create_curse(mage, &mage->attribs, &ct_skillmod, level,
            duration, effect, 1);
        c->data.i = SK_MAGIC;
        ADDMSG(&caster->faction->msgs, msg_message("patzer2", "unit region", caster, r));
        break;
 
    case 3:
    case 4:
        /* Spruch schlaegt fehl, alle Magiepunkte weg */
        set_spellpoints(mage, 0);
        ADDMSG(&caster->faction->msgs, msg_message("patzer3", "unit region spell",
            caster, r, sp));
        break;

    case 5:
    case 6:
        /* Spruch gelingt, aber alle Magiepunkte weg */
        co->level = cast_spell(co);
        set_spellpoints(mage, 0);
        ADDMSG(&caster->faction->msgs, msg_message("patzer4", "unit region spell",
            caster, r, sp));
        break;

    case 7:
    case 8:
    case 9:
    default:
        /* Spruch gelingt, alle nachfolgenden Sprueche werden 2^4 so teuer */
        co->level = cast_spell(co);
        ADDMSG(&caster->faction->msgs, msg_message("patzer5", "unit region spell",
            caster, r, sp));
        countspells(caster, 3);
    }
}

/* ------------------------------------------------------------- */
/* Regeneration von Aura */
/* ------------------------------------------------------------- */

/* Ein Magier regeneriert pro Woche W(Stufe^1.5/2+1), mindestens 1
 * Zwerge nur die Haelfte
 */
static double regeneration(unit * u)
{
    int sk;
    double aura, d;
    double potenz = 1.5;
    double divisor = 2.0;

    sk = effskill(u, SK_MAGIC, NULL);
    /* Rassenbonus/-malus */
    d = pow(sk, potenz) * u_race(u)->regaura / divisor;
    d++;

    /* Einfluss von Artefakten */
    /* TODO (noch gibs keine) */

    /* Wuerfeln */
    aura = (rng_double() * d + rng_double() * d) / 2 + 1;

    aura *= MagicRegeneration();

    return aura;
}

void regenerate_aura(void)
{
    region *r;
    unit *u;
    int aura, auramax;
    double reg_aura;
    int regen;
    double mod;
    int regen_enabled = config_get_int("magic.regeneration.enable", 1);

    if (!regen_enabled) return;

    for (r = regions; r; r = r->next) {
        for (u = r->units; u; u = u->next) {
            if (u->number && u->attribs) {
                sc_mage *m = get_mage(u);
                if (m) {
                    aura = mage_get_spellpoints(m);
                    auramax = max_spellpoints(u, r);
                    if (aura < auramax) {
                        struct building *b = inside_building(u);
                        const struct building_type *btype = building_is_active(b) ? b->type : NULL;
                        reg_aura = regeneration(u);

                        /* Magierturm erhoeht die Regeneration um 75% */
                        /* Steinkreis erhoeht die Regeneration um 50% */
                        if (btype)
                            reg_aura *= btype->auraregen;

                        /* Bonus/Malus durch Zauber */
                        mod = get_curseeffect(u->attribs, &ct_auraboost);
                        if (mod > 0) {
                            reg_aura = (reg_aura * mod) / 100.0;
                        }

                        /* Einfluss von Artefakten */
                        /* TODO (noch gibs keine) */

                        /* maximal Differenz bis Maximale-Aura regenerieren
                         * mindestens 1 Aura pro Monat */
                        regen = (int)reg_aura;
                        reg_aura -= regen;
                        if (chance(reg_aura)) {
                            ++regen;
                        }
                        if (regen < 1) regen = 1;
                        if (regen > auramax - aura) regen = auramax - aura;

                        aura += regen;
                        ADDMSG(&u->faction->msgs, msg_message("regenaura",
                            "unit region amount", u, r, regen));
                    }
                    if (aura > auramax) aura = auramax;
                    mage_set_spellpoints(m, aura);
                }
            }
        }
    }
}

static bool
verify_ship(region * r, unit * mage, const spell * sp, spellparameter * spobj,
    order * ord)
{
    ship *sh = findship(spobj->data.i);

    if (sh != NULL && sh->region != r && (sp->sptyp & GLOBALTARGET) == 0) {
        /* Schiff muss in gleicher Region sein */
        sh = NULL;
    }

    if (sh == NULL || sh->number < 1) {
        /* Schiff nicht gefunden */
        spobj->flag = TARGET_NOTFOUND;
        ADDMSG(&mage->faction->msgs, msg_message("spellshipnotfound",
            "unit region command id", mage, mage->region, ord, spobj->data.i));
        return false;
    }
    spobj->flag = 0;
    spobj->data.sh = sh;
    return true;
}

static bool
verify_building(region * r, unit * mage, const spell * sp, spellparameter* spobj,
    order * ord)
{
    building *b = findbuilding(spobj->data.i);

    if (b != NULL && b->region != r && (sp->sptyp & GLOBALTARGET) == 0) {
        /* Burg muss in gleicher Region sein */
        b = NULL;
    }

    if (b == NULL) {
        /* Burg nicht gefunden */
        spobj->flag = TARGET_NOTFOUND;
        ADDMSG(&mage->faction->msgs, msg_message("spellbuildingnotfound",
            "unit region command id", mage, mage->region, ord, spobj->data.i));
        return false;
    }
    spobj->flag = 0;
    spobj->data.b = b;
    return true;
}

message *msg_unitnotfound(const struct unit * mage, struct order * ord,
    const struct spellparameter * spobj)
{
    /* Einheit nicht gefunden */
    char tbuf[20];
    const char *uid = NULL;

    if (spobj->typ == SPP_TEMP) {
        sprintf(tbuf, "%s %s", param_name(P_TEMP, mage->faction->locale), itoa36(spobj->data.i));
        uid = tbuf;
    }
    else if (spobj->typ == SPP_UNIT) {
        uid = itoa36(spobj->data.i);
    }
    assert(uid);
    return msg_message("unitnotfound_id",
        "unit region command id", mage, mage->region, ord, uid);
}

static bool
verify_unit(region * r, unit * mage, const spell * sp, spellparameter * spobj,
    order * ord)
{
    unit *u = NULL;
    switch (spobj->typ) {
    case SPP_UNIT:
        u = findunit(spobj->data.i);
        break;
    case SPP_TEMP:
        u = findnewunit(r, mage->faction, spobj->data.i);
        if (u == NULL)
            u = findnewunit(mage->region, mage->faction, spobj->data.i);
        break;
    default:
        assert(!"shouldn't happen, this");
    }
    if (u != NULL) {
        if (u->region == r) {
            if (!cansee(mage->faction, r, u, 0) && !ucontact(u, mage)) {
                u = NULL;
            }
        }
        else if ((sp->sptyp & GLOBALTARGET) == 0) {
            u = NULL;
        }
    }

    if (u == NULL) {
        /* Einheit nicht gefunden */
        spobj->flag = TARGET_NOTFOUND;
        ADDMSG(&mage->faction->msgs, msg_unitnotfound(mage, ord, spobj));
        return false;
    }
    /* Einheit wurde gefunden, pointer setzen */
    spobj->flag = 0;
    spobj->data.u = u;
    return true;
}

/* ------------------------------------------------------------- */
/* Zuerst wird versucht alle noch nicht gefundenen Objekte zu finden
 * oder zu pruefen, ob das gefundene Objekt wirklich haette gefunden
 * werden duerfen (nicht alle Zauber wirken global). Dabei zaehlen wir die
 * Misserfolge (failed).
 * Dann folgen die Tests der gefundenen Objekte auf Magieresistenz und
 * Sichtbarkeit. Dabei zaehlen wir die magieresistenten (resists)
 * Objekte. Alle anderen werten wir als Erfolge (success) */

 /* gibt bei Misserfolg 0 zurueck, bei Magieresistenz zumindeste eines
  * Objektes 1 und bei Erfolg auf ganzer Linie 2 */
static void
verify_targets(castorder * co, int *invalid, int *resist, int *success)
{
    unit *caster = co_get_caster(co);
    const spell *sp = co->sp;
    region *target_r = co_get_region(co);

    *invalid = 0;
    *resist = 0;
    *success = 0;

    if (co->a_params) {
        size_t i, len = arrlen(co->a_params);
        /* zuerst versuchen wir vorher nicht gefundene Objekte zu finden.
         * Wurde ein Objekt durch globalsuche gefunden, obwohl der Zauber
         * gar nicht global haette suchen duerften, setzen wir das Objekt
         * zurueck. */
        for (i = 0; i < len; i++) {
            spellparameter *spobj = co->a_params + i;

            switch (spobj->typ) {
            case SPP_TEMP:
            case SPP_UNIT:
                if (!verify_unit(target_r, caster, sp, spobj, co->order))
                    ++*invalid;
                break;
            case SPP_BUILDING:
                if (!verify_building(target_r, caster, sp, spobj, co->order))
                    ++*invalid;
                break;
            case SPP_SHIP:
                if (!verify_ship(target_r, caster, sp, spobj, co->order))
                    ++*invalid;
                break;
            default:
                break;
            }
        }

        /* Nun folgen die Tests auf cansee und Magieresistenz */
        for (i = 0; i < len; i++) {
            spellparameter *spobj = co->a_params + i;
            unit *u;
            building *b;
            ship *sh;
            region *tr;

            if (spobj->flag == TARGET_NOTFOUND)
                continue;
            switch (spobj->typ) {
            case SPP_TEMP:
            case SPP_UNIT:
                u = spobj->data.u;

                if (((sp->sptyp & NORESISTANCE) == 0)
                    && target_resists_magic(caster, u, TYP_UNIT, 0)) {
                    /* Fehlermeldung */
                    spobj->data.i = u->no;
                    spobj->flag = TARGET_RESISTS;
                    ++*resist;
                    ADDMSG(&caster->faction->msgs, msg_message("spellunitresists",
                        "unit region command target", caster, caster->region, co->order, u));
                    break;
                }

                /* TODO: Test auf Parteieigenschaft Magieresistsenz */
                ++*success;
                break;
            case SPP_BUILDING:
                b = spobj->data.b;

                if (((sp->sptyp & NORESISTANCE) == 0)
                    && target_resists_magic(caster, b, TYP_BUILDING, 0)) {  /* Fehlermeldung */
                    spobj->data.i = b->no;
                    spobj->flag = TARGET_RESISTS;
                    ++*resist;
                    ADDMSG(&caster->faction->msgs, msg_message("spellbuildingresists",
                        "unit region command id",
                        caster, caster->region, co->order, spobj->data.i));
                    break;
                }
                ++*success;
                break;
            case SPP_SHIP:
                sh = spobj->data.sh;

                if (((sp->sptyp & NORESISTANCE) == 0)
                    && target_resists_magic(caster, sh, TYP_SHIP, 0)) {     /* Fehlermeldung */
                    spobj->data.i = sh->no;
                    spobj->flag = TARGET_RESISTS;
                    ++*resist;
                    ADDMSG(&caster->faction->msgs, msg_feedback(caster, co->order,
                        "spellshipresists", "ship", sh));
                    break;
                }
                ++*success;
                break;

            case SPP_REGION:
                /* haben wir ein Regionsobjekt, dann wird auch dieses und
                   nicht target_r ueberprueft. */
                tr = spobj->data.r;

                if (((sp->sptyp & NORESISTANCE) == 0)
                    && target_resists_magic(caster, tr, TYP_REGION, 0)) {   /* Fehlermeldung */
                    spobj->flag = TARGET_RESISTS;
                    ++*resist;
                    ADDMSG(&caster->faction->msgs, msg_message("spellregionresists",
                        "unit region command", caster, caster->region, co->order));
                    break;
                }
                ++*success;
                break;
            case SPP_INT:
            case SPP_STRING:
                ++*success;
                break;

            default:
                break;
            }
        }
    }
    else {
        /* der Zauber hat keine expliziten Parameter/Ziele, es kann sich
         * aber um einen Regionszauber handeln. Wenn notwendig hier die
         * Magieresistenz der Region pruefen. */
        if ((sp->sptyp & REGIONSPELL)) {
            /* Zielobjekt Region anlegen */
            spellparameter *spobj = arraddnptr(co->a_params, 1);
            spobj->flag = 0;
            spobj->typ = SPP_REGION;
            spobj->data.r = target_r;

            if (((sp->sptyp & NORESISTANCE) == 0)) {
                if (target_resists_magic(caster, target_r, TYP_REGION, 0)) {
                    /* Fehlermeldung */
                    ADDMSG(&caster->faction->msgs, msg_message("spellregionresists",
                        "unit region command", caster, caster->region, co->order));
                    spobj->flag = TARGET_RESISTS;
                    ++*resist;
                }
                else {
                    ++*success;
                }
            }
            else {
                ++*success;
            }
        }
        else {
            ++*success;
        }
    }
}

/* ------------------------------------------------------------- */
/* Hilfsstrukturen fuer ZAUBERE */
/* ------------------------------------------------------------- */

static void free_spellparameter(spellparameter * param)
{
    assert(param);
    switch (param->typ) {
    case SPP_STRING:
        free(param->data.s);
        break;
    default:
        break;
    }
}

static int addparam_string(const char *const param[], spellparameter * spobj)
{
    assert(param[0]);

    spobj->flag = 0;
    spobj->typ = SPP_STRING;
    spobj->data.xs = str_strdup(param[0]);
    return 1;
}

static int addparam_int(const char *const param[], spellparameter * spobj)
{
    spobj->flag = 0;
    spobj->typ = SPP_INT;
    spobj->data.i = atoi((char *)param[0]);
    return 1;
}

static int addparam_ship(const char *const param[], spellparameter * spobj)
{
    int id = atoi36((const char *)param[0]);

    spobj->flag = 0;
    spobj->typ = SPP_SHIP;
    spobj->data.i = id;
    return 1;
}

static int addparam_building(const char *const param[], spellparameter * spobj)
{
    int id = atoi36((const char *)param[0]);

    spobj->flag = 0;
    spobj->typ = SPP_BUILDING;
    spobj->data.i = id;
    return 1;
}

static int
addparam_region(const char *const param[], spellparameter * spobj, const unit * u,
    order * ord, message **err)
{
    assert(param[0]);
    if (param[1] == NULL) {
        /* Fehler: Zielregion vergessen */
        *err = msg_error(u, ord, 194);
        return -1;
    }
    else {
        plane *pl = getplanebyid(0);
        int tx = atoi((const char *)param[0]), ty = atoi((const char *)param[1]);
        int x = rel_to_abs(pl, u->faction, tx, 0);
        int y = rel_to_abs(pl, u->faction, ty, 1);
        region *rt;

        pnormalize(&x, &y, pl);
        rt = findregion(x, y);

        if (rt != NULL) {
            spobj->flag = 0;
            spobj->typ = SPP_REGION;
            spobj->data.r = rt;
        }
        else {
            /* Fehler: Zielregion vergessen */
            *err = msg_error(u, ord, 194);
            return -1;
        }
        return 2;
    }
}

static int
addparam_unit(const char *const param[], spellparameter * spobj, const unit * u,
    order * ord, message **err)
{
    int i = 0;
    sppobj_t otype = SPP_UNIT;

    if (isparam(param[0], u->faction->locale, P_TEMP)) {
        if (param[1] == NULL) {
            /* Fehler: Ziel vergessen */
            *err = msg_error(u, ord, 203);
            return -1;
        }
        ++i;
        otype = SPP_TEMP;
    }

    spobj->flag = 0;
    spobj->typ = otype;
    spobj->data.i = atoi36((const char*)param[i]);
    return i + 1;
}

static spellparameter *add_spellparameters(region * target_r, unit * u,
    const char *syntax, const char *const param[], size_t size, struct order *ord)
{
    struct message *err = NULL;
    size_t i = 0;
    const char *c;
    spellparameter *par = NULL;
    ptrdiff_t minlen = 0;

    for (c = syntax; *c != 0; ++c) {
        /* this makes sure that:
         *  minlen("kc?") = 0
         *  minlen("kc+") = 1
         *  minlen("cccc+c?") = 4
         */
        if (*c == '?')
            --minlen;
        else if (*c != '+' && *c != 'k')
            ++minlen;
    }
    c = syntax;

    /* mindestens ein Ziel (Ziellose Zauber werden nicht
     * geparst) */
    if (size == 0) {
        if (minlen > 0) {
            /* Fehler: Ziel vergessen */
            cmistake(u, ord, 203, MSG_MAGIC);
        }
        return NULL;
    }

    while (!err && *c && i < size && param[i] != NULL) {
        spellparameter spobj;
        int j = -1;
        spobj.typ = SPP_NONE;
        switch (*c) {
        case '?':
            /* tja. das sollte moeglichst nur am Ende passieren,
             * weil sonst die kacke dampft. */
            j = 0;
            ++c;
            assert(*c == 0);
            break;
        case '+':
            /* das vorhergehende Element kommt ein oder mehrmals vor, wir
             * springen zum key zurueck */
            j = 0;
            --c;
            break;
        case 'u':
            /* Parameter ist eine Einheit, evtl. TEMP */
            j = addparam_unit(param + i, &spobj, u, ord, &err);
            ++c;
            break;
        case 'r':
            /* Parameter sind zwei Regionskoordinaten innerhalb der "normalen" Plane */
            if (i + 1 < size) {
                j = addparam_region(param + i, &spobj, u, ord, &err);
            }
            ++c;
            break;
        case 'b':
            /* Parameter ist eine Burgnummer */
            j = addparam_building(param + i, &spobj);
            ++c;
            break;
        case 's':
            j = addparam_ship(param + i, &spobj);
            ++c;
            break;
        case 'c':
            /* Text, wird im Spruch ausgewertet */
            j = addparam_string(param + i, &spobj);
            ++c;
            break;
        case 'i':                  /* Zahl */
            j = addparam_int(param + i, &spobj);
            ++c;
            break;
        case 'k':
            ++c;
            switch (findparam_ex(param[i++], u->faction->locale)) {
            case P_REGION:
                spobj.flag = 0;
                spobj.typ = SPP_REGION;
                spobj.data.r = target_r;
                j = 0;
                ++c;
                break;
            case P_UNIT:
                if (i < size) {
                    j = addparam_unit(param + i, &spobj, u, ord, &err);
                    ++c;
                }
                break;
            case P_BUILDING:
            case P_GEBAEUDE:
                if (i < size) {
                    j = addparam_building(param + i, &spobj);
                    ++c;
                }
                break;
            case P_SHIP:
                if (i < size) {
                    j = addparam_ship(param + i, &spobj);
                    ++c;
                }
                break;
            default:
                j = -1;
                break;
            }
            break;
        default:
            j = -1;
            break;
        }
        if (j < 0 && !err) {
            /* syntax error */
            err = msg_error(u, ord, 209);
        }
        else {
            if (spobj.typ != SPP_NONE) {
                arrput(par, spobj);
            }
            i += j;
        }
    }

    /* im Endeffekt waren es evtl. nur p parameter (weniger weil TEMP nicht gilt) */

    if (arrlen(par) < minlen) {
        /* syntax error */
        if (!err) {
            err = msg_error(u, ord, 209);
        }
    }
    if (err) {
        ADDMSG(&u->faction->msgs, err);
        if (par) {
            free_spellparameter(par);
            par = NULL;
        }
    }
    return par;
}

struct unit * co_get_caster(const struct castorder * co) {
    return co->_familiar ? co->_familiar : co->magician.u;
}

struct unit * co_get_magician(const struct castorder * co) {
    return co->magician.u;
}

struct region * co_get_region(const struct castorder * co) {
    return co->_rtarget;
}

castorder *create_castorder(castorder * co, unit *caster, unit * familiar, const spell * sp, region * r,
    int lev, double force, int range, struct order * ord, spellparameter * a_params)
{
    if (!co) co = malloc(sizeof(castorder));
    if (!co) abort();

    assert(a_params == NULL || arrlen(a_params) > 0);
    co->next = NULL;
    co->magician.u = caster;
    co->_familiar = familiar;
    co->sp = sp;
    co->level = lev;
    co->force = MagicPower(force);
    co->_rtarget = r ? r : (familiar ? familiar->region : (caster ? caster->region : NULL));
    co->distance = range;
    co->order = copy_order(ord);
    co->a_params = a_params;

    return co;
}

void free_castorder(struct castorder *co)
{
    size_t i;
    for (i = arrlen(co->a_params); i > 0; --i) {
        free_spellparameter(co->a_params + i - 1);
    }
    arrfree(co->a_params);
    if (co->order) free_order(co->order);
}

/* Haenge c-order co an die letze c-order von cll an */
void add_castorder(spellrank * cll, castorder * co)
{
    if (cll->begin == NULL) {
        cll->handle_end = &cll->begin;
    }

    *cll->handle_end = co;
    cll->handle_end = &co->next;

    return;
}

void free_castorders(castorder * co)
{

    while (co) {
        castorder *co2 = co;
        co = co->next;
        free_castorder(co2);
        free(co2);
    }
    return;
}

bool is_familiar(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_familiarmage);
    return a != NULL;
}

static void
a_write_unit(const variant *var, const void *owner, struct storage *store)
{
    unit *u = (unit *)var->v;
    UNUSED_ARG(owner);
    write_unit_reference(u, store);
}

static int sm_familiar(const unit * u, const region * r, skill_t sk, int value)
{                               /* skillmod */
    if (sk == SK_MAGIC)
        return value;
    else {
        int mod;
        unit *familiar = get_familiar(u);
        if (familiar == NULL) {
            /* the familiar is dead */
            return value;
        }
        mod = effskill(familiar, sk, r) / 2;
        if (r != familiar->region) {
            mod /= distance(r, familiar->region);
        }
        return value + mod;
    }
}

void set_familiar(unit * mage, unit * familiar)
{
    /* if the skill modifier for the mage does not yet exist, add it */
    attrib *a = a_find(mage->attribs, &at_skillmod);
    while (a && a->type == &at_skillmod) {
        skillmod_data *smd = (skillmod_data *)a->data.v;
        if (smd->special == sm_familiar)
            break;
        a = a->next;
    }
    if (a == NULL) {
        a = make_skillmod(NOSKILL, sm_familiar, 0.0, 0);
        a_add(&mage->attribs, a);
    }

    a = a_find(mage->attribs, &at_familiar);
    if (a == NULL) {
        a = a_add(&mage->attribs, a_new(&at_familiar));
        a->data.v = familiar;
    }
    else {
        assert(!a->data.v || a->data.v == familiar);
    }

    /* TODO: Diese Attribute beim Tod des Familiars entfernen: */
    a = a_find(familiar->attribs, &at_familiarmage);
    if (a == NULL) {
        a = a_add(&familiar->attribs, a_new(&at_familiarmage));
        a->data.v = mage;
    }
    else {
        assert(!a->data.v || a->data.v == mage);
    }
}

void remove_familiar(unit * mage)
{
    attrib *a = a_find(mage->attribs, &at_familiar);
    attrib *an;

    if (a != NULL) {
        a_remove(&mage->attribs, a);
    }
    a = a_find(mage->attribs, &at_skillmod);
    while (a && a->type == &at_skillmod) {
        skillmod_data *smd = (skillmod_data *)a->data.v;
        an = a->next;
        if (smd->special == sm_familiar) {
            a_remove(&mage->attribs, a);
        }
        a = an;
    }
}

static void equip_familiar(unit *fam) {
    /* items, skills and spells: */
    char eqname[64];
    const race *rc = u_race(fam);

    snprintf(eqname, sizeof(eqname), "fam_%s", rc->_name);
    if (!equip_unit(fam, eqname)) {
        log_debug("could not perform initialization for familiar %s.\n", rc->_name);
    }
}

static int copy_spell_cb(spellbook_entry *sbe, void *udata) {
    spellbook *sb = (spellbook *)udata;
    spell * sp = spellref_get(&sbe->spref);
    if (!spellbook_get(sb, sp)) {
        spellbook_add(sb, sp, sbe->level);
    }
    return 0;
}

/**
 * Entferne Magie-Attribut von Migranten, die keine Vertrauten sind.
 *
 * Einmalige Reparatur von Vertrauten (Bug 2585).
 */
void fix_fam_migrant(unit *u) {
    if (!is_familiar(u)) {
        a_removeall(&u->attribs, &at_mage);
    }
}

/**
 * Einheiten, die Vertraute sind, bekommen ihre fehlenden Zauber.
 *
 * Einmalige Reparatur von Vertrauten (Bugs 2451, 2517).
 */
void fix_fam_spells(unit *u) {
    sc_mage *dmage;
    unit *du = unit_create(0);

    if (!is_familiar(u)) {
        return;
    }

    u_setrace(du, u_race(u));
    dmage = create_mage(du, M_GRAY);
    equip_familiar(du);
    if (dmage) {
        sc_mage *mage = get_mage(u);
        if (!mage) {
            mage = create_mage(u, dmage->magietyp);
        }
        else if (dmage->magietyp != mage->magietyp) {
            mage->magietyp = dmage->magietyp;
        }
        if (dmage->spellbook) {
            if (!mage->spellbook) {
                mage->spellbook = create_spellbook(NULL);
                spellbook_foreach(dmage->spellbook, copy_spell_cb, mage->spellbook);
            }
        }
        else {
            if (mage->spellbook) {
                spellbook_clear(mage->spellbook);
                mage->spellbook = NULL;
            }
        }
    }
    free_unit(du);
}

void create_newfamiliar(unit * mage, unit * fam)
{
    create_mage(fam, M_GRAY);
    set_familiar(mage, fam);
    equip_familiar(fam);

    /* TODO: Diese Attribute beim Tod des Familiars entfernen: */
    /* Wenn der Magier stirbt, dann auch der Vertraute */
    add_trigger(&mage->attribs, "destroy", trigger_killunit(fam));
    /* Wenn der Vertraute stirbt, dann bekommt der Magier einen Schock */
    add_trigger(&fam->attribs, "destroy", trigger_shock(mage));
}

static void * resolve_familiar(int id, void *data) {
    UNUSED_ARG(id);
    if (data) {
        unit *familiar = (unit *)data;
        attrib *a = a_find(familiar->attribs, &at_familiarmage);
        if (a != NULL && a->data.v) {
            unit *mage = (unit *)a->data.v;
            set_familiar(mage, familiar);
        }
    }
    return data;
}

static int read_familiar(variant *var, void *owner, struct gamedata *data)
{
    UNUSED_ARG(owner);
    if (read_unit_reference(data, (unit **)&var->v, resolve_familiar) <= 0) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

/* clones */

void create_newclone(unit * mage, unit * clone)
{
    attrib *a;

    a = a_find(mage->attribs, &at_clone);
    if (a == NULL) {
        a = a_add(&mage->attribs, a_new(&at_clone));
        a->data.v = clone;
    }
    else
        assert(!a->data.v || a->data.v == clone);
    /* TODO: Diese Attribute beim Tod des Klons entfernen: */

    a = a_find(clone->attribs, &at_clonemage);
    if (a == NULL) {
        a = a_add(&clone->attribs, a_new(&at_clonemage));
        a->data.v = mage;
    }
    else
        assert(!a->data.v || a->data.v == mage);

    /* Wenn der Magier stirbt, wird das in destroy_unit abgefangen.
     * Kein Trigger, zu kompliziert. */

     /* Wenn der Klon stirbt, dann bekommt der Magier einen Schock */
    add_trigger(&clone->attribs, "destroy", trigger_clonedied(mage));
}

static void set_clone(unit * mage, unit * clone)
{
    attrib *a;

    a = a_find(mage->attribs, &at_clone);
    if (a == NULL) {
        a = a_add(&mage->attribs, a_new(&at_clone));
        a->data.v = clone;
    }
    else
        assert(!a->data.v || a->data.v == clone);

    a = a_find(clone->attribs, &at_clonemage);
    if (a == NULL) {
        a = a_add(&clone->attribs, a_new(&at_clonemage));
        a->data.v = mage;
    }
    else
        assert(!a->data.v || a->data.v == mage);
}

static void * resolve_clone(int id, void *data) {
    UNUSED_ARG(id);
    if (data) {
        unit *clone = (unit *)data;
        attrib *a = a_find(clone->attribs, &at_clonemage);
        if (a != NULL && a->data.v) {
            unit *mage = (unit *)a->data.v;
            set_clone(mage, clone);
        }
    }
    return data;
}

static int read_clone(variant *var, void *owner, struct gamedata *data)
{
    UNUSED_ARG(owner);
    if (read_unit_reference(data, (unit **)&var->v, resolve_clone) <= 0) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

/* mages */
static void * resolve_mage(int id, void *data) {
    UNUSED_ARG(id);
    if (data) {
        unit *mage = (unit *)data;
        attrib *a = a_find(mage->attribs, &at_familiar);
        if (a != NULL && a->data.v) {
            unit *familiar = (unit *)a->data.v;
            set_familiar(mage, familiar);
        }
    }
    return data;
}

static int read_magician(variant *var, void *owner, struct gamedata *data)
{
    UNUSED_ARG(owner);
    if (read_unit_reference(data, (unit **)&var->v, resolve_mage) <= 0) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

static int age_unit(attrib * a, void *owner)
/* if unit is gone or dead, remove the attribute */
{
    unit *u = (unit *)a->data.v;
    UNUSED_ARG(owner);
    return (u != NULL && u->number > 0) ? AT_AGE_KEEP : AT_AGE_REMOVE;
}

attrib_type at_familiarmage = {
    "familiarmage",
    NULL,
    NULL,
    age_unit,
    a_write_unit,
    read_magician,
    NULL,
    ATF_UNIQUE
};

attrib_type at_familiar = {
    "familiar",
    NULL,
    NULL,
    age_unit,
    a_write_unit,
    read_familiar,
    NULL,
    ATF_UNIQUE
};

attrib_type at_clonemage = {
    "clonemage",
    NULL,
    NULL,
    age_unit,
    a_write_unit,
    read_magician,
    NULL,
    ATF_UNIQUE
};

attrib_type at_clone = {
    "clone",
    NULL,
    NULL,
    age_unit,
    a_write_unit,
    read_clone,
    NULL,
    ATF_UNIQUE
};

unit *get_familiar(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_familiar);
    if (a != NULL) {
        unit *uf = (unit *)a->data.v;
        if (uf->number > 0)
            return uf;
    }
    return NULL;
}

unit *get_familiar_mage(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_familiarmage);
    if (a != NULL) {
        unit *um = (unit *)a->data.v;
        if (um->number > 0)
            return um;
    }
    return NULL;
}

unit *get_clone(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_clone);
    if (a != NULL) {
        unit *uc = (unit *)a->data.v;
        if (uc->number > 0) {
            return uc;
        }
    }
    return NULL;
}

static bool is_moving_ship(ship * sh)
{
    const unit *u = ship_owner(sh);

    if (u) {
        switch (getkeyword(u->thisorder)) {
        case K_ROUTE:
        case K_MOVE:
        case K_FOLLOW:
            return true;
        default:
            return false;
        }
    }
    return false;
}

static int default_spell_level(const unit *u, const spell *sp) {
    if (sp && sp->components) {
        const struct spell_component *spc;
        for (spc = sp->components; spc->type; ++spc) {
            if (spc->cost != SPC_FIX) {
                return 0;
            }
        }
    }
    return unit_spell_level(u, sp);
}

#define MAX_PARAMETERS 48
static castorder *cast_cmd(unit * u, order * ord)
{
    char token[128];
    region *r = u->region;
    region *target_r = r;
    int level = -1, range, skill;
    unit *familiar = NULL;
    const char *s;
    spell *sp = NULL;
    plane *pl;
    spellparameter *args = NULL;
    unit * mage = NULL;
    param_t param;

    assert(u);
    if (LongHunger(u)) {
        cmistake(u, ord, 224, MSG_MAGIC);
        return 0;
    }
    pl = getplane(r);
    if (pl && fval(pl, PFL_NOMAGIC)) {
        cmistake(u, ord, 269, MSG_MAGIC);
        return 0;
    }

    init_order(ord, NULL);
    s = gettoken(token, sizeof(token));
    param = get_param(s, u->faction->locale);
    /* fuer Syntax ' STUFE x REGION y z ' */
    if (param == P_LEVEL) {
        level = getint();
        s = gettoken(token, sizeof(token));
        param = get_param(s, u->faction->locale);
    }
    if (param == P_REGION) {
        int t_x = getint();
        int t_y = getint();

        t_x = rel_to_abs(pl, u->faction, t_x, 0);
        t_y = rel_to_abs(pl, u->faction, t_y, 1);
        pnormalize(&t_x, &t_y, pl);
        target_r = findregion(t_x, t_y);
        if (!target_r) {
            /* Fehler "Die Region konnte nicht verzaubert werden" */
            ADDMSG(&u->faction->msgs, msg_message("spellregionresists",
                "unit region command", u, u->region, ord));
            return 0;
        }
        s = gettoken(token, sizeof(token));
        param = get_param(s, u->faction->locale);
    }
    /* fuer Syntax `REGION x y STUFE z` hier nach REGION nochmal pruefen */
    if (param == P_LEVEL) {
        level = getint();
        s = gettoken(token, sizeof(token));
    }
    if (!s || !s[0]) {
        /* Fehler "Es wurde kein Zauber angegeben" */
        cmistake(u, ord, 172, MSG_MAGIC);
        return 0;
    }

    /**
    * skill = der maximale durch das Magietalent erlaubte Level.
    * Entspricht dem Talent des Zaubernden, oder im Falle, dass ein
    * Vertrauter einen Spruch seines Magiers zaubert, dessen halbes Talent.
    */
    skill = effskill(u, SK_MAGIC, NULL);
    if (skill > 0) {
        sp = unit_getspell(u, s, u->faction->locale);

        /*
         * u = Die Einheit, die den Befehl gegeben hat.
         * mage = Die Einheit, deren Spruchliste und Aura benutzt wird.
         *
         * Vertraute koennen auch Zauber sprechen, die sie selbst nicht
         * koennen. `unit_getspell` findet aber nur jene Sprueche, die
         * die Einheit beherrscht. In diesem Fall ist `familiar` der Vertraute.
         */
        if (sp) {
            /* wir zaubern selbst */
            mage = u;
        }
        else {
            /* als Vertrauter suchen wir einen Spender-Magier mit dem Spruch */
            mage = get_familiar_mage(u);
            if (mage) {
                int limit = effskill(mage, SK_MAGIC, NULL) / 2;
                if (limit < skill) {
                    skill = limit;
                }
                sp = unit_getspell(mage, s, mage->faction->locale);
                if (sp == NULL || sp->sptyp & NOTFAMILIARCAST) {
                    /* Fehler: "Diesen Spruch kann der Vertraute nicht zaubern" */
                    cmistake(u, ord, 177, MSG_MAGIC);
                    return 0;
                }
                familiar = u;
            }
        }
    }

    if (!sp || !mage) {
        /* Fehler 'Spell not found' */
        cmistake(u, ord, 173, MSG_MAGIC);
        return 0;
    }

    if (sp->sptyp & ISCOMBATSPELL) {
        /* Fehler: "Dieser Zauber ist nur im Kampf sinnvoll" */
        cmistake(u, ord, 174, MSG_MAGIC);
        return 0;
    }

    /* Auf dem Ozean Zaubern als quasi-langer Befehl koennen
     * normalerweise nur Meermenschen, ausgenommen explizit als
     * OCEANCASTABLE deklarierte Sprueche */
    if (fval(r->terrain, SEA_REGION)) {
        if (u_race(u) != get_race(RC_AQUARIAN)
            && !fval(u_race(u), RCF_SWIM)
            && !(sp->sptyp & OCEANCASTABLE)) {
            /* Fehlermeldung */
            ADDMSG(&u->faction->msgs, msg_message("spellfail_onocean",
                "unit region command", u, u->region, ord));
            return 0;
        }
        /* Auf bewegenden Schiffen kann man nur explizit als
         * ONSHIPCAST deklarierte Zauber sprechen */
    }
    else if (u->ship) {
        if (is_moving_ship(u->ship)) {
            if (!(sp->sptyp & ONSHIPCAST)) {
                /* Fehler: "Diesen Spruch kann man nicht auf einem sich
                 * bewegenden Schiff stehend zaubern" */
                cmistake(u, ord, 175, MSG_MAGIC);
                return 0;
            }
        }
    }
    /* Farcasting bei nicht farcastbaren Spruechen abfangen */
    range = farcasting(u, target_r);
    if (range > 1) {
        if (!(sp->sptyp & FARCASTING)) {
            /* Fehler "Diesen Spruch kann man nicht in die Ferne
             * richten" */
            cmistake(u, ord, 176, MSG_MAGIC);
            return 0;
        }
        if (familiar) {
            /* Magier zaubert durch Vertrauten: keine Fernzauber erlaubt */
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "familiar_farcast",
                "mage", mage));
            return 0;
        }
        if (range > 1024) {
            /* (2^10) weiter als 10 Regionen entfernt */
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "spellfail::nocontact",
                "target", target_r));
            return 0;
        }
    }
    if (familiar) {
        /*
        * Magier zaubert durch Vertrauten: Doppelte Kosten.
        *
        * Das hier �ber range zu machen ist ein Hack, deshalb passiert es erst
        * nach allen anderen Range-Checks.
        */
        range *= 2;
    }

    /**
     * level = Die Stufe, auf der gezaubert werden soll. 
     * Kann nicht h�her sein als das Talent des Zaubernden, oder im
     * Falle, dass ein Vertrauter einen Spruch seines Magiers zaubert,
     * nicht h�her als dessen halbes Talent.
     */
    if (level < 0) {
        level = default_spell_level(mage, sp);
        if (level <= 0) {
            level = skill;
        }
        if (level > skill) {
            /* die Einheit ist nicht erfahren genug fuer diesen Zauber */
            cmistake(u, ord, 169, MSG_MAGIC);
            return 0;
        }
    }
    else if (!(sp->sptyp & SPELLLEVEL)) {
        /* Stufenangabe bei nicht Stufenvariierbaren Spruechen abfangen */
        if (skill != level) {
            level = skill;
            ADDMSG(&u->faction->msgs, msg_message("spellfail::nolevel",
                "mage region command", u, u->region, ord));
        }
    }
    
    if (level > skill) {
        /* STUFE kann nicht mehr als das erlaubte Maximum sein */
        level = skill;
    }

    /* Weitere Argumente zusammenbasteln */
    if (sp->parameter) {
        char *params[MAX_PARAMETERS];
        int i, p;
        for (p = 0; p != MAX_PARAMETERS; ++p) {
            s = gettoken(token, sizeof(token));
            if (!s || *s == 0) {
                break;
            }
            params[p] = str_strdup(s);
        }
        if (p == MAX_PARAMETERS) {
            log_error("%s: MAX_PARAMETERS (%d) too small to CAST %s, parsing stopped early.",
                unitname(u), MAX_PARAMETERS, sp->sname);
        }
        args =
            add_spellparameters(target_r, mage, sp->parameter,
            (const char *const *)params, p, ord);
        for (i = 0; i != p; ++i) {
            free(params[i]);
        }
        if (args == NULL) {
            /* Syntax war falsch */
            return 0;
        }
    }
    return create_castorder(0, mage, familiar, sp, target_r, level, 0, range, ord,
        args);
}

/* ------------------------------------------------------------- */
/* Damit man keine Rituale in fremden Gebiet machen kann, diese vor
 * Bewegung zaubern. Magier sind also in einem fremden Gebiet eine Runde
 * lang verletzlich, da sie es betreten, und angegriffen werden koennen,
 * bevor sie ein Ritual machen koennen.
 *
 * Syntax: ZAUBER [REGION X Y] [STUFE <stufe>] "Spruchname" [Einheit-1
 * Einheit-2 ..]
 *
 * Nach Prioritaet geordnet die Zauber global auswerten.
 *
 * Die Kosten fuer Farcasting multiplizieren sich mit der Entfernung,
 * cast_level gibt die virtuelle Stufe an, die den durch das Farcasten
 * entstandenen Spruchkosten entspricht.  Sind die Spruchkosten nicht
 * levelabhaengig, so sind die Kosten nur von der Entfernung bestimmt,
 * die Staerke/Level durch den realen Skill des Magiers
 */

void magic(void)
{
    region *r;
    int rank;
    spellrank spellranks[MAX_SPELLRANK];
    const race *rc_insect = get_race(RC_INSECT);

    memset(spellranks, 0, sizeof(spellranks));

    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            order *ord;

            if (u->number <= 0)
                continue;

            if (u_race(u) == rc_insect && r_insectstalled(r) &&
                !is_cursed(u->attribs, &ct_insectfur))
                continue;

            if (fval(u, UFL_WERE | UFL_LONGACTION) || is_paused(u->faction)) {
                continue;
            }

            for (ord = u->orders; ord; ord = ord->next) {
                if (getkeyword(ord) == K_CAST) {
                    castorder *co = cast_cmd(u, ord);
                    fset(u, UFL_LONGACTION | UFL_NOTMOVING);
                    if (co) {
                        const spell *sp = co->sp;
                        add_castorder(&spellranks[sp->rank], co);
                    }
                }
            }
        }
    }

    /* Da sich die Aura und Komponenten in der Zwischenzeit veraendert
     * haben koennen und sich durch vorherige Sprueche das Zaubern
     * erschwert haben kann, muss beim zaubern erneut geprueft werden, ob der
     * Spruch ueberhaupt gezaubert werden kann.
     * (level) die effektive Staerke des Spruchs (= Stufe, auf der der
     * Spruch gezaubert wird) */

    for (rank = 0; rank < MAX_SPELLRANK; rank++) {
        castorder *co;
        for (co = spellranks[rank].begin; co; co = co->next) {
            order *ord = co->order;
            int invalid, resist, success, cast_level = co->level;
            bool fumbled = false;
            unit *mage = co_get_magician(co);
            unit *caster = co_get_caster(co);
            const spell *sp = co->sp;
            region *target_r = co_get_region(co);
            resource *reslist = NULL;

            /* reichen die Komponenten nicht, kann der Level reduziert werden. */
            co->level = max_spell_level(mage, caster, sp, cast_level, co->distance, &reslist);

            if (co->level < 1) {
                /* Es fehlt eine Komponente vollst�ndig: */
                assert(reslist);
                report_missing_components(mage, ord, reslist);
                free_components(reslist);
                continue;
            }

            if (cast_level > co->level) {
                /* Es gibt von einer Komponente zu wenig, deshalb 
                 * wird mit reduzierter Stufe gezaubert: */
                ADDMSG(&caster->faction->msgs, msg_message("missing_components",
                    "unit spell level", caster, sp, cast_level));
                free_components(reslist);
            }

            co->force = MagicPower(spellpower(target_r, mage, sp, co->level));
            /* die Staerke kann durch Antimagie auf 0 sinken */
            if (co->force <= 0) {
                co->force = 0;
                ADDMSG(&caster->faction->msgs, msg_message("missing_force",
                    "unit spell level", caster, sp, co->level));
            }

            /* Ziele auf Existenz pruefen und Magieresistenz feststellen. Wurde
             * kein Ziel gefunden, so ist verify_targets=0. Scheitert der
             * Spruch an der Magieresistenz, so ist verify_targets = 1, bei
             * Erfolg auf ganzer Linie ist verify_targets= 2
             */
            verify_targets(co, &invalid, &resist, &success);
            if (success + resist == 0) {
                /* kein Ziel gefunden, Fehlermeldungen sind in verify_targets */
                /* keine kosten fuer den zauber */
                continue;               /* aeussere Schleife, naechster Zauberer */
            }
            else if (co->force > 0 && resist > 0) {
                /* einige oder alle Ziele waren magieresistent */
                if (success == 0) {
                    co->force = 0;
                    /* zwar wurde mindestens ein Ziel gefunden, das widerstand
                     * jedoch dem Zauber. Kosten abziehen und abbrechen. */
                    ADDMSG(&caster->faction->msgs, msg_message("spell_resist",
                        "unit region spell", caster, r, sp));
                }
            }

            /* Auch fuer Patzer gibt es Erfahrung, muessen die Spruchkosten
             * bezahlt werden und die nachfolgenden Sprueche werden teurer */
            if (co->force > 0) {
                if (fumble(target_r, mage, sp, co->level)) {
                    /* zuerst bezahlen, dann evt in do_fumble alle Aura verlieren */
                    fumbled = true;
                }
                else {
                    co->level = cast_spell(co);
                    if (co->level <= 0) {
                        /* Kosten nur fuer real benoetige Stufe berechnen */
                        continue;
                    }
                }
            }
            /* erst bezahlen, dann Kostenzaehler erhoehen */
            if (co->level > 0) {
                pay_spell(mage, caster, sp, co->level, co->distance);
            }
            if (fumbled) {
                do_fumble(co);
            }
            countspells(caster, 1);
        }
    }

    /* Sind alle Zauber gesprochen gibts Erfahrung */
    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            if (is_mage(u) && spellcount(u) > 0) {
                produceexp(u, SK_MAGIC);
                /* Spruchlistenaktualisierung ist in Regeneration */
            }
        }
    }
    for (rank = 0; rank < MAX_SPELLRANK; rank++) {
        free_castorders(spellranks[rank].begin);
    }
    remove_empty_units();
}

const char *spell_info(const spell * sp, const struct locale *lang)
{
    return LOC(lang, mkname("spellinfo", sp->sname));
}

const char *mkname_spell(const spell *sp)
{
    return mkname("spell", sp->sname);
}

const char *spell_name(const char *spname, const struct locale *lang)
{
    return LOC(lang, spname);
}

const char *curse_name(const curse_type * ctype, const struct locale *lang)
{
    return LOC(lang, mkname("spell", ctype->cname));
}

static void select_spellbook(void **tokens, spellbook *sb, const struct locale * lang) {
    selist * ql;
    int qi;

    assert(sb);
    assert(lang);

    for (qi = 0, ql = sb->spells; ql; selist_advance(&ql, &qi, 1)) {
        spellbook_entry *sbe = (spellbook_entry *)selist_get(ql, qi);
        const spell *sp = spellref_get(&sbe->spref);
        const char *spname = mkname("spell", sp->sname);
        const char *n = spell_name(spname, lang);
        if (!n) {
            log_error("no translation in locale %s for spell %s\n", locale_name(lang), sp->sname);
        }
        else {
            variant token;
            const str_alias *aliases = alias_get(lang, spname);
            token.v = (void *)sp;
            addtoken((struct tnode **)tokens, n, token);
            if (aliases) {
                int i;
                for (i = 0; i != MAXSTRINGS && aliases->strings[i]; ++i) {
                    addtoken((struct tnode **)tokens, aliases->strings[i], token);
                }
            }
        }
    }
}

spell *unit_getspell(struct unit *u, const char *name, const struct locale * lang)
{
    spellbook *sb;

    sb = unit_get_spellbook(u);
    if (sb) {
        void * tokens = NULL;
        select_spellbook(&tokens, sb, lang);
        if (tokens) {
            variant token;
            if (findtoken(tokens, name, &token) != E_TOK_NOMATCH) {
                freetokens(tokens);
                return (spell *)token.v;
            }
            freetokens(tokens);
        }
    }
    return NULL;
}

int cast_spell(struct castorder *co)
{
    const char *fname = co->sp->sname;
    const char *hashpos = strchr(fname, '#');
    char fbuf[64];
    spell_f fun;

    const spell *sp = co->sp;
    if (hashpos != NULL) {
        ptrdiff_t len = hashpos - fname;
        assert(len < (ptrdiff_t) sizeof(fbuf));
        memcpy(fbuf, fname, len);
        fbuf[len] = '\0';
        fname = fbuf;
    }

    fun = get_spellcast(sp->sname);
    if (!fun) {
        log_debug("no spell function for %s, try callback", sp->sname);
        return callbacks.cast_spell(co, fname);
    }
    return fun(co);
}

const char *magic_name(magic_t mtype, const struct locale *lang)
{
    return LOC(lang, mkname("school", magic_school[mtype]));
}

static critbit_tree cb_spellbooks;

#define SBNAMELEN 16

typedef struct sb_entry {
    char key[SBNAMELEN];
    spellbook *value;
} sb_entry;

spellbook * get_spellbook(const char * name)
{
    size_t len = strlen(name);
    const void * match;

    if (len >= SBNAMELEN) {
        log_error("spellbook name is longer than %d bytes: %s", SBNAMELEN - 1, name);
        return NULL;
    }

    match = cb_find_str(&cb_spellbooks, name);
    if (!match) {
        sb_entry ent;
        memset(ent.key, 0, SBNAMELEN);
        memcpy(ent.key, name, len);
        ent.value = create_spellbook(name);
        if (cb_insert(&cb_spellbooks, &ent, sizeof(ent)) == CB_EXISTS) {
            log_error("cb_insert failed although cb_find returned nothing for spellbook=%s", name);
            assert(!"should not happen");
        }
        return ent.value;
    }
    return ((const sb_entry *)match)->value;
}

void free_spellbook(spellbook *sb) {
    spellbook_clear(sb);
    free(sb);
}

static int free_spellbook_cb(void *match, const void *key, size_t keylen, void *data) {
    const sb_entry *ent = (const sb_entry *)match;
    UNUSED_ARG(data);
    UNUSED_ARG(keylen);
    UNUSED_ARG(key);
    free_spellbook(ent->value);
    return 0;
}

void free_spellbooks(void)
{
    cb_foreach(&cb_spellbooks, "", 0, free_spellbook_cb, NULL);
    cb_clear(&cb_spellbooks);
}

static struct message* cinfo_magicresistance(const void* obj, objtype_t typ, const struct curse* c, int self)
{
    if (typ == TYP_UNIT) {
        if (self != 0) {
            const struct unit* u = (const struct unit*)obj;
            return msg_message("magicresistance_unit", "unit id", u,
                c->no);
        }
        return NULL;
    }
    if (typ == TYP_BUILDING) {
        const struct building* b = (const struct building*)obj;
        return msg_message("magicresistance_building", "id building", c->no, b);
    }
    return 0;
}

const struct curse_type ct_magicresistance = {
    "magicresistance", CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN, cinfo_magicresistance
};

void register_magicresistance(void)
{
    ct_register(&ct_magicresistance);
}
