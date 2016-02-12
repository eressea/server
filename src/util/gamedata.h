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

void gamedata_close(struct gamedata *data);
struct gamedata *gamedata_open(const char *filename, const char *mode, int version);
int gamedata_openfile(gamedata *data, const char *filename, const char *mode, int version);

#define STREAM_VERSION 2 /* internal encoding of binary files */

#endif
