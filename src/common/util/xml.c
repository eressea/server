/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/
#include <config.h>
#include "xml.h"

/* util includes */
#include "log.h"

/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int
xml_ivalue(xmlNodePtr node, const char * name, int dflt)
{
  int i = dflt;
  xmlChar * property = xmlGetProp(node, BAD_CAST name);
  if (property!=NULL) {
    i = atoi((const char*)property);
    xmlFree(property);
  }
  return i;
}

boolean
xml_bvalue(xmlNodePtr node, const char * name, boolean dflt)
{
  boolean result = dflt;
  xmlChar * property = xmlGetProp(node, BAD_CAST name);
  if (property!=NULL) {
    if (strcmp((const char*)property, "yes")==0) result = true;
    if (strcmp((const char*)property, "1")==0) {
      log_warning(("boolean value is '1': %s::%s\n", node->name, name));
      result = true;
    }
    xmlFree(property);
  }
  return result;
}

double
xml_fvalue(xmlNodePtr node, const char * name, double dflt)
{
  double result = dflt;
  xmlChar * property = xmlGetProp(node, BAD_CAST name);
  if (property!=NULL) {
    result = atof((const char*)property);
    xmlFree(property);
  }
  return result;
}

/* new xml functions */
/* libxml includes */
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

typedef struct xml_reader {
  struct xml_reader * next;
  xml_callback callback;
} xml_reader;

static xml_reader * xmlReaders;

void
xml_register_callback(xml_callback callback)
{
  xml_reader * reader = malloc(sizeof(xml_reader));
  xml_reader ** insert = &xmlReaders;
  reader->callback = callback;
  reader->next = NULL;

  while (*insert) insert = &(*insert)->next;
  *insert = reader;
}

int
read_xml(const char * filename)
{
  xml_reader * reader = xmlReaders;
  xmlDocPtr doc;

  doc = xmlReadFile(filename, NULL, XML_PARSE_XINCLUDE);
  if (doc==NULL) {
    log_error(("could not open %s\n", filename));
    return -1;
  }

  xmlXIncludeProcess(doc);

  while (reader!=NULL) {
    int i = reader->callback(doc);
    if (i!=0) return i;
    reader = reader->next;
  }
  xmlFreeDoc(doc);
  return 0;
}

