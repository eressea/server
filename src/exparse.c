#ifdef _MSC_VER
#include <platform.h>
#endif
#include "exparse.h"

#include "kernel/build.h"
#include "kernel/item.h"
#include "kernel/resources.h"

#include "util/log.h"

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

static const char * attr_get(const XML_Char **attr, const char *key) {
    int i;
    for (i = 0; attr[i]; i += 2) {
        if (xml_strcmp(attr[i], key) == 0) {
            return (const char *)attr[i + 1];
        }
    }
    return NULL;
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

static bool attr_bool(XML_Char **pair, const char *key) {
    if (xml_strcmp(pair[0], key) == 0) {
        return xml_bool(pair[1]);
    }
    return false;
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
            /* TODO: appearance should be a property of item, not resource */
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
        if (xml_strcmp(attr[i], "weight") == 0) {
            itype->weight = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "capacity") == 0) {
            itype->capacity = xml_int(attr[i + 1]);
        }
        else if (xml_strcmp(attr[i], "score") == 0) {
            itype->score = xml_int(attr[i + 1]);
        }
        else if (!handle_flag(&flags, attr + i, flag_names)) {
            handle_bad_input(ud, el, attr[i]);
        }
    }
    itype->flags = flags;
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
            }
            else if (xml_strcmp(el, "luxury") == 0) {
                /* TODO */
            }
            else if (xml_strcmp(el, "potion") == 0) {
                /* TODO */
            }
            else if (xml_strcmp(el, "armor") == 0) {
                /* TODO */
                rtype->atype = new_armortype(itype, 0.0, frac_zero, 0, 0);
            }
            else if (xml_strcmp(el, "weapon") == 0) {
                rtype->wtype = new_weapontype(itype, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE);
                /* TODO */
            }
            else if (xml_strcmp(el, "damage") == 0) {
                assert(rtype->wtype);
                /* TODO */
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
    int err = 1;
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
