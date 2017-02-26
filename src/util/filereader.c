#include <platform.h>
#include "filereader.h"

#include <util/log.h>
#include <util/unicode.h>

#include <stdbool.h>
#include <ctype.h>
#include <wctype.h>

#define COMMENT_CHAR    ';'
#define CONTINUE_CHAR    '\\'
#define MAXLINE 4096*16
static char lbuf[MAXLINE];
static char fbuf[MAXLINE];

static void unicode_warning(const char *bp)
{
    log_warning("invalid sequence in UTF-8 string: %s\n", bp);
}

static int eatwhite(const char *ptr, size_t * total_size)
{
    int ret = 0;

    *total_size = 0;

    while (*ptr) {
        ucs4_t ucs;
        size_t size = 0;
        ret = unicode_utf8_to_ucs4(&ucs, ptr, &size);
        if (ret != 0)
            break;
        if (!iswspace((wint_t)ucs))
            break;
        *total_size += size;
        ptr += size;
    }
    return ret;
}

static const char *getbuf_latin1(FILE * F)
{
    bool cont = false;
    char quote = 0;
    bool comment = false;
    char *cp = fbuf;
    char *tail = lbuf + MAXLINE - 2;

    tail[1] = '@';                /* if this gets overwritten by fgets then the line was very long. */
    do {
        const char *bp = fgets(lbuf, MAXLINE, F);

        if (bp == NULL)
            return NULL;
        while (*bp && isspace(*(unsigned char *)bp))
            ++bp;                     /* eatwhite */

        comment = (bool)(comment && cont);
        quote = (bool)(quote && cont);

        if (tail[1] == 0) {
            /* we read he maximum number of bytes! */
            if (tail[0] != '\n') {
                /* it wasn't enough space to finish the line, eat the rest */
                for (;;) {
                    tail[1] = '@';
                    bp = fgets(lbuf, MAXLINE, F);
                    if (bp == NULL)
                        return NULL;
                    if (tail[1]) {
                        /* read enough this time to end the line */
                        break;
                    }
                }
                comment = false;
                cont = false;
                bp = NULL;
                continue;
            }
            else {
                tail[1] = '@';
            }
        }
        cont = false;
        while (*bp && cp < fbuf + MAXLINE) {
            int c = *(unsigned char *)bp;

            if (c == '\n' || c == '\r') {
                /* line breaks, shmine breaks */
                break;
            }
            if (c == COMMENT_CHAR && !quote) {
                /* comment begins. we need to keep going, to look for CONTINUE_CHAR */
                comment = true;
                ++bp;
                continue;
            }
            if (!comment && (c == '"' || c == '\'')) {
                if (quote == c) {
                    quote = 0;
                    if (cp < fbuf + MAXLINE)
                        *cp++ = *bp;
                    ++bp;
                    continue;
                }
                else if (!quote) {
                    quote = *bp++;
                    if (cp < fbuf + MAXLINE)
                        *cp++ = quote;
                    continue;
                }
            }

            if (iscntrl(c)) {
                if (!comment && cp < fbuf + MAXLINE) {
                    *cp++ = isspace(c) ? ' ' : '?';
                }
                ++bp;
                continue;
            }
            else if (isspace(c)) {
                if (!quote) {
                    ++bp;
                    while (*bp && isspace(*(unsigned char *)bp))
                        ++bp;               /* eatwhite */
                    if (!comment && *bp && *bp != COMMENT_CHAR && cp < fbuf + MAXLINE)
                        *(cp++) = ' ';
                }
                else if (!comment && cp + 1 <= fbuf + MAXLINE) {
                    *(cp++) = *(bp++);
                }
                else {
                    ++bp;
                }
                continue;
            }
            else if (c == CONTINUE_CHAR) {
                const char *end = ++bp;
                while (*end && isspace(*(unsigned char *)end))
                    ++end;                /* eatwhite */
                if (*end == '\0') {
                    bp = end;
                    cont = true;
                    continue;
                }
                if (comment) {
                    ++bp;
                    continue;
                }
            }
            else if (comment) {
                ++bp;
                continue;
            }

            if (c < 0x80) {
                if (cp + 1 <= fbuf + MAXLINE) {
                    *(cp++) = *(bp++);
                }
            }
            else {
                char inbuf = (char)c;
                size_t inbytes = 1;
                size_t outbytes = MAXLINE - (cp - fbuf);
                int ret = unicode_latin1_to_utf8(cp, &outbytes, &inbuf, &inbytes);
                if (ret > 0)
                    cp += ret;
                else {
                    log_error("input data was not iso-8859-1! assuming utf-8\n");
                    return NULL;
                }

                ++bp;
                continue;
            }
        }
        if (cp == fbuf + MAXLINE) {
            --cp;
        }
        *cp = 0;
    } while (cont || cp == fbuf);
    return fbuf;
}

static const char *getbuf_utf8(FILE * F)
{
    bool cont = false;
    char quote = 0;
    bool comment = false;
    char *cp = fbuf;
    char *tail = lbuf + MAXLINE - 2;

    tail[1] = '@';                /* if this gets overwritten by fgets then the line was very long. */
    do {
        const char *bp = fgets(lbuf, MAXLINE, F);
        size_t white;
        if (bp == NULL) {
            return NULL;
        }

        eatwhite(bp, &white);       /* decoding errors will get caught later on, don't have to check */
        bp += white;

        comment = (bool)(comment && cont);
        quote = (bool)(quote && cont);

        if (tail[1] == 0) {
            /* we read the maximum number of bytes! */
            if (tail[0] != '\n') {
                /* it wasn't enough space to finish the line, eat the rest */
                for (;;) {
                    tail[1] = '@';
                    bp = fgets(lbuf, MAXLINE, F);
                    if (bp == NULL)
                        return NULL;
                    if (tail[1]) {
                        /* read enough this time to end the line */
                        break;
                    }
                }
                comment = false;
                cont = false;
                bp = NULL;
                continue;
            }
            else {
                tail[1] = '@';
            }
        }
        cont = false;
        while (*bp && cp < fbuf + MAXLINE) {
            ucs4_t ucs;
            size_t size;
            int ret;

            if (!quote) {
                while (*bp == COMMENT_CHAR) {
                    /* comment begins. we need to keep going, to look for CONTINUE_CHAR */
                    comment = true;
                    ++bp;
                }
            }

            if (*bp == '\n' || *bp == '\r') {
                /* line breaks, shmine breaks */
                break;
            }

            if (*bp == '"' || *bp == '\'') {
                if (quote == *bp) {
                    quote = 0;
                    if (!comment && cp < fbuf + MAXLINE)
                        *cp++ = *bp;
                    ++bp;
                    continue;
                }
                else if (!quote) {
                    quote = *bp++;
                    if (!comment && cp < fbuf + MAXLINE)
                        *cp++ = quote;
                    continue;
                }
            }

            ret = unicode_utf8_to_ucs4(&ucs, bp, &size);

            if (ret != 0) {
                unicode_warning(bp);
                break;
            }

            if (iswspace((wint_t)ucs)) {
                if (!quote) {
                    bp += size;
                    ret = eatwhite(bp, &size);
                    bp += size;
                    if (!comment && *bp && *bp != COMMENT_CHAR && cp < fbuf + MAXLINE)
                        *(cp++) = ' ';
                    if (ret != 0) {
                        unicode_warning(bp);
                        break;
                    }
                }
                else if (!comment) {
                    if (cp + size <= fbuf + MAXLINE) {
                        while (size--) {
                            *(cp++) = *(bp++);
                        }
                    }
                    else
                        bp += size;
                }
                else {
                    bp += size;
                }
            }
            else if (iswcntrl((wint_t)ucs)) {
                if (!comment && cp < fbuf + MAXLINE) {
                    *cp++ = '?';
                }
                bp += size;
            }
            else {
                if (*bp == CONTINUE_CHAR) {
                    const char *end;
                    eatwhite(bp + 1, &white);
                    end = bp + 1 + white;
                    if (*end == '\0') {
                        bp = end;
                        cont = true;
                        continue;
                    }
                    if (!comment && cp < fbuf + MAXLINE)
                        *cp++ = *bp++;
                    else
                        ++bp;
                }
                else {
                    if (!comment && cp + size <= fbuf + MAXLINE) {
                        while (size--) {
                            *(cp++) = *(bp++);
                        }
                    }
                    else {
                        bp += size;
                    }
                }
            }
        }
        if (cp == fbuf + MAXLINE) {
            --cp;
        }
        *cp = 0;
    } while (cont || cp == fbuf);
    return fbuf;
}

const char *getbuf(FILE * F, int encoding)
{
    if (encoding == ENCODING_UTF8)
        return getbuf_utf8(F);
    return getbuf_latin1(F);
}
