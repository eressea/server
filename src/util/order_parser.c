#ifdef _MSC_VER
#include <platform.h>
#endif

#include "order_parser.h"

#include <stdlib.h>

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

void OP_SetUnitHandler(OP_Parser op, OP_UnitHandler handler)
{
    op->m_unitHandler = handler;
}

void OP_SetFactionHandler(OP_Parser op, OP_FactionHandler handler) {
    op->m_factionHandler = handler;
}

void OP_SetOrderHandler(OP_Parser op, OP_OrderHandler handler) {
    op->m_orderHandler = handler;
}

void OP_SetUserData(OP_Parser op, void *userData) {
    op->m_userData = userData;
}

OP_Parser OP_ParserCreate(void)
{
    OP_Parser parser = calloc(1, sizeof(struct OrderParserStruct));
    return parser;
}

void OP_ParserFree(OP_Parser op) {
    free(op);
}

enum OP_Status OP_Parse(OP_Parser op, const char *s, int len, int isFinal)
{
    return OP_STATUS_OK;
}
