#include <kernel/config.h>
#include "plane.h"

/* kernel includes */
#include "region.h"
#include "faction.h"

/* util includes */
#include <kernel/attrib.h>
#include <util/resolve.h>
#include <util/lists.h>

#include <storage.h>
#include <strings.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct plane *planes;

int plane_width(const plane * pl)
{
    if (pl) {
        return pl->maxx - pl->minx + 1;
    }
    return 0;
}

int plane_height(const plane * pl)
{
    if (pl) {
        return pl->maxy - pl->miny + 1;
    }
    return 0;
}

plane *get_homeplane(void)
{
    return getplanebyid(0);
}

plane *getplane(const region * r)
{
    if (r) {
        return r->_plane;
    }
    return get_homeplane();
}

plane *getplanebyid(int id)
{
    plane *p;

    for (p = planes; p; p = p->next) {
        if (p->id == id) {
            return p;
        }
    }
    return NULL;
}

plane *getplanebyname(const char *name)
{
    plane *p;

    for (p = planes; p; p = p->next)
        if (p->name && !strcmp(p->name, name))
            return p;
    return NULL;
}

plane *findplane(int x, int y)
{
    plane *pl;

    for (pl = planes; pl; pl = pl->next) {
        if (x >= pl->minx && x <= pl->maxx && y >= pl->miny && y <= pl->maxy) {
            return pl;
        }
    }
    return NULL;
}

int getplaneid(const region * r)
{
    if (r) {
        plane *pl = getplane(r);
        if (pl)
            return pl->id;

        for (pl = planes; pl; pl = pl->next) {
            if (r->x >= pl->minx && r->x <= pl->maxx
                && r->y >= pl->miny && r->y <= pl->maxy) {
                return pl->id;
            }
        }
    }
    return 0;
}

static int
ursprung_x(const faction * f, const plane * pl, const region * rdefault)
{
    origin *ur;
    int id = 0;

    if (!f)
        return 0;

    if (pl)
        id = pl->id;

    for (ur = f->origin; ur; ur = ur->next) {
        if (ur->id == id)
            return ur->x;
    }
    if (!rdefault)
        return 0;
    faction_setorigin((faction *)f, id, rdefault->x - plane_center_x(pl),
        rdefault->y - plane_center_y(pl));
    return rdefault->x - plane_center_x(pl);
}

static int
ursprung_y(const faction * f, const plane * pl, const region * rdefault)
{
    origin *ur;
    int id = 0;

    if (!f)
        return 0;

    if (pl)
        id = pl->id;

    for (ur = f->origin; ur; ur = ur->next) {
        if (ur->id == id)
            return ur->y;
    }
    if (!rdefault)
        return 0;
    faction_setorigin((faction *)f, id, rdefault->x - plane_center_x(pl),
        rdefault->y - plane_center_y(pl));
    return rdefault->y - plane_center_y(pl);
}

int plane_center_x(const plane * pl)
{
    if (pl == NULL)
        return 0;

    return (pl->minx + pl->maxx) / 2;
}

int plane_center_y(const plane * pl)
{
    if (pl == NULL)
        return 0;

    return (pl->miny + pl->maxy) / 2;
}

void
adjust_coordinates(const faction * f, int *x, int *y, const plane * pl)
{
    int nx = *x;
    int ny = *y;
    if (f) {
        int ux = 0, uy = 0;
        faction_getorigin(f, pl?pl->id:0, &ux, &uy);
        nx -= ux;
        ny -= uy;
    }
    if (pl) {
        int plx = plane_center_x(pl);
        int ply = plane_center_y(pl);
        int width = plane_width(pl);
        int height = plane_height(pl);
        int width_2 = width / 2;
        int height_2 = height / 2;

        nx = (nx - plx) % width;
        ny = (ny - ply) % height;

        if (nx < 0)
            nx = (width - (-nx) % width);
        if (nx > width_2)
            nx -= width;
        if (ny < 0)
            ny = (height - (-ny) % height);
        if (ny > height_2)
            ny -= height;

        assert(nx <= pl->maxx - plx);
        assert(nx >= pl->minx - plx);
        assert(ny <= pl->maxy - ply);
        assert(ny >= pl->miny - ply);

    }

    *x = nx;
    *y = ny;
}

plane *create_new_plane(int id, const char *name, int minx, int maxx, int miny,
    int maxy, int flags)
{
    plane *pl = getplanebyid(id);

    if (pl)
        return pl;
    pl = calloc(1, sizeof(plane));
    if (!pl) abort();
    pl->next = NULL;
    pl->id = id;
    if (name)
        pl->name = str_strdup(name);
    pl->minx = minx;
    pl->maxx = maxx;
    pl->miny = miny;
    pl->maxy = maxy;
    pl->flags = flags;

    addlist(&planes, pl);
    return pl;
}

/* Umrechnung Relative-Absolute-Koordinaten */
int
rel_to_abs(const struct plane *pl, const struct faction *f, int rel,
unsigned char index)
{
    assert(index == 0 || index == 1);

    if (index == 0)
        return (rel + ursprung_x(f, pl, NULL) + plane_center_x(pl));

    return (rel + ursprung_y(f, pl, NULL) + plane_center_y(pl));
}

void free_plane(plane *pl) {
    free(pl->name);
    free(pl);
}

void remove_plane(plane *pl) {
    region **rp = &regions;
    plane **pp = &planes;
    assert(pl);
    while (*rp) {
        region *r = *rp;
        if (r->_plane == pl) {
            remove_region(rp, r);
        }
        else {
            rp = &r->next;
        }
    }
    while (*pp) {
        if (pl==*pp) {
            *pp = pl->next;
            free_plane(pl);
            break;
        }
        else {
            pp = &(*pp)->next;
        }
    }
}
