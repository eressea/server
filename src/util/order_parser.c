#ifdef _MSC_VER
#include <platform.h>
#endif

#include "order_parser.h"

#include <assert.h>
#include <wctype.h>
#include <stdlib.h>
#include <string.h>

struct OrderParserStruct {
    void *m_userData;
    char *m_buffer;
    char *m_bufferPtr;
    size_t m_bufferSize;
    const char *m_bufferEnd;
    OP_OrderHandler m_orderHandler;
    enum OP_Error m_errorCode;
    int m_lineNumber;
};

enum OP_Error OP_GetErrorCode(OP_Parser parser) {
    return parser->m_errorCode;
}

void OP_SetOrderHandler(OP_Parser parser, OP_OrderHandler handler) {
    parser->m_orderHandler = handler;
}

void OP_SetUserData(OP_Parser parser, void *userData) {
    parser->m_userData = userData;
}

static void buffer_free(OP_Parser parser)
{
    /* TODO: recycle buffers, reduce mallocs. */
    free(parser->m_buffer);
    parser->m_bufferSize = 0;
    parser->m_bufferEnd = parser->m_bufferPtr = parser->m_buffer = NULL;
}

void OP_ParserReset(OP_Parser parser) {
    parser->m_lineNumber = 1;
    buffer_free(parser);
}


OP_Parser OP_ParserCreate(void)
{
    OP_Parser parser = calloc(1, sizeof(struct OrderParserStruct));
    OP_ParserReset(parser);
    return parser;
}

void OP_ParserFree(OP_Parser parser) {
    free(parser->m_buffer);
    free(parser);
}

static enum OP_Error buffer_append(OP_Parser parser, const char *s, int len)
{
    size_t total = len + 1;
    size_t remain = parser->m_bufferEnd - parser->m_bufferPtr;
    total += remain;
    if (remain > 0) {
        /* there is remaining data in the buffer, should we move it to the front? */
        if (total <= parser->m_bufferSize) {
            /* reuse existing buffer */
            memmove(parser->m_buffer, parser->m_bufferPtr, remain);
            memcpy(parser->m_buffer + remain, s, len);
            parser->m_buffer[total - 1] = '\0';
            parser->m_bufferPtr = parser->m_buffer;
            parser->m_bufferEnd = parser->m_bufferPtr + total - 1;
            return OP_ERROR_NONE;
        }
    }
    else if (parser->m_bufferPtr >= parser->m_bufferEnd) {
        buffer_free(parser);
    }

    if (parser->m_buffer == NULL) {
        parser->m_bufferSize = len + 1;
        parser->m_buffer = malloc(parser->m_bufferSize);
        if (!parser->m_buffer) {
            return OP_ERROR_NO_MEMORY;
        }
        memcpy(parser->m_buffer, s, len);
        parser->m_buffer[len] = '\0';
        parser->m_bufferPtr = parser->m_buffer;
        parser->m_bufferEnd = parser->m_buffer + len;
    }
    else {
        char * buffer;
        /* TODO: recycle buffers, reduce mallocs. */
        if (parser->m_bufferSize < total) {
            parser->m_bufferSize = total;
            buffer = malloc(parser->m_bufferSize);
            if (!buffer) {
                return OP_ERROR_NO_MEMORY;
            }
            memcpy(buffer, parser->m_bufferPtr, total - len - 1);
            memcpy(buffer + total - len - 1, s, len);
            free(parser->m_buffer);
            parser->m_buffer = buffer;
        }
        else {
            memcpy(parser->m_buffer, parser->m_bufferPtr, total - len);
            memcpy(parser->m_buffer + total - len, s, len);
        }
        parser->m_buffer[total - 1] = '\0';
        parser->m_bufferPtr = parser->m_buffer;
        parser->m_bufferEnd = parser->m_buffer + total - 1;
    }
    return OP_ERROR_NONE;
}

static char *skip_spaces(char *pos) {
    char *next;
    for (next = pos; *next && *next != '\n'; ++next) {
        wint_t wch = *(unsigned char *)next;
        /* TODO: handle unicode whitespace */
        if (!iswspace(wch)) break;
    }
    return next;
}

static enum OP_Error handle_line(OP_Parser parser) {
    if (parser->m_orderHandler) {
        char * str = skip_spaces(parser->m_bufferPtr);
        if (*str) {
            parser->m_orderHandler(parser->m_userData, str);
        }
    }
    return OP_ERROR_NONE;
}

static enum OP_Status parse_buffer(OP_Parser parser, int isFinal)
{
    char * pos = strpbrk(parser->m_bufferPtr, "\\;\n");
    while (pos) {
        enum OP_Error code;
        size_t len = pos - parser->m_bufferPtr;
        char *next;
        int continue_comment = 0;

        switch (*pos) {
        case '\n':
            *pos = '\0';
            code = handle_line(parser);
            ++parser->m_lineNumber;
            if (code != OP_ERROR_NONE) {
                parser->m_errorCode = code;
                return OP_STATUS_ERROR;
            }
            parser->m_bufferPtr = pos + 1;
            pos = strpbrk(parser->m_bufferPtr, "\\;\n");
            break;
        case '\\':
            /* if this is the last non-space before the line break, then lines need to be joined */
            next = skip_spaces(pos + 1);
            if (*next == '\n') {
                ptrdiff_t shift = (next + 1 - pos);
                assert(shift > 0);
                memmove(parser->m_bufferPtr + shift, parser->m_bufferPtr, len);
                parser->m_bufferPtr += shift;
                pos = strpbrk(next + 1, "\\;\n");
                ++parser->m_lineNumber;
            }
            else {
                /* this is not multi-line input yet, so do nothing */
                if (pos[1] == '\0') {
                    /* end of available input */
                    if (isFinal) {
                        /* input ends on a pointless backslash, kill it */
                        pos[0] = '\0';
                        pos = NULL;
                    }
                    else {
                        /* backslash is followed by data that we do not know */
                        pos = NULL;
                    }
                }
                else {
                    pos = strpbrk(pos + 1, "\\;\n");
                }
            }
            break;
        case ';':
            /* the current line ends in a comment */
            *pos++ = '\0';
            handle_line(parser);
            /* find the end of the comment so we can skip it.
             * obs: multi-line comments are possible with a backslash. */
            do {
                next = strpbrk(pos, "\\\n");
                if (next) {
                    if (*next == '\n') {
                        /* no more lines in this comment, we're done: */
                        ++parser->m_lineNumber;
                        break; /* exit loop */
                    }
                    else {
                        /* is this backslash the final character? */
                        next = skip_spaces(next + 1);
                        if (*next == '\n') {
                            /* we have a multi-line comment! */
                            pos = next + 1;
                            ++parser->m_lineNumber;
                        }
                        else if (*next == '\0') {
                            /* cannot find the EOL char yet, stream is dry. keep ; and \ */
                            continue_comment = 2;
                        }
                        else {
                            /* keep looking for a backslash */
                            pos = next;
                        }
                    }
                }
            } while (next && *next);

            if (!next) {
                /* we exhausted the buffer before we finished the line */
                if (isFinal) {
                    /* this comment was at the end of the file, it just has no newline. done! */
                    return OP_STATUS_OK;
                }
                else {
                    /* there is more of this line in the next buffer, save the semicolon */
                    continue_comment = 1;
                }
            }
            else { 
                if (*next) {
                    /* end comment parsing, begin parsing a new line */
                    pos = next + 1;
                    continue_comment = 0;
                }
                else if (!continue_comment) {
                    /* reached end of input naturally, need more data to finish */
                    continue_comment = 1;
                }
            }

            if (continue_comment) {
                ptrdiff_t skip = parser->m_bufferEnd - parser->m_bufferPtr;
                assert(skip >= continue_comment);
                if (skip >= continue_comment) {
                    /* should always be true */
                    parser->m_bufferPtr += (skip - continue_comment);
                    parser->m_bufferPtr[0] = ';';
                }
                if (continue_comment == 2) {
                    parser->m_bufferPtr[1] = '\\';
                }
                continue_comment = 0;
                return OP_STATUS_OK;
            }
            /* continue the outer loop */
            parser->m_bufferPtr = pos;
            pos = strpbrk(pos, "\\;\n");
            break;
        default:
            parser->m_errorCode = OP_ERROR_SYNTAX;
            return OP_STATUS_ERROR;
        }
    }
    if (isFinal && parser->m_bufferPtr < parser->m_bufferEnd) {
        /* this line ends without a line break */
        handle_line(parser);
    }
    return OP_STATUS_OK;
}

enum OP_Status OP_Parse(OP_Parser parser, const char *s, int len, int isFinal)
{
    enum OP_Error code;

    code = buffer_append(parser, s, len);
    if (code != OP_ERROR_NONE) {
        parser->m_errorCode = code;
        return OP_STATUS_ERROR;
    }

    return parse_buffer(parser, isFinal);
}
