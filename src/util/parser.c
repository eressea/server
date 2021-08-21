#ifdef _MSC_VER
#include <platform.h>
#endif
#include "parser.h"
#include "unicode.h"
#include "base36.h"
#include "log.h"

#include <assert.h>
#include <stdlib.h>
#include <wctype.h>
#include <memory.h>

#define SPACE_REPLACEMENT '~'
#define ESCAPE_CHAR       '\\'
#define MAXTOKENSIZE      8192

typedef struct parse_state {
    const char *current_token;
    struct parse_state *next;
    void *data;
    void(*dtor)(void *);
} parse_state;

static parse_state *states;

static int eatwhitespace_c(const char **str_p)
{
    int ret = 0;
    wint_t wc;
    size_t len;
    const char *str = *str_p;

    /* skip over potential whitespace */
    for (;;) {
        unsigned char utf8_character = (unsigned char)*str;
        if (~utf8_character & 0x80) {
            if (!iswspace(utf8_character))
                break;
            ++str;
        }
        else {
            ret = unicode_utf8_decode(&wc, str, &len);
            if (ret != 0) {
                log_warning("illegal character sequence in UTF8 string: %s\n", str);
                break;
            }
            if (!iswspace(wc))
                break;
            str += len;
        }
    }
    *str_p = str;
    return ret;
}

void init_tokens_ex(const char *initstr, void *data, void (*dtor)(void *))
{
    if (states == NULL) {
        states = calloc(1, sizeof(parse_state));
        if (!states) abort();
    }
    else if (states->dtor) {
        states->dtor(states->data);
    }
    states->dtor = dtor;
    states->data = data;
    states->current_token = initstr;
}

void init_tokens_str(const char *initstr) {
    init_tokens_ex(initstr, NULL, NULL);
}

void parser_pushstate(void)
{
    parse_state *new_state = calloc(1, sizeof(parse_state));
    if (!new_state) abort();
    new_state->current_token = NULL;
    new_state->next = states;
    states = new_state;
}

void parser_popstate(void)
{
    parse_state *new_state = states->next;
    if (states->dtor) {
        states->dtor(states->data);
    }
    free(states);
    states = new_state;
}

bool parser_end(void)
{
    if (states->current_token) {
        eatwhitespace_c(&states->current_token);
        return *states->current_token == 0;
    }
    return true;
}

void skip_token(void)
{
    char quotechar = 0;
    eatwhitespace_c(&states->current_token);

    while (*states->current_token) {
        wint_t wc;
        size_t len;

        unsigned char utf8_character = (unsigned char)states->current_token[0];
        if (~utf8_character & 0x80) {
            wc = utf8_character;
            ++states->current_token;
        }
        else {
            int ret = unicode_utf8_decode(&wc, states->current_token, &len);
            if (ret == 0) {
                states->current_token += len;
            }
            else {
                log_warning("illegal character sequence in UTF8 string: %s\n", states->current_token);
            }
        }
        if (iswspace(wc) && quotechar == 0) {
            return;
        }
        else {
            switch (utf8_character) {
            case '"':
            case '\'':
                if (utf8_character == quotechar)
                    return;
                quotechar = utf8_character;
                break;
            case ESCAPE_CHAR:
                ++states->current_token;
                break;
            }
        }
    }
}

char *parse_token(const char **str, char *lbuf, size_t buflen)
{
    char *cursor = lbuf;
    char quotechar = 0;
    bool escape = false;
    const char *ctoken = *str, *cstart;

    if (!ctoken) {
        return 0;
    }
    eatwhitespace_c(&ctoken);
    if (!*ctoken) {
        if (buflen > 0) {
            *cursor = 0;
        }
        return 0;
    }
    cstart = ctoken;
    while (*ctoken) {
        wint_t wc;
        size_t len;
        bool copy = false;

        unsigned char utf8_character = *(unsigned char *)ctoken;
        if (~utf8_character & 0x80) {
            wc = utf8_character;
            len = 1;
        }
        else {
            int ret = unicode_utf8_decode(&wc, ctoken, &len);
            if (ret != 0) {
                log_info("falling back to ISO-8859-1: %s\n", cstart);
                if (cursor - buflen < lbuf - 2) {
                    size_t inlen = 1;
                    len = 2;
                    unicode_latin1_to_utf8(cursor, &len, ctoken, &inlen);
                    cursor += len;
                    ctoken += inlen;
                    continue;
                }
            }
        }
        if (escape) {
            copy = true;
            escape = false;
        }
        else if (iswspace(wc)) {
            if (quotechar == 0)
                break;
            copy = true;
        }
        else if (utf8_character == '"' || utf8_character == '\'') {
            if (utf8_character == quotechar) {
                quotechar = 0;
                ++ctoken;
                break;
            }
            else if (quotechar == 0 && cstart == ctoken) {
                quotechar = utf8_character;
            }
            else {
                if (cursor - buflen < lbuf - len) {
                    *cursor++ = *ctoken;
                }
            }
            ++ctoken;
        }
        else if (utf8_character == SPACE_REPLACEMENT) {
            if (cursor - buflen < lbuf - len) {
                *cursor++ = ' ';
            }
            ++ctoken;
        }
        else if (utf8_character == ESCAPE_CHAR) {
            escape = true;
            ++ctoken;
        }
        else {
            copy = true;
        }
        if (copy) {
            if (cursor - buflen < lbuf - len) {
                memcpy(cursor, ctoken, len);
                cursor += len;
            }
            ctoken += len;
        }
    }

    *cursor = '\0';
    unicode_utf8_trim(lbuf);
    *str = ctoken;
    return lbuf;
}

static char pbuf[MAXTOKENSIZE];       /* STATIC_RESULT: used for return, not across calls */
const char *parse_token_depr(const char **str)
{
    return parse_token(str, pbuf, MAXTOKENSIZE);
}

char *getstrtoken(void)
{
    return parse_token((const char **)&states->current_token, pbuf, MAXTOKENSIZE);
}

char *gettoken(char *lbuf, size_t bufsize)
{
    return parse_token((const char **)&states->current_token, lbuf, bufsize);
}

int getint(void)
{
    char token[16];
    const char * s = gettoken(token, sizeof(token));
    return s ? atoi(s) : 0;
}

int getuint(void)
{
    int n = getint();
    return (n < 0) ? 0 : n;
}

int getid(void)
{
    char token[8];
    const char *str = gettoken(token, sizeof(token));
    int i = str ? atoi36(str) : 0;
    if (i < 0) {
        return -1;
    }
    return i;
}

unsigned int atoip(const char *s)
{
    int n;

    assert(s);
    n = (s[0] >= '0' && s[0] <= '9');
    n = n ? atoi(s) : 0;

    if (n < 0)
        n = 0;

    return n;
}
