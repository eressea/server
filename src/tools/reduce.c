#include <eressea.h>
#include <region.h>
#include <save.h>

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

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

int
main(int argc, char ** argv)
{
	char * datadir = "data";
	char outfile[64];
	FILE * in = stdin;

	setlocale(LC_ALL, "");

	if (argc>1) {
		in = fopen(argv[1], "rt+");
		if (in==NULL) return -1;
	}

	initgame();
	readgame(false);

	markup(in);
	reduce();

	remove_empty_factions();
	sprintf(outfile, "%s/%d.cut", datadir, turn);
	writegame(outfile, 0);

	if (in!=stdin) fclose(in);
	return 0;
}
