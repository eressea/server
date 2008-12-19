/* libxml includes */
#include <libxml/tree.h>
#include <libxml/xpath.h>

/* libc includes */
#include <string.h>

const char * tmp_includes = 
"#include <lua.h>\n"
"#include <tolua.h>\n"
;
void
read_templates()
{
}

xmlDocPtr
readfile(const char * filename)
{
  xmlDocPtr doc;
#ifdef XML_PARSE_XINCLUDE
  doc = xmlReadFile(filename, NULL, XML_PARSE_XINCLUDE);
#else
  doc = xmlParseFile(filename);
#endif
  return doc;
}

void
parse_module(xmlXPathContextPtr xpath, FILE * out)
{
  xmlChar * name = xmlGetProp(xpath->node, BAD_CAST "name");
  xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST "module", xpath);
  xmlNodePtr node;

  xmlXPathFreeObject(result);
  if (name) {
    fprintf(out, "  tolua_module(tolua_S, \"%s\", 0);\n", name);
    fprintf(out, "  tolua_beginmodule(tolua_S, \"%s\");\n", name);
    xmlFree(name);
  } else {
    fputs("  tolua_module(tolua_S, 0, 0);\n", out);
    fputs("  tolua_beginmodule(tolua_S, 0);\n", out);
  }

  for (node=xpath->node->children;node;node=node->next) {
    if (strcmp((const char *)node->name, "class")==0) {
      xmlChar * lname = xmlGetProp(node, BAD_CAST "name");
      xmlChar * name = xmlGetProp(node, BAD_CAST "ctype");
      xmlChar * base = xmlGetProp(node, BAD_CAST "base");
      const char * col = "NULL";
      fprintf(out, "  tolua_cclass(tolua_S, \"%s\", \"%s\", \"%s\", %s);\n",
        lname, name, base, col);
      xmlFree(lname);
      xmlFree(name);
      xmlFree(base);
    } else if (strcmp((const char *)node->name, "module")==0) {
      xpath->node = node;
      parse_module(xpath, out);
      xpath->node = node->parent;
    }
  }
  fputs("  tolua_endmodule(tolua_S);\n", out);
}

int
writefile(xmlDocPtr doc, FILE * out)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST "/package", xpath);
  xmlChar * pkg_name = xmlGetProp(doc->children, BAD_CAST "name");

  fputs(tmp_includes, out);
  fputc('\n', out);
  fprintf(out, "int tolua_%s_open(struct lua_State * L) {\n", pkg_name);
  xmlFree(pkg_name);

  fputs("  tolua_open(L);\n", out);

  result = xmlXPathEvalExpression(BAD_CAST "/package/type", xpath);
  if (result->nodesetval!=NULL) {
    int i;
    xmlNodeSetPtr nodes = result->nodesetval;
    for (i=0;i!=nodes->nodeNr;++i) {
      xmlChar * name = xmlGetProp(nodes->nodeTab[i], BAD_CAST "name");
      fprintf(out, "  tolua_usertype(tolua_S, \"%s\");\n", name);
    }
  }
  xmlXPathFreeObject(result);

  xpath->node = doc->children;
  parse_module(xpath, out);

  fputs("}\n", out);

  xmlXPathFreeContext(xpath);

  return 0;
}

int
main(int argc, char* argv[])
{
  xmlDocPtr doc;
  if (argc>1) {
    FILE * out = stdout;
    read_templates();
    doc = readfile(argv[1]);
    if (doc) {
      return writefile(doc, stdout);
    }
  }
  return 1;
}
