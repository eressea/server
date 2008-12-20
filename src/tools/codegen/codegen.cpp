/* libxml includes */
#include <libxml/tree.h>
#include <libxml/xpath.h>

/* libc includes */
#include <string.h>

const char * prefix = "tolua_";
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

static void
open_function(xmlNodePtr node, FILE * out)
{
  xmlChar * name = xmlGetProp(node, BAD_CAST "name");
  xmlNodePtr call;

  if (fname) {
    fprintf(out, "  tolua_function(tolua_S, \"%s\", %s);\n", name, fname);
  }
  xmlFree(name);
}

static void
open_variable(xmlNodePtr node, FILE * out)
{
  xmlChar * name = xmlGetProp(node, BAD_CAST "name");
  xmlChar * getter = BAD_CAST "0";
  xmlChar * setter = BAD_CAST "0";
  fprintf(out, "  tolua_variable(tolua_S, \"%s\", %s%s, %s%s);\n", name, prefix, getter, prefix, setter);
  xmlFree(name);
}

static void
open_method(xmlNodePtr node, FILE * out)
{
  xmlChar * name = xmlGetProp(node, BAD_CAST "name");
  xmlFree(name);
}

static void
open_class(xmlNodePtr node, FILE * out)
{
  xmlChar * lname = xmlGetProp(node, BAD_CAST "name");
  xmlChar * name = xmlGetProp(node, BAD_CAST "ctype");
  xmlChar * base = xmlGetProp(node, BAD_CAST "base");
  const char * col = "NULL";
  fprintf(out, "  tolua_cclass(tolua_S, \"%s\", \"%s\", \"%s\", %s);\n",
    lname, name, base, col);
  xmlFree(lname);
  xmlFree(name);
  xmlFree(base);
}

static void
open_module(xmlNodePtr root, FILE * out)
{
  xmlNodePtr node;
  xmlChar * name = xmlGetProp(root, BAD_CAST "name");
  if (name) {
    fprintf(out, "  tolua_module(tolua_S, \"%s\", 0);\n", name);
    fprintf(out, "  tolua_beginmodule(tolua_S, \"%s\");\n", name);
    xmlFree(name);
  } else {
    fputs("  tolua_module(tolua_S, 0, 0);\n", out);
    fputs("  tolua_beginmodule(tolua_S, 0);\n", out);
  }

  for (node=root->children;node;node=node->next) {
    if (strcmp((const char *)node->name, "class")==0) {
      open_class(node, out);
    } else if (strcmp((const char *)node->name, "module")==0) {
      open_module(node, out);
    } else if (strcmp((const char *)node->name, "function")==0) {
      open_function(node, out);
    } else if (strcmp((const char *)node->name, "method")==0) {
      open_method(node, out);
    } else if (strcmp((const char *)node->name, "variable")==0) {
      open_variable(node, out);
    }
  }
  fputs("  tolua_endmodule(tolua_S);\n", out);
}

static void
open_type(xmlNodePtr root, FILE * out)
{
  xmlChar * name = xmlGetProp(root, BAD_CAST "name");
  fprintf(out, "  tolua_usertype(tolua_S, \"%s\");\n", name);
  xmlFree(name);
}

void
open_package(xmlNodePtr root, FILE * out)
{
  xmlNodePtr node;
  xmlChar * pkg_name = xmlGetProp(root, BAD_CAST "name");
  fprintf(out, "int %s%s_open(struct lua_State * L) {\n", prefix, pkg_name);
  xmlFree(pkg_name);

  fputs("  tolua_open(L);\n", out);

  for (node=root->children;node;node=node->next) {
    if (strcmp((const char *)node->name, "type")==0) {
      open_type(node, out);
    } else if (strcmp((const char *)node->name, "module")==0) {
      open_module(node, out);
    }
  }
  fputs("  return 0;\n", out);
  fputs("}\n", out);
}

int
writefile(xmlDocPtr doc, FILE * out)
{
  /* includes etc. */
  fputs(tmp_includes, out);
  fputc('\n', out);
  /* functions */

  /* open */
  open_package(doc->children, out);
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
