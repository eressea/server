typedef struct xml_stack {
	struct xml_stack * next;
	struct xml_tag * tag;
} xml_stack;

typedef struct xml_tag {
	char * name;
	struct xml_attrib * attribs;
} xml_tag;

typedef struct xml_attrib {
	struct xml_attrib * next;
	char * name;
	char * value;
} xml_attrib;

#define XML_OK 0
#define XML_INVALIDCHAR -1
#define XML_NESTINGERROR -2
#define XML_BROKENSTREAM -3

/* callbacks */
typedef struct xml_callbacks {
	int (*plaintext)(const struct xml_stack *, const char*);
	int (*tagbegin)(const struct xml_stack *);
	int (*tagend)(const struct xml_stack *);
	int (*error)(const struct xml_stack *, const char*, unsigned int, int);
} xml_callbacks;

/* parser */
#include <stdio.h>
extern int xml_parse(FILE * stream, struct xml_callbacks *);

