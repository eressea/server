/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include "eressea.h"
#include "building.h"

/* kernel includes */
#include "item.h"
#include "curse.h" /* für C_NOCOST */
#include "unit.h"
#include "region.h"
#include "skill.h"
#include "save.h"

/* util includes */
#include <base36.h>
#include <resolve.h>
#include <event.h>
#include <language.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* attributes includes */
#include <attributes/matmod.h>

building_typelist *buildingtypes;

const building_type *
bt_find(const char* name)
{
	const struct building_typelist * btl = buildingtypes;

	if (global.data_version < RELEASE_VERSION) {
		const char * translation[3][2] = { 
			{ "illusion", "illusioncastle" }, 
			{ "generic", "genericbuilding" }, 
			{ NULL, NULL } 
		};
		int i;
		for (i=0;translation[i][0];++i) {
			/* calling a building "illusion" was a bad idea" */
			if (strcmp(translation[i][0], name)==0) {
				name = translation[i][1];
				break;
			}
		}
	}
	while (btl && strcasecmp(btl->type->_name, name)) btl = btl->next;
	if (!btl) {
		btl = buildingtypes;
		while (btl && strncasecmp(btl->type->_name, name, strlen(name))) btl = btl->next;
	}
	return btl?btl->type:NULL;
}

void
bt_register(const building_type * type)
{
	struct building_typelist * btl = malloc(sizeof(building_type));
	btl->type = type;
	btl->next = buildingtypes;
	buildingtypes = btl;
}

int
buildingcapacity(const building * b)
{
	if (b->type->capacity>=0) {
		if (b->type->maxcapacity>=0)
			return min(b->type->maxcapacity, b->size * b->type->capacity);
		return b->size * b->type->capacity;
	}
	if (b->size>=b->type->maxsize) return b->type->maxcapacity;
	return 0;
}

attrib_type at_building_generic_type = {
	  "building_generic_type", NULL, NULL, NULL, a_writestring, a_readstring, ATF_UNIQUE
};

const char *
buildingtype(const building * b, int bsize)
{
	const char * s = NULL;
	const building_type * btype = b->type;

	if (btype == &bt_generic) {
		const attrib *a = a_find(b->attribs, &at_building_generic_type);
		if (a) s = (const char*)a->data.v;
	}

	if (btype->name) s = btype->name(bsize);
	if (s==NULL) s = btype->_name;
	return s;
}

int
buildingmaintenance(const building * b, resource_t rtype)
{
	const building_type * bt = b->type;
	int c, cost=0;
	if (is_cursed(b->attribs, C_NOCOST, 0)) {
		return 0;
	}
	for (c=0;bt->maintenance && bt->maintenance[c].number;++c) {
		const maintenance * m = bt->maintenance + c;
		if (m->type==rtype) {
			if (fval(m, MTF_VARIABLE))
				cost += (b->size * m->number);
			else
				cost += m->number;
		}
	}
	return cost;
}

#define BMAXHASH 8191
static building *buildhash[BMAXHASH];
void
bhash(building * b)
{
	building *old = buildhash[b->no % BMAXHASH];

	buildhash[b->no % BMAXHASH] = b;
	b->nexthash = old;
}

void
bunhash(building * b)
{
	building **show;

	for (show = &buildhash[b->no % BMAXHASH]; *show; show = &(*show)->nexthash) {
		if ((*show)->no == b->no)
			break;
	}
	if (*show) {
		assert(*show == b);
		*show = (*show)->nexthash;
		b->nexthash = 0;
	}
}

static building *
bfindhash(int i)
{
	building *old;

	for (old = buildhash[i % BMAXHASH]; old; old = old->nexthash)
		if (old->no == i)
			return old;
	return 0;
}

building *
findbuilding(int i)
{
	return bfindhash(i);
}
/* ** old building types ** */


/** Building: Fortification */
#ifdef NEW_BUILDINGS
enum {
	B_SITE,
#if LARGE_CASTLES
	B_TRADEPOST,
#endif
	B_FORTIFICATION,
	B_TOWER,
	B_CASTLE,
	B_FORTRESS,
	B_CITADEL,
	MAXBUILDINGS
};
#endif

static const char *
castle_name(int bsize)
{
	const char * fname[MAXBUILDINGS] = {
	  "site",
#if LARGE_CASTLES
		"tradepost",
#endif
	  "fortification",
	  "tower",
	  "castle",
	  "fortress",
	  "citadel" };
	const construction * ctype = bt_castle.construction;
	int i = 0;
	while (ctype &&
			 ctype->maxsize != -1
			 && ctype->maxsize<=bsize) {
		bsize-=ctype->maxsize;
		ctype=ctype->improvement;
		++i;
	}
	return fname[i];
}

static requirement castle_req[] = {
	{ R_STONE, 1, 0.5 },
	{ NORESOURCE, 0, 0.0 },
};

#if LARGE_CASTLES
static construction castle_bld[MAXBUILDINGS] = {
	{ SK_BUILDING, 1,     2, 1, castle_req, &castle_bld[1] },
	{ SK_BUILDING, 1,     8, 1, castle_req, &castle_bld[2] },
	{ SK_BUILDING, 2,    40, 1, castle_req, &castle_bld[3] },
	{ SK_BUILDING, 3,   200, 1, castle_req, &castle_bld[4] },
	{ SK_BUILDING, 4,  1000, 1, castle_req, &castle_bld[5] },
	{ SK_BUILDING, 5,  5000, 1, castle_req, &castle_bld[6] },
	{ SK_BUILDING, 6,    -1, 1, castle_req, NULL }
};
#else
static construction castle_bld[MAXBUILDINGS] = {
	{ SK_BUILDING, 1,    2, 1, castle_req, &castle_bld[1] },
	{ SK_BUILDING, 2,    8, 1, castle_req, &castle_bld[2] },
	{ SK_BUILDING, 3,   40, 1, castle_req, &castle_bld[3] },
	{ SK_BUILDING, 4,  200, 1, castle_req, &castle_bld[4] },
	{ SK_BUILDING, 5, 1000, 1, castle_req, &castle_bld[5] },
	{ SK_BUILDING, 6,   -1, 1, castle_req, NULL }
};
#endif

building_type bt_castle = {
	"castle",
	BFL_NONE,
	1, 4, -1,
	NULL,
	&castle_bld[0],
	castle_name
};


/** Building: Lighthouse */
static const maintenance lighthouse_keep[] = {
	{ R_SILVER, 100, MTF_VITAL },
	{ NORESOURCE, 0, MTF_NONE },
};
static requirement lighthouse_req[] = {
	{ R_IRON, 1, 0.5 },
	{ R_WOOD, 1, 0.5 },
	{ R_STONE, 2, 0.5 },
	{ R_SILVER, 100, 0.0 },
	{ NORESOURCE, 0, 0.0 },
};
static const construction lighthouse_bld = {
	SK_BUILDING, 3,
	-1, 1, lighthouse_req,
	NULL
};
building_type bt_lighthouse = {
	"lighthouse",
	BFL_NONE,
	1, 4, -1,
	lighthouse_keep,
	&lighthouse_bld
};


/** Building: Mine */
static const maintenance mine_keep[] = {
	/* resource, number, flags */
	{ R_SILVER, 500, MTF_VITAL },
	{ NORESOURCE, 0, MTF_NONE },
};
static requirement mine_req[] = {
	/* resource, number, recycle */
	{ R_IRON, 1, 0.5 },
	{ R_WOOD, 10, 0.5 },
	{ R_STONE, 5, 0.5 },
	{ R_SILVER, 250, 0.0 },
	{ NORESOURCE, 0, 0.0 },
};
static const construction mine_bld = {
	SK_BUILDING, 4, /* skill, minskill */
	-1, 1, mine_req,    /* maxsize, reqsize, required */
	NULL            /* improvement */
};
building_type bt_mine = {
	"mine",    /* _name */
	BFL_NONE,  /* flags */
	1, -1, -1, /* capac, maxcap, maxsize */
	mine_keep, /* maintenance */
	&mine_bld, /* construction */
	NULL       /* name() */
};


/** Building: Quarry */
static const maintenance quarry_keep[] = {
	/* resource, number, flags */
	{ R_SILVER, 250, MTF_VITAL },
	{ NORESOURCE, 0, MTF_NONE },
};
static requirement quarry_req[] = {
	/* resource, number, recycle */
	{ R_IRON, 1, 0.5 },
	{ R_WOOD, 5, 0.5 },
	{ R_STONE, 1, 0.5 },
	{ R_SILVER, 250, 0.0 },
	{ NORESOURCE, 0, 0.0 },
};
static const construction quarry_bld = {
	SK_BUILDING, 2, /* skill, minskill */
	-1, 1, quarry_req, /* maxsize, reqsize, required */
	NULL            /* improvement */
};
building_type bt_quarry = {
	"quarry",    /* _name */
	BFL_NONE,  /* flags */
	1, -1, -1, /* capac, maxcap, maxsize */
	quarry_keep, /* maintenance */
	&quarry_bld, /* construction */
	NULL       /* name() */
};


/** Building: harbour */
static const maintenance harbour_keep[] = {
	/* resource, number, flags */
	{ R_SILVER, 250, MTF_VITAL },
	{ NORESOURCE, 0, MTF_NONE },
};
static requirement harbour_req[] = {
	/* resource, number, recycle */
	{ R_WOOD, 125, 0.5 },
	{ R_STONE, 125, 0.5 },
	{ R_SILVER, 6250, 0.0 },
	{ NORESOURCE, 0, 0.0 },
};
static const construction harbour_bld = {
	SK_BUILDING, 3, /* skill, minskill */
	25, 25, harbour_req,  /* maxsize, reqsize, required for size */
	NULL            /* improvement */
};
building_type bt_harbour = {
	"harbour",     /* _name */
	BFL_NONE,     /* flags */
	1, 25, 25,    /* capac/size, maxcapac, maxsize */
	harbour_keep,  /* maintenance */
	&harbour_bld,  /* construction */
	NULL          /* name() */
};


/** Building: academy */
static const maintenance academy_keep[] = {
	/* resource, number, flags */
	{ R_SILVER, 1000, MTF_VITAL },
	{ NORESOURCE, 0, MTF_NONE },
};
static requirement academy_req[] = {
	/* resource, number, recycle */
	{ R_WOOD, 125, 0.5 },
	{ R_STONE, 125, 0.5 },
	{ R_IRON, 25, 0.0 },
	{ R_SILVER, 12500, 0.0 },
	{ NORESOURCE, 0, 0.0 },
};
static const construction academy_bld = {
	SK_BUILDING, 3, /* skill, minskill */
	25, 25, academy_req,  /* maxsize, reqsize, required for size */
	NULL            /* improvement */
};
building_type bt_academy = {
	"academy",     /* _name */
	BFL_NONE,     /* flags */
	-1, 25, 25,    /* capac/size, maxcapac, maxsize */
	academy_keep,  /* maintenance */
	&academy_bld,  /* construction */
	NULL          /* name() */
};


/** Building: magictower */
static const maintenance magictower_keep[] = {
	/* resource, number, flags */
	{ R_SILVER, 1000, MTF_VITAL },
	{ NORESOURCE, 0, MTF_NONE },
};
static requirement magictower_req[] = {
	/* resource, number, recycle */
	{ R_WOOD, 150, 0.5 },
	{ R_STONE, 250, 0.5 },
	{ R_MALLORN, 100, 0.5 },
	{ R_IRON, 150, 0.5 },
	{ R_EOG, 100, 0.5 },
	{ R_SILVER, 25000, 0.0 },
	{ NORESOURCE, 0, 0.0 },
};

static const construction magictower_bld = {
	SK_BUILDING, 5, /* skill, minskill */
	50, 50, magictower_req,  /* maxsize, reqsize, required for size */
	NULL            /* improvement */
};
building_type bt_magictower = {
	"magictower",     /* _name */
	BFL_NONE,     /* flags */
	-1, 2, 50,    /* capac/size, maxcapac, maxsize */
	magictower_keep,  /* maintenance */
	&magictower_bld,  /* construction */
	NULL          /* name() */
};


/** Building: smithy */
static const maintenance smithy_keep[] = {
	/* resource, number, flags */
	{ R_SILVER, 300, MTF_VITAL },
	{ R_WOOD, 1, MTF_NONE },
	{ NORESOURCE, 0, MTF_NONE },
};
static requirement smithy_req[] = {
	/* resource, number, recycle */
	{ R_WOOD, 5, 0.5 },
	{ R_STONE, 5, 0.5 },
	{ R_IRON, 2, 0.5 },
	{ R_SILVER, 200, 0.0 },
	{ NORESOURCE, 0, 0.0 },
};
static const construction smithy_bld = {
	SK_BUILDING, 3,   /* skill, minskill */
	-1, 1, smithy_req,  /* maxsize, reqsize, required for size */
	NULL              /* improvement */
};
building_type bt_smithy = {
	"smithy",     /* _name */
	BFL_NONE,     /* flags */
	1, -1, -1,    /* capac/size, maxcapac, maxsize */
	smithy_keep,  /* maintenance */
	&smithy_bld,  /* construction */
	NULL          /* name() */
};


/** Building: sawmill */
static const maintenance sawmill_keep[] = {
	/* resource, number, flags */
	{ R_SILVER, 250, MTF_VITAL },
	{ NORESOURCE, 0, MTF_NONE },
};
static requirement sawmill_req[] = {
	/* resource, number, recycle */
	{ R_WOOD, 5, 0.5 },
	{ R_STONE, 5, 0.5 },
	{ R_IRON, 3, 0.5 },
	{ R_SILVER, 200, 0.0 },
	{ NORESOURCE, 0, 0.0 },
};
static const construction sawmill_bld = {
	SK_BUILDING, 3,   /* skill, minskill */
	-1, 1, sawmill_req,    /* maxsize, reqsize, required for size */
	NULL              /* improvement */
};
building_type bt_sawmill = {
	"sawmill",     /* _name */
	BFL_NONE,     /* flags */
	1, -1, -1,    /* capac/size, maxcapac, maxsize */
	sawmill_keep,  /* maintenance */
	&sawmill_bld,  /* construction */
	NULL          /* name() */
};


/** Building: stables */
static const maintenance stables_keep[] = {
	/* resource, number, flags */
	{ R_SILVER, 150, MTF_VITAL },
	{ NORESOURCE, 0, MTF_NONE },
};
static requirement stables_req[] = {
	/* resource, number, recycle */
	{ R_WOOD, 4, 0.5 },
	{ R_STONE, 2, 0.5 },
	{ R_IRON, 1, 0.5 },
	{ R_SILVER, 100, 0.0 },
	{ NORESOURCE, 0, 0.0 },
};
static const construction stables_bld = {
	SK_BUILDING, 2,   /* skill, minskill */
	-1, 1, stables_req,    /* maxsize, reqsize, required for size */
	NULL              /* improvement */
};
building_type bt_stables = {
	"stables",     /* _name */
	BFL_NONE,     /* flags */
	1, -1, -1,    /* capac/size, maxcapac, maxsize */
	stables_keep,  /* maintenance */
	&stables_bld,  /* construction */
	NULL          /* name() */
};


static requirement monument_req[] = {
	/* resource, number, recycle */
	{ R_WOOD,     1, 0.5 },
	{ R_STONE,    1, 0.5 },
	{ R_IRON,     1, 0.5 },
	{ R_SILVER, 400, 0.0 },
	{ NORESOURCE, 0, 0.0 },
};
static const construction monument_bld = {
	SK_BUILDING, 4,   /* skill, minskill */
	-1, 1, monument_req,    /* maxsize, reqsize, required for size */
	NULL              /* improvement */
};
building_type bt_monument = {
	"monument",     /* _name */
	BFL_NONE,     /* flags */
	1, -1, -1,    /* capac/size, maxcapac, maxsize */
	NULL,  /* maintenance */
	&monument_bld,  /* construction */
	NULL          /* name() */
};


/** Building: dam */
static const maintenance dam_keep[] = {
	/* resource, number, flags */
	{ R_WOOD,      3, MTF_NONE },
	{ R_SILVER, 1000, MTF_VITAL },
	{ NORESOURCE,  0, MTF_NONE },
};
static requirement dam_req[] = {
	/* resource, number, recycle */
	{ R_IRON,      50, 0.5 },
	{ R_WOOD,     500, 0.5 },
	{ R_STONE,    250, 0.5 },
	{ R_SILVER, 25000, 0.0 },
	{ NORESOURCE,   0, 0.0 },
};
static const construction dam_bld = {
	SK_BUILDING, 4,   /* skill, minskill */
	50, 50, dam_req,    /* maxsize, reqsize, required for size */
	NULL              /* improvement */
};
building_type bt_dam = {
	"dam",     /* _name */
	BFL_NONE,     /* flags */
	1, -1, 50,    /* capac/size, maxcapac, maxsize */
	dam_keep,  /* maintenance */
	&dam_bld,  /* construction */
	NULL          /* name() */
};


/** Building: caravan */
static const maintenance caravan_keep[] = {
	/* resource, number, flags */
	{ R_HORSE,     2, MTF_NONE },
	{ R_SILVER, 3000, MTF_VITAL },
	{ NORESOURCE,  0, MTF_NONE },
};
static requirement caravan_req[] = {
	/* resource, number, recycle */
	{ R_IRON,     10, 0.5 },
	{ R_WOOD,     50, 0.5 },
	{ R_STONE,    10, 0.5 },
	{ R_SILVER, 5000, 0.0 },
	{ NORESOURCE,  0, 0.0 },
};
static const construction caravan_bld = {
	SK_BUILDING, 2,   /* skill, minskill */
	10, 10, caravan_req,    /* maxsize, reqsize, required for size */
	NULL              /* improvement */
};
building_type bt_caravan = {
	"caravan",     /* _name */
	BFL_NONE,     /* flags */
	1, -1, 10,    /* capac/size, maxcapac, maxsize */
	caravan_keep,  /* maintenance */
	&caravan_bld,  /* construction */
	NULL          /* name() */
};


/** Building: tunnel */
static const maintenance tunnel_keep[] = {
	/* resource, number, flags */
	{ R_STONE,     2, MTF_NONE },
	{ R_SILVER,  100, MTF_VITAL },
	{ NORESOURCE,  0, MTF_NONE },
};
static requirement tunnel_req[] = {
	/* resource, number, recycle */
	{ R_IRON,    100, 0.5 },
	{ R_WOOD,    500, 0.5 },
	{ R_STONE,  1000, 0.5 },
	{ R_SILVER,30000, 0.0 },
	{ NORESOURCE,  0, 0.0 },
};
static const construction tunnel_bld = {
	SK_BUILDING, 6,   /* skill, minskill */
	100, 100, tunnel_req,    /* maxsize, reqsize, required for size */
	NULL              /* improvement */
};
building_type bt_tunnel = {
	"tunnel",     /* _name */
	BFL_NONE,     /* flags */
	1, -1, 100,   /* capac/size, maxcapac, maxsize */
	tunnel_keep,  /* maintenance */
	&tunnel_bld,  /* construction */
	NULL          /* name() */
};


/** Building: inn */
static const maintenance inn_keep[] = {
	/* resource, number, flags */
	{ R_SILVER,    5, MTF_VARIABLE|MTF_VITAL },
	{ NORESOURCE,  0, MTF_NONE },
};
static requirement inn_req[] = {
	/* resource, number, recycle */
	{ R_IRON,     10, 0.5 },
	{ R_WOOD,     30, 0.5 },
	{ R_STONE,    40, 0.5 },
	{ R_SILVER, 2000, 0.0 },
	{ NORESOURCE,  0, 0.0 },
};
static const construction inn_bld = {
	SK_BUILDING, 2,   /* skill, minskill */
	10, 10, inn_req,    /* maxsize, reqsize, required for size */
	NULL              /* improvement */
};
building_type bt_inn = {
	"inn",     /* _name */
	BFL_NONE,     /* flags */
	1, 10, -1,    /* capac/size, maxcapac, maxsize */
	inn_keep,  /* maintenance */
	&inn_bld,  /* construction */
	NULL          /* name() */
};


/** Building: stonecircle */
static requirement stonecircle_req[] = {
	/* resource, number, recycle */
	{ R_WOOD,   500, 0.5 },
	{ R_STONE,  500, 0.5 },
	{ R_SILVER,   0, 0.0 },
	{ NORESOURCE, 0, 0.0 },
};
static const construction stonecircle_bld = {
	SK_BUILDING, 2,   /* skill, minskill */
	100, 100, stonecircle_req,    /* maxsize, reqsize, required for size */
	NULL              /* improvement */
};
building_type bt_stonecircle = {
	"stonecircle",     /* _name */
	BFL_NONE,     /* flags */
	-1, -1, 100,    /* capac/size, maxcapac, maxsize */
	NULL,  /* maintenance */
	&stonecircle_bld,  /* construction */
	NULL          /* name() */
};


/** Building: blessedstonecircle */
building_type bt_blessedstonecircle = {
	"blessedstonecircle",     /* _name */
	BTF_NOBUILD,     /* flags */
	-1, -1, 100,    /* capac/size, maxcapac, maxsize */
	NULL,  /* maintenance */
	&stonecircle_bld,  /* construction */
	NULL          /* name() */
};


/** Building: illusion */
building_type bt_illusion = {
	"illusioncastle",     /* _name */
	BTF_NOBUILD,     /* flags */
	0, 0, 0,    /* capac/size, maxcapac, maxsize */
	NULL,  /* maintenance */
	NULL,  /* construction */
	NULL      /* name() */
};

/** Building: Generisches Gebäude */
building_type bt_generic = {
	"genericbuilding",					/* _name */
	BTF_NOBUILD,  			/* flags */
	-1, -1, 1,  				/* capac/size, maxcapac, maxsize */
	NULL,								/* maintenance */
	NULL,								/* construction */
	NULL								/* name() */
};

/*
name, maxsize, minskill, kapazitaet,
M_EISEN, M_HOLZ, M_STEIN, M_SILBER, M_EOG, M_MALLORN, M_MAX_MAT,
unterhalt, per_size, spezial, unterhalt_spezial, flags
buildingt buildingdaten[MAXBUILDINGTYPES] =
{
	{"Burg", -1, 1, -1, {0, 0, 1, 0, 0, 0}, 0, 0, 0, 0, 0},
	{"Leuchtturm", -1, 3, 4, {1, 1, 2, 100, 0, 0}, 100, 0, 0, 0, 0},
	{"Bergwerk", -1, 4, -1, {1, 10, 5, 250, 0, 0}, 500, 0, 0, 0, 0},
	{"Steinbruch", -1, 2, -1, {1, 5, 1, 250, 0, 0}, 250, 0, 0, 0, 0},
	{"Hafen", 25, 3, -1, {0, 5, 5, 250, 0, 0}, 250, 0, 0, 0, 0},
	{"Akademie", 25, 3, 25, {1, 5, 5, 500, 0, 0}, 1000, 0, 0, 0, 0},
	{"Magierturm", 50, 5, 2, {3, 3, 5, 500, 2, 2}, 1000, 0, 0, 0, 0},
	{"Schmiede", -1, 3, -1, {2, 5, 5, 200, 0, 0}, 300, 0, I_WOOD, 1, 0},
	{"Sägewerk", -1, 3, -1, {3, 5, 5, 200, 0, 0}, 250, 0, 0, 0, 0},
	{"Pferdezucht", -1, 2, -1, {1, 4, 2, 100, 0, 0}, 150, 0, 0, 0, 0},
	{"Monument", -1, 4, -1, {1, 1, 1, 400, 0, 0}, 0, 0, 0, 0, 0},
	{"Damm", 50, 4, -1, {1, 10, 5, 500, 0, 0}, 1000, 0, I_WOOD, 3, 0},
	{"Karawanserei", 10, 2, -1, {1, 5, 1, 500, 0, 0}, 3000, 0, I_HORSE, 2, 0},
	{"Tunnel", 100, 6, -1, {1, 5, 10, 300, 0, 0}, 100, 0, I_STONE, 2, 0},
	{"Taverne", -1, 2, 10, {1, 3, 4, 200, 0, 0}, 0, 5, 0, 0, 0},
	{"Tempel", 100, 5, -1, {0, 5, 5, 0, 0, 0}, 0, 0, 0, 0, 0},
	{"Gesegneter Tempel", 100, 5, -1, {0, 5, 5, 0, 0, 0}, 0, 0, 0, 0, NO_BUILD},
	{"Traumschlößchen", 0, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, NO_BUILD}
};
*/
const building_type * oldbuildings[MAXBUILDINGTYPES] = {
    &bt_castle,
    &bt_lighthouse,
    &bt_mine,
    &bt_quarry,
    &bt_harbour,
    &bt_academy,
    &bt_magictower,
    &bt_smithy,
    &bt_sawmill,
    &bt_stables,
    &bt_monument,
    &bt_dam,
    &bt_caravan,
    &bt_tunnel,
    &bt_inn,
    &bt_stonecircle,
    &bt_blessedstonecircle,
    &bt_illusion,
};

/* for finding out what was meant by a particular building string */

static local_names * bnames;

const building_type *
findbuildingtype(const char * name, const locale * lang)
{
	local_names * bn = bnames;
	void * i;

	while (bn) {
		if (bn->lang==lang) break;
		bn=bn->next;
	}
	if (!bn) {
		struct building_typelist * btl = buildingtypes;
		bn = calloc(sizeof(local_names), 1);
		bn->next = bnames;
		bn->lang = lang;
		while (btl) {
			const char * n = locale_string(lang, btl->type->_name);
			addtoken(&bn->names, n, (void*)btl->type);
			btl=btl->next;
		}
		bnames = bn;
	}
	if (findtoken(&bn->names, name, &i)==E_TOK_NOMATCH) return NULL;
	return (const building_type*)i;
}

static int
sm_smithy(const unit * u, const region * r, skill_t sk, int value) /* skillmod */
{
	if (sk==SK_WEAPONSMITH || sk==SK_ARMORER) {
		if (u->region == r) return value + 1;
	}
	return value;
}

static int
mm_smithy(const unit * u, const resource_type * rtype, int value) /* material-mod */
{
	if (rtype == oldresourcetype[R_IRON]) return value * 2;
	return value;
}

void
init_buildings(void)
{
	a_add(&bt_smithy.attribs, make_skillmod(NOSKILL, SMF_PRODUCTION, sm_smithy, 0, 0));
	a_add(&bt_smithy.attribs, make_matmod(mm_smithy));
}

void
bt_write(FILE * F, const building_type * bt)
{
	fprintf(F, "BUILDINGTYPE %s\n", bt->_name);
	a_write(F, bt->attribs); /* scheisse, weil nicht CR-Format */
	fputs("\n", F);
	fprintf(F, "\"%s\";name\n", bt->_name);
	fprintf(F, "%d;flags\n", bt->flags);
	fprintf(F, "%d;capacity\n", bt->capacity);
	fprintf(F, "%d;maxcapacity\n", bt->maxcapacity);
	fprintf(F, "%d;maxsize\n", bt->maxsize);
	if (bt->maintenance!=NULL) assert(!"not implemented");
	if (bt->construction!=NULL) assert(!"not implemented");
	if (bt->construction!=NULL) assert(!"not implemented");
	if (bt->name!=NULL) assert(!"not implemented");
	fputs("END BUILDINGTYPE\n", F);
}

building_type *
bt_read(FILE * F)
	/* this function is pretty picky */
{
	building_type * bt = calloc(sizeof(building_type), 1);
	int i = fscanf(F, "%s\n", buf);
	if (i==0 || i==EOF) {
		free(bt);
		return NULL;
	}
	bt->_name = strdup(buf);
	a_read(F, &bt->attribs); /* scheisse, weil nicht CR. */
	for (;;) {
		char * semi = buf;
		fgets(buf, sizeof(buf), F);
		if (strlen(buf)==1) continue;
		buf[strlen(buf)-1]=0;
		for(;;) {
			char * s = strchr(semi, ';');
			if (s==NULL) break;
			semi = s + 1;
		}
		if (semi==buf) {
			assert(!strcmp(buf, "END BUILDINGTYPE"));
			break;
		}
		*(semi-1)=0;
		if (buf[0]=='\"') {
			char * s = buf+1;
			assert(*(semi-2)=='\"');
			*(semi-2)=0;
			if (!strcmp(semi, "name") && !bt->_name) bt->_name = strdup(s);
		}
		else {
			int j = atoi(buf);
			switch (semi[0]) {
			case 'c':
				if (!strcmp(semi, "capacity")) bt->capacity=j;
				break;
			case 'f':
				if (!strcmp(semi, "flags")) bt->flags=j;
				break;
			case 'm':
				if (!strcmp(semi, "maxcapacity")) bt->maxcapacity=j;
				else if (!strcmp(semi, "maxsize")) bt->maxsize=j;
				break;
			}
		}
	}
	bt_register(bt);
	return bt;
}

building_type *
bt_make(const char * name, int flags, int capacity, int maxcapacity, int maxsize)
{
	building_type * btype = calloc(sizeof(building_type), 1);
	btype->_name = name;
	btype->flags = flags | BTF_DYNAMIC;
	btype->capacity = capacity;
	btype->maxcapacity = maxcapacity;
	btype->maxsize = maxsize;

	bt_register(btype);
	return btype;
}

void *
resolve_building(void * id) {
   return findbuilding((int)id);
}

void
write_building_reference(const struct building * b, FILE * F)
{
	fprintf(F, "%s ", b?itoa36(b->no):"0");
}

int
read_building_reference(struct building ** b, FILE * F)
{
	int id;
	char zText[10];
	fscanf(F, "%s ", zText);
	id = atoi36(zText);
	if (id==0) {
		*b = NULL;
		return 0;
	}
	else {
		*b = findbuilding(id);
		if (*b==NULL) ur_add((void*)id, (void**)b, resolve_building);
		return 1;
	}
}

building *
new_building(const struct building_type * btype, region * r, const struct locale * lang)
{
	building *b = (building *) calloc(1, sizeof(building));

	b->no  = newcontainerid();
	bhash(b);

	b->type = btype;
	set_string(&b->display, "");
	fset(b, FL_UNNAMED);
	b->region = r;
	addlist(&r->buildings, b);

	{
		static char buffer[IDSIZE + 1 + NAMESIZE + 1];
		if(b->type==&bt_castle)
			sprintf(buffer, "%s", locale_string(lang, btype->_name));
		else
			sprintf(buffer, "%s", LOC(lang, buildingtype(b, 0)));
		set_string(&b->name, buffer);
	}
	return b;
}

void
destroy_building(building * b)
{
	unit *u;

	if(!bfindhash(b->no)) return;
	for(u=b->region->units; u; u=u->next) {
		if(u->building == b) leave(b->region, u);
	}

	b->size = 0;
	if (b->type == &bt_lighthouse) update_lighthouse(b);
	bunhash(b);

#if 0	/* Memoryleak. Aber ohne klappt das Rendern nicht! */
	removelist(&b->region->buildings, b);
#endif
	/* Stattdessen nur aus Liste entfernen, aber im Speicher halten. */
	choplist(&b->region->buildings, b);
	handle_event(&b->attribs, "destroy", b);
}

extern attrib_type at_icastle;

int
buildingeffsize(const building * b, boolean img)
{
	int i = b->size, n = 0;
	const building_type * btype = &bt_castle;
	const construction * cons = btype->construction;

	if (b==NULL) return 0;

	if (b->type!=btype) {
		if (img) {
			const attrib * a = a_find(b->attribs, &at_icastle);
			if (!a || a->data.v != btype) return 0;
		} else return 0;
	}
	assert(cons);

	while (cons && cons->maxsize != -1 && i>=cons->maxsize) {
		i-=cons->maxsize;
		cons = cons->improvement;
		++n;
	}

	if (n>0)
	  return n;
	return 0;

}

unit *
buildingowner(const region * r, const building * b)
{
	unit *u = NULL;
	unit *first = NULL;
#ifndef BROKEN_OWNERS
	assert(r == b->region);
#endif
	/* Prüfen ob Eigentümer am leben. */

	for (u = r->units; u; u = u->next) {
		if (u->building == b) {
			if (!first && u->number > 0)
				first = u;
			if (fval(u, FL_OWNER) && u->number > 0)
				return u;
			if (u->number == 0)
				freset(u, FL_OWNER);
		}
	}

	/* Eigentümer tot oder kein Eigentümer vorhanden. Erste lebende Einheit
	 * nehmen. */

	if (first)
		fset(first, FL_OWNER);
	return first;
}
