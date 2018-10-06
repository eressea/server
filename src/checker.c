#ifdef _MSV_VER
#include <platform.h>
#endif

#include "util/order_parser.h"
#include "util/keyword.h"
#include "util/language.h"
#include "util/path.h"
#include "util/pofile.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

typedef struct parser_state {
    FILE * F;
} parser_state;

static void handle_order(void *userData, const char *str) {
    parser_state * state = (parser_state*)userData;
    fputs(str, state->F);
    fputc('\n', state->F);
}

int parsefile(FILE *F) {
    OP_Parser parser;
    char buf[1024];
    int done = 0, err = 0;
    parser_state state = { NULL };

    state.F = stdout;

    parser = OP_ParserCreate();
    OP_SetOrderHandler(parser, handle_order);
    OP_SetUserData(parser, &state);

    while (!done) {
        size_t len = (int)fread(buf, 1, sizeof(buf), F);
        if (ferror(F)) {
            /* TODO: error message */
            err = errno;
            break;
        }
        done = feof(F);
        if (OP_Parse(parser, buf, len, done) == OP_STATUS_ERROR) {
            /* TODO: error message */
            err = (int)OP_GetErrorCode(parser);
            break;
        }
    }
    OP_ParserFree(parser);
    return err;
}

static int handle_po(const char *msgid, const char *msgstr, const char *msgctxt, void *data) {
    struct locale *lang = (struct locale *)data;
    if (msgctxt) {
        if (strcmp(msgctxt, "keyword") == 0) {
            keyword_t kwd = findkeyword(msgid);
            init_keyword(lang, kwd, msgstr);
            locale_setstring(lang, mkname("keyword", keywords[kwd]), msgstr);
        }
    }
    return 0;
}

static void read_config(const char *respath) {
    char path[PATH_MAX];
    struct locale *lang;
    lang = get_or_create_locale("de");
    path_join(respath, "translations/strings.de.po", path, sizeof(path));
    pofile_read(path, handle_po, lang);
}

int main(int argc, char **argv) {
    FILE * F = stdin;
    if (argc > 1) {
        const char *filename = argv[1];
        F = fopen(filename, "r");
        if (!F) {
            perror(filename);
            return -1;
        }
    }
    read_config("../git");
    parsefile(F);
    if (F != stdin) {
        fclose(F);
    }
    return 0;
}
