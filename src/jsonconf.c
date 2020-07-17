#include <platform.h>
#include "jsonconf.h"

/* kernel includes */
#include "kernel/building.h"
#include "kernel/calendar.h"
#include "kernel/config.h"
#include "kernel/equipment.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/resources.h"
#include "kernel/ship.h"
#include "kernel/terrain.h"
#include "kernel/spell.h"
#include "kernel/spellbook.h"

/* util includes */
#include "kernel/attrib.h"
#include "util/aliases.h"
#include "util/crmessage.h"
#include "util/functions.h"
#include "util/keyword.h"
#include "util/language.h"
#include "util/log.h"
#include "util/message.h"
#include "util/nrmessage.h"
#include "util/path.h"
#include "util/pofile.h"
#include "util/strings.h"

/* game modules */
#include "direction.h"
#include "move.h"
#include "prefix.h"
#include "skill.h"
#include "exparse.h"

/* external libraries */
#include <cJSON.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static int json_flags(cJSON *json, const char *flags[]) {
    cJSON *entry;
    int result = 0;
    assert(json->type == cJSON_Array);
    for (entry = json->child; entry; entry = entry->next) {
        if (entry->type == cJSON_String) {
            int i;
            for (i = 0; flags[i]; ++i) {
                if (strcmp(flags[i], entry->valuestring) == 0) {
                    result |= (1 << i);
                    break;
                }
            }
        }
    }
    return result;
}

static void json_requirements(cJSON *json, requirement **matp) {
    cJSON *child;
    int i;
    requirement *mat = calloc(1 + cJSON_GetArraySize(json), sizeof(requirement));
    if (!mat) abort();
    for (i = 0, child = json->child; child; child = child->next, ++i) {
        mat[i].number = child->valueint;
        mat[i].rtype = rt_get_or_create(child->string);
    }
    *matp = mat;
}

static void json_maintenance_i(cJSON *json, maintenance *mt) {
    cJSON *child;
    for (child = json->child; child; child = child->next) {
        switch (child->type) {
        case cJSON_Number:
            if (strcmp(child->string, "amount") == 0) {
                mt->number = child->valueint;
            }
            else {
                log_error("maintenance contains unknown attribute %s", child->string);
            }
            break;
        case cJSON_String:
            if (strcmp(child->string, "type") == 0) {
                mt->rtype = rt_get_or_create(child->valuestring);
            }
            else {
                log_error("maintenance contains unknown attribute %s", child->string);
            }
            break;
        case cJSON_Array:
            if (strcmp(child->string, "flags") == 0) {
                const char * flags[] = { "variable", 0 };
                mt->flags = json_flags(child, flags);
            }
            else {
                log_error("maintenance contains unknown array %s", child->string);
            }
            break;
        default:
            log_error("maintenance contains unknown attribute %s of type %d", child->string, child->type);
        }
    }
}

static void json_maintenance(cJSON *json, maintenance **mtp) {
    cJSON *child;
    maintenance *mt;
    int size = 1;

    if (json->type == cJSON_Array) {
        size = cJSON_GetArraySize(json);
    }
    else if (json->type != cJSON_Object) {
        log_error("maintenance is not a json object or array (%d)", json->type);
        return;
    }
    *mtp = mt = (struct maintenance *) calloc(size + 1, sizeof(struct maintenance));
    if (json->type == cJSON_Array) {
        int i;
        for (i = 0, child = json->child; child; child = child->next, ++i) {
            if (child->type == cJSON_Object) {
                json_maintenance_i(child, mt + i);
            }
        }
    }
    else {
        json_maintenance_i(json, mt);
    }
}

static void json_construction(cJSON *json, construction **consp) {
    cJSON *child;
    construction * cons;
    
    if (json->type != cJSON_Object) {
        log_error("construction %s is not a json object: %d", json->string, json->type);
        return;
    }
    cons = (construction *)calloc(1, sizeof(construction));
    if (!cons) abort();
    for (child = json->child; child; child = child->next) {
        switch (child->type) {
        case cJSON_Object:
            if (strcmp(child->string, "materials") == 0) {
                json_requirements(child, &cons->materials);
            }
            break;
        case cJSON_Number:
            if (strcmp(child->string, "maxsize") == 0) {
                cons->maxsize = child->valueint;
            }
            else if (strcmp(child->string, "reqsize") == 0) {
                cons->reqsize = child->valueint;
            }
            else if (strcmp(child->string, "minskill") == 0) {
                cons->minskill = child->valueint;
            }
            break;
        default:
            log_error("construction %s contains unknown attribute %s", json->string, child->string);
        }
    }
    *consp = cons;
}

static void json_terrain_production(cJSON *json, terrain_production *prod) {
    cJSON *child;
    assert(json->type == cJSON_Object);
    for (child = json->child; child; child = child->next) {
        char **dst = 0;
        switch (child->type) {
        case cJSON_Number:
            if (strcmp(child->string, "chance") == 0) {
                prod->chance = (float)child->valuedouble;
            }
            else {
                log_error("terrain_production %s contains unknown number %s", json->string, child->string);
            }
            break;
        case cJSON_String:
            if (strcmp(child->string, "base") == 0) {
                dst = &prod->base;
            }
            else if (strcmp(child->string, "level") == 0) {
                dst = &prod->startlevel;
            }
            else if (strcmp(child->string, "div") == 0) {
                dst = &prod->divisor;
            }
            else {
                log_error("terrain_production %s contains unknown string %s", json->string, child->string);
            }
            break;
        default:
            log_error("terrain_production %s contains unknown attribute %s", json->string, child->string);
        }
        if (dst) {
            free(*dst);
            assert(child->type == cJSON_String);
            *dst = str_strdup(child->valuestring);
        }
    }
}

static void json_terrain(cJSON *json, terrain_type *ter) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("terrain %s is not a json object: %d", json->string, json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        switch (child->type) {
        case cJSON_Object:
            if (strcmp(child->string, "production") == 0) {
                cJSON *entry;
                int size = cJSON_GetArraySize(child);
                if (size > 0) {
                    int n;
                    ter->production = (terrain_production *)calloc(size + 1, sizeof(terrain_production));
                    if (!ter->production) abort();
                    ter->production[size].type = 0;
                    for (n = 0, entry = child->child; entry; entry = entry->next, ++n) {
                        ter->production[n].type = rt_get_or_create(entry->string);
                        if (entry->type != cJSON_Object) {
                            log_error("terrain %s contains invalid production %s", json->string, entry->string);
                        }
                        else {
                            json_terrain_production(entry, ter->production + n);
                        }
                    }
                }
            }
            else {
                log_error("terrain %s contains unknown attribute %s", json->string, child->string);
            }
            break;
        case cJSON_Array:
            if (strcmp(child->string, "flags") == 0) {
                const char * flags[] = {
                    "land", "sea", "forest", "arctic", "cavalry", "forbidden", "sail", "fly", "swim", "walk", 0
                };
                ter->flags = json_flags(child, flags);
            }
            else if (strcmp(child->string, "herbs") == 0) {
                cJSON *entry;
                int size = cJSON_GetArraySize(child);
                if (size > 0) {
                    int n;
                    free(ter->herbs);
                    ter->herbs = malloc(sizeof(const item_type *) * (size + 1));
                    if (!ter->herbs) abort();
                    ter->herbs[size] = 0;
                    for (n = 0, entry = child->child; entry; entry = entry->next) {
                        ter->herbs[n++] = it_get_or_create(rt_get_or_create(entry->valuestring));
                    }
                }
            }
            else {
                log_error("terrain %s contains unknown attribute %s", json->string, child->string);
            }
            break;
        case cJSON_Number:
            if (strcmp(child->string, "size") == 0) {
                ter->size = child->valueint;
            }
            else if (strcmp(child->string, "road") == 0) {
                ter->max_road = (short)child->valueint;
            }
            else if (strcmp(child->string, "seed") == 0) {
                ter->distribution = (short)child->valueint;
            }
            else {
                log_error("terrain %s contains unknown attribute %s", json->string, child->string);
            }
            break;
        default:
            log_error("terrain %s contains unknown attribute %s", json->string, child->string);
        }
    }
}

static void json_stage(cJSON *json, building_stage *stage) {
    cJSON *child;

    if (json->type != cJSON_Object) {
        log_error("building stages is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        switch (child->type) {
        case cJSON_Object:
            if (strcmp(child->string, "construction") == 0) {
                json_construction(child, &stage->construction);
            }
            break;
        case cJSON_String:
            if (strcmp(child->string, "name") == 0) {
                stage->name = str_strdup(child->valuestring);
            }
            break;
        }
    }
}

static void json_stages(cJSON *json, building_type *bt) {
    cJSON *child;
    building_stage *stage, **sp = &bt->stages;
    int size = 0;

    if (json->type != cJSON_Array) {
        log_error("building stages is not a json array: %d", json->type);
        return;
    }

    for (child = json->child; child; child = child->next) {
        switch (child->type) {
        case cJSON_Object:
            stage = calloc(1, sizeof(building_stage));
            if (!stage) abort();
            json_stage(child, stage);
            if (stage->construction->maxsize > 0) {
                stage->construction->maxsize -= size;
                size += stage->construction->maxsize;
            }
            *sp = stage;
            sp = &stage->next;
            break;
        default:
            log_error("building stage contains non-object type %d", child->type);
        }
    }
}

static void json_building(cJSON *json, building_type *bt) {
    cJSON *child;
    const char *flags[] = {
        "nodestroy", "nobuild", "unique", "decay", "dynamic", "magic", "oneperturn", "namechange", "fort", 0
    };
    if (json->type != cJSON_Object) {
        log_error("building %s is not a json object: %d", json->string, json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        switch (child->type) {
        case cJSON_Array:
            if (strcmp(child->string, "stages") == 0) {
                if (!bt->stages) {
                    json_stages(child, bt);
                }
            }
            else if (strcmp(child->string, "maintenance") == 0) {
                json_maintenance(child, &bt->maintenance);
            }
            else if (strcmp(child->string, "flags") == 0) {
                json_flags(child, flags);
            }
            break;
        case cJSON_Object:
            if (strcmp(child->string, "construction") == 0) {
                /* simple, single-stage building */
                if (!bt->stages) {
                    building_stage *stage = calloc(1, sizeof(building_stage));
                    if (!stage) abort();
                    json_construction(child, &stage->construction);
                    bt->stages = stage;
                }
            }
            if (strcmp(child->string, "maintenance") == 0) {
                json_maintenance(child, &bt->maintenance);
            }
            break;
        case cJSON_String:
            log_error("building %s contains unknown attribute %s", json->string, child->string);
            break;
        default:
            log_error("building %s contains unknown attribute %s", json->string, child->string);
        }
    }
}

static void json_item(cJSON *json, item_type *itype) {
    cJSON *child;
    const char *flags[] = {
        "herb", "cursed", "nodrop", "big", "animal", "vehicle", 0
    };
    if (json->type != cJSON_Object) {
        log_error("ship %s is not a json object: %d", json->string, json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        switch (child->type) {
        case cJSON_Number:
            if (strcmp(child->string, "weight") == 0) {
                itype->weight = child->valueint;
                break;
            }
            if (strcmp(child->string, "capacity") == 0) {
                itype->capacity = child->valueint;
                break;
            }
            log_error("item %s contains unknown attribute %s", json->string, child->string);
            break;
        case cJSON_Array:
            if (strcmp(child->string, "flags") == 0) {
                itype->flags = json_flags(child, flags);
                break;
            }
            log_error("item %s contains unknown attribute %s", json->string, child->string);
        case cJSON_Object:
        default:
            log_error("item %s contains unknown attribute %s", json->string, child->string);
        }
    }
}

static void json_ship(cJSON *json, ship_type *st) {
    cJSON *child, *iter;
    if (json->type != cJSON_Object) {
        log_error("ship %s is not a json object: %d", json->string, json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        int i;
        switch (child->type) {
        case cJSON_Object:
            if (strcmp(child->string, "construction") == 0) {
                json_construction(child, &st->construction);
            }
            else {
                log_error("ship %s contains unknown attribute %s", json->string, child->string);
            }
            break;
        case cJSON_Array:
            st->coasts = (terrain_type **)
                malloc(sizeof(terrain_type *) * (1 + cJSON_GetArraySize(child)));
            if (!st->coasts) abort();
            for (i = 0, iter = child->child; iter; iter = iter->next) {
                if (iter->type == cJSON_String) {
                    terrain_type *ter = get_or_create_terrain(iter->valuestring);
                    if (ter) {
                        st->coasts[i++] = ter;
                    }
                }
            }
            st->coasts[i] = 0;
            break;
        case cJSON_Number:
            if (strcmp(child->string, "range") == 0) {
                st->range = child->valueint;
            }
            else if (strcmp(child->string, "maxrange") == 0) {
                st->range_max = child->valueint;
            }
            else {
                log_error("ship %s contains unknown attribute %s", json->string, child->string);
            }
            break;
        default:
            log_error("ship %s contains unknown attribute %s", json->string, child->string);
        }
    }
}

static void json_race(cJSON *json, race *rc) {
    cJSON *child;
    const char *flags[] = {
        "player", "killpeasants", "scarepeasants",
        "nosteal", "moverandom", "cannotmove",
        "learn", "fly", "swim", "walk", "nolearn",
        "noteach", "horse", "desert",
        "illusionary", "absorbpeasants", "noheal",
        "noweapons", "shapeshift", "", "undead", "dragon",
        "coastal", "", "cansail", NULL
    };
    const char *ecflags[] = {
        "giveperson", "giveunit", "getitem", NULL
    };
    if (json->type != cJSON_Object) {
        log_error("race %s is not a json object: %d", json->string, json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        switch (child->type) {
        case cJSON_String:
            if (strcmp(child->string, "damage") == 0) {
                rc->def_damage = str_strdup(child->valuestring);
            }
            break;
        case cJSON_Number:
            if (strcmp(child->string, "magres") == 0) {
                rc->magres = frac_make(child->valueint, 100);
            }
            else if (strcmp(child->string, "maxaura") == 0) {
                rc->maxaura = child->valueint;
            }
            else if (strcmp(child->string, "regaura") == 0) {
                rc->regaura = (float)child->valuedouble;
            }
            else if (strcmp(child->string, "speed") == 0) {
                rc->speed = (float)child->valuedouble;
            }
            else if (strcmp(child->string, "recruitcost") == 0) {
                rc->recruitcost = child->valueint;
            }
            else if (strcmp(child->string, "maintenance") == 0) {
                rc->maintenance = child->valueint;
            }
            else if (strcmp(child->string, "weight") == 0) {
                rc->weight = child->valueint;
            }
            else if (strcmp(child->string, "capacity") == 0) {
                rc->capacity = child->valueint;
            }
            else if (strcmp(child->string, "income") == 0) {
                rc->income = child->valueint;
            }
            else if (strcmp(child->string, "hp") == 0) {
                rc->hitpoints = child->valueint;
            }
            else if (strcmp(child->string, "ac") == 0) {
                rc->armor = child->valueint;
            }
            /* TODO: studyspeed (orcs only) */
            break;
        case cJSON_Array:
            if (strcmp(child->string, "flags") == 0) {
                rc->flags = json_flags(child, flags);
                rc->ec_flags = json_flags(child, ecflags);
            }
            break;
        }
    }
}

static void json_prefixes(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Array) {
        log_error("prefixes is not a json array: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        add_raceprefix(child->valuestring);
    }
}

/** disable a feature.
 * features are identified by one of:
 * 1. the keyword for their orders,
 * 2. the name of the skill they use,
 * 3. a "module.enabled" flag in the settings
 */
static void disable_feature(const char *str) {
    char name[32];
    int k;
    skill_t sk;
    size_t len;

    sk = findskill(str);
    if (sk != NOSKILL) {
        enable_skill(sk, false);
        return;
    }
    k = findkeyword(str);
    if (k!=NOKEYWORD) {
        log_debug("disable keyword %s\n", str);
        enable_keyword(k, false);
        return;
    }
    len = strlen(str);
    assert(len <= sizeof(name) - 9);
    memcpy(name, str, len);
    strcpy(name+len, ".enabled");
    log_info("disable feature %s\n", name);
    config_set(name, "0");
}

static void json_disable_features(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Array) {
        log_error("disabled is not a json array: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        disable_feature(child->valuestring);
    }
}

static void json_terrains(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("terrains is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        json_terrain(child, get_or_create_terrain(child->string));
    }
}

static void json_buildings(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("buildings is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        json_building(child, bt_get_or_create(child->string));
    }
}

static void json_spells(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("spells is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        if (child->type == cJSON_Object) {
            spell *sp;
            cJSON * item;
            sp = create_spell(child->string);
            for (item = child->child; item; item = item->next) {
                if (strcmp(item->string, "index") == 0) {
                    continue;
                }
                else if (strcmp(item->string, "syntax") == 0) {
                    sp->syntax = str_strdup(item->valuestring);
                }
            }
        }
    }
}

static void json_items(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("items is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        resource_type *rtype = rt_get_or_create(child->string);
        item_type *itype = rtype->itype;
        if (!itype) {
            rtype->itype = itype = it_get_or_create(rtype);
        }
        json_item(child, itype);
    }
}

static void json_ships(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("ships is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        json_ship(child, st_get_or_create(child->string));
    }
}

static void json_locale(cJSON *json, struct locale *lang) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("strings is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        if (child->type == cJSON_String) {
            locale_setstring(lang, child->string, child->valuestring);
        }
    }
}

static void json_strings(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("strings is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        if (child->type == cJSON_Object) {
            struct locale *lang = get_locale(child->string);
            json_locale(child, lang);
        }
        else {
            log_error("strings for locale `%s` are not a json object: %d", child->string, child->type);
        }
    }
}

static void json_add_aliases(cJSON *json, const struct locale *lang) {
    cJSON *child;
    str_aliases *aliases = get_aliases(lang);
    for (child = json->child; child; child = child->next) {
        if (child->type == cJSON_String) {
            alias_add(aliases, child->string, child->valuestring);
        }
        else if (child->type == cJSON_Array) {
            cJSON *entry;
            for (entry = child->child; entry; entry = entry->next) {
                if (entry->type == cJSON_String) {
                    alias_add(aliases, child->string, entry->valuestring);
                }
            }
        }
    }
}

static void json_aliases(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("aliases is not a json array: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        if (child->type == cJSON_Object) {
            struct locale *lang = get_locale(child->string);
            json_add_aliases(child, lang);
        }
        else {
            log_error("strings for locale `%s` are not a json object: %d", child->string, child->type);
        }
    }
}

static void json_direction(cJSON *json, struct locale *lang) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("directions for locale `%s` not a json object: %d", locale_name(lang), json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        direction_t dir = finddirection(child->string);
        if (dir != NODIRECTION) {
            if (child->type == cJSON_String) {
                init_direction(lang, dir, child->valuestring);
            }
            else if (child->type == cJSON_Array) {
                cJSON *entry;
                for (entry = child->child; entry; entry = entry->next) {
                    init_direction(lang, dir, entry->valuestring);
                }
            }
            else {
                log_error("invalid type %d for direction `%s`", child->type, child->string);
            }
        }
    }
}

static void json_calendar(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("calendar is not an object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        if (strcmp(child->string, "start") == 0) {
            config_set_int("game.start", child->valueint);
        }
        else if (strcmp(child->string, "weeks") == 0) {
            cJSON *entry;
            int i;
            if (child->type != cJSON_Array) {
                log_error("calendar.weeks is not an array: %d", json->type);
                return;
            }
            weeks_per_month = cJSON_GetArraySize(child);
            free(weeknames);
            weeknames = malloc(sizeof(char *) * weeks_per_month);
            if (!weeknames) abort();
            for (i = 0, entry = child->child; entry; entry = entry->next, ++i) {
                if (entry->type == cJSON_String) {
                    weeknames[i] = str_strdup(entry->valuestring);
                }
                else {
                    log_error("calendar.weeks[%d] is not a string: %d", i, json->type);
                    free(weeknames);
                    weeknames = NULL;
                    return;
                }
            }
            assert(i == weeks_per_month);
            free(weeknames2);
            weeknames2 = malloc(sizeof(char *) * weeks_per_month);
            if (!weeknames2) abort();
            for (i = 0; i != weeks_per_month; ++i) {
                weeknames2[i] = malloc(strlen(weeknames[i]) + 3);
                sprintf(weeknames2[i], "%s_d", weeknames[i]);
            }
        }
        else if (strcmp(child->string, "months") == 0) {
            cJSON *jmonth;
            int i;
            if (child->type != cJSON_Array) {
                log_error("calendar.seasons is not an array: %d", json->type);
                return;
            }
            free(month_season);
            month_season = NULL;
            free(storms);
            months_per_year = cJSON_GetArraySize(child);
            storms = malloc(sizeof(int) * months_per_year);
            if (!storms) abort();
            month_season = malloc(sizeof(int) * months_per_year);
            if (!month_season) abort();
            for (i = 0, jmonth = child->child; jmonth; jmonth = jmonth->next, ++i) {
                if (jmonth->type == cJSON_Object) {
                    storms[i] = cJSON_GetObjectItem(jmonth, "storm")->valueint;
                    month_season[i] = (season_t) cJSON_GetObjectItem(jmonth, "season")->valueint;
                }
                else {
                    log_error("calendar.months[%d] is not an object: %d", i, json->type);
                    free(storms);
                    storms = NULL;
                    return;
                }
            }
        }
    }
}

static void json_directions(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("directions is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        struct locale * lang = get_locale(child->string);
        json_direction(child, lang);
    }
}

static void json_skill(cJSON *json, struct locale *lang) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("skill for locale `%s` not a json object: %d", locale_name(lang), json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        skill_t sk = findskill(child->string);
        if (sk != NOSKILL) {
            if (child->type == cJSON_String) {
                init_skill(lang, sk, child->valuestring);
                locale_setstring(lang, mkname("skill", skillnames[sk]), child->valuestring);
            }
            else if (child->type == cJSON_Array) {
                cJSON *entry;
                for (entry = child->child; entry; entry = entry->next) {
                    init_skill(lang, sk, entry->valuestring);
                    if (entry == child->child) {
                        locale_setstring(lang, mkname("skill", skillnames[sk]), entry->valuestring);
                    }
                }
            }
            else {
                log_error("invalid type %d for skill `%s`", child->type, child->string);
            }
        }
        else {
            log_error("unknown skill `%s` for locale `%s`", child->string, locale_name(lang));
        }
    }
}

static void json_keyword(cJSON *json, struct locale *lang) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("keywords for locale `%s` not a json object: %d", locale_name(lang), json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        keyword_t kwd = findkeyword(child->string);
        if (kwd != NOKEYWORD && keywords[kwd]) {
            if (child->type == cJSON_String) {
                init_keyword(lang, kwd, child->valuestring);
                locale_setstring(lang, mkname("keyword", keywords[kwd]), child->valuestring);
            }
            else if (child->type == cJSON_Array) {
                cJSON *entry;
                for (entry = child->child; entry; entry = entry->next) {
                    init_keyword(lang, kwd, entry->valuestring);
                    if (entry == child->child) {
                        locale_setstring(lang, mkname("keyword", keywords[kwd]), entry->valuestring);
                    }
                }
            }
            else {
                log_error("invalid type %d for keyword `%s`", child->type, child->string);
            }
        }
        else {
            log_error("unknown keyword `%s` for locale `%s`", child->string, locale_name(lang));
        }
    }
}

static void json_skills(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("skills is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        struct locale * lang = get_locale(child->string);
        json_skill(child, lang);
    }
}

static void json_keywords(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("keywords is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        struct locale * lang = get_locale(child->string);
        json_keyword(child, lang);
    }
}

static void json_settings(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("settings is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        if (config_get(child->string) == NULL) {
            if (child->valuestring) {
                config_set(child->string, child->valuestring);
            }
            else {
                char value[32];
                if (child->type == cJSON_Number && child->valuedouble && child->valueint < child->valuedouble) {
                    sprintf(value, "%f", child->valuedouble);
                }
                else {
                    sprintf(value, "%d", child->valueint);
                }
                config_set(child->string, value);
            }
        }
    }
}

static void json_races(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Object) {
        log_error("races is not a json object: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        json_race(child, rc_get_or_create(child->string));
    }
}

const char * json_relpath;

/* TODO: much more configurable authority-to-file lookup */
static const char * authority_to_path(const char *authority, char *name, size_t size) {
    /* source and destination cannot share the same buffer */
    assert(authority < name || authority > name + size);

    return join_path(json_relpath, authority, name, size);
}

static const char * uri_to_file(const char * uri, char *name, size_t size) {
    const char *pos, *scheme, *path = uri;

    /* source and destination cannot share the same buffer */
    assert(uri < name || uri > name + size);

    /* identify scheme */
    scheme = uri;
    pos = strstr(scheme, "://");
    if (pos) {
        size_t slen = pos - scheme;
        if (strncmp(scheme, "config", slen) == 0) {
            const char *authority = pos + 3;
            /* authority */
            pos = strstr(authority, "/");
            if (pos) {
                char buffer[16];
                size_t alen = pos - authority;
                assert(alen < sizeof(buffer));
                memcpy(buffer, authority, alen);
                buffer[alen] = 0;

                path = authority_to_path(buffer, name, size);
                path = path_join(path, pos + 1, name, size);
            }
        }
        else {
            log_fatal("unknown URI scheme: %s", uri);
        }
    }
    return path;
}

static int include_json(const char *uri) {
    FILE *F;
    char name[PATH_MAX];
    const char *filename = uri_to_file(uri, name, sizeof(name));
    int result = -1;

    F = fopen(filename, "r");
    if (F) {
        long pos;
        fseek(F, 0, SEEK_END);
        pos = ftell(F);
        rewind(F);
        if (pos > 0) {
            cJSON *config;
            char *data;
            size_t sz;

            data = malloc(pos + 1);
            if (!data) abort();
            sz = fread(data, 1, (size_t)pos, F);
            data[sz] = 0;
            config = cJSON_Parse(data);
            free(data);
            if (config) {
                json_config(config);
                cJSON_Delete(config);
                result = 0;
            }
            else {
                log_error("could not parse JSON from %s", uri);
                result = -1;
            }
        }
        fclose(F);
    }
    return result;
}

static int include_xml(const char *uri) {
    char name[PATH_MAX];
    const char *filename = uri_to_file(uri, name, sizeof(name));
    int err;
    err = exparse_readfile(filename);
    if (err != 0) {
        log_error("could not parse XML from %s", uri);
    }
    return err;
}

static int add_po_string(const char *msgid, const char *msgstr, const char *msgctxt, void *data) {
    struct locale * lang = (struct locale *)data;
    const char * key = msgid;
    if (msgctxt) {
        key = mkname(msgctxt, msgid);
    }
    locale_setstring(lang, key, msgstr);
    return 0;
}

static int include_po(const char *uri) {
    char name[PATH_MAX];
    const char *filename = uri_to_file(uri, name, sizeof(name));
    const char *pos = strstr(filename, ".po");
    if (pos) {
        size_t len;
        const char *str = --pos;
        char lname[8];

        while (str > filename && *str != '.') --str;
        len = (size_t)(pos - str);
        if (len < sizeof(lname)) {
            struct locale * lang;
            memcpy(lname, str+1, len);
            lname[len] = 0;
            lang = get_or_create_locale(lname);
            if (lang) {
                int err = pofile_read(filename, add_po_string, lang);
                if (err < 0) {
                    log_error("could not parse translations from %s", uri);
                }
                return err;
            }
        }
    }
    return -1;
}

static void json_include(cJSON *json) {
    cJSON *child;
    if (json->type != cJSON_Array) {
        log_error("config is not a json array: %d", json->type);
        return;
    }
    for (child = json->child; child; child = child->next) {
        const char *uri = child->valuestring;
        int err;

        if (strstr(uri, ".po") != NULL) {
            err = include_po(uri);
        }
        else if (strstr(uri, ".xml") != NULL) {
            err = include_xml(uri);
        }
        else {
            err = include_json(uri);
        }
        if (err != 0) {
            log_error("no data found in %s", uri);
        }
    }
}

void json_config(cJSON *json) {
    cJSON *child;
    assert(json);
    if (json->type != cJSON_Object) {
        log_error("config is not a json object: %d", json->type);
        return;
    }
    reset_locales();
    for (child = json->child; child; child = child->next) {
        if (strcmp(child->string, "races") == 0) {
            json_races(child);
        }
        else if (strcmp(child->string, "items") == 0) {
            json_items(child);
        }
        else if (strcmp(child->string, "include") == 0) {
            json_include(child);
        }
        else if (strcmp(child->string, "ships") == 0) {
            json_ships(child);
        }
        else if (strcmp(child->string, "strings") == 0) {
            json_strings(child);
        }
        else if (strcmp(child->string, "aliases") == 0) {
            json_aliases(child);
        }
        else if (strcmp(child->string, "calendar") == 0) {
            json_calendar(child);
        }
        else if (strcmp(child->string, "directions") == 0) {
            json_directions(child);
        }
        else if (strcmp(child->string, "keywords") == 0) {
            json_keywords(child);
        }
        else if (strcmp(child->string, "settings") == 0) {
            json_settings(child);
        }
        else if (strcmp(child->string, "skills") == 0) {
            json_skills(child);
        }
        else if (strcmp(child->string, "buildings") == 0) {
            json_buildings(child);
        }
        else if (strcmp(child->string, "spells") == 0) {
            json_spells(child);
        }
        else if (strcmp(child->string, "prefixes") == 0) {
            json_prefixes(child);
        }
        else if (strcmp(child->string, "disabled") == 0) {
            json_disable_features(child);
        }
        else if (strcmp(child->string, "terrains") == 0) {
            json_terrains(child);
            init_terrains();
        }
        else {
            log_error("config contains unknown attribute %s", child->string);
        }
    }
}

