/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/
#include <config.h>
#include <xml.h>

#include <stdio.h>
#include <string.h>

static int 
cbplaintext(const struct xml_stack * stack, const char * c)
{
	puts(c);
	fflush(stdout);
	return XML_OK;
}

static int 
cbtagend(const struct xml_stack * stack)
{
	xml_tag * tag = stack->tag;
	printf("</%s>\n", tag->name);
	fflush(stdout);
	return XML_OK;
}

static int 
cbtagbegin(const struct xml_stack * stack)
{
	xml_tag * tag = stack->tag;
	xml_attrib * xa= tag->attribs;
	printf("<%s", tag->name);
	while (xa) {
		printf(" %s=\"%s\"", xa->name, xa->value);
		xa = xa->next;
	}
	printf(">\n");
	fflush(stdout);
	return XML_OK;
}

int 
main(int argc, char** argv)
{
	FILE * istream = stdin;
	int nretval = -1;
	if (argc>1) istream = fopen(argv[1], "rt+");
	if (istream) {
		xml_callbacks xml_cb = { NULL };
		xml_cb.plaintext = cbplaintext;
		xml_cb.tagbegin = cbtagbegin;

		nretval = xml_parse(istream, &xml_cb, NULL);
		if (istream!=stdin) fclose(istream);
		if (ostream!=stdout) fclose(ostream);
	}
	return nretval;
}
