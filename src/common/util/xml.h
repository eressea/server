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

#ifndef H_UTIL_XML
#define H_UTIL_XML
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

struct xml_hierarchy;

#define XML_CB_IGNORE  1<<0

typedef struct xml_stack {
	FILE * stream;
	const struct xml_hierarchy * callbacks;
	struct xml_stack * next;
	struct xml_tag * tag;
	void * state;
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
	int (*tagbegin)(struct xml_stack *);
	int (*tagend)(struct xml_stack *);
	int (*plaintext)(struct xml_stack *, const char*);
} xml_callbacks;

/* parser */
#include <stdio.h>
extern void xml_register(struct xml_callbacks * cb, const char * parent, unsigned int flags);
extern int xml_read(FILE *, const char *filename, struct xml_stack * stack);
extern const char * xml_value(const struct xml_tag * tag, const char * name);
extern double xml_fvalue(const xml_tag * tag, const char * name);
extern int xml_ivalue(const xml_tag * tag, const char * name);
extern boolean xml_bvalue(const xml_tag * tag, const char * name);

#ifdef __cplusplus
}
#endif
#endif
