#include <platform.h>


#ifdef USE_LIBXML2
#include <kernel/xmlreader.h>
#include <util/xml.h>
#endif
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
#ifdef USE_LIBXML2
    register_xmlreader();
#endif
    if (argc < 2) return usage();
    mode = argv[1];
#ifdef USE_LIBXML2
    if (strcmp(mode, "rules")==0) {
        const char *xmlfile, *catalog;
        if (argc < 4) return usage();
        xmlfile = argv[2];
        catalog = argv[3];
        read_xml(xmlfile, catalog);
        write_rules("rules.dat");
        return 0;
    }
#endif
    if (strcmp(mode, "po")==0) {
        return 0;
    }
    return usage();
}
