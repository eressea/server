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
    enum CR_Error m_errorCode;
    int m_lineNumber;
};

OP_Parser OP_ParserCreate(void)
{
    return NULL;
}

void OP_ParserFree(OP_Parser op) {
    free(op);
}

enum OP_Status OP_Parse(OP_Parser op, const char *s, int len, int isFinal)
{
    return OP_STATUS_OK;
}
