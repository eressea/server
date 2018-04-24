#ifdef _MSC_VER
#include <platform.h>
#endif
#include "exparse.h"

#include "util/log.h"

#include <expat.h>

int exparse_readfile(const char * filename) {
    XML_Parser xp;
    FILE *F;
    int err = 1;
    char buf[4096];

    F = fopen(filename, "r");
    if (!F) {
        return 2;
    }
    xp = XML_ParserCreate("UTF-8");
    for (;;) {
        size_t len = (int) fread(buf, 1, sizeof(buf), F);
        int done;
        
        if (ferror(F)) {
            log_error("read error in %s", filename);
            err = -2;
            break;
        }
        done = feof(F);
        if (XML_Parse(xp, buf, len, done) == XML_STATUS_ERROR) {
            log_error("parse error at line %u of %s: %s", 
                    XML_GetCurrentLineNumber(xp),
                    filename,
                    XML_ErrorString(XML_GetErrorCode(xp)));
            err = -1;
            break;
        }
        if (done) {
            break;
        }
    }
    XML_ParserFree(xp);
    fclose(F);
    return err;
}
