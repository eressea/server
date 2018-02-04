/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */
#include <platform.h>
#include "xml.h"

/* util includes */
#include "log.h"
#include "assert.h"

#include <libxml/catalog.h>
#include <libxml/xmlstring.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int xml_ivalue(xmlNodePtr node, const char *name, int dflt)
{
    int i = dflt;
    xmlChar *propValue = xmlGetProp(node, BAD_CAST name);
    if (propValue != NULL) {
        i = atoi((const char *)propValue);
        xmlFree(propValue);
    }
    return i;
}

bool xml_bvalue(xmlNodePtr node, const char *name, bool dflt)
{
    bool result = dflt;
    xmlChar *propValue = xmlGetProp(node, BAD_CAST name);
    if (propValue != NULL) {
        if (strcmp((const char *)propValue, "no") == 0)
            result = false;
        else if (strcmp((const char *)propValue, "yes") == 0)
            result = true;
        else if (strcmp((const char *)propValue, "false") == 0)
            result = false;
        else if (strcmp((const char *)propValue, "true") == 0)
            result = true;
        else if (strcmp((const char *)propValue, "1") == 0) {
            log_warning("bool value is '1': %s::%s\n", node->name, name);
            result = true;
        }
        else if (strcmp((const char *)propValue, "0") == 0) {
            log_warning("bool value is '0': %s::%s\n", node->name, name);
            result = false;
        }
        xmlFree(propValue);
    }
    return result;
}

double xml_fvalue(xmlNodePtr node, const char *name, double dflt)
{
    double result = dflt;
    xmlChar *propValue = xmlGetProp(node, BAD_CAST name);
    if (propValue != NULL) {
        result = atof((const char *)propValue);
        xmlFree(propValue);
    }
    return result;
}

/* new xml functions */
/* libxml includes */
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

typedef struct xml_reader {
    struct xml_reader *next;
    xml_callback callback;
} xml_reader;

static xml_reader *xmlReaders;

void xml_done(void) {
    xml_reader ** xrp = &xmlReaders;
    while (*xrp) {
        xml_reader *xr = *xrp;
        *xrp = xr->next;
        free(xr);
    }
}

void xml_register_callback(xml_callback callback)
{
    xml_reader *reader = (xml_reader *)malloc(sizeof(xml_reader));
    xml_reader **insert = &xmlReaders;
    assert_alloc(reader);
    reader->callback = callback;
    reader->next = NULL;

    while (*insert)
        insert = &(*insert)->next;
    *insert = reader;
}

int read_xml(const char *filename, const char *catalog)
{
    xml_reader *reader = xmlReaders;
    xmlDocPtr doc;
    int result = 0;

    if (catalog) {
        xmlLoadCatalog(catalog);
    }
    doc = xmlReadFile(filename, NULL, XML_PARSE_XINCLUDE | XML_PARSE_NONET | XML_PARSE_PEDANTIC | XML_PARSE_COMPACT);
    if (doc == NULL) {
        log_error("could not open '%s'\n", filename);
        return -1;
    }
    if (catalog) {
        result = xmlXIncludeProcessFlags(doc, XML_PARSE_XINCLUDE | XML_PARSE_NONET | XML_PARSE_PEDANTIC | XML_PARSE_COMPACT);
    }
    if (result >= 0) {
        while (reader != NULL) {
            int i = reader->callback(doc);
            if (i != 0) {
                return i;
            }
            reader = reader->next;
        }
        result = 0;
    }
    xmlFreeDoc(doc);
    return result;
}
