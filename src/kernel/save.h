#pragma once

#include <stream.h>

struct attrib;
struct item;
struct storage;
struct spell;
struct spellbook;
struct unit;
struct building;
struct faction;
struct region;
struct ship;
struct gamedata;

/* Nach MAX_INPUT_SIZE brechen wir das Einlesen der Zeile ab und nehmen an,
 * dass hier ein Fehler (fehlende ") vorliegt */
 /* TODO: is this *really* still in use? */
#define MAX_INPUT_SIZE	DISPLAYSIZE*2

int readgame(const char *filename);
int writegame(const char *filename);

int current_turn(void);

void write_unit(struct gamedata *data, const struct unit *u);
struct unit *read_unit(struct gamedata *data);
    
void write_faction(struct gamedata *data, const struct faction *f);
struct faction *read_faction(struct gamedata *data);

void write_region(struct gamedata *data, const struct region *r);
struct region *read_region(struct gamedata *data);

void write_building(struct gamedata *data, const struct building *b);
struct building *read_building(struct gamedata *data);

void write_ship(struct gamedata *data, const struct ship *sh);
struct ship *read_ship(struct gamedata *data);

int write_game(struct gamedata *data);
int read_game(struct gamedata *data);

/* test-only functions that give access to internal implementation details (BAD) */
void _test_write_password(struct gamedata *data, const struct faction *f);
void _test_read_password(struct gamedata *data, struct faction *f);

void fix_shadows(void);
