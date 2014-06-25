/* vi: set ts=2:
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2004   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <platform.h>
#include <kernel/config.h>
#include "jsonconf.h"

/* kernel includes */
#include "building.h"
#include "direction.h"
#include "keyword.h"
#include "equipment.h"
#include "item.h"
#include "messages.h"
#include "race.h"
#include "region.h"
#include "resources.h"
#include "ship.h"
#include "terrain.h"
#include "skill.h"
#include "spell.h"
#include "spellbook.h"
#include "calendar.h"

/* util includes */
#include <util/attrib.h>
#include <util/bsdstring.h>
#include <util/crmessage.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/log.h>
#include <util/message.h>
#include <util/nrmessage.h>
#include <util/xml.h>

/* external libraries */
#include <cJSON.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

int json_flags(cJSON *json, const char *flags[]) {
    cJSON *entry;
    int result = 0;
    assert(json->type==cJSON_Array);
    for (entry=json->child;entry;entry=entry->next) {
        if (entry->type == cJSON_String) {
            int i;
            for (i = 0; flags[i]; ++i) {
                if (strcmp(flags[i], entry->valuestring)==0) {
                    result |= (1<<i);
                }
            }
        }
    }
    return result;
}

void json_construction(cJSON *json, construction **consp) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("building %s is not a json object: %d", json->string, json->type);
        return;
    }
    construction * cons = (construction *)calloc(sizeof(construction), 1);
    for (child=json->child;child;child=child->next) {
        switch(child->type) {
        case cJSON_Number:
            if (strcmp(child->string, "maxsize")==0) {
                cons->maxsize = child->valueint;
            }
            else if (strcmp(child->string, "reqsize")==0) {
                cons->reqsize = child->valueint;
            }
            else if (strcmp(child->string, "minskill")==0) {
                cons->minskill = child->valueint;
            }
            break; 
        default:
            log_error_n("building %s contains unknown attribute %s", json->string, child->string);
        }
    }
    *consp = cons;
}

void json_terrain(cJSON *json, terrain_type *ter) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("terrain %s is not a json object: %d", json->string, json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        switch(child->type) {
        case cJSON_Array:
            if (strcmp(child->string, "flags")==0) {
                const char * flags[] = {
                    "land", "sea", "forest", "arctic", "cavalry", "forbidden", "sail", "fly", "swim", "walk", 0
                };
                ter->flags = json_flags(child, flags);
            } else {
                log_error_n("terrain %s contains unknown attribute %s", json->string, child->string);
            }
            break;
        default:
            log_error_n("terrain %s contains unknown attribute %s", json->string, child->string);
        }
    }
}

void json_building(cJSON *json, building_type *bt) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("building %s is not a json object: %d", json->string, json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        switch(child->type) {
        case cJSON_Object:
            if (strcmp(child->string, "construction")==0) {
                json_construction(child, &bt->construction);
            }
            break;
        default:
            log_error_n("building %s contains unknown attribute %s", json->string, child->string);
        }
    }
}

void json_item(cJSON *json, item_type *st) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("ship %s is not a json object: %d", json->string, json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        switch(child->type) {
        case cJSON_Object:
        default:
            log_error_n("item %s contains unknown attribute %s", json->string, child->string);
        }
    }
}

void json_ship(cJSON *json, ship_type *st) {
    cJSON *child, *iter;
    if (json->type!=cJSON_Object) {
        log_error_n("ship %s is not a json object: %d", json->string, json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        int i;
        switch(child->type) {
        case cJSON_Object:
            if (strcmp(child->string, "construction")==0) {
                json_construction(child, &st->construction);
            } else {
                log_error_n("ship %s contains unknown attribute %s", json->string, child->string);
            }
            break;
        case cJSON_Array:
            st->coasts = (const terrain_type **)
                malloc(sizeof(terrain_type *) * (1+cJSON_GetArraySize(child)));
            for (i=0,iter=child->child;iter;iter=iter->next) {
                if (iter->type==cJSON_String) {
                    terrain_type *ter = get_or_create_terrain(iter->valuestring);
                    if (ter) {
                        st->coasts[i++] = ter;
                    }
                }
            }
            st->coasts[i] = 0;
            break;
        case cJSON_Number:
            if (strcmp(child->string, "range")==0) {
                st->range = child->valueint;
            } else {
                log_error_n("ship %s contains unknown attribute %s", json->string, child->string);
            }
            break;
        default:
            log_error_n("ship %s contains unknown attribute %s", json->string, child->string);
        }
    }
}

void json_race(cJSON *json, race *rc) {
    cJSON *child;
    const char *flags[] = {
        "playerrace", "killpeasants", "scarepeasants",
        "cansteal", "moverandom", "cannotmove",
        "learn", "fly", "swim", "walk", "nolearn",
        "noteach", "horse", "desert",
        "illusionary", "absorbpeasants", "noheal", 
        "noweapons", "shapeshift", "", "undead", "dragon",
        "coastal", "", "cansail", 0
    };
    if (json->type!=cJSON_Object) {
        log_error_n("race %s is not a json object: %d", json->string, json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        switch(child->type) {
        case cJSON_String:
            if (strcmp(child->string, "damage")==0) {
                rc->def_damage = _strdup(child->valuestring);
            }
            break;
        case cJSON_Number:
            if (strcmp(child->string, "magres")==0) {
                rc->magres = (float)child->valuedouble;
            }
            else if (strcmp(child->string, "maxaura")==0) {
                rc->maxaura = (float)child->valuedouble;
            }
            else if (strcmp(child->string, "regaura")==0) {
                rc->regaura = (float)child->valuedouble;
            }
            else if (strcmp(child->string, "speed")==0) {
                rc->speed = (float)child->valuedouble;
            }
            else if (strcmp(child->string, "recruitcost")==0) {
                rc->recruitcost = child->valueint;
            }
            else if (strcmp(child->string, "maintenance")==0) {
                rc->maintenance = child->valueint;
            }
            else if (strcmp(child->string, "weight")==0) {
                rc->weight = child->valueint;
            }
            else if (strcmp(child->string, "capacity")==0) {
                rc->capacity = child->valueint;
            }
            else if (strcmp(child->string, "hp")==0) {
                rc->hitpoints = child->valueint;
            }
            else if (strcmp(child->string, "ac")==0) {
                rc->armor = child->valueint;
            }
            // TODO: studyspeed (orcs only)
            break;
        case cJSON_Array:
            if (strcmp(child->string, "flags")==0) {
                rc->flags = json_flags(child, flags);
            }
            break;
        }
    }
}

void json_terrains(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("terrains is not a json object: %d", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        json_terrain(child, get_or_create_terrain(child->string));
    }
}

void json_buildings(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("buildings is not a json object: %d", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        json_building(child, bt_get_or_create(child->string));
    }
}

void json_items(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("items is not a json object: %d", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        resource_type *rtype = rt_get_or_create(child->string);
        item_type *itype = rtype->itype;
        if (!itype) {
            rtype->itype = itype = it_get_or_create(rtype);
        }
        json_item(child, itype);
    }
}

void json_ships(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("ships is not a json object: %d", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        json_ship(child, st_get_or_create(child->string));
    }
}

static void json_direction(cJSON *json, struct locale *lang) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("directions for locale `%s` not a json object: %d", locale_name(lang), json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        direction_t dir = finddirection(child->string);
        if (dir!=NODIRECTION) {
            if (child->type==cJSON_String) {
                init_direction(lang, dir, child->valuestring);
            }
            else if (child->type==cJSON_Array) {
                cJSON *entry;
                for (entry=child->child;entry;entry=entry->next) {
                    init_direction(lang, dir, entry->valuestring);
                }
            } else {
                log_error_n("invalid type %d for direction `%s`", child->type, child->string);
            }
        }
    }
}

void json_directions(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("directions is not a json object: %d", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        struct locale * lang = get_or_create_locale(child->string);
        json_direction(child, lang);
    }
}

static void json_skill(cJSON *json, struct locale *lang) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("skill for locale `%s` not a json object: %d", locale_name(lang), json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        skill_t sk = findskill(child->string);
        if (sk!=NOSKILL) {
            if (child->type==cJSON_String) {
                init_skill(lang, sk, child->valuestring);
                locale_setstring(lang, mkname("skill", skillnames[sk]), child->valuestring);
            }
            else if (child->type==cJSON_Array) {
                cJSON *entry;
                for (entry=child->child;entry;entry=entry->next) {
                    init_skill(lang, sk, entry->valuestring);
                    if ((entry==child->child)) {
                        locale_setstring(lang, mkname("skill", skillnames[sk]), entry->valuestring); 
                    }
                }
            } else {
                log_error_n("invalid type %d for skill `%s`", child->type, child->string);
            }
        } else {
            log_error_n("unknown skill `%s` for locale `%s`", child->string, locale_name(lang));
        }
    }
}

static void json_keyword(cJSON *json, struct locale *lang) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("keywords for locale `%s` not a json object: %d", locale_name(lang), json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        keyword_t kwd = findkeyword(child->string);
        if (kwd!=NOKEYWORD) {
            if (child->type==cJSON_String) {
                init_keyword(lang, kwd, child->valuestring);
                locale_setstring(lang, mkname("keyword", keywords[kwd]), child->valuestring);
            }
            else if (child->type==cJSON_Array) {
                cJSON *entry;
                for (entry=child->child;entry;entry=entry->next) {
                    init_keyword(lang, kwd, entry->valuestring);
                    if ((entry==child->child)) {
                        locale_setstring(lang, mkname("keyword", keywords[kwd]), entry->valuestring); 
                    }
                }
            } else {
                log_error_n("invalid type %d for keyword `%s`", child->type, child->string);
            }
        } else {
            log_error_n("unknown keyword `%s` for locale `%s`", child->string, locale_name(lang));
        }
    }
}

void json_skills(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("skills is not a json object: %d", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        struct locale * lang = get_or_create_locale(child->string);
        json_skill(child, lang);
    }
}

void json_keywords(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("keywords is not a json object: %d", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        struct locale * lang = get_or_create_locale(child->string);
        json_keyword(child, lang);
    }
}

void json_races(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("races is not a json object: %d", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        json_race(child, rc_get_or_create(child->string));
    }
}

void json_config(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error_n("config is not a json object: %d", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        if (strcmp(child->string, "races")==0) {
            json_races(child);
        }
        else if (strcmp(child->string, "ships")==0) {
            json_ships(child);
        }
        else if (strcmp(child->string, "directions")==0) {
            json_directions(child);
        }
        else if (strcmp(child->string, "keywords")==0) {
            json_keywords(child);
        }
        else if (strcmp(child->string, "skills")==0) {
            json_skills(child);
        }
        else if (strcmp(child->string, "buildings")==0) {
            json_buildings(child);
        }
        else if (strcmp(child->string, "terrains")==0) {
            json_terrains(child);
        } else {
            log_error_n("config contains unknown attribute %s", child->string);
        }
    }
}

