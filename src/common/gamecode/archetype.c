#include <config.h>
#include "eressea.h"
#include "archetype.h"

/* kernel includes */
#include <kernel/equipment.h>
#include <kernel/building.h>
#include <kernel/xmlkernel.h>
#include <kernel/xmlreader.h>

/* util includes */
#include <util/umlaut.h>
#include <util/language.h>
#include <util/xml.h>

/* libxml includes */
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/encoding.h>

/* libc includes */
#include <string.h>
#include <assert.h>

static struct archetype * archetypes;

const struct archetype *
find_archetype(const char * s, const struct locale * lang)
{
  struct tnode * tokens = get_translations(lang, UT_ARCHETYPES);
  variant token;

  if (findtoken(tokens, s, &token)==E_TOK_SUCCESS) {
    return (const struct archetype *)token.v;
  }
  return NULL;
}

void
register_archetype(archetype * arch)
{
  arch->next = archetypes;
  archetypes = arch;
}

const archetype * 
get_archetype(const char * name)
{
  const archetype * arch = archetypes;
  for (;arch;arch=arch->next) {
    if (strcmp(name, arch->name)==0) {
      return arch;
    }
  }
  return NULL;
}

void
init_archetypes(void)
{
  const struct locale * lang = locales;
  for (;lang;lang=nextlocale(lang)) {
    variant var;
    archetype * arch = archetypes;
    struct tnode * tokens = get_translations(lang, UT_ARCHETYPES);
    for (;arch;arch=arch->next) {
      var.v = arch;
      addtoken(tokens, LOC(lang, arch->name), var);
    }
  }
}

static int
parse_archetypes(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST "/eressea/archetypes/archetype", xpath);
  xmlNodeSetPtr nodes = result->nodesetval;

  xmlChar * property;
  if (nodes && nodes->nodeNr>0) {
    xmlNodePtr node = nodes->nodeTab[0];
    archetype * arch = calloc(1, sizeof(archetype));
    xmlXPathObjectPtr sub;

    property = xmlGetProp(node, BAD_CAST "name");
    assert(property!=NULL);
    arch->name = strdup((const char *)property);
    xmlFree(property);

    property = xmlGetProp(node, BAD_CAST "equip");
    if (property!=NULL) {
      arch->equip = get_equipment((const char*)property);
      xmlFree(property);
    } else {
      arch->equip = get_equipment(arch->name);
    }

    property = xmlGetProp(node, BAD_CAST "building");
    if (property!=NULL) {
      arch->btype = bt_find((const char*)property);
      xmlFree(property);
    }

    arch->size = xml_ivalue(node, "cost", 1);

    xpath->node = node;
    sub = xmlXPathEvalExpression(BAD_CAST "construction", xpath);
    if (sub->nodesetval) {
      xml_readconstruction(xpath, sub->nodesetval, &arch->ctype);
    }
    xmlXPathFreeObject(sub);
  }
  xmlXPathFreeObject(result);

  xmlXPathFreeContext(xpath);
  return 0;
}

void
register_archetypes(void)
{
  xml_register_callback(parse_archetypes);
}