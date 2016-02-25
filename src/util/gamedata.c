#include <platform.h>

#include "gamedata.h"
#include "log.h"

#include <storage.h>
#include <filestream.h>
#include <memstream.h>
#include <binarystore.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void gamedata_done(gamedata *data) {
    binstore_done(data->store);
}

void gamedata_init(gamedata *data, storage *store, int version) {
    data->version = version;
    data->store = store;
    binstore_init(data->store, &data->strm);
}

void gamedata_close(gamedata *data) {
    gamedata_done(data);
    fstream_done(&data->strm);
}

int gamedata_openfile(gamedata *data, const char *filename, const char *mode, int version) {
    FILE *F = fopen(filename, mode);
    if (F) {
        int err = 0;

        if (strchr(mode, 'r')) {
            size_t sz;
            sz = fread(&version, 1, sizeof(int), F);
            if (sz != sizeof(int)) {
                err = ferror(F);
            }
            else {
                err = fseek(F, sizeof(int), SEEK_CUR);
            }
        }
        else if (strchr(mode, 'w')) {
            int n = STREAM_VERSION;
            fwrite(&version, sizeof(int), 1, F);
            fwrite(&n, sizeof(int), 1, F);
        }
        if (err) {
            log_error("could not open %s: %s", filename, strerror(errno));
            fclose(F);
        }
        else {
            storage *store = malloc(sizeof(storage));
            fstream_init(&data->strm, F);
            gamedata_init(data, store, version);
        }
        return err;
    }
    return errno;
}

gamedata *gamedata_open(const char *filename, const char *mode, int version) {
    gamedata *data = (gamedata *)calloc(1, sizeof(gamedata));
    if (gamedata_openfile(data, filename, mode, version) != 0) {
        free(data);
        return NULL;
    }
    return data;
}
