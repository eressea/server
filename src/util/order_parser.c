#ifdef _MSC_VER
#include <platform.h>
#endif

#include "order_parser.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

struct OrderParserStruct {
    void *m_userData;
    char *m_buffer;
    char *m_bufferPtr;
    const char *m_bufferEnd;
    OP_FactionHandler m_factionHandler;
    OP_UnitHandler m_unitHandler;
    OP_OrderHandler m_orderHandler;
    enum OP_Error m_errorCode;
    int m_lineNumber;
};

void OP_SetUnitHandler(OP_Parser parser, OP_UnitHandler handler)
{
    parser->m_unitHandler = handler;
}

void OP_SetFactionHandler(OP_Parser parser, OP_FactionHandler handler) {
    parser->m_factionHandler = handler;
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
    if (parser->m_buffer == NULL) {
        parser->m_buffer = malloc(len + 1);
        if (!parser->m_buffer) {
            return OP_ERROR_NO_MEMORY;
        }
        memcpy(parser->m_buffer, s, len);
        parser->m_buffer[len] = '\0';
        parser->m_bufferPtr = parser->m_buffer;
        parser->m_bufferEnd = parser->m_buffer + len;
    }
    else {
        size_t total = len;
        char * buffer;
        total += (parser->m_bufferEnd - parser->m_bufferPtr);
        /* TODO: recycle buffers, reduce mallocs. */
        buffer = malloc(total + 1);
        memcpy(buffer, parser->m_bufferPtr, total - len);
        memcpy(buffer + total - len, s, len);
        buffer[total] = '\0';
        free(parser->m_buffer);
        parser->m_buffer = buffer;
        if (!parser->m_buffer) {
            return OP_ERROR_NO_MEMORY;
        }
        parser->m_bufferPtr = parser->m_buffer;
        parser->m_bufferEnd = parser->m_buffer + total;
    }
    return OP_ERROR_NONE;
}

static enum OP_Error handle_line(OP_Parser parser) {
    if (parser->m_orderHandler) {
        parser->m_orderHandler(parser->m_userData, parser->m_bufferPtr);
    }
    return OP_ERROR_NONE;
}

static char *skip_spaces(char *pos) {
    char *next;
    for (next = pos; *next && *next != '\n'; ++next) {
        /* TODO: handle unicode whitespace */
        if (!isspace(*next)) break;
    }
    return next;
}

static enum OP_Status parse_buffer(OP_Parser parser, int isFinal)
{
    char * pos = strpbrk(parser->m_bufferPtr, "\\;\n");
    while (pos) {
        enum OP_Error code;
        size_t len = pos - parser->m_bufferPtr;
        char *next;

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
                        pos = next + 1;
                        ++parser->m_lineNumber;
                        break;
                    }
                    else {
                        /* is this backslash the final character? */
                        next = skip_spaces(pos + 1);
                        if (*next == '\n') {
                            /* we have a multi-line comment! */
                            pos = next + 1;
                            ++parser->m_lineNumber;
                        }
                        else {
                            /* keep looking for a backslash */
                            pos = next;
                        }
                    }
                }
            } while (next && *next);

            if (next && pos < parser->m_bufferEnd) {
                /* we skip the comment, and there is more data in the buffer */
                parser->m_bufferPtr = pos;
            }
            else {
                /* we exhausted the buffer before we got to the end of the comment */
                if (isFinal) {
                    /* the input ended on this comment line, which is fine */
                    return OP_STATUS_OK;
                }
                else {
                    /* skip what we have of the comment, keep the semicolon, keep going */
                    ptrdiff_t skip = parser->m_bufferEnd - parser->m_bufferPtr;
                    if (skip > 1) {
                        parser->m_bufferPtr += (skip - 1);
                        parser->m_bufferPtr[0] = ';';
                    }
                }
            }
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

    if (parser->m_bufferPtr >= parser->m_bufferEnd) {
        buffer_free(parser);
    }

    code = buffer_append(parser, s, len);
    if (code != OP_ERROR_NONE) {
        parser->m_errorCode = code;
        return OP_STATUS_ERROR;
    }

    return parse_buffer(parser, isFinal);
}
