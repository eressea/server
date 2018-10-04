#ifdef _MSV_VER
#include <platform.h>
#endif

#include "util/order_parser.h"

#include <stdio.h>

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

int main(int argc, char **argv) {
    FILE * F = stdin;
    if (argc >= 1) {
        const char *filename = argv[1];
        F = fopen(filename, "r");
        if (!F) {
            perror(filename);
            return -1;
        }
    }
    parsefile(F);
    if (F != stdin) {
        fclose(F);
    }
    return 0;
}
