#ifdef _MSV_VER
#include <platform.h>
#endif

#include <stdio.h>
#include "orderfile.h"

int main(int argc, char **argv) {
    FILE * F = stdin;
    if (argc >= 1) {
        const char *filename = argv[1];
        F = fopen(filename, "r");
        if (!F) {
            perror(filename);
            return -1;
        }
        readorders(F);
        fclose(F);
    }
    return 0;
}

