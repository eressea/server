/* vi: set ts=2 ai sw=2
 *
 *	$Id: filter.c,v 1.2 2001/01/26 16:19:41 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1997-99
 *		Enno Rehling (rehling@usa.net)
 *		Christian Schlittchen (corwin@amber.kn-bremen.de)
 *		Katja Zedel (katze@felidae.kn-bremen.de)
 *		Henning Peters (faroul@gmx.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed. It may not be
 * sold or used commercially without prior written permission from the
 * authors.
 *
 * Eressea PBeM CR Filter
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct tag {
	struct tag * next;
	char * name;
} tag;

typedef struct section {
	struct section * next;
	char * name;
	tag * tags;
} section;

section *
read_tags(FILE * in)
{
	char buf[64];
	section * sec = NULL;

	while (fgets(buf, 64, in)) {
		size_t end = strlen(buf);
		if (end && buf[end-1]=='\n') buf[end-1] = ' ';
		if (buf[0]=='@') {
			section * last = sec;
			sec = (section *) calloc(1, sizeof(section));
			sec->next = last;
			sec->name = (char*)calloc(strlen(buf), sizeof(char));
			strcpy(sec->name, buf+1);
		} else if (sec) {
			tag * last = sec->tags;
			sec->tags = (tag *) calloc(1, sizeof(tag));
			sec->tags->next = last;
			sec->tags->name = (char*)calloc(strlen(buf)+1, sizeof(char));
			strcpy(sec->tags->name, buf);
		}
	}
	return sec;
}

void
filter_stream(FILE * in, FILE * out, section * sec)
{
	char buf[8092];
	section * s = NULL;
	while (fgets(buf, 8092, in)) {
		section * c;
		int found = 0;
		size_t end = strlen(buf);
		if (end && buf[end-1]=='\n') buf[end-1] = ' ';
		for (c = sec;c;c=c->next) {
			if (!strncmp(buf, c->name, min(strlen(buf), strlen(c->name)))) {
				s = c;
				fprintf(out, "%s\n", buf);
				found = 1;
				break;
			}
		}
		if (s && !found) {
			if (strchr(buf, ';')) {
				tag * t;
				for (t=s->tags;t;t = t->next) {
					size_t len = strlen(t->name);
					if (!strcmp(buf+strlen(buf)-len, t->name)) {
						fprintf(out, "%s\n", buf);
						found = 1;
						break;
					}
				}
			} else s = NULL;
		}
	}
}

int
usage(const char* message)
{
	fprintf(stderr, "usage: filter <filter.crf> <input.cr> <output.cr>\n");
	if (message) fprintf(stderr, "\nERROR: %s\n", message);
	return -1;
}

int
main(int argc, char** argv)
{
	FILE * filter;
	FILE * in = stdin;
	FILE * out = stdout;
	section * sec;
	if (argc<1) return usage(0);

	filter = fopen(argv[1], "rt");
	if (!filter) usage("cannot open filter definitions");

	sec = read_tags(filter);

	if (argc>2) {
		in = fopen(argv[2], "rt");
		if (!in) usage("cannot open input file");
	}

	if (argc>3) {
		out = fopen(argv[3], "wt+");
		if (!out) usage("cannot open output file");
	}

	filter_stream(in, out, sec);
}
