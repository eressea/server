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

void json_construction(cJSON *json, construction **consp) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error("building %s is not a json object: %d\n", json->string, json->type);
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
            log_error("building %s contains unknown attribute %s\n", json->string, child->string);
        }
    }
    *consp = cons;
}

void json_terrain(cJSON *json, terrain_type *ter) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error("terrain %s is not a json object: %d\n", json->string, json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        log_error("terrain %s contains unknown attribute %s\n", json->string, child->string);
    }
}

void json_building(cJSON *json, building_type *bt) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error("building %s is not a json object: %d\n", json->string, json->type);
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
            log_error("building %s contains unknown attribute %s\n", json->string, child->string);
        }
    }
}

void json_ship(cJSON *json, ship_type *st) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error("ship %s is not a json object: %d\n", json->string, json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        switch(child->type) {
        case cJSON_Object:
            if (strcmp(child->string, "construction")==0) {
                json_construction(child, &st->construction);
            }
            break;
        default:
            log_error("ship %s contains unknown attribute %s\n", json->string, child->string);
        }
    }
}

void json_race(cJSON *json, race *rc) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error("race %s is not a json object: %d\n", json->string, json->type);
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
        case cJSON_True: {
            const char *flags[] = {
                "playerrace", "killpeasants", "scarepeasants",
                "cansteal", "moverandom", "cannotmove",
                "learn", "fly", "swim", "walk", "nolearn",
                "noteach", "horse", "desert",
                "illusionary", "absorbpeasants", "noheal", 
                "noweapons", "shapeshift", "", "undead", "dragon",
                "coastal", "", "cansail", 0
            };
            int i;
            for(i=0;flags[i];++i) {
                const char * flag = flags[i];
                if (*flag && strcmp(child->string, flag)==0) {
                    rc->flags |= (1<<i);
                    break;
                }
            }
            break;
        }
        }
    }
}

void json_terrains(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error("terrains is not a json object: %d\n", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        json_terrain(child, terrain_get_or_create(child->string));
    }
}

void json_buildings(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error("buildings is not a json object: %d\n", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        json_building(child, bt_get_or_create(child->string));
    }
}

void json_ships(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error("ships is not a json object: %d\n", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        json_ship(child, st_get_or_create(child->string));
    }
}

void json_races(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error("races is not a json object: %d\n", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        json_race(child, rc_get_or_create(child->string));
    }
}

void json_config(cJSON *json) {
    cJSON *child;
    if (json->type!=cJSON_Object) {
        log_error("config is not a json object: %d\n", json->type);
        return;
    }
    for (child=json->child;child;child=child->next) {
        if (strcmp(child->string, "races")==0) {
            json_races(child);
        }
        else if (strcmp(child->string, "ships")==0) {
            json_ships(child);
        }
        else if (strcmp(child->string, "buildings")==0) {
            json_buildings(child);
        }
        else if (strcmp(child->string, "terrains")==0) {
            json_terrains(child);
        } else {
            log_error("config contains unknown attribute %s\n", child->string);
        }
    }
}

