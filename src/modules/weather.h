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

#ifndef H_MOD_WEATHER_H
#define H_MOD_WEATHER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef WEATHER
# error "the weather system is disabled"
#endif

  enum {
    WEATHER_NONE,
    WEATHER_STORM,
    WEATHER_HURRICANE,
    MAXWEATHERS
  };

  typedef unsigned char weather_t;

  typedef struct weather {
    struct weather *next;
    weather_t type;             /* Typ der Wetterzone */
    int center[2];              /* Koordinaten des Zentrums */
    int radius;
    int move[2];
  } weather;

  weather *weathers;

  void set_weather(void);
  void move_weather(void);

#ifdef __cplusplus
}
#endif
#endif
