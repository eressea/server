#include <platform.h>
#include <kernel/types.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include "cJSON.h"

void export_json(const char *filename) {
    region * r;
    cJSON *json, *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "regions", json = cJSON_CreateObject());
    for (r=regions; r; r=r->next) {
        char id[32];
        cJSON *data;
        snprintf(id, sizeof(id), "%u", r->uid);
        cJSON_AddItemToObject(json, id, data = cJSON_CreateObject());
        cJSON_AddNumberToObject(data, "x", r->x);
        cJSON_AddNumberToObject(data, "y", r->y);
        cJSON_AddStringToObject(data, "terrain", r->terrain->_name);
    }
}
