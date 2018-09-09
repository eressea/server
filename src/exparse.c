#ifdef _MSC_VER
#include <platform.h>
#endif
#include "exparse.h"

#include "alchemy.h"

#include "modules/score.h"

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
#include "util/message.h"
#include "util/crmessage.h"
#include "util/nrmessage.h"
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

static bool xml_strequal(const XML_Char *xs, const char *cs) {
    if (xs && cs) {
        return strcmp(xs, cs) == 0;
    }
    return false;
}

static bool xml_bool(const XML_Char *val) {
    if (xml_strequal(val, "yes")) return true;
    if (xml_strequal(val, "true")) return true;
    if (xml_strequal(val, "1")) return true;
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
        if (xml_strequal(attr[i], key)) {
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
            if (xml_strequal(pair[0], name+1)) {
                if (xml_bool(pair[1])) {
                    *flags &= ~(1 << i);
                }
                else {
                    *flags |= (1 << i);
                }
                return true;
            }
        }
        else if (xml_strequal(pair[0], name)) {
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
        if (xml_strequal(attr[i], "name")) {
            name = attr[i + 1];
        }
        else if (xml_strequal(attr[i], "appearance")) {
            /* TODO: appearance should be a property of item, not resource */
            appear = attr[i + 1];
            flags |= RTF_ITEM;
        }
        else if (xml_strequal(attr[i], "material")) {
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
        if (xml_strequal(attr[i], "weight")) {
            itype->weight = xml_int(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "capacity")) {
            itype->capacity = xml_int(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "score")) {
            itype->score = xml_int(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "allow")) {
            size_t len = strlen(attr[i + 1]);
            assert(len < sizeof(buffer));
            memcpy(buffer, attr[i + 1], len + 1);
            itype->mask_allow = rc_get_mask(buffer);
        }
        else if (xml_strequal(attr[i], "deny")) {
            size_t len = strlen(attr[i + 1]);
            assert(len < sizeof(buffer));
            memcpy(buffer, attr[i + 1], len + 1);
            itype->mask_deny = rc_get_mask(buffer);
        }
        else if (!handle_flag(&flags, attr + i, flag_names)) {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    if (itype->score == 0) {
        itype->score = default_score(itype);
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
        if (xml_strequal(attr[i], "penalty")) {
            atype->penalty = xml_float(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "projectile")) {
            atype->projectile = xml_float(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "ac")) {
            atype->prot = xml_int(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "magres")) {
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
        if (xml_strequal(attr[i], "offmod")) {
            wtype->offmod = xml_int(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "defmod")) {
            wtype->defmod = xml_int(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "reload")) {
            wtype->reload = xml_int(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "skill")) {
            wtype->skill = findskill(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "magres")) {
            wtype->magres = xml_fraction(attr[i + 1]);;
        }
        else if (!handle_flag(&flags, attr + i, flag_names)) {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    wtype->flags = flags;
}

static int msg_nargs;
static char * msg_args[MSG_MAXARGS];

static void end_messages(parseinfo *pi, const XML_Char *el) {
    if (xml_strequal(el, "message")) {
        int i;
        struct message_type *mtype = (struct message_type *)pi->object;
        assert(mtype);
        assert(msg_nargs < MSG_MAXARGS);
        mt_create(mtype, (const char **)msg_args, msg_nargs);
        /* register the type for CR and NR */
        crt_register(mtype);
        nrt_register(mtype);

        for (i = 0; i != msg_nargs; ++i) {
            free(msg_args[i]);
            msg_args[i] = NULL;
        }
        msg_nargs = 0;
    }
    else if (xml_strequal(el, "messages")) {
        pi->type = EXP_UNKNOWN;
    }
}

static void start_messages(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    if (xml_strequal(el, "arg")) {
        int i;
        const XML_Char *name = NULL, *type = NULL;

        assert(msg_nargs < MSG_MAXARGS);
        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strequal(key, "name")) {
                name = val;
            }
            else if (xml_strequal(key, "type")) {
                type = val;
            }
            else {
                handle_bad_input(pi, el, key);
            }
        }
        if (name && type) {
            char zBuffer[128];
            sprintf(zBuffer, "%s:%s", name, type);
            msg_args[msg_nargs++] = str_strdup(zBuffer);
        }
    }
    else if (xml_strequal(el, "message")) {
        const XML_Char *name = NULL, *section = NULL;
        int i;
        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strequal(key, "name")) {
                name = val;
            }
            else if (xml_strequal(key, "section")) {
                section = val;
            }
            else {
                handle_bad_input(pi, el, key);
            }
        }
        if (name) {
            pi->object = mt_new(name, section);
        }
    }
    else if (!xml_strequal(el, "type")) {
        handle_bad_input(pi, el, NULL);
    }
}

#define MAX_COMPONENTS 8
static spell_component components[MAX_COMPONENTS];
static int ncomponents;

static void start_spells(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = {
        "far", "variable", "ocean", "ship", "los", 
        "unittarget", "shiptarget", "buildingtarget", "regiontarget", "globaltarget", NULL };

    if (xml_strequal(el, "resource")) {
        spell_component *spc;
        int i;

        assert(ncomponents < MAX_COMPONENTS);
        spc = components + ncomponents;
        spc->cost = SPC_FIX;
        ++ncomponents;
        memset(spc, 0, sizeof(spell_component));
        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strequal(key, "name")) {
                spc->type = rt_get_or_create(val);
            }
            else if (xml_strequal(key, "amount")) {
                spc->amount = xml_int(val);
            }
            else if (xml_strequal(key, "cost")) {
                if (xml_strequal(val, "level")) {
                    spc->cost = SPC_LEVEL;
                }
                else if (xml_strequal(val, "linear")) {
                    spc->cost = SPC_LINEAR;
                }
                else if (xml_strequal(val, "fixed")) {
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
    else if (xml_strequal(el, "spell")) {
        spell *sp;
        const XML_Char *name = NULL, *syntax = NULL, *parameter = NULL;
        int i, rank = 0, flags = 0;
        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strequal(key, "name")) {
                name = val;
            }
            else if (xml_strequal(key, "syntax")) {
                syntax = val;
            }
            else if (xml_strequal(key, "parameters")) {
                parameter = val;
            }
            else if (xml_strequal(key, "rank")) {
                rank = xml_int(val);
            }
            else if (xml_strequal(key, "combat")) {
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

static void start_spellbooks(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    spellbook * sb = (spellbook *)pi->object;
    if (xml_strequal(el, "spellbook")) {
        const XML_Char *name = attr_get(attr, "name");

        if (name) {
            pi->object = sb = get_spellbook(name);
        }
        else {
            handle_bad_input(pi, el, NULL);
        }
    }
    else if (xml_strequal(el, "entry")) {
        int i, level = 0;
        const XML_Char *name = NULL;

        assert(sb);
        for (i = 0; attr[i]; i += 2) {
            if (xml_strequal(attr[i], "spell")) {
                name = attr[i + 1];
            }
            else if (xml_strequal(attr[i], "level")) {
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

static void start_weapon(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    resource_type *rtype = (resource_type *)pi->object;

    assert(rtype && rtype->wtype);
    if (xml_strequal(el, "function")) {
        const XML_Char *name = NULL, *type = NULL;
        int i;

        for (i = 0; attr[i]; i += 2) {
            if (xml_strequal(attr[i], "name")) {
                type = attr[i + 1];
            }
            else if (xml_strequal(attr[i], "value")) {
                name = attr[i + 1];
            }
            else {
                handle_bad_input(pi, el, attr[i]);
            }
        }
        if (name && type && xml_strequal(type, "attack")) {
            pf_generic fun = get_function(name);
            rtype->wtype->attack = (wtype_attack)fun;
        }
        else {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    else if (xml_strequal(el, "modifier")) {
        const XML_Char *type = NULL;
        int i, flags = 0, race_mask = 0;
        int value = 0;

        for (i = 0; attr[i]; i += 2) {
            if (xml_strequal(attr[i], "type")) {
                type = attr[i + 1];
            }
            else if (xml_strequal(attr[i], "value")) {
                value = xml_int(attr[i + 1]);
            }
            else if (xml_strequal(attr[i], "races")) {
                char list[64];
                str_strlcpy(list, attr[i + 1], sizeof(list));
                race_mask = rc_get_mask(list);
            }
            else if (xml_strequal(attr[i], "offensive")) {
                if (xml_bool(attr[i + 1])) {
                    flags |= WMF_OFFENSIVE;
                }
            }
            else if (xml_strequal(attr[i], "defensive")) {
                if (xml_bool(attr[i + 1])) {
                    flags |= WMF_DEFENSIVE;
                }
            }
            else if (xml_strequal(attr[i], "walking")) {
                if (xml_bool(attr[i + 1])) {
                    flags |= WMF_WALKING;
                }
            }
            else if (xml_strequal(attr[i], "riding")) {
                if (xml_bool(attr[i + 1])) {
                    flags |= WMF_RIDING;
                }
            }
            else if (xml_strequal(attr[i], "against_riding")) {
                if (xml_bool(attr[i + 1])) {
                    flags |= WMF_AGAINST_RIDING;
                }
            }
            else if (xml_strequal(attr[i], "against_walking")) {
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
            if (xml_strequal(type, "missile_target")) {
                flags |= WMF_MISSILE_TARGET;
            }
            else if (xml_strequal(type, "damage")) {
                flags |= WMF_DAMAGE;
            }
            else if (xml_strequal(type, "skill")) {
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
    else if (xml_strequal(el, "damage")) {
        weapon_type *wtype = rtype->wtype;
        int i, pos = 0;
        for (i = 0; attr[i]; i += 2) {
            if (xml_strequal(attr[i], "type")) {
                /* damage vs. rider(1) or not(0)? */
                if (xml_strequal(attr[i + 1], "rider")) {
                    pos = 1;
                }
            }
            else if (xml_strequal(attr[i], "value")) {
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
    req->number = 1;
    for (i = 0; attr[i]; i += 2) {
        if (xml_strequal(attr[i], "type")) {
            req->rtype = rt_get_or_create(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "quantity")) {
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
        if (xml_strequal(attr[i], "type")) {
            up->rtype = rt_get_or_create(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "amount")) {
            up->number = xml_int(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "variable")) {
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
        if (xml_strequal(attr[i], "type")) {
            type = attr[i + 1];
        }
        else if (xml_strequal(attr[i], "building")) {
            mod->btype = bt_get_or_create(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "skill")) {
            sk = findskill(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "races")) {
            char list[64];
            str_strlcpy(list, attr[i + 1], sizeof(list));
            mod->race_mask = rc_get_mask(list);
        }
        else if (xml_strequal(attr[i], "value")) {
            value = attr[i + 1];
        }
        else {
            handle_bad_input(pi, el, attr[i]);
        }
    }
    if (xml_strequal(type, "skill")) {
        mod->type = RMT_PROD_SKILL;
        if (value) {
            mod->value.sa[0] = (short)sk;
            mod->value.sa[1] = (short)xml_int(value);
        }
        else {
            handle_bad_input(pi, el, type);
        }
    }
    else if (xml_strequal(type, "require")) {
        mod->type = RMT_PROD_REQUIRE;
    }
    else if (xml_strequal(type, "material")) {
        mod->type = RMT_PROD_SAVE;
        if (value) {
            mod->value = xml_fraction(value);
        }
        else {
            handle_bad_input(pi, el, type);
        }
    }
    else if (xml_strequal(type, "save")) {
        mod->type = RMT_USE_SAVE;
        if (value) {
            mod->value = xml_fraction(value);
        }
        else {
            handle_bad_input(pi, el, type);
        }
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
        if (xml_strequal(attr[i], "skill")) {
            con->skill = findskill(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "maxsize")) {
            con->maxsize = xml_int(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "reqsize")) {
            con->reqsize = xml_int(attr[i + 1]);
        }
        else if (xml_strequal(attr[i], "minskill")) {
            con->minskill = xml_int(attr[i + 1]);
        }
        else if (stage != NULL && xml_strequal(attr[i], "name")) {
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
    if (xml_strequal(el, "resource")) {
        handle_resource(pi, el, attr);
    }
    else if (rtype) {
        if (xml_strequal(el, "item")) {
            assert(rtype);
            handle_item(pi, el, attr);
        }
        else if (xml_strequal(el, "function")) {
            const XML_Char *name = NULL;
            pf_generic fun = NULL;
            int i;

            for (i = 0; attr[i]; i += 2) {
                if (xml_strequal(attr[i], "name")) {
                    name = attr[i + 1];
                }
                else if (xml_strequal(attr[i], "value")) {
                    fun = get_function(attr[i + 1]);
                }
                else {
                    handle_bad_input(pi, el, attr[i]);
                }
            }

            assert(rtype);
            if (name && fun) {
                if (xml_strequal(name, "change")) {
                    rtype->uchange = (rtype_uchange)fun;
                }
                else if (xml_strequal(name, "name")) {
                    rtype->name = (rtype_name)fun;
                }
                else if (xml_strequal(name, "attack")) {
                    assert(rtype->wtype);
                    rtype->wtype->attack = (wtype_attack)fun;
                }
            }
        }
        else if (xml_strequal(el, "modifier")) {
            handle_modifier(pi, el, attr);
        }
        else if (rtype->itype) {
            item_type *itype = rtype->itype;
            if (xml_strequal(el, "construction")) {
                itype->construction = parse_construction(pi, el, attr);
            }
            else if (xml_strequal(el, "requirement")) {
                assert(itype->construction);
                handle_requirement(pi, el, attr);
            }
            else if (xml_strequal(el, "luxury")) {
                int price = atoi(attr_get(attr, "price"));
                assert(price > 0);
                rtype->ltype = new_luxurytype(itype, price);
            }
            else if (xml_strequal(el, "potion")) {
                int i, level = 0;
                for (i = 0; attr[i]; i += 2) {
                    if (xml_strequal(attr[i], "level")) {
                        level = xml_int(attr[i + 1]);
                    }
                    else {
                        handle_bad_input(pi, el, attr[i]);
                    }
                }
                new_potiontype(itype, level);
            }
            else if (xml_strequal(el, "armor")) {
                handle_armor(pi, el, attr);
            }
            else if (xml_strequal(el, "weapon")) {
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

static void start_ships(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "opensea", "fly", "nocoast", "speedy", NULL };
    if (xml_strequal(el, "ship")) {
        const XML_Char *name;

        name = attr_get(attr, "name");
        if (name) {
            ship_type *stype = st_get_or_create(name);
            int i, flags = SFL_DEFAULT;

            for (i = 0; attr[i]; i += 2) {
                if (xml_strequal(attr[i], "range")) {
                    stype->range = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "maxrange")) {
                    stype->range_max = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "cabins")) {
                    stype->cabins = PERSON_WEIGHT * xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "cargo")) {
                    stype->cargo = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "combat")) {
                    stype->combat = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "fishing")) {
                    stype->fishing = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "cptskill")) {
                    stype->cptskill = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "minskill")) {
                    stype->minskill = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "sumskill")) {
                    stype->sumskill = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "damage")) {
                    stype->damage = xml_float(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "storm")) {
                    stype->storm = xml_float(attr[i + 1]);
                }
                else if (!handle_flag(&flags, attr + i, flag_names)) {
                    /* we already handled the name earlier */
                    if (!xml_strequal(attr[i], "name")) {
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
        if (xml_strequal(el, "modifier")) {
            /* these modifiers are not like buildings */
            int i;
            const XML_Char *type = NULL, *value = NULL;
            for (i = 0; attr[i]; i += 2) {
                if (xml_strequal(attr[i], "type")) {
                    type = attr[i + 1];
                }
                else if (xml_strequal(attr[i], "value")) {
                    value = attr[i + 1];
                }
                else if (xml_strequal(attr[i], "factor")) {
                    value = attr[i + 1];
                }
                else {
                    handle_bad_input(pi, el, attr[i]);
                }
            }
            if (type) {
                if (!value) {
                    handle_bad_input(pi, el, attr[i]);
                }
                else if (xml_strequal(type, "tactics")) {
                    stype->tac_bonus = xml_float(value);
                }
                else if (xml_strequal(type, "attack")) {
                    stype->at_bonus = xml_int(value);
                }
                else if (xml_strequal(type, "defense")) {
                    stype->df_bonus = xml_int(value);
                }
                else {
                    handle_bad_input(pi, el, attr[i]);
                }
            }
        }
        else if (xml_strequal(el, "requirement")) {
            assert(stype->construction);
            handle_requirement(pi, el, attr);
        }
        else if (xml_strequal(el, "construction")) {
            assert(!stype->construction);
            stype->construction = parse_construction(pi, el, attr);
        }
        else if (xml_strequal(el, "coast")) {
            handle_coast(pi, el, attr);
        }
        else {
            handle_bad_input(pi, el, NULL);
        }
    }
}

static int nattacks;
static int nfamiliars;

static void start_races(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    race *rc = (race *)pi->object;
    const char *flag_names[] = {
        "playerrace", "killpeasants", "scarepeasants", "!cansteal",
        "moverandom", "cannotmove", "learn", "fly", "swim", "walk",
        "!learn", "!teach", "horse", "desert", "illusionary",
        "absorbpeasants", "noheal", "noweapons", "shapeshift",
        "shapeshiftany", "undead", "dragon", "coastal", "unarmedguard",
        "cansail", "invisible", "shipspeed", "moveattack", "migrants", NULL };
    const char *bflag_names[] = {
        "equipment", "noblock", "resistpierce", "resistcut", "resistbash",
        "invinciblenonmagic", "noattack", NULL };
    const char *eflag_names[] = {
        "giveperson", "giveunit", "getitem", "recruitethereal", 
        "recruitunlimited", "stonegolem", "irongolem", NULL };

    if (xml_strequal(el, "attack")) {
        int i;
        struct att * at;
        assert(rc);
        at = rc->attack + nattacks;
        at->type = AT_NONE;
        ++nattacks;
        if (nattacks >= RACE_ATTACKS) {
            log_fatal("too many attacks for race '%s'\n", rc->_name);
        }
        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strequal(key, "type")) {
                at->type = xml_int(val);
            }
            else if (xml_strequal(key, "flags")) {
                at->flags = xml_int(val);
            }
            else if (xml_strequal(key, "level")) {
                at->level = xml_int(val);
            }
            else if (xml_strequal(key, "damage")) {
                at->data.dice = str_strdup(val);
            }
            else if (xml_strequal(key, "spell")) {
                at->data.sp = spellref_create(NULL, val);
            }
            else {
                handle_bad_input(pi, el, key);
            }
        }
    }
    else if (xml_strequal(el, "familiar")) {
        race *frc = NULL;
        int i;

        assert(rc);
        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strequal(key, "race")) {
                frc = rc_get_or_create(val);
                frc->flags |= RCF_FAMILIAR;
            }
            else {
                handle_bad_input(pi, el, key);
            }
        }
        if (frc) {
            if (nfamiliars < MAXMAGIETYP) {
                rc->familiars[nfamiliars++] = frc;
            }
        }
    }
    else if (xml_strequal(el, "skill")) {
        const XML_Char *name = NULL;
        int i, speed = 0, mod = 0;

        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strequal(key, "name")) {
                name = val;
            }
            else if (xml_strequal(key, "modifier")) {
                mod = xml_int(val);
            }
            else if (xml_strequal(key, "speed")) {
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
    else if (xml_strequal(el, "param")) {
        const XML_Char *key = attr_get(attr, "name"), *val = attr_get(attr, "value");
        if (key && val) {
            rc_set_param(rc, key, val);
        }
    }
    else if (xml_strequal(el, "ai")) {
        /* AI flags are cumulative to race flags. XML format is dumb */
        int i, flags = 0;
        assert(rc);
        for (i = 0; attr[i]; i += 2) {
            const XML_Char *key = attr[i], *val = attr[i + 1];
            if (xml_strequal(key, "splitsize")) {
                rc->splitsize = xml_int(val);
            }
            else if (xml_strequal(key, "scare")) {
                rc_set_param(rc, "scare", val);
            }
            else if (!handle_flag(&flags, attr + i, flag_names)) {
                handle_bad_input(pi, el, key);
            }
        }
        rc->flags |= flags;
    }
    else if (xml_strequal(el, "race")) {
        const XML_Char *name;

        nfamiliars = 0;
        nattacks = 0;

        name = attr_get(attr, "name");
        if (name) {
            int i;

            assert(!rc);
            pi->object = rc = rc_get_or_create(name);
            while (AT_NONE != rc->attack[nattacks].type) {
                ++nattacks;
            }

            for (i = 0; attr[i]; i += 2) {
                const XML_Char *key = attr[i], *val = attr[i + 1];
                if (xml_strequal(key, "maxaura")) {
                    rc->maxaura = (int)(100 * xml_float(val));
                }
                else if (xml_strequal(key, "magres")) {
                    /* specified in percent: */
                    rc->magres = frac_make(xml_int(val), 100);
                }
                else if (xml_strequal(key, "healing")) {
                    rc->healing = (int)(xml_float(val) * 100);
                }
                else if (xml_strequal(key, "regaura")) {
                    rc->regaura = xml_float(val);
                }
                else if (xml_strequal(key, "recruitcost")) {
                    rc->recruitcost = xml_int(val);
                }
                else if (xml_strequal(key, "maintenance")) {
                    rc->maintenance = xml_int(val);
                }
                else if (xml_strequal(key, "income")) {
                    rc->income = xml_int(val);
                }
                else if (xml_strequal(key, "weight")) {
                    rc->weight = xml_int(val);
                }
                else if (xml_strequal(key, "capacity")) {
                    rc->capacity = xml_int(val);
                }
                else if (xml_strequal(key, "speed")) {
                    rc->speed = xml_float(val);
                }
                else if (xml_strequal(key, "hp")) {
                    rc->hitpoints = xml_int(val);
                }
                else if (xml_strequal(key, "ac")) {
                    rc->armor = xml_int(val);
                }
                else if (xml_strequal(key, "damage")) {
                    rc->def_damage = str_strdup(val);
                }
                else if (xml_strequal(key, "unarmedattack")) {
                    rc->at_default = xml_int(val);
                }
                else if (xml_strequal(key, "unarmeddefense")) {
                    rc->df_default = xml_int(val);
                }
                else if (xml_strequal(key, "attackmodifier")) {
                    rc->at_bonus = xml_int(val);
                }
                else if (xml_strequal(key, "defensemodifier")) {
                    rc->df_bonus = xml_int(val);
                }
                else if (xml_strequal(key, "studyspeed")) {
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
                            if (!xml_strequal(key, "name")) {
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

static void start_buildings(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "nodestroy", "nobuild", "unique", "decay", "magic", "namechange", "fort", "oneperturn", NULL };
    if (xml_strequal(el, "building")) {
        const XML_Char *name;

        assert(stage == NULL);
        name = attr_get(attr, "name");
        if (name) {
            building_type *btype = bt_get_or_create(name);
            int i, flags = BTF_DEFAULT;

            for (i = 0; attr[i]; i += 2) {
                if (xml_strequal(attr[i], "maxsize")) {
                    btype->maxsize = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "capacity")) {
                    btype->capacity = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "maxcapacity")) {
                    btype->maxcapacity = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "magresbonus")) {
                    btype->magresbonus = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "fumblebonus")) {
                    btype->fumblebonus = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "taxes")) {
                    btype->taxes = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "auraregen")) {
                    btype->auraregen = xml_int(attr[i + 1]);
                }
                else if (xml_strequal(attr[i], "magres")) {
                    /* magres is specified in percent! */
                    btype->magres = frac_make(xml_int(attr[i + 1]), 100);
                }
                else if (!handle_flag(&flags, attr + i, flag_names)) {
                    /* we already handled the name earlier */
                    if (!xml_strequal(attr[i], "name")) {
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
        if (xml_strequal(el, "modifier")) {
            handle_modifier(pi, el, attr);
        }
        else if (xml_strequal(el, "requirement")) {
            assert(stage);
            assert(stage->construction);
            handle_requirement(pi, el, attr);
        }
        else if (xml_strequal(el, "construction")) {
            assert(stage == NULL);
            stage = calloc(1, sizeof(building_stage));
            stage->construction = parse_construction(pi, el, attr);
        }
        else if (xml_strequal(el, "maintenance")) {
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
        if (!xml_strequal(el, "eressea")) {
            handle_bad_input(pi, el, NULL);
        }
    }
    else if (pi->depth == 1) {
        if (xml_strequal(el, "resources")) {
            pi->type = EXP_RESOURCES;
        }
        else if (xml_strequal(el, "buildings")) {
            pi->type = EXP_BUILDINGS;
        }
        else if (xml_strequal(el, "ships")) {
            pi->type = EXP_SHIPS;
        }
        else if (xml_strequal(el, "messages")) {
            pi->type = EXP_MESSAGES;
        }
        else if (xml_strequal(el, "spells")) {
            pi->type = EXP_SPELLS;
        }
        else if (xml_strequal(el, "spellbook")) {
            pi->type = EXP_SPELLBOOKS;
            start_spellbooks(pi, el, attr);
        }
        else if (xml_strequal(el, "races")) {
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
        case EXP_MESSAGES:
            start_messages(pi, el, attr);
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
    if (xml_strequal(el, "spells")) {
        pi->type = EXP_UNKNOWN;
    }
    else if (xml_strequal(el, "spell")) {
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

    if (xml_strequal(el, "weapon")) {
        pi->type = EXP_RESOURCES;
    }
    else if (xml_strequal(el, "modifier")) {
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
    if (xml_strequal(el, "resource")) {
        if (nrmods > 0) {
            rtype->modifiers = calloc(sizeof(resource_mod), nrmods + 1);
            memcpy(rtype->modifiers, rmods, sizeof(resource_mod) * nrmods);
            nrmods = 0;
        }
    }
    else if (xml_strequal(el, "construction")) {
        if (nreqs > 0) {
            construction *con = rtype->itype->construction;
            con->materials = calloc(sizeof(requirement), nreqs + 1);
            memcpy(con->materials, reqs, sizeof(requirement) * nreqs);
            nreqs = 0;
        }
    }
    else if (xml_strequal(el, "resources")) {
        pi->type = EXP_UNKNOWN;
    }
}

static void end_races(parseinfo *pi, const XML_Char *el) {
    race *rc = (race *)pi->object;
    if (xml_strequal(el, "race")) {
        assert(rc);
        rc->attack[nattacks].type = AT_NONE;
        nattacks = 0;
        if (nfamiliars > 0 && nfamiliars < MAXMAGIETYP) {
            int i;
            for (i = nfamiliars - 1; i != MAXMAGIETYP; ++i) {
                rc->familiars[i] = rc->familiars[nfamiliars - 1];
            }
        }
        nfamiliars = 0;
        pi->object = NULL;
    }
    else if (xml_strequal(el, "races")) {
        pi->type = EXP_UNKNOWN;
    }
}

static void end_ships(parseinfo *pi, const XML_Char *el) {
    ship_type *stype = (ship_type *)pi->object;
    if (xml_strequal(el, "construction")) {
        assert(stype);
        assert(stype->construction);
        if (nreqs > 0) {
            construction *con = stype->construction;
            con->materials = calloc(sizeof(requirement), nreqs + 1);
            memcpy(con->materials, reqs, sizeof(requirement) * nreqs);
            nreqs = 0;
        }
    }
    else if (xml_strequal(el, "ship")) {
        if (ncoasts > 0) {
            stype->coasts = calloc(sizeof(const terrain_type *), ncoasts + 1);
            memcpy(stype->coasts, coasts, sizeof(const terrain_type *) * ncoasts);
            ncoasts = 0;
        }
        pi->object = NULL;
    }
    else if (xml_strequal(el, "ships")) {
        pi->type = EXP_UNKNOWN;
    }
}

static void end_buildings(parseinfo *pi, const XML_Char *el) {
    /* stores the end of the building's stage list: */
    static building_stage **stage_ptr;

    building_type *btype = (building_type *)pi->object;
    if (xml_strequal(el, "construction")) {
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
    else if (xml_strequal(el, "building")) {
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
    else if (xml_strequal(el, "buildings")) {
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
    case EXP_MESSAGES:
        end_messages(pi, el);
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
