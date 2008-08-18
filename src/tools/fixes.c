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
#include <eressea.h>

#include <attributes/attributes.h>
#include <attributes/key.h>
#include <spells/spells.h>
#include <triggers/triggers.h>
#include <items/weapons.h>
#include <items/items.h>

#include <modules/xmas2000.h>
#include <modules/arena.h>
#include <modules/museum.h>
#include <modules/gmcmd.h>

#include <item.h>
#include <faction.h>
#include <race.h>
#include <region.h>
#include <reports.h>
#include <resources.h>
#include <save.h>
#include <unit.h>
#include <plane.h>
#include <teleport.h>

#include <ctype.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern boolean quiet;

extern char *reportdir;
extern char *datadir;
extern char *basedir;
extern int maxregions;
char datafile[256];

extern char * g_datadir;
extern char * g_resourcedir;
extern char * g_basedir;

/**/

static struct attrib_type at_age = { 
	"age", NULL, NULL, NULL, NULL, NULL, ATF_UNIQUE 
};

/* make sure that this is done only once! */
#define do_once(magic, fun) \
{ \
	attrib * a = find_key(global.attribs, atoi36(magic)); \
	if (a) { \
		log_warning(("[do_once] a unique fix %d=\"%s\" was called a second time\n", atoi36(magic), magic)); \
	} else { \
		(fun); \
		a_add(&global.attribs, make_key(atoi36(magic))); \
	} \
}

#if NEW_RESOURCEGROWTH
static void
convert_resources(void)
{
	region *r;
	FILE * fixes = fopen("resource.fix", "w");
	log_open("resourcefix.log");
	for(r=regions;r;r=r->next) {
		attrib *a = a_find(r->attribs, &at_resources);
		r->resources = 0;
		terraform_resources(r);
		
		if (a==NULL) continue;
		else {
			int INIT_STONE = 20; /* skip this many weeks */
			double ironmulti = 0.40; 
			double laenmulti = 0.50; 
			double stonemulti = 0.30; /* half the stones used */
			rawmaterial * rmiron = rm_get(r, rm_iron.rtype);
			rawmaterial * rmlaen = rm_get(r, rm_laen.rtype);
			rawmaterial * rmstone = rm_get(r, rm_stones.rtype);

			int oldiron;
			int oldlaen = MAXLAENPERTURN * MIN(r->age, 100) / 2;
			int oldstone = terrain[rterrain(r)].quarries * MAX(0, r->age - INIT_STONE);
			int iron = a->data.sa[0];
			int laen = a->data.sa[1];
			int stone, level;
			int i, base;

			/** STONE **/
			for (i=0;terrain[r->terrain].rawmaterials[i].type;++i) {
				if (terrain[r->terrain].rawmaterials[i].type == &rm_stones) break;
			}
			if (terrain[r->terrain].rawmaterials[i].type) {
				base = terrain[r->terrain].rawmaterials[i].base;
				stone = max (0, (int)(oldstone * stonemulti));
				level = 1;
				base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
				while (stone >= base) {
					stone-=base;
					++level;
					base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
				}
				rmstone->level = level;
				rmstone->amount = base - stone;
				assert (rmstone->amount > 0);
				log_printf("CONVERSION: %d stones @ level %d in %s\n", rmstone->amount, rmstone->level, regionname(r, NULL));
			} else {
				log_error(("found stones in %s of %s\n", terrain[r->terrain].name, regionname(r, NULL)));
			}

			/** IRON **/
			if (r_isglacier(r) || r->terrain==T_ICEBERG) {
				oldiron = GLIRONPERTURN * MIN(r->age, 100) / 2;
			} else {
				oldiron = IRONPERTURN * r->age;
			}
			for (i=0;terrain[r->terrain].rawmaterials[i].type;++i) {
				if (terrain[r->terrain].rawmaterials[i].type == &rm_iron) break;
			}
			if (terrain[r->terrain].rawmaterials[i].type) {
				base = terrain[r->terrain].rawmaterials[i].base;
				iron = MAX(0, (int)(oldiron * ironmulti - iron ));
				level = 1;
				base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
				while (iron >= base) {
					iron-=base;
					++level;
					base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
				}
				rmiron->level = level;
				rmiron->amount = base - iron;
				assert (rmiron->amount > 0);
				log_printf("CONVERSION: %d iron @ level %d in %s\n", rmiron->amount, rmiron->level, regionname(r, NULL));
			} else {
				log_error(("found iron in %s of %s\n", terrain[r->terrain].name, regionname(r, NULL)));
			}

			/** LAEN **/
			if (laen>=0) {
				if (rmlaen==NULL) {
					rmlaen = calloc(sizeof(rawmaterial), 1);
					rmlaen->next = r->resources;
					r->resources = rmlaen;
				}
				for (i=0;terrain[r->terrain].rawmaterials[i].type;++i) {
					if (terrain[r->terrain].rawmaterials[i].type == &rm_laen) break;
				}
				if (terrain[r->terrain].rawmaterials[i].type) {
					laen = MAX(0, (int)(oldlaen * laenmulti - laen));
					level = 1;
					base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
					while (laen >= base) {
						laen-=base;
						++level;
						base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
					}
					rmlaen->level = level;
					rmlaen->amount = base - laen;
					assert(rmlaen->amount>0);
					log_printf("CONVERSION: %d laen @ level %d in %s\n", rmlaen->amount, rmlaen->level, regionname(r, NULL));
					rmlaen = NULL;
				}
			} 
			if (rmlaen) {
				struct rawmaterial *res;
				struct rawmaterial ** pres = &r->resources;
				if (laen!=-1) log_error(("found laen in %s of %s\n", terrain[r->terrain].name, regionname(r, NULL)));
				while (*pres!=rmlaen) pres = &(*pres)->next;
				res = *pres;
				*pres = (*pres)->next;
				free(res);
			}
#ifndef NDEBUG
			{
				rawmaterial * res = r->resources;
				while (res) {
					assert(res->amount>0);
					assert(res->level>0);
					res = res->next;
				}
			}
#endif
			{
				rawmaterial * rmiron = rm_get(r, rm_iron.rtype);
				rawmaterial * rmlaen = rm_get(r, rm_laen.rtype);
				rawmaterial * rmstone = rm_get(r, rm_stones.rtype);
				fprintf(fixes, "%d %d %d %d %d %d %d %d %d\n", 
				r->x, r->y, r->age, 
				(rmstone)?rmstone->amount:-1, (rmiron)?rmiron->amount:-1, (rmlaen)?rmlaen->amount:-1, 
				(rmstone)?rmstone->level:-1, (rmiron)?rmiron->level:-1, (rmlaen)?rmlaen->level:-1);
			}
		}
	}
	log_close();
	fclose(fixes);
}
#endif

static void
fix_age(void)
{
	region * r;
	int c;
	for (r=regions;r;r=r->next) {
		const unit * u;
		attrib * a = a_find(r->attribs, &at_age);
		if (a==NULL) {
			a = a_add(&r->attribs, a_new(&at_age));
		}

		for (u=r->units;u;u=u->next) {
			faction * f = u->faction;
			if (f->age<=r->age) {
				a->data.sa[0]+= f->age;
				a->data.sa[1]++;
				if (a->data.sa[0]>0x7000) {
					a->data.sa[0] /= 10;
					a->data.sa[1] /= 10;
				}
			}
		}
	}
#define STEPS 3
	for (c=0;c!=STEPS;++c) for (r=regions;r;r=r->next) {
		attrib * a = a_find(r->attribs, &at_age);
		int x = 0;
		direction_t i;
		int age = a->data.sa[0], div = a->data.sa[1];
		if (age) ++x;
		for (i=0;i!=MAXDIRECTIONS;++i) {
			region * nr = rconnect(r, i);
			if (nr!=NULL) {
				attrib * na = a_find(nr->attribs, &at_age);
				age += na->data.sa[0];
				div += na->data.sa[1];
				++x;
			}
		}
		if (x) {
			a->data.sa[0] = (short)(age / x);
			a->data.sa[1] = (short)(div / x);
		}
	}

	log_open("agefix.log");
	for (r=regions;r;r=r->next) {
		attrib * a = a_find(r->attribs, &at_age);
		int age = a->data.sa[0];
		if (a->data.sa[1]==0) continue;
		age /= a->data.sa[1];
		if (age*100<r->age*90) {
			log_printf("AGE: from %d to %d in region %s\n", r->age, age, regionname(r, NULL));
			r->age = age;
		}
	}
	log_close();
}

static void
fixit(void)
{
	fix_age();
	do_once("rgrw", convert_resources());
}

int
main(int argc, char *argv[])
{
	char zText[MAX_PATH];
	boolean backup = true;
	boolean save = true;
	int i = 1;

	setlocale(LC_ALL, "");

	*datafile = 0;

	while (i < argc && argv[i][0] == '-') {
		switch (argv[i][1]) {
		case 't':
			if (argv[i][2])
				turn = atoi((char *)(argv[i] + 2));
			else
				turn = atoi(argv[++i]);
			break;
		case 'x':
			maxregions = atoi(argv[++i]);
			maxregions = (maxregions*81+80) / 81;
			break;
		case 'q': quiet = true; break;
		case 'n':
			switch (argv[i][2]) {
			case 'b' : backup = false; break;
			case 's' : save = false; break;
			}
			break;
		case 'd':
			g_datadir = argv[++i];
			break;
		case 'r':
			g_resourcedir = argv[++i];
			break;
		case 'o':
			strcpy(datafile, argv[++i]);
			break;
		case 'b':
			g_basedir = argv[++i];
			break;
		}
		i++;
	}

	kernel_init();

	init_triggers();

	register_races();
	register_spells();

	debug_language("locales.log");
	sprintf(zText, "%s/%s", resourcepath(), "eressea.xml");
	init_data(zText);
	init_locales();

	init_attributes();
	init_resources();
	register_items();

	init_rawmaterials();
	init_museum();
	init_arena();
	init_xmas2000();

	init_gmcmd();

	if(!*datafile)
		sprintf(datafile, "%s/%d", datapath(), turn);

	readgame(backup);

	fixit();

	if (save) writegame(datafile, 1);
	return 0;
}
