/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef WEATHER_H
#define WEATHER_H

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
	struct weather 	*next;
	weather_t	type;						/* Typ der Wetterzone */
	int				center[2];			/* Koordinaten des Zentrums */
	int				radius;
	int       move[2];
} weather;

weather *weathers;

void     set_weather(void);
void     move_weather(void);

#endif
