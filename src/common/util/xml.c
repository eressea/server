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
#include "xml.h"

/* util includes */
#include "log.h"

/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct xml_hierarchy {
	const char * name;
	unsigned int flags;
	struct xml_callbacks * functions;
	struct xml_hierarchy * next;
	struct xml_hierarchy * children;
	struct xml_hierarchy * parent;
} xml_hierarchy;

static int 
__cberror(struct xml_stack * stack, const char* parsed, unsigned int line, int error)
{
	struct xml_stack * s = stack;
	log_error(("Error #%d in line %u while parsing \"%s\"\n", error, line, parsed));

	log_printf("XML stacktrace:\n");
	while (s) {
		if (s->tag && s->tag->name) log_printf("\t%s\n", s->tag->name);
		else log_printf("\t<unknown>\n");
		s = s->next;
	}
	return error;
}

static xml_tag * 
make_tag(const char * name)
{
	xml_tag * tag = calloc(sizeof(xml_tag), 1);
	tag->name = strdup(name);
	return tag;
}

static void
push_tag(xml_stack ** ostack, xml_tag * tag)
{
	xml_stack * stack = calloc(sizeof(xml_stack), 1);
	stack->next = *ostack;
	stack->tag = tag;
	if (*ostack) {
		stack->stream = (*ostack)->stream;
		stack->callbacks = (*ostack)->callbacks;
		stack->state = (*ostack)->state;
	}
	*ostack = stack;
}

static void
free_attribs(xml_attrib * xa)
{
	free(xa->name);
	free(xa->value);
	free(xa);
}

static void
free_tag(xml_tag * tag)
{
	while (tag->attribs) {
		xml_attrib * p = tag->attribs;
		tag->attribs = tag->attribs->next;
		free_attribs(p);
	}
	free(tag->name);
	free(tag);
}

static xml_attrib *
make_attrib(xml_tag * tag, const char * name)
{
	xml_attrib * xa = calloc(sizeof(xml_attrib), 1);
	xa->name = strdup(name);
	xa->next = tag->attribs;
	return tag->attribs = xa;
}

static xml_tag *
pop_tag(xml_stack ** ostack)
{
	xml_stack * stack = *ostack;
	xml_tag * tag = stack->tag;
	*ostack = stack->next;
	free(stack);
	return tag;
}

static xml_hierarchy * callbacks;

void 
xml_register(struct xml_callbacks * cb, const char * path, unsigned int flags)
{
	xml_hierarchy ** node=&callbacks;
	xml_hierarchy * parent = NULL;
	size_t len = strlen(path);

	for (;;) {
		const char * nextspace = path;
		while (*nextspace && !isspace(*nextspace)) ++nextspace;
		if (*nextspace == '\0') {
			/* advance to the last child: */
			while (*node && strcmp((*node)->name, path)!=0) node=&(*node)->next;
			if (*node == NULL) {
				*node = calloc(sizeof(xml_hierarchy), 1);
				(*node)->name = strdup(path);
				(*node)->parent = parent;
			} 
			(*node)->flags = flags;
			(*node)->functions = cb;
			break;
		} else {
			size_t lpath = nextspace-path;
			if (*node == NULL) {
				/* adding path into graph */
				*node = calloc(sizeof(xml_hierarchy), 1);
				(*node)->name = strncpy(calloc(sizeof(char), 1+lpath), path, lpath);
				(*node)->parent = parent;
				(*node)->functions = NULL;
				path = nextspace+1;
				while (isspace(*path)) ++path;
				assert(*path || !"trailing blanks in path");
				parent = *node;
				node=&parent->children;
			} else if (strlen((*node)->name)==lpath && strncmp((*node)->name, path, lpath)==0) {
				path = nextspace+1;
				while (isspace(*path)) ++path;
				assert(*path || !"trailing blanks in path");
				parent = *node;
				node=&parent->children;
			} else {
				node=&(*node)->next;
				break;
			}
		}
	}
}

static int
xml_parse(xml_stack * stack)
{
	FILE * stream = stack->stream;
	xml_stack * start = stack;
	enum { TAG, ENDTAG, ATNAME, ATVALUE, PLAIN } state = PLAIN;
	boolean startline = true;
	char tokbuffer[1024];
	char * pos = tokbuffer;
	int quoted = 0;
	unsigned int line = 0;
	xml_tag * tag = NULL;
	xml_attrib * attrib = NULL;
	int (*cb_error)(struct xml_stack*, const char*, unsigned int, int) = __cberror;

	for (;;) {
		int reparse;
		int c = fgetc(stream);
		if (c=='\n') {
			++line;
		} else if (c==EOF) {
			if (state==PLAIN) {
				const xml_hierarchy * cb = stack->callbacks;
				*pos='\0';
				if (cb && cb->functions->plaintext && pos!=tokbuffer) {
					cb->functions->plaintext(stack, tokbuffer);
				}
				break;
			} else {
				*pos='\0';
				return cb_error(stack, tokbuffer, line, XML_BROKENSTREAM);
			}
		}
		do {
			reparse = 0;
			switch (state) {
			case ATVALUE:
				switch (c) {
				case '<':
					*pos='\0';
					return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR);
				case '"':
					quoted = !quoted;
					break;
				case '>':
					if (quoted) {
						*pos='\0';
						return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR);
					}
					state = TAG;
					/* intentional fallthrough */
				default:
					if (quoted) *pos++ = (char)c;
					else {
						if (isspace(c) || c=='>') {
							assert(attrib || !"internal error");
							*pos='\0';
							attrib->value = strdup(tokbuffer);
							state = TAG;
							pos = tokbuffer;
							if (c=='>') reparse = 1;
						}
					}
				}
				break; /* case ATVALUE */
			case ATNAME:
				switch (c) {
				case '=':
					*pos='\0';
					assert(tag || !"internal error");
					attrib = make_attrib(tag, tokbuffer);
					state = ATVALUE;
					pos = tokbuffer;
					break;
				case '<':
				case '"':
				case '/':
					*pos='\0';
					return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR);
				default:
					if (isspace(c) || c == '>') {
						*pos++='\0';
						attrib = make_attrib(tag, tokbuffer);
						attrib->value = NULL;
						state = TAG;
						pos = tokbuffer;
						if (c=='>') reparse = 1;
						break;
					}
					*pos++ = (char)c;
				}
				break; /* case ATNAME */
			case PLAIN:
				switch (c) {
				case '<':
					if (pos!=tokbuffer) {
						const xml_hierarchy * cb = stack->callbacks;
						if (cb && cb->functions->plaintext) {
							*pos = '\0';
							cb->functions->plaintext(stack, tokbuffer);
						}
					}
					state = TAG;
					tag = NULL;
					pos = tokbuffer;
					break;
				case '>':
					*pos='\0';
					return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR);
				case '\n':
					/* ignore */
					if (!startline) *pos++ = ' ';
					startline = true;
					break;
				case ' ':
				case '\t':
					if (!startline) {
						*pos++ = (char)c;
					}
					break;
				default:
					*pos++ = (char)c;
					startline = false;
				}
				break; /* case PLAIN */
			case TAG:
				switch (c) {
				case '/':
					if (pos==tokbuffer) state = ENDTAG;
					else {
						*pos='\0';
						return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR);
					}
					break;
				case '>':
					if (tag==NULL) {
						*pos='\0';
						push_tag(&stack, tag = make_tag(tokbuffer));
					}
					{
						const xml_hierarchy * cnext = stack->callbacks?stack->callbacks->children:callbacks;
						while (cnext && strcmp(cnext->name, tag->name)!=0) cnext=cnext->next;
						if (cnext==NULL) {
							/* unknown tag. assume same handler again: */
							cnext = stack->callbacks;
						} else if ((cnext->flags & XML_CB_IGNORE) == 0) {
							stack->callbacks = cnext;
						}
						if (cnext->functions->tagbegin) {
							cnext->functions->tagbegin(stack);
						}
					}
					state = PLAIN;
					startline = true;
					pos = tokbuffer;
					break;
				default:
					if (isspace(c)) {
						if (tag==NULL) {
							*pos='\0';
							push_tag(&stack, tag = make_tag(tokbuffer));
							state = ATNAME;
							pos = tokbuffer;
						}
					} else {
						if (tag!=NULL) {
							state = ATNAME;
							pos = tokbuffer;
							reparse = 1;
						}
						else *pos++ = (char)c;
					}
				}
				break; /* case TAG */
			case ENDTAG:
				switch (c) {
				case '>':
					*pos = '\0';
					/* might be an unknown tag: */
					if (stack->callbacks) {
						/*  && strcmp(stack->callbacks->name, stack->tag->name)==0 */
						const xml_hierarchy * cb = stack->callbacks;
						if (strcmp(stack->tag->name, tokbuffer)!=0) {
							xml_stack * top = stack;
							cb_error(stack, tokbuffer, line, XML_NESTINGERROR);
							while (top && strcmp(top->tag->name, tokbuffer)!=0) top = top->next;
							if (top==NULL) return XML_NESTINGERROR;
							while (stack && stack!=top) {
								if (cb->functions->tagend) {
									cb->functions->tagend(stack);
								}
								free_tag(pop_tag(&stack));
								cb = stack->callbacks;
							}
						}
						if (cb->functions->tagend) {
							cb->functions->tagend(stack);
						}
						cb = cb->parent;
					}
					if (strcmp(stack->tag->name, tokbuffer)!=0) {
						return XML_NESTINGERROR;
					}
					free_tag(pop_tag(&stack));
					if (stack==start) {
						return XML_OK;
					}
					state = PLAIN;
					pos = tokbuffer;
					break;
				case '<':
				case ' ':
				case '=':
				case '/':
					*pos='\0';
					return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR);
				default:
					*pos++ = (char)c;
				}
				break; /* case ENDTAG */
			} /* switch(state) */
		} while (reparse);
	}  /* for(;;) */
	return XML_OK; /* SUCCESS */
}

int
xml_read(FILE * stream, xml_stack * stack)
{
	xml_stack root;
	FILE * save;
	if (!stack) {
		stack = &root;
		memset(stack, 0, sizeof(xml_stack));
	}
	save = stack->stream;
	stack->stream = stream;
	while (!feof(stream)) {
		int i = xml_parse(stack);
		if (i!=0) return i;
	}
	stack->stream = save;
	return XML_OK;
}

const char * 
xml_value(const xml_tag * tag, const char * name)
{
	const xml_attrib * xa = tag->attribs;
	while (xa && strcmp(name, xa->name)!=0) xa = xa->next;
	if (xa) return xa->value;
	return NULL;
}

int
xml_ivalue(const xml_tag * tag, const char * name)
{
	const xml_attrib * xa = tag->attribs;
	while (xa && strcmp(name, xa->name)!=0) xa = xa->next;
	if (xa) return atoi(xa->value);
	return 0;
}

boolean
xml_bvalue(const xml_tag * tag, const char * name)
{
	const xml_attrib * xa = tag->attribs;
	while (xa && strcmp(name, xa->name)!=0) xa = xa->next;
	if (xa) return true;
	return false;
}

double
xml_fvalue(const xml_tag * tag, const char * name)
{
	const xml_attrib * xa = tag->attribs;
	while (xa && strcmp(name, xa->name)!=0) xa = xa->next;
	if (xa) return atof(xa->value);
	return 0.0;
}
