#include <platform.h>
#include <kernel/types.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <stream.h>
#include "cJSON.h"

void export_json(stream * out) {
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
        cJSON_AddStringToObject(data, "type", r->terrain->_name);
    }
    if (out) {
        char *tok, *output;
        output = cJSON_Print(json);
        tok = strtok(output, "\n\r");
        while (tok) {
            if (tok[0]) {
                out->api->writeln(out->handle, tok);
            }
            tok = strtok(NULL, "\n\r");
        }
        free(output);
    }
}
