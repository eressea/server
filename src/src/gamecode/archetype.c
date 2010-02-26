#include <platform.h>
#include <kernel/config.h>
#include "archetype.h"

/* kernel includes */
#include <kernel/equipment.h>
#include <kernel/building.h>
#include <kernel/xmlkernel.h>
#include <kernel/xmlreader.h>

/* util includes */
#include <util/attrib.h>
#include <util/umlaut.h>
#include <util/language.h>
#include <util/xml.h>
#include <util/functions.h>

/* libxml includes */
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/encoding.h>

/* libc includes */
#include <string.h>
#include <assert.h>

static struct archetype * archetypes;

struct attrib_type at_recruit = {
  "recruit", NULL, NULL, NULL, NULL, NULL, ATF_UNIQUE
};

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

void
init_archetypes(void)
{
  const struct locale * lang = locales;
  for (;lang;lang=nextlocale(lang)) {
    variant var;
    archetype * arch = archetypes;
    struct tnode * tokens = get_translations(lang, UT_ARCHETYPES);
    for (;arch;arch=arch->next) {
      const char *s1, *s2;
      var.v = arch;

      s1 = LOC(lang, arch->name[0]);
      addtoken(tokens, s1, var);
      s2 = LOC(lang, arch->name[1]);
      if (strcmp(s2, s1)!=0) {
        addtoken(tokens, s2, var);
      }
    }
  }
}

static int
parse_archetypes(xmlDocPtr doc)
{
  char zName[64];
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST "/eressea/archetypes/archetype", xpath);
  xmlNodeSetPtr nodes = result->nodesetval;

  xmlChar * propValue;
  if (nodes) {
    int i;
    for (i=0;i!=nodes->nodeNr;++i) {
      xmlNodePtr node = nodes->nodeTab[i];
      xmlNodePtr child;
      archetype * arch = calloc(1, sizeof(archetype));
      xmlXPathObjectPtr sub;

      propValue = xmlGetProp(node, BAD_CAST "name");
      assert(propValue!=NULL);
      arch->name[0] = strdup((const char *)propValue);
      sprintf(zName, "%s_p", arch->name[0]);
      arch->name[1] = strdup(zName);
      xmlFree(propValue);

      propValue = xmlGetProp(node, BAD_CAST "equip");
      if (propValue!=NULL) {
        arch->equip = get_equipment((const char*)propValue);
        xmlFree(propValue);
      } else {
        arch->equip = get_equipment(arch->name[0]);
      }

      propValue = xmlGetProp(node, BAD_CAST "building");
      if (propValue!=NULL) {
        arch->btype = bt_find((const char*)propValue);
        xmlFree(propValue);
      }

      arch->size = xml_ivalue(node, "cost", 0);

      for (child=node->children;child;child=child->next) {
        if (strcmp((const char *)child->name, "function")==0) {
          xmlChar * propName = xmlGetProp(child, BAD_CAST "name");
          xmlChar * propValue = xmlGetProp(child, BAD_CAST "value");
          if (strcmp((const char *)propName, "create")) {
            pf_generic foo = get_function((const char *)propValue);
            arch->exec = (archetype_function)foo;
          }
          xmlFree(propValue);
          xmlFree(propName);
        }
      }
      xpath->node = node;
      sub = xmlXPathEvalExpression(BAD_CAST "allow|deny", xpath);
      if (sub->nodesetval && sub->nodesetval->nodeNr) {
        int k;
        arch->rules = calloc(sub->nodesetval->nodeNr+1, sizeof(rule));
        for (k=0;k!=sub->nodesetval->nodeNr;++k) {
          xmlNodePtr rule = sub->nodesetval->nodeTab[k];
          arch->rules[k].allow = (rule->name[0]=='a');

          propValue = xmlGetProp(rule, BAD_CAST "property");
          arch->rules[k].property = strdup((const char *)propValue);
          xmlFree(propValue);

          propValue = xmlGetProp(rule, BAD_CAST "value");
          arch->rules[k].value = strdup((const char *)propValue);
          xmlFree(propValue);
        }
      }
      xmlXPathFreeObject(sub);

      xpath->node = node;
      sub = xmlXPathEvalExpression(BAD_CAST "construction", xpath);
      if (sub->nodesetval) {
        xml_readconstruction(xpath, sub->nodesetval, &arch->ctype);
      }
      xmlXPathFreeObject(sub);
      register_archetype(arch);
    }
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
