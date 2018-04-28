#ifdef _MSC_VER
#include <platform.h>
#endif
#include "exparse.h"

#include "kernel/build.h"
#include "kernel/item.h"
#include "kernel/race.h"
#include "kernel/resources.h"

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
    EXP_BUILDINGS,
    EXP_SHIPS,
    EXP_MESSAGES,
    EXP_STRINGS,
};

typedef struct userdata {
    int type;
    int depth;
    int errors;
    XML_Char *cdata;
    size_t clength;
    void *object;
} userdata;

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
    return atoi((const char *)val);
}

static double xml_float(const XML_Char *val) {
    return atof((const char *)val);
}

static variant xml_fraction(const XML_Char *val) {
    int num, den = 100;
    double fval = atof((const char *)val);
    num = (int)(fval * den + 0.5);
    return frac_make(num, den);
}

static void handle_bad_input(userdata *ud, const XML_Char *el, const XML_Char *attr) {
    if (attr) {
        log_error("unknown attribute in <%s>: %s", (const char *)el, (const char *)attr);
    }
    else {
        log_error("unexpected element <%s>", (const char *)el);
    }
    ++ud->errors;
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

static void handle_resource(userdata *ud, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "item", "limited", "pooled", NULL };
    int i;
    const char *name = NULL, *appear = NULL;
    int flags = RTF_POOLED;
    bool material = false;
    (void)el;
    for (i = 0; attr[i]; i += 2) {
        if (xml_strcmp(attr[i], "name") == 0) {
            name = (const char *)attr[i + 1];
        }
        else if (xml_strcmp(attr[i], "appearance") == 0) {
            /* TODO: appearance should be a property of item, not resource */
            appear = (const char *)attr[i + 1];
            flags |= RTF_ITEM;
        }
        else if (xml_strcmp(attr[i], "material") == 0) {
            material = xml_bool(attr[i + 1]);
        }
        else if (!handle_flag(&flags, attr + i, flag_names)) {
            handle_bad_input(ud, el, attr[i]);
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
        ud->object = rtype;
    }
}

static void handle_item(userdata *ud, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "herb", "cursed", "notlost", "big", "animal", "vehicle", "use", NULL };
    int i, flags = ITF_NONE;
    resource_type *rtype = (resource_type *)ud->object;
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
            itype->mask_allow = rc_mask(buffer);
        }
        else if (xml_strcmp(attr[i], "deny") == 0) {
            size_t len = strlen(attr[i + 1]);
            assert(len < sizeof(buffer));
            memcpy(buffer, attr[i + 1], len + 1);
            itype->mask_deny = rc_mask(buffer);
        }
        else if (!handle_flag(&flags, attr + i, flag_names)) {
            handle_bad_input(ud, el, attr[i]);
        }
    }
    itype->flags = flags;
}

static void handle_armor(userdata *ud, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "shield", "laen", NULL };
    resource_type *rtype = (resource_type *)ud->object;
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
            handle_bad_input(ud, el, attr[i]);
        }
    }
    atype->flags = flags;
}

static void handle_weapon(userdata *ud, const XML_Char *el, const XML_Char **attr) {
    const char *flag_names[] = { "missile", "magical", "pierce", "cut", "bash", "siege", "armorpiercing", "horse", "useshield", NULL };
    resource_type *rtype = (resource_type *)ud->object;
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
            handle_bad_input(ud, el, attr[i]);
        }
    }
    wtype->flags = flags;
}

static void XMLCALL handle_resources(userdata *ud, const XML_Char *el, const XML_Char **attr) {
    resource_type *rtype = (resource_type *)ud->object;
    if (xml_strcmp(el, "resource") == 0) {
        handle_resource(ud, el, attr);
    }
    else if (rtype) {
        if (xml_strcmp(el, "item") == 0) {
            assert(rtype);
            handle_item(ud, el, attr);
        }
        else if (xml_strcmp(el, "function") == 0) {
            assert(rtype);
            ++ud->errors;
            /* TODO */
        }
        else if (rtype->itype) {
            item_type *itype = rtype->itype;
            if (xml_strcmp(el, "construction") == 0) {
                construction *con = calloc(sizeof(construction), 1);
                int i;
                for (i = 0; attr[i]; i += 2) {
                    if (xml_strcmp(attr[i], "skill") == 0) {
                        con->skill = findskill((const char *)attr[i + 1]);
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
                    else {
                        handle_bad_input(ud, el, attr[i]);
                    }
                }
                itype->construction = con;
            }
            else if (xml_strcmp(el, "requirement") == 0) {
                assert(itype->construction);
                /* TODO */
                ++ud->errors;
            }
            else if (xml_strcmp(el, "luxury") == 0) {
                rtype->ltype = new_luxurytype(itype, 0);
            }
            else if (xml_strcmp(el, "potion") == 0) {
                /* TODO */
                ++ud->errors;
            }
            else if (xml_strcmp(el, "armor") == 0) {
                handle_armor(ud, el, attr);
            }
            else if (xml_strcmp(el, "weapon") == 0) {
                handle_weapon(ud, el, attr);
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
                            handle_bad_input(ud, el, NULL);
                        }
                    }
                }
                else {
                    handle_bad_input(ud, el, NULL);
                }
            }
            else {
                handle_bad_input(ud, el, NULL);
            }
        }
        else {
            handle_bad_input(ud, el, NULL);
        }
    }
    else {
        handle_bad_input(ud, el, NULL);
    }
}

static void XMLCALL handle_start(void *data, const XML_Char *el, const XML_Char **attr) {
    userdata *ud = (userdata *)data;
    if (ud->depth == 0) {
        ud->type = EXP_UNKNOWN;
        if (xml_strcmp(el, "eressea") != 0) {
            handle_bad_input(ud, el, NULL);
        }
    }
    else if (ud->depth == 1) {
        if (xml_strcmp(el, "resources") == 0) {
            ud->type = EXP_RESOURCES;
        }
        else if (xml_strcmp(el, "buildings") == 0) {
            ud->type = EXP_BUILDINGS;
        }
        else if (xml_strcmp(el, "ships") == 0) {
            ud->type = EXP_SHIPS;
        }
        else if (xml_strcmp(el, "messages") == 0) {
            ud->type = EXP_MESSAGES;
        }
        else if (xml_strcmp(el, "strings") == 0) {
            ud->type = EXP_STRINGS;
        }
        else {
            handle_bad_input(ud, el, NULL);
        }
    }
    else {
        switch (ud->type) {
        case EXP_RESOURCES:
            handle_resources(ud, el, attr);
            break;
        default:
            /* not implemented */
            handle_bad_input(ud, el, NULL);
        }
    }
    ++ud->depth;
}

static void XMLCALL handle_end(void *data, const XML_Char *el) {
    userdata *ud = (userdata *)data;
    --ud->depth;
    if (ud->cdata) {
        free(ud->cdata);
        ud->cdata = NULL;
        ud->clength = 0;
    }
    if (ud->depth == 0) {
        ud->type = EXP_UNKNOWN;
    }
}

static void XMLCALL handle_data(void *data, const XML_Char *xs, int len) {
    userdata *ud = (userdata *)data;
    if (len > 0) {
        if (ud->type == EXP_MESSAGES && ud->depth == 4) {
            size_t bytes = (size_t)len;
            ud->cdata = realloc(ud->cdata, ud->clength + bytes);
            memcpy(ud->cdata + ud->clength, xs, bytes);
            ud->clength = ud->clength + bytes;
        }
    }
}


int exparse_readfile(const char * filename) {
    XML_Parser xp;
    FILE *F;
    int err = 0;
    char buf[4096];
    userdata ud;

    F = fopen(filename, "r");
    if (!F) {
        return 2;
    }
    xp = XML_ParserCreate("UTF-8");
    XML_SetElementHandler(xp, handle_start, handle_end);
    XML_SetCharacterDataHandler(xp, handle_data);
    XML_SetUserData(xp, &ud);
    memset(&ud, 0, sizeof(ud));
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
    assert(ud.depth == 0);
    XML_ParserFree(xp);
    fclose(F);
    if (err != 0) {
        return err;
    }
    return ud.errors;
}
