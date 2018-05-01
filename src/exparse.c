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
    EXP_MESSAGES,
    EXP_STRINGS,
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

static building_stage *stage;

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
        if (xml_strcmp(pair[0], names[i]) == 0) {
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

static void XMLCALL start_weapon(parseinfo *pi, const XML_Char *el, const XML_Char **attr) {
    resource_type *rtype = (resource_type *)pi->object;

    assert(rtype && rtype->wtype);
    if (xml_strcmp(el, "function") == 0) {
        ++pi->errors;
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
            ++pi->errors;
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
        else if (rtype->itype) {
            item_type *itype = rtype->itype;
            if (xml_strcmp(el, "construction") == 0) {
                itype->construction = parse_construction(pi, el, attr);
            }
            else if (xml_strcmp(el, "modifier") == 0) {
                handle_modifier(pi, el, attr);
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
            else if (rtype->wtype) {
                weapon_type *wtype = rtype->wtype;
                if (xml_strcmp(el, "damage") == 0) {
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

const XML_Char *attr_get(const XML_Char **attr, const char *key) {
    int i;
    for (i = 0; attr[i]; i += 2) {
        if (xml_strcmp(attr[i], key) == 0) {
            return attr[i + 1];
        }
    }
    return NULL;
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
        else {
            handle_bad_input(pi, el, NULL);
        }
    }
    else {
        switch (pi->type) {
        case EXP_BUILDINGS:
            start_buildings(pi, el, attr);
            break;
        case EXP_RESOURCES:
            start_resources(pi, el, attr);
            break;
        case EXP_WEAPON:
            start_weapon(pi, el, attr);
            break;
        default:
            /* not implemented */
            handle_bad_input(pi, el, NULL);
        }
    }
    ++pi->depth;
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
    else {
        handle_bad_input(pi, el, NULL);
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


static void end_buildings(parseinfo *pi, const XML_Char *el) {
    /* stores the end of the building's stage list: */
    static building_stage **stage_ptr;

    building_type *btype = (building_type *)pi->object;
    if (xml_strcmp(el, "construction") == 0) {
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
        if (nrmods > 0) {
            btype->modifiers = calloc(sizeof(resource_mod), nrmods + 1);
            memcpy(btype->modifiers, rmods, sizeof(resource_mod) * nrmods);
            nrmods = 0;
        }
    }
    else if (xml_strcmp(el, "buildings") == 0) {
        pi->type = EXP_UNKNOWN;
    }
}

static void XMLCALL handle_end(void *data, const XML_Char *el) {
    parseinfo *pi = (parseinfo *)data;

    switch (pi->type) {
    case EXP_BUILDINGS:
        end_buildings(pi, el);
        break;
    case EXP_RESOURCES:
        end_resources(pi, el);
        break;
    case EXP_WEAPON:
        end_weapon(pi, el);
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
        size_t len = (int) fread(buf, 1, sizeof(buf), F);
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
    assert(pi.depth == 0);
    XML_ParserFree(xp);
    fclose(F);
    if (err != 0) {
        return err;
    }
    return pi.errors;
}
