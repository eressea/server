#include <config.h>
#include "xml.h"

/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int 
__cberror(const struct xml_stack * stack, const char* parsed, unsigned int line, int error, void *data)
{
	unused(data);
	fprintf(stderr, "Error #%d in line %u while parsing \"%s\"\n", -error, line, parsed);
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

int
xml_parse(FILE * stream, struct xml_callbacks * cb, void * data)
{
	xml_stack * stack = NULL;
	enum { TAG, ENDTAG, ATNAME, ATVALUE, PLAIN } state = PLAIN;
	char tokbuffer[1024];
	char * pos = tokbuffer;
	int quoted = 0;
	unsigned int line = 0;
	xml_tag * tag = NULL;
	xml_attrib * attrib = NULL;
	int (*cb_error)(const struct xml_stack*, const char*, unsigned int, int, void*) = __cberror;
	if (cb && cb->error) cb_error = cb->error;

	for (;;) {
		int reparse;
		int c = fgetc(stream);
		if (c=='\n') {
			++line;
		} else if (c==EOF) {
			if (state==PLAIN) {
				*pos='\0';
				if (cb && cb->plaintext && pos!=tokbuffer) cb->plaintext(stack, tokbuffer, data);
				break;
			} else {
				*pos='\0';
				return cb_error(stack, tokbuffer, line, XML_BROKENSTREAM, data);
			}
		}
		do {
			reparse = 0;
			switch (state) {
			case ATVALUE:
				switch (c) {
				case '<':
				case '/':
					*pos='\0';
					return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR, data);
				case '"':
					quoted = !quoted;
					break;
				case '>':
					if (quoted) {
						*pos='\0';
						return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR, data);
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
				case '>':
				case '<':
				case '/':
					*pos='\0';
					return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR, data);
				default:
					if (isspace(c)) {
						*pos='\0';
						return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR, data);
					}
					*pos++ = (char)c;
				}
				break; /* case ATNAME */
			case PLAIN:
				switch (c) {
				case '<':
					if (cb && cb->plaintext && pos!=tokbuffer) {
						*pos = '\0';
						cb->plaintext(stack, tokbuffer, data);
					}
					state = TAG;
					tag = NULL;
					pos = tokbuffer;
					break;
				case '>':
					*pos='\0';
					return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR, data);
				default:
					*pos++ = (char)c;
				}
				break; /* case PLAIN */
			case TAG:
				switch (c) {
				case '/':
					if (pos==tokbuffer) state = ENDTAG;
					else {
						*pos='\0';
						return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR, data);
					}
					break;
				case '>':
					if (tag==NULL) {
						*pos='\0';
						push_tag(&stack, make_tag(tokbuffer));
					}
					if (cb && cb->tagbegin) cb->tagbegin(stack, data);
					state = PLAIN;
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
					while (stack && strcmp(stack->tag->name, tokbuffer)!=0) free_tag(pop_tag(&stack));
					if (stack==NULL) return cb_error(stack, tokbuffer, line, XML_NESTINGERROR, data);
					if (cb && cb->tagend) cb->tagend(stack, data);
					free_tag(pop_tag(&stack));
					state = PLAIN;
					pos = tokbuffer;
					break;
				case '<':
				case ' ':
				case '=':
				case '/':
					*pos='\0';
					return cb_error(stack, tokbuffer, line, XML_INVALIDCHAR, data);
				default:
					*pos++ = (char)c;
				}
				break; /* case ENDTAG */
			} /* switch(state) */
		} while (reparse);
	}  /* for(;;) */
	return XML_OK; /* SUCCESS */
}

const char * 
xml_value(const xml_tag * tag, const char * name)
{
	const xml_attrib * xa = tag->attribs;
	while (xa && strcmp(name, xa->name)!=0) xa = xa->next;
	if (xa) return xa->value;
	return NULL;
}
