#ifdef _MSC_VER
#include <platform.h>
#endif
#include "exparse.h"

#include "alchemy.h"

#include "kernel/build.h"
#include "kernel/building.h"
#include "kernel/item.h"
#include "kernel/race.h"
#include "kernel/resources.h"
#include "kernel/ship.h"
#include "kernel/spellbook.h"
#include "kernel/terrain.h"

#include "util/functions.h"
#include "util/log.h"
#include "util/strings.h"

#include <expat.h>

#include <assert.h>
#include <string.h>

#ifdef XML_LARGE_SIZE
# if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#  define XML_FMT_INT_MOD "I64"
# else
#  define XML_FMT_INT_MOD "ll"
# endif
#else
# define XML_FMT_INT_MOD "l"
#endif

#ifdef XML_UNICODE_WCHAR_T
# define XML_FMT_STR "ls"
#else
# define XML_FMT_STR "s"
#endif

enum {
    EXP_UNKNOWN,
    EXP_RESOURCES,
    EXP_WEAPON,
    EXP_BUILDINGS,
    EXP_SHIPS,
    EXP_RACES,
    EXP_MESSAGES,
    EXP_STRINGS,
    EXP_SPELLS,
    EXP_SPELLBOOKS,
};

typedef struct parseinfo {
    int type;
    int depth;
    int errors;
    XML_Char *cdata;
    size_t clength;
    void *object;
} parseinfo;

static int xml_strcmp(const XML_Char *xs, const char *cs) {
    return strcmp(xs, cs);
}

static bool xml_bool(const XML_Char *val) {
    if (xml_strcmp(val, "yes") == 0) return true;
    if (xml_strcmp(val, "true") == 0) return true;
    if (xml_strcmp(val, "1") == 0) return true;
    return false;
}

static int xml_int(const XML_Char *val) {
    return atoi(val);
}

static double xml_float(const XML_Char *val) {
    return atof(val);
}

static variant xml_fraction(const XML_Char *val) {
    int num, den = 100;
    double fval = atof(val);
    num = (int)(fval * den + 0.5);
    return frac_make(num, den);
}

const XML_Char *attr_get(const XML_Char **attr, const char *key) {
    int i;
    for (i = 0; attr[i]; i += 2) {
        if (xml_strcmp(attr[i], key) == 0) {
            return attr[i + 1];
        }
    }
    return NULL;
}

static building_stage *stage;

#define UPKEEP_MAX 4
static maintenance upkeep[UPKEEP_MAX];
static int nupkeep;

#define MAX_REQUIREMENTS 8
static requirement reqs[MAX_REQUIREMENTS];
static int nreqs;

#define RMOD_MAX 8
static resource_mod rmods[RMOD_MAX];
static int nrmods;

#define WMOD_MAX 8
static weapon_mod wmods[WMOD_MAX];
static int nwmods;

static void handle_bad_input(parseinfo *pi, const XML_Char *el, const XML_Char *attr) {
    if (attr) {
        log_error("unknown attribute in <%s>: %s", el, attr);
    }
    else {
        log_error("unexpected element <%s>", el);
    }
    ++pi->errors;
}

static bool handle_flag(int *flags, const XML_Char **pair, const char *names[]) {
    int i;
    for (i = 0; names[i]; ++i) {
        const char * name = names[i];
        if (name[0] == '!') {
            if (xml_strcmp(pair[0], name+1) == 0) {
                if (xml_bool(pair[1])) {
                    *flags &= ~(1 << i);
                }
                else {
                    *flags |= (1 << i);
                }
                return true;
            }
        }
        else if (xml_strcmp(pair[0], name) == 0) {
            if (xml_bool(pair[1])) {
                *flags |= (1 << i);
            }
            else {
                *flags &= ~(1 << i);
            }
            return true;
        }
    }
    return false;
}

static void handle_resource(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "item", "limited", "pooled", NULL };
    int i;
    const char *name = NULL, *appear = NULL;
    int flags = RTF_POOLED;
    bool material = false;
    (void)el;
    for (i = 0; attr[i]; i += 2) {
        if (xml_strcmp(attr[i], "name") == 0) {
            name = attr[i + 1];
        }
        else if (xml_strcmp(attr[i], "appearance") == 0) {
            /* TODO: appearance should be a property of item, not resource */
            appear = attr[i + 1];
            flags |= RTF_ITEM;
        }
        else if (xml_strcmp(attr[i], "material") == 0) {
            material = xml_bool(attr[i + 1]);
        }
        else if (!handle_flag(&flags, attr + i, flag_names)) {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    if (name) {
        resource_type *rtype = rt_get_or_create(name);
        rtype->flags = flags;
        if (appear) {
            /* TODO: appearance should be a property of item, not resource */
            rtype->itype = it_get_or_create(rtype);
            it_set_appearance(rtype->itype, appear);
        }
        if (material) {
            rmt_create(rtype);
        }
        pi->object = rtype;
    }
}

static void handle_item(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "herb", "cursed", "notlost", "big", "animal", "vehicle", "use", NULL };
    int i, flags = ITF_NONE;
    resource_type *rtype = (resource_type *)pi->object;
    item_type * itype = rtype->itype;
    assert(rtype);
    if (!itype) {
        itype = it_get_or_create(rtype);
    }
    for (i = 0; attr[i]; i += 2) {
        char buffer[64];
        if (xml_strcmp(attr[i], "weight") == 0) {
            itype->weight = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "capacity") == 0) {
            itype->capacity = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "score") == 0) {
            itype->score = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "allow") == 0) {
            size_t len = strlen(attr[i + 1]);
            assert(len < sizeof(buffer));
            memcpy(buffer, attr[i + 1], len + 1);
            itype->mask_allow = rc_get_mask(buffer);
        }
        else if (xml_strcmp(attr[i], "deny") == 0) {
            size_t len = strlen(attr[i + 1]);
            assert(len < sizeof(buffer));
            memcpy(buffer, attr[i + 1], len + 1);
            itype->mask_deny = rc_get_mask(buffer);
        }
        else if (!handle_flag(&flags, attr + i, flag_names)) {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    itype->flags = flags;
}

static void handle_armor(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "shield", "laen", NULL };
    resource_type *rtype = (resource_type *)pi->object;
    item_type * itype = rtype->itype;
    armor_type *atype = new_armortype(itype, 0.0, frac_zero, 0, 0);
    int i, flags = 0;
    for (i = 0; attr[i]; i += 2) {
        if (xml_strcmp(attr[i], "penalty") == 0) {
            atype->penalty = xml_float(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "projectile") == 0) {
            atype->projectile = xml_float(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "ac") == 0) {
            atype->prot = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "magres") == 0) {
            atype->magres = xml_fraction(attr[i + 1]);
        }
        else if (!handle_flag(&flags, attr + i, flag_names)) {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    atype->flags = flags;
}

static void handle_weapon(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "missile", "magical", "pierce", "cut", "bash", "siege", "armorpiercing", "horse", "useshield", NULL };
    resource_type *rtype = (resource_type *)pi->object;
    item_type * itype = rtype->itype;
    weapon_type *wtype = new_weapontype(itype, 0, frac_zero, NULL, 0, 0, 0, NOSKILL);
    int i, flags = 0;
    for (i = 0; attr[i]; i += 2) {
        if (xml_strcmp(attr[i], "offmod") == 0) {
            wtype->offmod = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "defmod") == 0) {
            wtype->defmod = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "reload") == 0) {
            wtype->reload = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "skill") == 0) {
            wtype->skill = findskill(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "magres") == 0) {
            wtype->magres = xml_fraction(attr[i + 1]);;
        }
        else if (!handle_flag(&flags, attr + i, flag_names)) {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    wtype->flags = flags;
}

#define MAX_COMPONENTS 8
static spell_component components[MAX_COMPONENTS];
static int ncomponents;

static void XMLCALL start_spells(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = {
        "far", "variable", "ocean", "ship", "los", 
        "unittarget", "shiptarget", "buildingtarget", "regiontarget", "globaltarget", NULL };

    if (xml_strcmp(el, "resource") == 0) {
        spell_component *spc;
        int i;

        assert(ncomponents < MAX_COMPONENTS);
        spc = components + ncomponents;
        spc->cost = SPC_FIX;
        ++ncomponents;
        memset(spc, 0, sizeof(spell_component));
        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strcmp(key, "name") == 0) {
                spc->type = rt_get_or_create(val);
            }
            else if (xml_strcmp(key, "amount") == 0) {
                spc->amount = xml_int(val);
            }
            else if (xml_strcmp(key, "cost") == 0) {
                if (xml_strcmp(val, "level") == 0) {
                    spc->cost = SPC_LEVEL;
                }
                else if (xml_strcmp(val, "linear") == 0) {
                    spc->cost = SPC_LINEAR;
                }
                else if (xml_strcmp(val, "fixed") == 0) {
                    spc->cost = SPC_FIX;
                }
                else {
                    handle_bad_input(pi, key, val);
                }
            }
            else {
                handle_bad_input(pi, el, key);
            }
        }
    }
    else if (xml_strcmp(el, "spell") == 0) {
        spell *sp;
        const XML_Char *name = NULL, *syntax = NULL, *parameter = NULL;
        int i, rank = 0, flags = 0;
        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strcmp(key, "name") == 0) {
                name = val;
            }
            else if (xml_strcmp(key, "syntax") == 0) {
                syntax = val;
            }
            else if (xml_strcmp(key, "parameters") == 0) {
                parameter = val;
            }
            else if (xml_strcmp(key, "rank") == 0) {
                rank = xml_int(val);
            }
            else if (xml_strcmp(key, "combat") == 0) {
                int mode = PRECOMBATSPELL;
                int k = xml_int(val);
                if (k > 1 && k <= 3) {
                   mode = mode << (k - 1);
                }
                flags |= mode;
            }
            else if (!handle_flag(&flags, attr + i, flag_names)) {
                handle_bad_input(pi, el, attr[i]);
            }
        }
        pi->object = sp = create_spell(name);
        sp->rank = rank;
        sp->syntax = str_strdup(syntax);
        sp->parameter = str_strdup(parameter);
        sp->sptyp = flags;
    }
    else {
        handle_bad_input(pi, el, NULL);
    }
}

static void XMLCALL start_spellbooks(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    spellbook * sb = (spellbook *)pi->object;
    if (xml_strcmp(el, "spellbook") == 0) {
        const XML_Char *name = attr_get(attr, "name");

        if (name) {
            pi->object = sb = get_spellbook(name);
        }
        else {
            handle_bad_input(pi, el, NULL);
        }
    }
    else if (xml_strcmp(el, "entry") == 0) {
        int i, level = 0;
        const XML_Char *name = NULL;

        assert(sb);
        for (i = 0; attr[i]; i += 2) {
            if (xml_strcmp(attr[i], "spell") == 0) {
                name = attr[i + 1];
            }
            else if (xml_strcmp(attr[i], "level") == 0) {
                level = xml_int(attr[i + 1]);
            }
            else {
                handle_bad_input(pi, el, attr[i]);
            }
        }
        if (name && level > 0) {
            spellbook_addref(sb, name, level);
        }
        else {
            handle_bad_input(pi, el, NULL);
        }
    }
    else {
        handle_bad_input(pi, el, NULL);
    }
}

static void XMLCALL start_weapon(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    resource_type *rtype = (resource_type *)pi->object;

    assert(rtype && rtype->wtype);
    if (xml_strcmp(el, "function") == 0) {
        const XML_Char *name = NULL, *type = NULL;
        int i;

        for (i = 0; attr[i]; i += 2) {
            if (xml_strcmp(attr[i], "name") == 0) {
                type = attr[i + 1];
            }
            else if (xml_strcmp(attr[i], "value") == 0) {
                name = attr[i + 1];
            }
            else {
                handle_bad_input(pi, el, attr[i]);
            }
        }
        if (type && xml_strcmp(type, "attack") == 0) {
            pf_generic fun = get_function(name);
            rtype->wtype->attack = (wtype_attack)fun;
        }
        else {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    else if (xml_strcmp(el, "modifier") == 0) {
        const XML_Char *type = NULL;
        int i, flags = 0, race_mask = 0;
        int value = 0;

        for (i = 0; attr[i]; i += 2) {
            if (xml_strcmp(attr[i], "type") == 0) {
                type = attr[i + 1];
            }
            else if (xml_strcmp(attr[i], "value") == 0) {
                value = xml_int(attr[i + 1]);
            }
            else if (xml_strcmp(attr[i], "races") == 0) {
                char list[64];
                strcpy(list, attr[i + 1]);
                race_mask = rc_get_mask(list);
            }
            else if (xml_strcmp(attr[i], "offensive") == 0) {
                if (xml_bool(attr[i + 1])) {
                    flags |= WMF_OFFENSIVE;
                }
            }
            else if (xml_strcmp(attr[i], "defensive") == 0) {
                if (xml_bool(attr[i + 1])) {
                    flags |= WMF_DEFENSIVE;
                }
            }
            else if (xml_strcmp(attr[i], "walking") == 0) {
                if (xml_bool(attr[i + 1])) {
                    flags |= WMF_WALKING;
                }
            }
            else if (xml_strcmp(attr[i], "riding") == 0) {
                if (xml_bool(attr[i + 1])) {
                    flags |= WMF_RIDING;
                }
            }
            else if (xml_strcmp(attr[i], "against_riding") == 0) {
                if (xml_bool(attr[i + 1])) {
                    flags |= WMF_AGAINST_RIDING;
                }
            }
            else if (xml_strcmp(attr[i], "against_walking") == 0) {
                if (xml_bool(attr[i + 1])) {
                    flags |= WMF_AGAINST_WALKING;
                }
            }
            else {
                handle_bad_input(pi, el, attr[i]);
            }
        }
        if (type) {
            weapon_mod *mod = wmods + nwmods;

            assert(nwmods < WMOD_MAX);
            ++nwmods;

            /* weapon modifiers */
            if (xml_strcmp(type, "missile_target") == 0) {
                flags |= WMF_MISSILE_TARGET;
            }
            else if (xml_strcmp(type, "damage") == 0) {
                flags |= WMF_DAMAGE;
            }
            else if (xml_strcmp(type, "skill") == 0) {
                flags |= WMF_SKILL;
            }
            else {
                handle_bad_input(pi, el, type);
            }
            mod->value = value;
            mod->flags = flags;
            mod->race_mask = race_mask;
        }
        else {
            handle_bad_input(pi, el, NULL);
        }
    }
    else if (xml_strcmp(el, "damage") == 0) {
        weapon_type *wtype = rtype->wtype;
        int i, pos = 0;
        for (i = 0; attr[i]; i += 2) {
            if (xml_strcmp(attr[i], "type") == 0) {
                /* damage vs. rider(1) or not(0)? */
                if (xml_strcmp(attr[i + 1], "rider") == 0) {
                    pos = 1;
                }
            }
            else if (xml_strcmp(attr[i], "value") == 0) {
                wtype->damage[pos] = str_strdup(attr[i + 1]);
            }
            else {
                handle_bad_input(pi, el, NULL);
            }
        }
    }
    else {
        handle_bad_input(pi, el, NULL);
    }
}

static void handle_requirement(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    requirement *req;
    int i;

    assert(nreqs < MAX_REQUIREMENTS);
    req = reqs + nreqs;
    for (i = 0; attr[i]; i += 2) {
        if (xml_strcmp(attr[i], "type") == 0) {
            req->rtype = rt_get_or_create(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "quantity") == 0) {
            req->number = xml_int(attr[i + 1]);
        }
        else {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    ++nreqs;
}

static void handle_maintenance(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    maintenance *up;
    int i;

    assert(nupkeep < UPKEEP_MAX);
    up = upkeep + nupkeep;
    memset(up, 0, sizeof(maintenance));
    for (i = 0; attr[i]; i += 2) {
        if (xml_strcmp(attr[i], "type") == 0) {
            up->rtype = rt_get_or_create(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "amount") == 0) {
            up->number = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "variable") == 0) {
            if (xml_bool(attr[i + 1])) {
                up->flags |= MTF_VARIABLE;
            }
        }
        else {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    ++nupkeep;
}

#define COASTS_MAX 16
static int ncoasts;
static struct terrain_type *coasts[COASTS_MAX];

static void handle_coast(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    const XML_Char *tname = attr_get(attr, "terrain");

    if (tname) {
        terrain_type *coast = get_or_create_terrain(tname);
        assert(ncoasts < COASTS_MAX);
        coasts[ncoasts++] = coast;
    }
}

static void handle_modifier(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    int i;
    skill_t sk = NOSKILL;
    const XML_Char *type = NULL;
    resource_mod * mod = rmods + nrmods;
    const XML_Char *value = NULL;

    mod->race_mask = 0;
    mod->btype = NULL;
    assert(nrmods < RMOD_MAX);
    ++nrmods;
    for (i = 0; attr[i]; i += 2) {
        if (xml_strcmp(attr[i], "type") == 0) {
            type = attr[i + 1];
        }
        else if (xml_strcmp(attr[i], "building") == 0) {
            mod->btype = bt_get_or_create(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "skill") == 0) {
            sk = findskill(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "races") == 0) {
            char list[64];
            strcpy(list, attr[i + 1]);
            mod->race_mask = rc_get_mask(list);
        }
        else if (xml_strcmp(attr[i], "value") == 0) {
            value = attr[i + 1];
        }
        else {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    if (xml_strcmp(type, "skill") == 0) {
        mod->type = RMT_PROD_SKILL;
        mod->value.sa[0] = (short)sk;
        mod->value.sa[1] = (short)xml_int(value);
    }
    else if (xml_strcmp(type, "require") == 0) {
        mod->type = RMT_PROD_REQUIRE;
    }
    else if (xml_strcmp(type, "material") == 0) {
        mod->type = RMT_PROD_SAVE;
        mod->value = xml_fraction(value);
    }
    else if (xml_strcmp(type, "save") == 0) {
        mod->type = RMT_USE_SAVE;
        mod->value = xml_fraction(value);
    }
    else {
        handle_bad_input(pi, el, type);
    }
}

static construction *parse_construction(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    construction *con = calloc(sizeof(construction), 1);
    int i;
    con->maxsize = -1;
    con->minskill = -1;
    con->reqsize = 1;
    for (i = 0; attr[i]; i += 2) {
        if (xml_strcmp(attr[i], "skill") == 0) {
            con->skill = findskill(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "maxsize") == 0) {
            con->maxsize = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "reqsize") == 0) {
            con->reqsize = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "minskill") == 0) {
            con->minskill = xml_int(attr[i + 1]);
        }
        else if (stage != NULL && xml_strcmp(attr[i], "name") == 0) {
            /* only building stages have names */
            stage->name = str_strdup(attr[i + 1]);
        }
        else {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    nreqs = 0;
    return con;
}

static void start_resources(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    resource_type *rtype = (resource_type *)pi->object;
    if (xml_strcmp(el, "resource") == 0) {
        handle_resource(pi, el, attr);
    }
    else if (rtype) {
        if (xml_strcmp(el, "item") == 0) {
            assert(rtype);
            handle_item(pi, el, attr);
        }
        else if (xml_strcmp(el, "function") == 0) {
            const XML_Char *name = NULL;
            pf_generic fun = NULL;
            int i;

            for (i = 0; attr[i]; i += 2) {
                if (xml_strcmp(attr[i], "name") == 0) {
                    name = attr[i + 1];
                }
                else if (xml_strcmp(attr[i], "value") == 0) {
                    fun = get_function(attr[i + 1]);
                }
                else {
                    handle_bad_input(pi, el, attr[i]);
                }
            }

            assert(rtype);
            if (name && fun) {
                if (xml_strcmp(name, "change") == 0) {
                    rtype->uchange = (rtype_uchange)fun;
                }
                else if (xml_strcmp(name, "name") == 0) {
                    rtype->name = (rtype_name)fun;
                }
                else if (xml_strcmp(name, "attack") == 0) {
                    assert(rtype->wtype);
                    rtype->wtype->attack = (wtype_attack)fun;
                }
            }
        }
        else if (xml_strcmp(el, "modifier") == 0) {
            handle_modifier(pi, el, attr);
        }
        else if (rtype->itype) {
            item_type *itype = rtype->itype;
            if (xml_strcmp(el, "construction") == 0) {
                itype->construction = parse_construction(pi, el, attr);
            }
            else if (xml_strcmp(el, "requirement") == 0) {
                assert(itype->construction);
                handle_requirement(pi, el, attr);
            }
            else if (xml_strcmp(el, "luxury") == 0) {
                rtype->ltype = new_luxurytype(itype, 0);
            }
            else if (xml_strcmp(el, "potion") == 0) {
                int i, level = 0;
                for (i = 0; attr[i]; i += 2) {
                    if (xml_strcmp(attr[i], "level") == 0) {
                        level = xml_int(attr[i + 1]);
                    }
                    else {
                        handle_bad_input(pi, el, attr[i]);
                    }
                }
                new_potiontype(itype, level);
            }
            else if (xml_strcmp(el, "armor") == 0) {
                handle_armor(pi, el, attr);
            }
            else if (xml_strcmp(el, "weapon") == 0) {
                pi->type = EXP_WEAPON;
                handle_weapon(pi, el, attr);
            }
            else {
                handle_bad_input(pi, el, NULL);
            }
        }
        else {
            handle_bad_input(pi, el, NULL);
        }
    }
    else {
        handle_bad_input(pi, el, NULL);
    }
}

static void XMLCALL start_ships(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "opensea", "fly", "nocoast", "speedy", NULL };
    if (xml_strcmp(el, "ship") == 0) {
        const XML_Char *name;

        name = attr_get(attr, "name");
        if (name) {
            ship_type *stype = st_get_or_create(name);
            int i, flags = SFL_DEFAULT;

            for (i = 0; attr[i]; i += 2) {
                if (xml_strcmp(attr[i], "range") == 0) {
                    stype->range = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "maxrange") == 0) {
                    stype->range_max = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "cabins") == 0) {
                    stype->cabins = PERSON_WEIGHT * xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "cargo") == 0) {
                    stype->cargo = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "combat") == 0) {
                    stype->combat = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "fishing") == 0) {
                    stype->fishing = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "cptskill") == 0) {
                    stype->cptskill = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "minskill") == 0) {
                    stype->minskill = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "sumskill") == 0) {
                    stype->sumskill = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "damage") == 0) {
                    stype->damage = xml_float(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "storm") == 0) {
                    stype->storm = xml_float(attr[i + 1]);
                }
                else if (!handle_flag(&flags, attr + i, flag_names)) {
                    /* we already handled the name earlier */
                    if (xml_strcmp(attr[i], "name") != 0) {
                        handle_bad_input(pi, el, attr[i]);
                    }
                }
            }
            stype->flags = flags;
            pi->object = stype;
        }
    }
    else {
        ship_type *stype = (ship_type *)pi->object;
        assert(stype);
        if (xml_strcmp(el, "modifier") == 0) {
            /* these modifiers are not like buildings */
            int i;
            const XML_Char *type = NULL, *value = NULL;
            for (i = 0; attr[i]; i += 2) {
                if (xml_strcmp(attr[i], "type") == 0) {
                    type = attr[i + 1];
                }
                else if (xml_strcmp(attr[i], "value") == 0) {
                    value = attr[i + 1];
                }
                else if (xml_strcmp(attr[i], "factor") == 0) {
                    value = attr[i + 1];
                }
                else {
                    handle_bad_input(pi, el, attr[i]);
                }
            }
            if (type) {
                if (xml_strcmp(type, "tactics") == 0) {
                    stype->tac_bonus = xml_float(value);
                }
                else if (xml_strcmp(type, "attack") == 0) {
                    stype->at_bonus = xml_int(value);
                }
                else if (xml_strcmp(type, "defense") == 0) {
                    stype->df_bonus = xml_int(value);
                }
            }
        }
        else if (xml_strcmp(el, "requirement") == 0) {
            assert(stype->construction);
            handle_requirement(pi, el, attr);
        }
        else if (xml_strcmp(el, "construction") == 0) {
            assert(!stype->construction);
            stype->construction = parse_construction(pi, el, attr);
        }
        else if (xml_strcmp(el, "coast") == 0) {
            handle_coast(pi, el, attr);
        }
        else {
            handle_bad_input(pi, el, NULL);
        }
    }
}

static void XMLCALL start_races(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    race *rc = (race *)pi->object;
    const char *flag_names[] = {
        "!playerrace", "killpeasants", "scarepeasants", "!cansteal",
        "moverandom", "cannotmove", "learn", "fly", "swim", "walk",
        "!canlearn", "!canteach", "horse", "desert", "illusionary",
        "absorbpeasants", "noheal", "noweapons", "shapeshift",
        "shapeshiftany", "undead", "dragon", "coastal", "unarmedguard",
        "cansail", "invisible", "shipspeed", "moveattack", "migrants", NULL };
    const char *bflag_names[] = {
        "equipment", "noblock", "resistpierce", "resistcut", "resistbash",
        "invinciblenonmagic", "noattack", NULL };
    const char *eflag_names[] = {
        "giveperson", "giveunit", "getitem", "recruitethereal", 
        "recruitunlimited", "stonegolem", "irongolem", NULL };

    if (xml_strcmp(el, "attack") == 0) {
        assert(rc);
    }
    else if (xml_strcmp(el, "familiar") == 0) {
        assert(rc);
    }
    else if (xml_strcmp(el, "skill") == 0) {
        const XML_Char *name = NULL;
        int i, speed = 0, mod = 0;

        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strcmp(key, "name") == 0) {
                name = val;
            }
            else if (xml_strcmp(key, "modifier") == 0) {
                mod = xml_int(val);
            }
            else if (xml_strcmp(key, "speed") == 0) {
                speed = xml_int(val);
            }
            else {
                handle_bad_input(pi, el, key);
            }
        }
        if (name) {
            skill_t sk = findskill(name);
            if (sk != NOSKILL) {
                rc->bonus[sk] = (char)mod;
                if (speed != 0) {
                    set_study_speed(rc, sk, speed);
                }
            }
        }
    }
    else if (xml_strcmp(el, "param") == 0) {
        const XML_Char *key = attr_get(attr, "name"), *val = attr_get(attr, "value");
        if (key && val) {
            rc_set_param(rc, key, val);
        }
    }
    else if (xml_strcmp(el, "ai") == 0) {
        /* AI flags are cumulative to race flags. XML format is dumb */
        int i, flags = 0;
        assert(rc);
        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strcmp(key, "splitsize") == 0) {
                rc->splitsize = xml_int(val);
            }
            else if (xml_strcmp(key, "scare") == 0) {
                rc_set_param(rc, "scare", val);
            }
            else if (!handle_flag(&flags, attr + i, flag_names)) {
                handle_bad_input(pi, el, key);
            }
        }
        rc->flags |= flags;
    }
    else if (xml_strcmp(el, "race") == 0) {
        const XML_Char *name;

        name = attr_get(attr, "name");
        if (name) {
            assert(!rc);
            pi->object = rc = rc_get_or_create(name);
            int i;

            for (i = 0; attr[i]; i += 2) {
                const XML_Char *key = attr[i], *val = attr[i + 1];
                if (xml_strcmp(key, "maxaura") == 0) {
                    rc->maxaura = (int)(100 * xml_float(val));
                }
                else if (xml_strcmp(key, "magres") == 0) {
                    /* specified in percent: */
                    rc->magres = frac_make(xml_int(val), 100);
                }
                else if (xml_strcmp(key, "healing") == 0) {
                    rc->healing = (int)(xml_float(val) * 100);
                }
                else if (xml_strcmp(key, "regaura") == 0) {
                    rc->regaura = xml_float(val);
                }
                else if (xml_strcmp(key, "recruitcost") == 0) {
                    rc->recruitcost = xml_int(val);
                }
                else if (xml_strcmp(key, "maintenance") == 0) {
                    rc->maintenance = xml_int(val);
                }
                else if (xml_strcmp(key, "income") == 0) {
                    rc->income = xml_int(val);
                }
                else if (xml_strcmp(key, "weight") == 0) {
                    rc->weight = xml_int(val);
                }
                else if (xml_strcmp(key, "capacity") == 0) {
                    rc->capacity = xml_int(val);
                }
                else if (xml_strcmp(key, "speed") == 0) {
                    rc->speed = xml_float(val);
                }
                else if (xml_strcmp(key, "hp") == 0) {
                    rc->hitpoints = xml_int(val);
                }
                else if (xml_strcmp(key, "ac") == 0) {
                    rc->armor = xml_int(val);
                }
                else if (xml_strcmp(key, "damage") == 0) {
                    rc->def_damage = str_strdup(val);
                }
                else if (xml_strcmp(key, "unarmedattack") == 0) {
                    rc->at_default = xml_int(val);
                }
                else if (xml_strcmp(key, "unarmeddefense") == 0) {
                    rc->df_default = xml_int(val);
                }
                else if (xml_strcmp(key, "attackmodifier") == 0) {
                    rc->at_bonus = xml_int(val);
                }
                else if (xml_strcmp(key, "defensemodifier") == 0) {
                    rc->df_bonus = xml_int(val);
                }
                else if (xml_strcmp(key, "studyspeed") == 0) {
                    int study_speed = xml_int(val);
                    if (study_speed != 0) {
                        skill_t sk;
                        for (sk = 0; sk < MAXSKILLS; ++sk) {
                            set_study_speed(rc, sk, study_speed);
                        }
                    }

                }
                else if (!handle_flag(&rc->flags, attr + i, flag_names)) {
                    if (!handle_flag(&rc->battle_flags, attr + i, bflag_names)) {
                        if (!handle_flag(&rc->ec_flags, attr + i, eflag_names)) {
                            /* we already handled the name earlier: */
                            if (xml_strcmp(key, "name") != 0) {
                                handle_bad_input(pi, el, attr[i]);
                            }
                        }
                    }
                }
            }
        }
    }
    else {
        assert(rc);
        handle_bad_input(pi, el, NULL);
    }
}

static void XMLCALL start_buildings(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "nodestroy", "nobuild", "unique", "decay", "magic", "namechange", "fort", "oneperturn", NULL };
    if (xml_strcmp(el, "building") == 0) {
        const XML_Char *name;

        assert(stage == NULL);
        name = attr_get(attr, "name");
        if (name) {
            building_type *btype = bt_get_or_create(name);
            int i, flags = BTF_DEFAULT;

            for (i = 0; attr[i]; i += 2) {
                if (xml_strcmp(attr[i], "maxsize") == 0) {
                    btype->maxsize = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "capacity") == 0) {
                    btype->capacity = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "maxcapacity") == 0) {
                    btype->maxcapacity = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "magresbonus") == 0) {
                    btype->magresbonus = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "fumblebonus") == 0) {
                    btype->fumblebonus = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "taxes") == 0) {
                    btype->taxes = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "auraregen") == 0) {
                    btype->auraregen = xml_int(attr[i + 1]);
                }
                else if (xml_strcmp(attr[i], "magres") == 0) {
                    /* magres is specified in percent! */
                    btype->magres = frac_make(xml_int(attr[i + 1]), 100);
                }
                else if (!handle_flag(&flags, attr + i, flag_names)) {
                    /* we already handled the name earlier */
                    if (xml_strcmp(attr[i], "name") != 0) {
                        handle_bad_input(pi, el, attr[i]);
                    }
                }
            }
            btype->flags = flags;
            pi->object = btype;
        }
    }
    else {
        building_type *btype = (building_type *)pi->object;
        assert(btype);
        if (xml_strcmp(el, "modifier") == 0) {
            handle_modifier(pi, el, attr);
        }
        else if (xml_strcmp(el, "requirement") == 0) {
            assert(stage);
            assert(stage->construction);
            handle_requirement(pi, el, attr);
        }
        else if (xml_strcmp(el, "construction") == 0) {
            assert(stage == NULL);
            stage = calloc(1, sizeof(building_stage));
            stage->construction = parse_construction(pi, el, attr);
        }
        else if (xml_strcmp(el, "maintenance") == 0) {
            assert(!btype->maintenance);
            handle_maintenance(pi, el, attr);
        }
        else {
            handle_bad_input(pi, el, NULL);
        }
    }
}

static void XMLCALL handle_start(void *data, const XML_Char *el, const XML_Char **attr) {
    parseinfo *pi = (parseinfo *)data;
    if (pi->depth == 0) {
        pi->type = EXP_UNKNOWN;
        if (xml_strcmp(el, "eressea") != 0) {
            handle_bad_input(pi, el, NULL);
        }
    }
    else if (pi->depth == 1) {
        if (xml_strcmp(el, "resources") == 0) {
            pi->type = EXP_RESOURCES;
        }
        else if (xml_strcmp(el, "buildings") == 0) {
            pi->type = EXP_BUILDINGS;
        }
        else if (xml_strcmp(el, "ships") == 0) {
            pi->type = EXP_SHIPS;
        }
        else if (xml_strcmp(el, "messages") == 0) {
            pi->type = EXP_MESSAGES;
        }
        else if (xml_strcmp(el, "strings") == 0) {
            pi->type = EXP_STRINGS;
        }
        else if (xml_strcmp(el, "spells") == 0) {
            pi->type = EXP_SPELLS;
        }
        else if (xml_strcmp(el, "spellbook") == 0) {
            pi->type = EXP_SPELLBOOKS;
            start_spellbooks(pi, el, attr);
        }
        else if (xml_strcmp(el, "races") == 0) {
            pi->type = EXP_RACES;
        }
        else {
            handle_bad_input(pi, el, NULL);
        }
    }
    else {
        switch (pi->type) {
        case EXP_RACES:
            start_races(pi, el, attr);
            break;
        case EXP_BUILDINGS:
            start_buildings(pi, el, attr);
            break;
        case EXP_SHIPS:
            start_ships(pi, el, attr);
            break;
        case EXP_RESOURCES:
            start_resources(pi, el, attr);
            break;
        case EXP_WEAPON:
            start_weapon(pi, el, attr);
            break;
        case EXP_SPELLBOOKS:
            start_spellbooks(pi, el, attr);
            break;
        case EXP_SPELLS:
            start_spells(pi, el, attr);
            break;
        case EXP_UNKNOWN:
            handle_bad_input(pi, el, NULL);
            break;
        default:
            /* not implemented */
            handle_bad_input(pi, el, NULL);
            return;
        }
    }
    ++pi->depth;
}

static void end_spells(parseinfo *pi, const XML_Char *el) {
    if (xml_strcmp(el, "spells") == 0) {
        pi->type = EXP_UNKNOWN;
    }
    else if (xml_strcmp(el, "spell") == 0) {
        spell *sp = (spell *)pi->object;
        if (ncomponents > 0) {
            sp->components = calloc(sizeof(spell_component), ncomponents + 1);
            memcpy(sp->components, components, sizeof(spell_component) * ncomponents);
            ncomponents = 0;
        }
        pi->object = NULL;
    }
}

static void end_weapon(parseinfo *pi, const XML_Char *el) {
    resource_type *rtype = (resource_type *)pi->object;
    assert(rtype && rtype->wtype);

    if (xml_strcmp(el, "weapon") == 0) {
        pi->type = EXP_RESOURCES;
    }
    else if (xml_strcmp(el, "modifier") == 0) {
        if (nwmods > 0) {
            weapon_type *wtype = rtype->wtype;
            wtype->modifiers = calloc(sizeof(weapon_mod), nwmods + 1);
            memcpy(wtype->modifiers, wmods, sizeof(weapon_mod) * nwmods);
            nwmods = 0;
        }
    }
}

static void end_resources(parseinfo *pi, const XML_Char *el) {
    resource_type *rtype = (resource_type *)pi->object;
    if (xml_strcmp(el, "resource") == 0) {
        if (nrmods > 0) {
            rtype->modifiers = calloc(sizeof(resource_mod), nrmods + 1);
            memcpy(rtype->modifiers, rmods, sizeof(resource_mod) * nrmods);
            nrmods = 0;
        }
    }
    else if (xml_strcmp(el, "construction") == 0) {
        if (nreqs > 0) {
            construction *con = rtype->itype->construction;
            con->materials = calloc(sizeof(requirement), nreqs + 1);
            memcpy(con->materials, reqs, sizeof(requirement) * nreqs);
            nreqs = 0;
        }
    }
    else if (xml_strcmp(el, "resources") == 0) {
        pi->type = EXP_UNKNOWN;
    }
}

static void end_races(parseinfo *pi, const XML_Char *el) {
    if (xml_strcmp(el, "race") == 0) {
        pi->object = NULL;
    }
    else if (xml_strcmp(el, "races") == 0) {
        pi->type = EXP_UNKNOWN;
    }
}

static void end_ships(parseinfo *pi, const XML_Char *el) {
    ship_type *stype = (ship_type *)pi->object;
    if (xml_strcmp(el, "construction") == 0) {
        assert(stype);
        assert(stype->construction);
        if (nreqs > 0) {
            construction *con = stype->construction;
            con->materials = calloc(sizeof(requirement), nreqs + 1);
            memcpy(con->materials, reqs, sizeof(requirement) * nreqs);
            nreqs = 0;
        }
    }
    else if (xml_strcmp(el, "ship") == 0) {
        if (ncoasts > 0) {
            stype->coasts = calloc(sizeof(const terrain_type *), ncoasts + 1);
            memcpy(stype->coasts, coasts, sizeof(const terrain_type *) * ncoasts);
            ncoasts = 0;
        }
        pi->object = NULL;
    }
    else if (xml_strcmp(el, "ships") == 0) {
        pi->type = EXP_UNKNOWN;
    }
}

static void end_buildings(parseinfo *pi, const XML_Char *el) {
    /* stores the end of the building's stage list: */
    static building_stage **stage_ptr;

    building_type *btype = (building_type *)pi->object;
    if (xml_strcmp(el, "construction") == 0) {
        assert(btype);
        if (stage) {
            if (nreqs > 0) {
                construction *con = stage->construction;
                con->materials = calloc(sizeof(requirement), nreqs + 1);
                memcpy(con->materials, reqs, sizeof(requirement) * nreqs);
                nreqs = 0;
            }
            if (stage_ptr == NULL) {
                /* at the first build stage, initialize stage_ptr: */
                assert(btype->stages == NULL);
                stage_ptr = &btype->stages;
            }
            *stage_ptr = stage;
            stage_ptr = &stage->next;
            stage = NULL;
        }
    }
    else if (xml_strcmp(el, "building") == 0) {
        stage_ptr = NULL;
        if (nupkeep > 0) {
            btype->maintenance = calloc(sizeof(maintenance), nupkeep + 1);
            memcpy(btype->maintenance, upkeep, sizeof(maintenance) * nupkeep);
            nupkeep = 0;
        }
        if (nrmods > 0) {
            btype->modifiers = calloc(sizeof(resource_mod), nrmods + 1);
            memcpy(btype->modifiers, rmods, sizeof(resource_mod) * nrmods);
            nrmods = 0;
        }
        pi->object = NULL;
    }
    else if (xml_strcmp(el, "buildings") == 0) {
        pi->type = EXP_UNKNOWN;
    }
}

static void XMLCALL handle_end(void *data, const XML_Char *el) {
    parseinfo *pi = (parseinfo *)data;

    switch (pi->type) {
    case EXP_RACES:
        end_races(pi, el);
        break;
    case EXP_SHIPS:
        end_ships(pi, el);
        break;
    case EXP_BUILDINGS:
        end_buildings(pi, el);
        break;
    case EXP_RESOURCES:
        end_resources(pi, el);
        break;
    case EXP_WEAPON:
        end_weapon(pi, el);
        break;
    case EXP_SPELLS:
        end_spells(pi, el);
        break;
    default:
        if (pi->depth == 1) {
            pi->object = NULL;
            pi->type = EXP_UNKNOWN;
        }
        if (pi->cdata) {
            free(pi->cdata);
            pi->cdata = NULL;
            pi->clength = 0;
        }
    }
    --pi->depth;
    if (pi->depth == 0) {
        assert(pi->type == EXP_UNKNOWN);
    }
}

static void XMLCALL handle_data(void *data, const XML_Char *xs, int len) {
    parseinfo *pi = (parseinfo *)data;
    if (len > 0) {
        if (pi->type == EXP_MESSAGES && pi->depth == 4) {
            size_t bytes = (size_t)len;
            pi->cdata = realloc(pi->cdata, pi->clength + bytes);
            memcpy(pi->cdata + pi->clength, xs, bytes);
            pi->clength = pi->clength + bytes;
        }
    }
}

int exparse_readfile(const char * filename) {
    XML_Parser xp;
    FILE *F;
    int err = 0;
    char buf[4096];
    parseinfo pi;

    F = fopen(filename, "r");
    if (!F) {
        return 2;
    }
    xp = XML_ParserCreate("UTF-8");
    XML_SetElementHandler(xp, handle_start, handle_end);
    XML_SetCharacterDataHandler(xp, handle_data);
    XML_SetUserData(xp, &pi);
    memset(&pi, 0, sizeof(pi));
    for (;;) {
        size_t len = (int)fread(buf, 1, sizeof(buf), F);
        int done;

        if (ferror(F)) {
            log_error("read error in %s", filename);
            err = -2;
            break;
        }
        done = feof(F);
        if (XML_Parse(xp, buf, len, done) == XML_STATUS_ERROR) {
            log_error("parse error at line %" XML_FMT_INT_MOD " of %s: %" XML_FMT_STR,
                XML_GetCurrentLineNumber(xp),
                filename,
                XML_ErrorString(XML_GetErrorCode(xp)));
            err = -1;
            break;
        }
        if (done) {
            break;
        }
    }
    if (pi.depth != 0) {
        err = -3;
    }
    XML_ParserFree(xp);
    fclose(F);
    if (err != 0) {
        return err;
    }
    return pi.errors;
}
