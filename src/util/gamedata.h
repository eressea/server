#pragma once

#ifndef _GAMEDATA_H
#define _GAMEDATA_H

#include <stream.h>

struct storage;

typedef struct gamedata {
    struct storage *store;
    stream strm;
    int version;
    int encoding;
} gamedata;

void gamedata_init(gamedata *data, struct storage *store, int version);
void gamedata_done(gamedata *data);

void gamedata_close(gamedata *data);
gamedata *gamedata_open(const char *filename, const char *mode, int version);
int gamedata_openfile(gamedata *data, const char *filename, const char *mode, int version);

#define STREAM_VERSION 2 /* internal encoding of binary files */

#endif
