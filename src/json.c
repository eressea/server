#include "platform.h"
#include <util/base36.h>

#include "json.h"

#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/terrain.h>
#include <stream.h>
#include "cJSON.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int json_import(struct stream * out) {
    cJSON *json, *child;
    char buffer[1024], *data = 0;
    size_t sz = 0;
    assert(out && out->api);
    while (!out->api->readln(out->handle, buffer, sizeof(buffer))) {
        size_t len = strlen(buffer);
        data = (char *)realloc(data, sz + len + 1);
        memcpy(data + sz, buffer, len);
        sz += len;
        data[sz] = 0;
    }
    json = cJSON_Parse(data);
    child = cJSON_GetObjectItem(json, "regions");
    if (child && child->type == cJSON_Object) {
        cJSON *j;
        for (j = child->child; j; j = j->next) {
            cJSON *attr;
            int id = 0;
            int x = 0, y = 0;
            region * r;

            id = atoi(j->string);
            if ((attr = cJSON_GetObjectItem(j, "x")) != 0 && attr->type == cJSON_Number) x = attr->valueint;
            if ((attr = cJSON_GetObjectItem(j, "y")) != 0 && attr->type == cJSON_Number) y = attr->valueint;
            r = new_region(x, y, 0, id);
            if ((attr = cJSON_GetObjectItem(j, "type")) != 0 && attr->type == cJSON_String) {
                const terrain_type *terrain = get_terrain(attr->valuestring);
                terraform_region(r, terrain);
            }
            if ((attr = cJSON_GetObjectItem(j, "name")) != 0 && attr->type == cJSON_String) {
                region_setname(r, attr->valuestring);
            }
        }
    }
    cJSON_Delete(json);
    return 0;
}

int json_export(stream * out, int flags) {
    cJSON *json, *root = cJSON_CreateObject();
    assert(out && out->api);
    if (regions && (flags & EXPORT_REGIONS)) {
        char id[32];
        region * r;
        plane * p;
        cJSON_AddItemToObject(root, "planes", json = cJSON_CreateObject());
        for (p = planes; p; p = p->next) {
            cJSON *data;
            _snprintf(id, sizeof(id), "%u", p->id);
            cJSON_AddItemToObject(json, id, data = cJSON_CreateObject());
            cJSON_AddNumberToObject(data, "x", p->minx);
            cJSON_AddNumberToObject(data, "y", p->miny);
            cJSON_AddNumberToObject(data, "width", p->maxx - p->minx);
            cJSON_AddNumberToObject(data, "height", p->maxy - p->miny);
            if (p->name) cJSON_AddStringToObject(data, "name", p->name);
        }

        cJSON_AddItemToObject(root, "regions", json = cJSON_CreateObject());
        for (r = regions; r; r = r->next) {
            cJSON *data;
            _snprintf(id, sizeof(id), "%u", r->uid);
            cJSON_AddItemToObject(json, id, data = cJSON_CreateObject());
            cJSON_AddNumberToObject(data, "x", r->x);
            cJSON_AddNumberToObject(data, "y", r->y);
            cJSON_AddStringToObject(data, "type", r->terrain->_name);
            if (r->land) {
                cJSON_AddStringToObject(data, "name", r->land->name);
            }
        }
    }
    if (factions && (flags & EXPORT_FACTIONS)) {
        faction *f;
        cJSON_AddItemToObject(root, "factions", json = cJSON_CreateObject());
        for (f = factions; f; f = f->next) {
            cJSON *data;
            cJSON_AddItemToObject(json, itoa36(f->no), data = cJSON_CreateObject());
            cJSON_AddStringToObject(data, "name", f->name);
            cJSON_AddStringToObject(data, "email", f->email);
            cJSON_AddNumberToObject(data, "score", f->score);
        }
    }
    if (flags) {
        char *tok, *output;
        output = cJSON_Print(root);
        tok = strtok(output, "\n\r");
        while (tok) {
            if (tok[0]) {
                out->api->writeln(out->handle, tok);
            }
            tok = strtok(NULL, "\n\r");
        }
        free(output);
    }
    cJSON_Delete(root);
    return 0;
}
