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

#include <stdio.h>

typedef struct xml_stack {
	FILE * stream;
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
#define XML_INVALIDCHAR   1
#define XML_NESTINGERROR  2
#define XML_BROKENSTREAM  3
#define XML_USERERROR    10

/* callbacks */
typedef struct xml_callbacks {
	int (*plaintext)(struct xml_stack *, const char*, void *);
	int (*tagbegin)(struct xml_stack *, void *);
	int (*tagend)(struct xml_stack *, void *);
	int (*error)(struct xml_stack *, const char*, unsigned int, int, void *);
} xml_callbacks;

/* parser */
#include <stdio.h>
extern int xml_parse(FILE * stream, struct xml_callbacks *, void *, struct xml_stack *);
extern const char * xml_value(const struct xml_tag * tag, const char * name);
extern double xml_fvalue(const xml_tag * tag, const char * name);
extern int xml_ivalue(const xml_tag * tag, const char * name);
extern boolean xml_bvalue(const xml_tag * tag, const char * name);
