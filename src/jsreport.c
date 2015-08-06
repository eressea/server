#include "reports.h"
#include "jsreport.h"
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/config.h>

#include <util/log.h>

#include <limits.h>
#include <stdio.h>
#include <math.h>

static void coor_to_tiled(int *x, int *y) {
    *y = -*y;
    *x = *x - (*y + 1) / 2;
}

static void coor_from_tiled(int *x, int *y) {
    *x = *x + (*y + 1) / 2;
    *y = -*y;
}

static int report_json(const char *filename, report_context * ctx, const char *charset)
{
    if (get_param_int(global.parameters, "feature.jsreport.enable", 0) != 0) {
        FILE * F = fopen(filename, "w");
        if (F) {
            int x, y, minx = INT_MAX, maxx = INT_MIN, miny = INT_MAX, maxy = INT_MIN;
            seen_region *sr;
            region *r;
            /* traverse all regions */
            for (sr = NULL, r = ctx->first; sr == NULL && r != ctx->last; r = r->next) {
                sr = find_seen(ctx->seen, r);
            }
            for (; sr != NULL; sr = sr->next) {
                int tx = sr->r->x;
                int ty = sr->r->y;
                coor_to_tiled(&tx, &ty);
                if (ty < miny) miny = ty;
                else if (ty > maxy) maxy = ty;
                if (tx < minx) minx = tx;
                else if (tx > maxx) maxx = tx;
            }
            if (maxx >= minx && maxy >= miny) {
                int w = maxx - minx + 1, h = maxy - miny + 1;
                fputs("{ \"orientation\":\"hexagonal\",\"staggeraxis\":\"y\",", F);
                fprintf(F, "\"staggerindex\":\"%s\", \"height\":%d, \"width\":%d, \"layers\":[", (miny & 1) ? "odd" : "even", h, w);
                fprintf(F, "{ \"height\":%d, \"width\":%d, ", h, w);
                fputs("\"visible\":true, \"opacity\":1, \"type\":\"tilelayer\", \"name\":\"terrain\", \"x\":0, \"y\":0, \"data\":[", F);
                for (y = miny; y <= maxy; ++y) {
                    for (x = minx; x <= maxx; ++x) {
                        int data = 0;
                        int tx = x, ty = y;
                        coor_from_tiled(&tx, &ty);
                        r = findregion(tx, ty);
                        if (r) {
                            sr = find_seen(ctx->seen, r);
                            if (sr) {
                                terrain_t ter = oldterrain(r->terrain);
                                if (ter == NOTERRAIN) {
                                    data = 1 + r->terrain->_name[0];
                                }
                                else {
                                    data = 1 + (int)ter;
                                }
                            }
                        }
                        fprintf(F, "%d", data);
                        if (x != maxx || y != maxy) fputs(", ", F);
                    }
                }
                fputs("]}], \"tilesets\": [{\"firstgid\": 1, \"image\": \"magellan.png\", \"imageheight\": 192, \"imagewidth\": 256,"
                    "\"margin\": 0, \"name\": \"hextiles\", \"properties\": { }, \"spacing\": 0, "
                    "\"tileheight\" : 64, \"tilewidth\" : 64 }], \"tilewidth\": 64, \"tileheight\": 96}", F);
            }
            return 0;
        }
        return -1;
    }
    return 0;
}

void register_jsreport(void)
{
    register_reporttype("json", &report_json, 1 << O_JSON);
}
