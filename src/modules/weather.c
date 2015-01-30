/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifdef WEATHER

#include <platform.h>
#include <config.h>
#include "weather.h"

/* libc includes */
#include <math.h>

weather *create_weather(region * r, weather_t type)
{
    weather *w;

    w = calloc(1, sizeof(weather));
    w->center[0] = r->x;
    w->center[1] = r->y;
    w->type = type;
    w->move[0] = (rng_int() % 3) - 1;
    w->move[1] = (rng_int() % 3) - 1;

    switch (type) {
    case WEATHER_STORM:
        w->radius = rng_int() % 2 + 1;
        break;
    case WEATHER_HURRICANE:
        w->radius = 1;
        break;
    default:
        w->radius = 0;
    }

    addlist(&weathers, w);

    return w;
}

double distance(int x1, int y1, int x2, int y2)
{
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

/* Diese Funktion ermittelt für jede Region, welches Wetter in ihr
   herrscht. Die Wettertypen sind dabei nach ihrer Reihenfolge in der
   enumeration priorisiert.

   - Einladen
   set_weather();
   - Eigentliche Auswertung
   - Veränderungen des Wetters
   set_weather();
   - Report generieren
   - Abspeichern

   Diese Routine ist sehr rechenaufwendig!
   */

void set_weather(void)
{
    weather_t i;
    weather *w;
    short x, y;
    int d;
    region *r;

    for (r = regions; r; r = r->next) {
        r->weathertype = WEATHER_NONE;
    }

    for (i = 0; i < MAXWEATHERS; i++) {
        for (w = weathers; w; w = w->next) {
            if (w->type == i) {
                for (x = w->center[0] - w->radius; x <= w->center[0] + w->radius; x++) {
                    for (y = w->center[1] - w->radius; y <= w->center[1] + w->radius; y++) {
                        d = distance(w->center[0], w->center[1], x, y);
                        if (floor(d + 0.5) <= w->radius) {
                            r = findregion(x, y);
                            if (r) {
                                r->weathertype = w->type;
                            }
                        }
                    }
                }
            }
        }
    }
}

void move_weather(void)
{
    weather *w, *wnext;
    region *r;

    for (w = weathers; w;) {
        wnext = w->next;
        w->center[0] = w->center[0] + w->move[0];
        w->center[1] = w->center[1] + w->move[1];
        r = findregion(w->center[0], w->center[1]);
        if (!r || rng_int() % 100 < 5) {
            removelist(&weathers, w);
        }
        w = wnext;
    }
}

#else
#include <stdio.h>
static const char *copyright = "(c) Eressea PBEM 2000";

void init_weather(void)
{
    fputs(copyright, stderr);
    /* TODO: Initialization */
}
#endif
