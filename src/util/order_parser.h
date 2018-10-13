#pragma once
/* 
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2007   |
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *
 */

#ifndef UTIL_ORDER_PARSER_H
#define UTIL_ORDER_PARSER_H

#include <stddef.h>
#include <stdbool.h>

struct OrderParserStruct;
typedef struct OrderParserStruct *OP_Parser;

enum OP_Status {
    OP_STATUS_ERROR = 0,
    OP_STATUS_OK = 1
};

enum OP_Error {
    OP_ERROR_NONE,
    OP_ERROR_NO_MEMORY,
    OP_ERROR_SYNTAX
};

typedef void(*OP_OrderHandler) (void *userData, const char *str);

OP_Parser OP_ParserCreate(void);
void OP_ParserFree(OP_Parser parser);
void OP_ParserReset(OP_Parser parser);
enum OP_Status OP_Parse(OP_Parser parser, const char *s, int len, int isFinal);
void OP_SetOrderHandler(OP_Parser parser, OP_OrderHandler handler);
void OP_SetUserData(OP_Parser parser, void *userData);
enum OP_Error OP_GetErrorCode(OP_Parser parser);

#endif
