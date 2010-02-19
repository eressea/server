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
#include <kernel/eressea.h>

#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/save.h>

#include <util/base36.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

static char * orderfile = NULL;

static void mark_region(region * r, boolean mark)
{
	if (!r) return;
	if (mark) fset(r, FL_MARK);
	else freset(r, FL_MARK);
}

static void 
markup(FILE * in)
{
	char command[16];
	char choice[16];
	int x, y;
	region * r;
	boolean mark;

	while (!feof(in)) {
		fscanf(in, "%s %s", choice, command);
		if (choice[0]=='#') {
			fgets(buf, sizeof(buf), in);
			continue;
		}
		if (choice[0]=='e') return; /* end */
		if (choice[0]=='o') {
			orderfile = strdup(command);
			readorders(orderfile);
			continue;
		}
		mark = (choice[0]=='a'); /* add/del */
		switch (command[0]) {
		case 'r':
			if (!strcmp(command, "region")) {
				fscanf(in, "%d %d", &x, &y);
				r = findregion(x, y);
				if (r==NULL)
					printf("region %d,%d not found\n", x, y);
				else mark_region(r, mark);
			}
			break;
		case 'a':
			if (!strcmp(command, "area")) {
				int radius;
				fscanf(in, "%d %d %d", &x, &y, &radius);
				for (r=regions;r!=NULL;r=r->next) {
					if (koor_distance(x, y, r->x, r->y)<=radius)
						mark_region(r, mark);
				}
			} else if (!strcmp(command, "all")) {
				for (r==regions;r!=NULL;r=r->next) {
					mark_region(r, mark);
				}
			}
			break;
		}
	}
}

static void
reduce(void)
{
	faction * f;
	region ** rp = &regions;
	/* let's leak regions, big time. */
	for (f=factions;f;f=f->next) f->alive=0;
	while (*rp) {
		region * r = *rp;
		if (fval(r, FL_MARK)) {
			unit * u;
			for (u=r->units;u;u=u->next) u->faction->alive=1;
			rp=&r->next;
		}
		else *rp = r->next;
	}
}

static int
usage(void)
{
	fputs("usage: reduce [infile]\n", stderr);
	fputs("\ninput file syntax:\n"
		"\torders orderfile\n"
		"\tadd all\n"
		"\tadd region x y\n"
		"\tadd area x y r\n"
		"\tdel all\n"
		"\tdel region x y\n"
		"\tdel area x y r\n"
		"\tend\n", stderr);
	return -1;
}

static void
writeorders(const char * orderfile)
{
	FILE * F = fopen(orderfile, "wt");
	faction * f;
	if (F==NULL) return;

	/* let's leak regions, big time. */
	for (f=factions;f;f=f->next) {
		region * r;
		if (f->alive==0) continue;
		fprintf(F, "PARTEI %s \"%s\"\n", itoa36(f->no), f->passw);
		for (r=regions;r;r=r->next) {
			if (fval(r, FL_MARK)) {
				unit * u;
				for (u=r->units;u;u=u->next) {
					strlist * o;
					fprintf(F, "EINHEIT %s\n", itoa36(u->no));
					for (o=u->orders;o;o=o->next) {
						fputs(o->s, F);
					}
				}
			}
		}
	}
	fclose(F);
}

int
main(int argc, char ** argv)
{
	char * datadir = "data";
	char outfile[64];
	FILE * in = stdin;

	setlocale(LC_ALL, "");

	if (argc>1) {
		in = fopen(argv[1], "rt+");
		if (in==NULL) return usage();
	} 

	kernel_init();
	readgame(false);

	markup(in);
	reduce();

	remove_empty_factions();
	sprintf(outfile, "%s.cut", orderfile);
	if (orderfile) {
		writeorders(orderfile);
	}
	sprintf(outfile, "%s/%d.cut", datadir, turn);
	writegame(outfile, 0);

	if (in!=stdin) fclose(in);
	return 0;
}
