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
