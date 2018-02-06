#include <platform.h>


#include "xmlreader.h"
#include <util/xml.h>
#include <kernel/race.h>
#include <kernel/rules.h>
#include <races/races.h>

#include <storage.h>

#include <string.h>

static int usage(void) {
    return -1;
}

int main(int argc, char **argv) {
    const char *mode;

    register_races();
    register_xmlreader();
    if (argc < 2) return usage();
    mode = argv[1];
    if (strcmp(mode, "rules")==0) {
        const char *xmlfile, *catalog;
        if (argc < 4) return usage();
        xmlfile = argv[2];
        catalog = argv[3];
        read_xml(xmlfile);
        write_rules("rules.dat");
        return 0;
    }
    if (strcmp(mode, "po")==0) {
        return 0;
    }
    return usage();
}
