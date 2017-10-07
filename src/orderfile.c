#include "orderfile.h"

#include <kernel/faction.h>
#include <kernel/unit.h>

#include <stream.h>
#include <filestream.h>

#include <stdio.h>
void read_orders(stream *strm)
{
    faction *f = NULL;
    unit *u = NULL;
    char line[1024];

    line = strm->api->readln(strm->handle, line, sizeof(line));
}

void read_orderfile(const char *filename)
{
    stream strm;
    FILE * F = fopen(filename, "r");
    if (!F) {
        return;
    }
    fstream_init(&strm, F);
    read_orders(&strm);
    fstream_done(&strm);
    fclose(F);
}
