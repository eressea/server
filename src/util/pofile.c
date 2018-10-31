#ifdef _MSC_VER
#include <platform.h>
#endif

#include "pofile.h"
#include "log.h"
#include "strings.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAXLINE 2048
static char po_line[MAXLINE];
static int po_lineno;

char * read_line(FILE *F) {
    char * read = fgets(po_line, MAXLINE, F);
    ++po_lineno;
    return read;
}

char * read_multiline(FILE *F, char *line, char *buffer, size_t size) {
    char *output = buffer;
    while (line) {
        size_t len;
        char *read = line;
        while (read[0] && isspace(read[0])) {
            /* eat whitespace */
            ++read;
        }
        if (read[0] != '"') {
            break;
        }
        ++read;
        str_unescape(read);
        len = strlen(read);
        if (len >= 2) {
            /* strip trailing quote (and possible newline) */
            if (read[len - 1] == '\n') {
                --len;
            }
            if (read[len - 1] == '"') {
                --len;
            }
            if (size > len) {
                /* copy into buffer */
                memcpy(output, read, len);
                output += len;
                size -= len;
                output[0] = '\0';
            }
        }
        line = read_line(F);
    }
    return line;
}

int pofile_read(const char *filename, int (*callback)(const char *msgid, const char *msgstr, const char *msgctxt, void *data), void *data) {
    FILE * F = fopen(filename, "rt");
    char msgctxt[32];
    char msgid[64];
    char msgstr[2048];
    char *line;
    int err = 0;

    if (!F) {
        log_error("could not open %s", filename);
        return -1;
    }

    msgctxt[0] = 0;
    msgid[0] = 0;
    line = read_line(F);
    while (line) {
        char token[8];
        int err = sscanf(line, "%8s", token);
        if (err == 1) {
            char *text = NULL;
            size_t size = 0, len = strlen(token);

            line = line + len + 1;
            if (len == 7 && memcmp(token, "msgctxt", 7) == 0) {
                text = msgctxt;
                size = sizeof(msgctxt);
            }
            else if (len == 5 && memcmp(token, "msgid", 5) == 0) {
                text = msgid;
                size = sizeof(msgid);
            }
            else if (len == 6 && memcmp(token, "msgstr", 6) == 0) {
                line = read_multiline(F, line, msgstr, sizeof(msgstr));
                if (msgid[0]) {
                    err = callback(msgid, msgstr, msgctxt[0] ? msgctxt : NULL, data);
                    if (err != 0) {
                        break;
                    }
                    msgctxt[0] = 0;
                    msgid[0] = 0;
                }
            }
            if (size > 0) {
                line = read_multiline(F, line, text, size);
            }
        }
        else {
            line = read_line(F);
        }
    }
    err = ferror(F);
    fclose(F);
    if (err) {
        log_error("read error %d in %s:%d.", err, filename, po_lineno);
    }
    return err;
}
