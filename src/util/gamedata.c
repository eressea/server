#include <platform.h>

#include "gamedata.h"
#include "log.h"

#include <storage.h>
#include <filestream.h>
#include <memstream.h>
#include <binarystore.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void gamedata_close(gamedata *data) {
    binstore_done(data->store);
    fstream_done(&data->strm);
}

void gamedata_init(gamedata *data, storage *store, int version) {
    data->version = version;
    data->store = store;
    binstore_init(data->store, &data->strm);
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
            fclose(F);
            free(data);
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
    FILE *F = fopen(filename, mode);

    if (F) {
        gamedata *data = (gamedata *)calloc(1, sizeof(gamedata));
        storage *store = (storage *)calloc(1, sizeof(storage));
        int err = 0;
        size_t sz;

        data->store = store;
        if (strchr(mode, 'r')) {
            sz = fread(&data->version, 1, sizeof(int), F);
            if (sz != sizeof(int)) {
                err = ferror(F);
            }
            else {
                err = fseek(F, sizeof(int), SEEK_CUR);
            }
        }
        else if (strchr(mode, 'w')) {
            int n = STREAM_VERSION;
            data->version = version;
            fwrite(&data->version, sizeof(int), 1, F);
            fwrite(&n, sizeof(int), 1, F);
        }
        if (err) {
            fclose(F);
            free(data);
            free(store);
        }
        else {
            fstream_init(&data->strm, F);
            binstore_init(store, &data->strm);
            return data;
        }
    }
    log_error("could not open %s: %s", filename, strerror(errno));
    return 0;
}
