/* vi: set ts=2 ai sw=2:
 *
 * $Id: echeck.c,v 1.3 2001/02/02 22:00:15 corwin Exp $
 *
 *	Eressea PB(E)M host Copyright (C) 1997-2000
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
 * This program is freeware. It may not be sold or used commercially.
 * Please send any changes or bugfixes to the authors.
 *
 * Eressea PBeM Order Syntax Checker
 */

#ifdef _MSC_VER
# pragma warning (disable: 4711)
#endif

#define MAINVERSION "3"
#define MINVERSION	"10"
#define PATCHLEVEL	"4"

#ifndef DEFAULT_PATH
#if defined(unix)
#define DEFAULT_PATH "/usr/local/lib/echeck:."
#define PATH_DELIM ":"
#else
#define DEFAULT_PATH NULL
#define PATH_DELIM ":"
#endif
#endif
	

#define VERSION MAINVERSION"."MINVERSION"."PATCHLEVEL

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#if macintosh
# include <console.h>  /* macintosh console handler zur eingabe von parametern
 													added 15.6.00 chartus */
# define UMLAUTE
#endif

enum {
	UT_NONE,
	UT_PARAM,
	UT_ITEM,
	UT_SKILL,
	UT_KEYWORD,
	UT_BUILDING,
	UT_HERB,
	UT_POTION,
	UT_RACE,
	UT_MAX
};


#if __STDC__
/* strdup ist nicht ANSI */
# if !(defined __USE_SVID) && !(defined __USE_BSD) && !(defined __USE_XOPEN_EXTENDED)
/* ... ist aber unter Linux trotzdem definiert. */
#  define strdup(s) (strcpy((char*)malloc(sizeof(char)*strlen(s)+1), s))
# endif
#endif

#ifdef AMIGA
	const char version[]="\0$VER: ECheck V"VERSION", "__DATE__"\0";
#endif

#ifdef _MSC_VER
# include <crtdbg.h>
#endif

#ifndef BOOL_DEFINED
# define BOOL_DEFINED
# ifndef bool
#  define bool char
#  define true ((bool)1)
#  define false ((bool)0)
# endif
#endif

#ifdef WIN32
# define strncasecmp _strnicmp
# define strcasecmp _stricmp
#endif

#if defined(AMIGA) || defined(UMLAUTE)
		/* ohne brauchbares locale.h wird das nix bei Umlauten */
int
a_isspace(char c) {
	return (c==' ' || c=='\t' || c=='\f');
}


char
a_tolower(char c) {
	switch (c) {
		case 'Ä': return 'ä';
		case 'Ö': return 'ö';
		case 'Ü': return 'ü';
		default: return (char) tolower(c);
	}
}


int
a_strncasecmp(const char *a, const char *b, size_t s) {
	int i,j;

	if (!a || !b || !*a || !*b || strlen(b) < s)
		return -1;
	do {
		i=*a++ & 0xDF;	/* 1101 1111 - Groß/Kleinschreibung weglassen */
		j=*b++ & 0xDF;
	} while (i-j==0 && *a && *b && --s);
	return i-j;
}


# define strncasecmp(x,y,n) a_strncasecmp(x,y,n)
# define strcasecmp(x,y) a_strncasecmp(x,y,strlen(x))
# ifdef tolower
#  undef tolower
# endif
# define tolower(x) a_tolower(x)

#else
# include <locale.h>
# define a_isspace(c) isspace(c)
#endif

#define SPACE_REPLACEMENT		'~'
#define SPACE								' '
#define ESCAPE_CHAR					'\\'
#define COMMENT_CHAR				';'
#define MARGIN							78
#define RECRUIT_COST				50

#define INDENT_FACTION			0
#define INDENT_UNIT					2
#define INDENT_ORDERS				4
#define INDENT_NEW_ORDERS		6

#ifndef min
# define min(a,b)	((a) < (b) ? (a) : (b))
#endif
#ifndef max
# define max(a,b)	((a) > (b) ? (a) : (b))
#endif

#define BUFSIZE	8192
#define MAXLINE	4096
#define DISPLAYSIZE 4095
#define NAMESIZE 127

FILE *ERR, *OUT=0;

int line_no,							/* count line number */
 MAXSPELLS,
 MAXITEMS;
char echo_it=0,						/* option: echo input lines */
 no_comment=-3,						/* Keine Infos in [] hinter EINHEIT */
 show_warnings=4,					/* option: print warnings (levels) */
 warnings_cl=0,						/* -w auf der Kommandozeile gegeben */
 warn_off=0,							/* ECHECK NOWARN */
 use_stderr=0,						/* option: use stderr for errors etc */
 brief=0,									/* option: don't list errors */
 ignore_NameMe=0,					/* option: ignoriere NameMe-Kommentare ;; */
 piping=0,								/* option: wird output als pipe-input benutzt? */
 empiria=0,								/* option: Empiria-Checking */
 lohn=10,									/* Lohn für Arbeit - je Region zu setzen */
 silberpool=0,						/* option: Silberpool-Verwaltung */
 line_start=0,						/* option: Beginn der Zeilenzählung */
 noship=0,
 noroute=0,
 nolost=0,
 has_version=0,
 compile=0;								/* option: compiler-style warnings */
int error_count=0,				/* counter: errors */
 warning_count=0;					/* counter: warnings */
char order_buf[BUFSIZE],	/* current order line */
 checked_buf[BUFSIZE],		/* checked order line */
 message_buf[BUFSIZE],		/* messages are composed here */
 warn_buf[BUFSIZE],				/* warnings are composed here */
 indent, next_indent,			/* indent index */
 does_default=0,					/* Ist DEFAULT aktiv? */
 befehle_ende,						/* EOF der Befehlsdatei */
 *filename;
int rec_cost=RECRUIT_COST,
 this_command,
 this_unit,								/* wird von getaunit gesetzt */
 Rx, Ry;									/* Koordinaten der aktuellen Region */
const char *path;
FILE *F;

/* ------------------------------------------------------------- */

enum {
	K_WORK,
	K_ATTACK,
	K_STEAL,
	K_BESIEGE,
	K_NAME,
	K_USE,
	K_DISPLAY,
	K_ENTER,
	K_GUARD,
	K_MAIL,
	K_END,
	K_DRIVE,
	K_FOLLOW,
	K_RESEARCH,
	K_GIVE,
	K_ALLY,
	K_STATUS,
	K_COMBAT,
	K_BUY,
	K_CONTACT,
	K_TEACH,
	K_STUDY,
	K_LIEFERE,
	K_MAKE,
	K_MOVE,
	K_PASSWORD,
	K_RECRUIT,
	K_REPORT,
	K_OPTION,
	K_SPY,
	K_SETSTEALTH,
	K_TRANSPORT,
	K_QUIT,
	K_TAX,
	K_TURMZAUBER,
	K_ENTERTAIN,
	K_SELL,
	K_LEAVE,
	K_CAST,
	K_RESHOW,
	K_DESTROY,
	K_VERGESSE,
	K_BIETE,
	K_DEFAULT,
	K_COMMENT,
	K_ROUTE,
	K_SABOTIERE,
	K_ZUECHTE,
	K_URSPRUNG,
	K_EMAIL,
	K_RESERVE,
	K_BANNER,
	K_MEINUNG,
	K_NUMMER,
	K_MAGIEGEBIET,
	K_PIRATERIE,
	K_NEUSTART,
	K_GRUPPE,
	K_SORTIERE,
		/* Empiria only: */
	K_SEND,
	K_FIND,
	K_ADDRESS,
	MAXKEYWORDS
};


static char *keywords[MAXKEYWORDS]={
	"Arbeiten",
	"Attackieren",
	"Beklauen",
	"Belagere",
	"Benennen",
	"Benutzen",
	"Beschreiben",
	"Betreten",
	"Bewachen",
	"Botschaft",
	"Ende",
	"Fahren",
	"Folgen",
	"Forschen",
	"Gib",
	"Helfen",
	"Kämpfen",
	"Kampfzauber",
	"Kaufen",
	"Kontaktieren",
	"Lehren",
	"Lernen",
	"Liefere",
	"Machen",
	"Nach",
	"Passwort",
	"Rekrutieren",
	"Report",
	"Option",
	"Spionieren",
	"Tarnen",
	"Transportieren",
	"Stirb",
	"Treiben",
	"Turmzauber",
	"Unterhalten",
	"Verkaufen",
	"Verlassen",
	"Zaubere",
	"Zeigen",
	"Zerstören",
	"Vergessen",
	"Bieten",
	"Default",
	"//",
	"Route",
	"Sabotieren",
	"Züchten",
	"Ursprung",
	"Email",
	"Reservieren",
	"Banner",
	"Meinung",
	"Nummer",
	"Magiegebiet",
	"Piraterie",
	"Neustart",
	"Gruppe",
	"Sortiere",
		/* Empiria only: */
	"Sende",
	"Finde",
	"Adresse"
};


static char *magiegebiet[]={
	"Illaun",
	"Tybied",
	"Cerddor",
	"Gwyrrd",
	"Draig"
};


enum {
	P_ALLES,
	P_PEASANT,
	P_CASTLE,
	P_GEBAEUDE,
	P_UNIT,
	P_FLEE,
	P_BEHIND,
	P_VORNE,
	P_CONTROL,
	P_HERBS,
	P_NOT,
	P_NEXT,
	P_FACTION,
	P_PERSON,
	P_REGION,
	P_SHIP,
	P_SILVER,
	P_ROAD,
	P_TEMP,
	P_PRIVAT,
	P_KAEMPFE,
	P_OBSERVE,
	P_GIVE,
	P_GUARD,
	P_WARN,
	P_SPELLBOOK,
	P_STUFE,
	P_HORSE,
	P_FREMD,
	MAXPARAMS
};


static char *parameters[MAXPARAMS]={
	"Alles",
	"Bauern",
	"Burg",
	"Gebäude",
	"Einheit",
	"Fliehe",
	"Hinten",
	"Vorne",
	"Kommando",
	"Kräuter",
	"Nicht",
	"Nächster",
	"Partei",
	"Personen",
	"Region",
	"Schiff",
	"Silber",
	"Straßen",
	"Temp",
	"Privat",
	"Kämpfe",
	"Wahrnehmung",
	"Gib",
	"Bewache",
	"Warnung",
	"Zauberbuch",
	"Stufe",	/* Eressea-Zauber */
	"Pferde",	/* ZÜCHTE PFERDE */
	"Fremdes"
};


static char *reports[]={	/* Fehler und Meldungen im Report */
	"Kampf",
	"Ereignisse",
	"Bewegung",
	"Einkommen",
	"Handel",
	"Produktion",
	"Orkvermehrung",
	"Alles"
};

static const int MAXREPORTS=sizeof(reports)/sizeof(reports[0]);


static char *message_levels[]={
	"Wichtig",
	"Debug",
	"Fehler",
	"Warnungen",
	"Infos"
};

static const int ML_MAX=sizeof(message_levels)/sizeof(message_levels[0]);


static char *options[]={
	"Auswertung",
	"Computer",
	"Statistik",
	"Zipped",
	"Merian",
	"Zugvorlage",	/* ab hier Eressea only */
	"Silberpool",
	"Materialpool",
	"Adressen",
	"Bzip2"
};

static const int MAXOPTIONS=sizeof(options)/sizeof(options[0]);


enum {
	D_NORTH,
	D_SOUTH,
	D_EAST,
	D_WEST,
	D_PAUSE,
	D_NORTHEAST,
	D_NORTHWEST,
	D_SOUTHEAST,
	D_SOUTHWEST,
	MAXDIRECTIONS
};


static char *directions[MAXDIRECTIONS]={
	"Norden",
	"Sueden",	/* Empiria hat keine Umlaute! */
	"Osten",
	"Westen",
	"Pause",
	"Nordosten",
	"Nordwesten",
	"Südosten",
	"Südwesten"
};


static char *Rassen[]={
	"Menschen",
	"Zwerge",
	"Elfen",
	"Orks",
	"Dämonen",
	"Meermenschen",
	"Halblinge",
	"Trolle",
	"Goblins",
	"Katzen",
	"Insekten"
};

static const int R_MAX=sizeof(Rassen)/sizeof(Rassen[0]);


static char *shiptypes[]={
	"Boot",
	"Langboot",
	"Drachenschiff",
	"Karavelle",
	"Trireme"
};

static const int MAXSHIPS=sizeof(shiptypes)/sizeof(shiptypes[0]);


enum {
	POT_FAST,
	POT_STRONG,
	POT_TREES,
	POT_DOMORE,
	POT_HEILWASSER,
	POT_BAUERNBLUT,
	POT_WISE,
	POT_FOOL,
	POT_WARMTH,
	POT_HORSE,
	POT_BERSERK,
	POT_PEOPLE,
	POT_WAHRHEIT,
	POT_MACHT,
	POT_HEAL,
	MAXPOTIONS
};


enum {
	H_PLAIN_1,
	H_PLAIN_2,
	H_PLAIN_3,
	H_FOREST_1,
	H_FOREST_2,
	H_FOREST_3,
	H_SWAMP_1,
	H_SWAMP_2,
	H_SWAMP_3,
	H_DESERT_1,
	H_DESERT_2,
	H_DESERT_3,
	H_HIGHLAND_1,
	H_HIGHLAND_2,
	H_HIGHLAND_3,
	H_MOUNTAIN_1,
	H_MOUNTAIN_2,
	H_MOUNTAIN_3,
	H_GLACIER_1,
	H_GLACIER_2,
	H_GLACIER_3,
	MAXHERBS
};


static char *herbdata[2][MAXHERBS]={
	{
		"Flachwurz",
		"Würziger Wagemut",
		"Eulenauge",
		"Grüner Spinnerich",
		"Blauer Baumringel",
		"Elfenlieb",
		"Gurgelkraut",
		"Knotiger Saugwurz",
		"Blasenmorchel",
		"Wasserfinder",
		"Kakteenschwitz",
		"Sandfäule",
		"Windbeutel",
		"Fjordwuchs",
		"Alraune",
		"Steinbeißer",
		"Spaltwachs",
		"Höhlenglimm",
		"Eisblume",
		"Weißer Wüterich",
		"Schneekristall"
	},
	{
		"Flachwurze",
		"Würzige Wagemute",
		"Eulenaugen",
		"Grüne Spinneriche",
		"Blaue Baumringel",
		"Elfenlieb",
		"Gurgelkräuter",
		"Knotige Saugwurze",
		"Blasenmorcheln",
		"Wasserfinder",
		"Kakteenschwitze",
		"Sandfäulen",
		"Windbeutel",
		"Fjordwuchse",
		"Alraunen",
		"Steinbeißer",
		"Spaltwachse",
		"Höhlenglimme",
		"Eisblumen",
		"Weiße Wüteriche",
		"Schneekristalle"
	}
};


typedef struct _names {
	char *txt;
	struct _names *next;
} t_names;


typedef struct _item {
	t_names *name;
	struct _item *next;
} t_item;

t_item *itemdata=NULL;


typedef struct t_spell {
	char *name;
	int kosten;
	char typ;
	struct t_spell *next;
} t_spell;

t_spell *spells=NULL;


typedef struct _skills {
	char *name;
	int kosten;
	struct _skills *next;
} t_skills;

t_skills *skilldata=NULL;


enum {
	I_BALM,
	I_SPICES,
	I_JEWELERY,
	I_MYRRH,
	I_OIL,
	I_SILK,
	I_INCENSE
};


#define isluxury(i)  (i<=I_INCENSE)

#define SP_ZAUBER 1
#define SP_KAMPF	2
#define SP_POST		4
#define SP_PRAE		8
#define SP_BATTLE	14	/* 2+4+8 */
#define SP_ALL		15

#define MAXPOTIONS 15

char* potionnames[2][MAXPOTIONS]={
	{
		/* Stufe 1: */
		"Siebenmeilentee",
		"Goliathwasser",
		"Wasser des Lebens",
		"Trank der Wahrheit",
		/* Stufe 2: */
		"Schaffenstrunk",
		"Wundsalbe",
		"Bauernblut",
		/* Stufe 3: */
		"Gehirnschmalz",
		"Dumpfbackenbrot",
		"Nestwärme",
		"Pferdeglück",
		"Berserkerblut",
		/* Stufe 4: */
		"Bauernlieb",
		"Elixier der Macht",
		"Heiltrank"
	}, {
		/* Stufe 1: */
		"Siebenmeilentees",
		"Goliathwasser",
		"Wasser des Lebens",
		"Tränke der Wahrheit",
		/* Stufe 2: */
		"Schaffenstrünke",
		"Wundsalben",
		"Bauernblut",
		/* Stufe 3: */
		"Gehirnschmalz",
		"Dumpfbackenbrote",
		"Nestwärme",
		"Pferdeglück",
		"Berserkerblut",
		/* Stufe 4: */
		"Bauernlieb",
		"Elixiere der Macht",
		"Heiltränke"
	}
};


static char* buildingtypes[]={
	"Burg",
	"Leuchtturm",
	"Bergwerk",
	"Steinbruch",
	"Hafen",
	"Akademie",	/* wird in inittokens() für Empiria rausgefiltert */
	"Magierturm",
	"Schmiede",
	"Sägewerk",
	"Pferdezucht",
	"Monument",
	"Damm",
	"Karawanserei",
	"Tunnel",
	"Taverne",
	"Universität",	/* wird in inittokens() für Eressea rausgefiltert */
	NULL
};


enum {
	POSSIBLE,
	NECESSARY
};


/* --------------------------------------------------------------------- */

typedef struct list {
	struct list *next;
} list;


typedef struct t_region {
	struct t_region *next;
	int personen, geld, x, y, line_no, reserviert;
	char *name;
} t_region;


typedef struct unit {
	struct unit *next;
	int no;
	int people;
	int money;
	int reserviert;
	int unterhalt;
	int line_no;
	int temp;
	int ship;	/* Nummer unseres Schiffes; ship<0: ich bin owner */
	char lives, hasmoved;
	t_region *region;
	int newx, newy;
	int transport;	/* hier steht drin, welche Einheit mich TRANSPORTIEREn wird */
	int drive;	/* hier steht drin, welche Einheit mich TRANSPORTIEREn soll */
					/* wenn drive!=0, muß transport==drive sein, sonst irgendwo Tippfehler */
	int start_of_orders_line;
	int long_order_line;
	char *start_of_orders;
	char *long_order;
	char *order;
	int schueler;
	int lehrer;
	int lernt;
	int spell;		/* Bit-Map: 2^Spell-Typ */
} unit;


typedef struct teach {
	struct teach *next;
	unit *teacher;
	unit *student;
} teach;


teach *teachings=NULL;

unit *units=NULL,
	*order_unit,		/* Die Einheit, die gerade dran ist */
	*mother_unit,		/* Die Einheit, die MACHE TEMP macht */
	*cmd_unit;			/* Die Einheit, die gerade angesprochen wird, z.B. mit GIB */

t_region *Regionen=NULL;

typedef struct tnode {
	char c;
	bool leaf;
	int id;
	struct tnode * nexthash;
	struct tnode * next[32];
} tnode;


tnode tokens[UT_MAX];

/* Prototypes */
unit * find_unit(int i, int t);
unit * newunit(int n, int t);


void *
cmalloc(int n) {
	void *p;

	if (n==0)
		n=1;
	p=calloc(n,1);
	if (p==NULL) {
		puts("\007 * Kein freier Speicher mehr *");
		exit(1);
	}
	return p;
}


int
Pow(int p) {
	if (!p)
		return 1;
	else
		return 2<<(p-1);
}


char nulls[]="\0\0\0\0\0\0\0\0";

const char*
itob(int i) {
	char *dst;
	int b=i;

	if (i==0) return "0";

	if (empiria) {
		static char s[8];
		sprintf(s, "%d", i);
		return s;
	}
	dst=nulls+6;
	while (b>0) {
		int x=b%36;
		b/=36; dst--;
		if (x<10)
			*dst=(char)('0'+x);
		else
			*dst=(char)('a'+(x-10));
	}
	return dst;
}


#define scat(X) strcat(checked_buf, X)
#define Scat(X) scat(" ");scat(X)

void
qcat(char *s) {
	char *x;

	x=strchr(s, ' ');
	if (x != NULL && does_default==0)
		scat(" \"");
	else
		scat(" ");
	scat(s);
	if (x != NULL && does_default==0)
		scat("\"");
}


void
icat(int n) {
	static char s[10];
	sprintf(s, " %d", n);
	scat(s);
}


void
bcat(int n) {
	if (empiria)
		icat(n);
	else {
		static char s[10];
		sprintf(s, " %s", itob(n));
		scat(s);
	}
}


char *
ItemName(int i, int plural) {
	static int ino=0;
	static t_item *item=NULL, *it=NULL;
	int x;

	if (!it) item=it=itemdata;
	if (i!=ino) {
		x=i;
		if (i<ino) it=itemdata;
		else i-=ino;
		ino=x;
		while (0<i-- && it) it=it->next;
		if (!it) {
			fprintf(ERR,"Fehler: Item %d (%d) nicht gefunden!\n",ino,i);
			exit(123);
		}
		item=it;
	}
	if (plural)
		return item->name->next->txt;
	return item->name->txt;
}

FILE *
path_fopen(const char *path, const char *file, const char *mode)
{
	FILE *f;
	char *pathw = strdup(path);
	char *token;
	char buf[4096];

	token = strtok(pathw, PATH_DELIM);
	while(token) {
		sprintf(buf, "%s/%s", token, file);
		f = fopen(buf, mode);
		if(f) {
			free(pathw);
			return f;
		}
		token = strtok(NULL, PATH_DELIM);
	}

	free(pathw);
	return NULL;
}


void
readspells(void) {
	char *s,*x;
	t_spell *it, *in;
	char *file;

	F = path_fopen(path, "zauber.txt", "r");
	if(!F) {
		fprintf(ERR, "Kann Datei 'zauber.txt' nicht lesen.\n", file);
		fprintf(ERR, "Suchpfad ist '%s'\n", path);
		return;
	}

	it=in=spells;
	for (;;) {
		do {
			s=fgets(order_buf, MAXLINE, F);
		} while (s && (*s=='#' || *s=='\n'));	/* Leer- und Kommentarzeilen überlesen */
		if (!s) {
			fclose(F);
			return;
		}
		it=cmalloc(sizeof(t_spell));
		x=strchr(s,';');
		if (!x) x=strchr(s,',');
		if (x) *x=0;
		else {
			x=strchr(s,'\n');
			if (x) *x=0;
			x=NULL;
		}
		it->name=strdup(s);
		if (x) {
			s=(char *)(x+1);
			while (a_isspace(*s)) s++;
			it->kosten=atoi(s);
			x=strchr(s,';');
			if (!x) x=strchr(s,',');
			if (x) {
				s=(char *)(x+1);
				it->typ=(char)Pow(atoi(s));
			}
		}
		if (in) in->next=it;
		in=it;
		if (!spells) spells=in;
	}	/* Abbruch oben */
}


void
readskills(void) {
	char *s,*x;
	t_skills *it, *in;
	char *file;

	F = path_fopen(path, "talente.txt", "r");
	if(!F) {
		fprintf(ERR, "Kann Datei 'talente.txt' nicht lesen.\n", file);
		fprintf(ERR, "Suchpfad ist '%s'\n", path);
		return;
	}

	it=in=skilldata;
	for (;;) {
		do {
			s=fgets(order_buf, MAXLINE, F);
		} while (s && (*s=='#' || *s=='\n'));	/* Leer- und Kommentarzeilen überlesen */
		if (!s) {
			fclose(F);
			return;
		}
		it=cmalloc(sizeof(t_skills));
		x=strchr(s,';');
		if (!x) x=strchr(s,',');
		if (x) *x=0;
		else {
			x=strchr(s,'\n');
			if (x) *x=0;
			x=NULL;
		}
		it->name=strdup(s);
		if (x) {
			s=(char *)(x+1);
			while (a_isspace(*s)) s++;
			it->kosten=atoi(s);
		}
		if (in) in->next=it;
		in=it;
		if (!skilldata) skilldata=in;
	}	/* Abbruch oben */
}


void
readitems(void) {
	char *s,*x;
	t_item *it, *in;
	t_names *n, *nn;
	char *file;
	
	F = path_fopen(path, "items.txt", "r");
	if(!F) {
		fprintf(ERR, "Kann Datei 'items.txt' nicht lesen.\n", file);
		fprintf(ERR, "Suchpfad ist '%s'\n", path);
		return;
	}

	it=in=itemdata;
	for (;;) {
		do {
			s=fgets(order_buf, MAXLINE, F);
		} while (s && (*s=='#' || *s=='\n'));	/* Leer- und Kommentarzeilen überlesen */
		if (!s) {
			fclose(F);
			return;
		}
		it=cmalloc(sizeof(t_item));
		nn=NULL;
		x=s;
		do {
			n=cmalloc(sizeof(t_names));
			x=strchr(s,';');
			if (x) *x=0;
			else {
				x=strchr(s,'\n');
				if (x) *x=0;
				x=NULL;
			}
			n->txt=strdup(s);
			if (x) {
				s=(char *)(x+1);
				while (a_isspace(*s)) s++;
			}
			if (nn) nn->next=n;
			nn=n;
			if (!it->name) it->name=nn;
		} while (x && *s);
		if (in) in->next=it;
		in=it;
		if (!itemdata) itemdata=in;
	}	/* Abbruch oben */
}


void
porder(void) {
	int i;

	if (echo_it) {
		if (does_default != 2)
			for (i=0; i != indent; i++)
				putc(' ',OUT);

		if (does_default > 0)
			fprintf(OUT,"%s%s",checked_buf, does_default==2 ? "\"\n" : "");
		else
		{
			fputs(checked_buf,OUT);
			putc('\n',OUT);
		}
	}
	checked_buf[0]=0;
	if (next_indent != indent)
		indent=next_indent;
}


char *
wrap(char *s) {
	int i, j, k;

	strcpy(message_buf,s);

	/* i zählt die Länge des Strings s ab. j zählt die Länge der Zeile ab.
		 Ist der Rand erreicht, wird k auf i gesetzt, und rückwärts eine
		 Leerstelle gesucht, die man mit einem '\n' ersetzen könnte. */

	for (i=0, j=25; s[i]; i++, j++) {	/* j=25: "Warnung zur Zeile xyz:" */
		if (j==MARGIN) {
			for (k=i; !a_isspace(s[k]) && k>0; k--);
				/* findet man eine Leerstelle, wird sie durch '\n' ersetzt: */
			if (k>0) {
				message_buf[k]='\n';
				j=i-k;
			}
		} else
			/* Hat man das letzte mal keine Leerstelle gefunden, so reagiert man
				 einfach auf die nächste Leerstelle, ohne nach vorwärts zu suchen
				 (das hat man ja schon einmal gemacht) */

			if (j>MARGIN && a_isspace(s[i])) {
				message_buf[i]='\n';
			j=0;
		}
	}
	return message_buf;
}


void
Error(char *s, int l, char *o) {
	error_count++;
	if (!brief) {
		if (!compile)
			fprintf(ERR, "Fehler in Zeile %d: %s.\n	`%s'\n\n", l, wrap(s), o);
		else
			fprintf(ERR, "%s(%d): Fehler: %s. `%s'\n", filename, l, s, o);
	}
}


#define anerror(s)	Error(s,line_no,order_buf)

int
btoi(char *s) {
	char *x=s;
	int i=0;

	if (!(*s)) return 0;
	while(isalnum(*s)) s++;
	if (s) *s=0;
	s=x;
	if (strlen(s)>(size_t)(4+empiria)) {
		sprintf(message_buf,"Zahl '%s' ist zu groß. Stattdessen 1 benutzt",s);
		anerror(message_buf);
		return 1;
	}
#ifdef HAVE_STRTOL
	i=(int)(strtol(x, NULL, empiria ? 10 : 36));
#else
	if (empiria)
		i=atoi(s);
	else {
		while (isalnum(*s)) {
			i*=36;
			if (isupper(*s)) i+=(*s)-'A'+10;
			else if (islower(*s)) i+=(*s)-'a'+10;
			else if (isdigit(*s)) i+=(*s)-'0';
			++s;
		}
	}
#endif
	return i;
}


const char *
uid(unit *u) {
	static char *bf=NULL;
	if (!bf) bf=cmalloc(18);
	sprintf(bf,"%s%s", u->temp!=0 ? "TEMP " : "", itob(u->no));
	return bf;
}


const char *
Uid(int i) {
	static char *bf=NULL;
	unit *u;
	u=find_unit(i,0);
	if (!u) u=find_unit(i,1);
	if (!u) {
		sprintf(warn_buf,"Konnte Einheit %s nicht ermitteln",itob(i));
		Error(warn_buf,line_no,"<interner Check>");
		u=newunit(-1,0);
	}
	if (!bf) bf=cmalloc(18);
	sprintf(bf,"%s%s", u->temp!=0 ? "TEMP " : "", itob(u->no));
	return bf;
}


void
warn(char *s, int line_no, char level) {
	if (warn_off) return;
	if (level>show_warnings) return;
	warning_count++;
	if (show_warnings && !brief) {
		if (!compile)
			fprintf(ERR, "Warnung zur Zeile %d: %s\n", line_no, wrap(s));
		else
			fprintf(ERR, "%s(%d): Warnung (%d): %s\n", filename, line_no, level, s);
	}
}


void
warning(char *s, int l, char *o, char level) {
	if (warn_off) return;
	if (level>show_warnings) return;
	warning_count++;
	if (show_warnings && !brief) {
		if (!compile)
			fprintf(ERR, "Warnung zur Zeile %d: %s.\n	`%s'\n\n", l, wrap(s), o);
		else
			fprintf(ERR, "%s(%d): Warnung (%d): %s. `%s'\n", filename, l, level, s, o);
	}
}


#define awarning(s,level)	warning(s,line_no,order_buf,level)

void
checkstring(char *s, size_t l, int type) {
	if (l>0 && strlen(s)>l) {
		sprintf(warn_buf, "Text zu lang (max. %d)", (int)(l));
		awarning(warn_buf,2);
	}
	if (s[0]==0 && type==NECESSARY)
		awarning("Kein Text",1);

	strncpy(warn_buf, s, l);

	if (echo_it && s[0]) {
		if (strlen(s) + strlen(checked_buf) > MARGIN) {
			qcat(wrap(s));
		} else
			qcat(s);
	}
}


/* - Speicher und Listen Funktionen ---------------------------- */

#define addlist(l,p)	(p->next=*l, *l=p)

int current_temp_no;
	/* Enthält die TEMP No, falls eine TEMP Einheit angesprochen wurde. */

int from_temp_unit_no;
	/* Enthält die TEMP No, falls Befehle von einer TEMP Einheit gelesen werden. */

#define val(x)	(x!=0)

unit *
find_unit(int i, int t) {
	unit *u;

	for (u=units; u; u=u->next)
		if ((i < 0 && u->no < 0) || (u->no==i && val(u->temp)==val(t)))
			break;
	return u;
}



t_region *
addregion(int x, int y, int pers) {
	static t_region *r=NULL;

	if (!r || r->x != x || r->y != y) {
		for (r=Regionen; r; r=r->next)
			if (x==r->x && y==r->y)
				break;
	}

	if (!r) {
		r=cmalloc(sizeof(t_region));
		r->x=x;
		r->y=y;
		r->personen=pers;
		r->geld=0;
		r->reserviert=0;
		r->name=strdup("Region");
		r->line_no=line_no;
		r->next=Regionen;
		Regionen=r;
	} else {
		r->personen+=pers;
	}
	return r;
}


void
addteach(unit *teacher, unit *student) {
	teach *t, **teachlist=&teachings;

	for (t=teachings; t; t=t->next) {
		if (t->student==student) {
			if (!teacher)	/* Aufruf durch Schüler, aber Lehrer hat schon Struktur angelegt */
				return;
			if (t->teacher==teacher)	/* kann eigentlich nicht vorkommen, aber egal */
				return;
			if (t->teacher==NULL) {	/* Schüler hat Struct angelegt (mit unbek. Lehrer),
																	 wir tragen jetzt nur noch den Lehrer nach */
				t->teacher=teacher;
				return;
			}
		}
	}
	t=cmalloc(sizeof(teach));
	t->next=NULL;
	t->teacher=teacher;
	t->student=student;
	addlist(teachlist, t);
}


unit *
newunit(int n, int t) {
	unit *u=find_unit(n,t), **up=&units;

	if (!u) {
		u=cmalloc(sizeof(unit));
		u->no=n;
		u->line_no=line_no;
		u->order=strdup(order_buf);
		u->region=addregion(Rx, Ry, 0);
		u->newx=Rx;
		u->newy=Ry;
		u->temp=t;
		addlist(up, u);
	}

	if (u->temp<0) {
		sprintf(warn_buf, "`TEMP %s' wird in Region %d,%d und Region %d,%d "
				"(Zeile %d) gebraucht", itob(u->no), Rx, Ry, u->newx, u->newy,
				u->start_of_orders_line);
		anerror(warn_buf);
		u->long_order_line=0;
	}

	if (u->temp<42)
		u->temp=t;

	if (t)
		current_temp_no=n;
	else
		current_temp_no=0;

	return u;
}


/* - In/Output ------------------------------------------------- */

int
atoip(char *s) {
	int n;

	n=atoi(s);
	if (n<0)
		n=0;
	return n;
}


char *
igetstr(char *s1) {
	int i;
	static char *s;
	static char buf[BUFSIZE];

	if (s1)
		s=s1;
	while (s[0]==SPACE)
		s++;

	for (i=0; s[0] && s[0] != SPACE && i < sizeof(buf); i++, s++) {
		buf[i]=s[0];

		if (s[0]==SPACE_REPLACEMENT) {
			if (i>0 && buf[i-1]==ESCAPE_CHAR)
				buf[--i]=SPACE_REPLACEMENT;
			else
				buf[i]=SPACE;
		}
	}
	buf[i]=0;

	return buf;
}


#define getstr()	igetstr(NULL)
#define getb()	btoi(igetstr(NULL))
#define geti()	atoi(igetstr(NULL))
#define getI()	empiria?atoi(igetstr(NULL)):getb()

int
findstr(char **v, const char *s, int max) {
	int i;
	size_t ss=strlen(s);

	if (!s[0])
		return -1;
	for (i=0; i < max; i++)
		if (v[i] && strncasecmp(s, v[i], ss)==0)
			return i;
	return -1;
}


int
findtoken(const char *str, int type) {
	tnode * tk = &tokens[type];
	while (*str) {
		char c = (char)tolower(*str);
		int index = ((unsigned char)c) % 32;
		tk = tk->next[index];
		while (tk && tk->c!=c) tk = tk->nexthash;
		++str;
		if (!tk) return -1;
	}
	if (tk->id>=0) return tk->id;
	else return -1;
}


int
findparam(const char *s) {
	int p;
	p=findtoken(s, UT_PARAM);
	if (p != -1)
		return p;
	p=findtoken(s, UT_BUILDING);
	if (p != -1)
		return P_GEBAEUDE;
	return -1;
}


int
isparam(char *s, int i, char print) {
	if (i != findparam(s))
		return 0;

	if (print) {
		Scat(parameters[i]);
	}
	return 1;
}


t_spell *
findspell(char *s) {
	t_spell *sp;

	if (!s[0] || !spells)
		return NULL;
	for (sp=spells; sp; sp=sp->next)
		if (sp->name && !strncasecmp(sp->name, s, strlen(s)))
			return sp;
	return NULL;
}


t_skills *
findskill(char *s) {
	int i;
	t_skills *sk=skilldata;

	i=findtoken(s, UT_SKILL);
	if (i<0)
		return NULL;
	while (i--) sk=sk->next;
	return sk;
}


#define findherb(s) findtoken(s, UT_HERB)
#define findpotion(s) findtoken(s, UT_POTION)
#define finditem(s) findtoken(s, UT_ITEM)
#define findbuildingtype(s) findtoken(s, UT_BUILDING)
#define findreport(s) 	findstr(reports,s,MAXREPORTS)
#define findoption(s) 	findstr(options,s,MAXOPTIONS)
#define findshiptype(s) 	findstr(shiptypes,s,MAXSHIPS);
#define getskill()	findskill(getstr())
#define getparam()	findparam(getstr())
#define igetparam(s)	findparam(igetstr(s))


int
finddirection(char *s) {
#define MAXDIRNAMES 13
	char* dirs[MAXDIRNAMES]={
		"NW",
		"NO",
		"Nordwesten",
		"Nordosten",
		"Suedosten",
		"Südosten",
		"Suedwesten",
		"Südwesten",
		"SO",
		"SW",
		"Pause",
		"Osten",
		"Westen"
		/* mit dieser routine kann man mehrere namen für eine direction geben,
		 * das ist für die hexes ideal. */
	};
	static const int dir_xref[MAXDIRNAMES]={
		D_NORTHWEST,
		D_NORTHEAST,
		D_NORTHWEST,
		D_NORTHEAST,
		D_SOUTHEAST,
		D_SOUTHEAST,
		D_SOUTHWEST,
		D_SOUTHWEST,
		D_SOUTHEAST,
		D_SOUTHWEST,
		D_PAUSE,
		D_EAST,
		D_WEST
	};
	int uc;

	if (!*s)
		return -2;
	if (empiria)
		return findstr(directions, s, 4);
	else {
		if (strcmp(s,"//")==0)		/* "NACH NW NO NO // nach Xontormia" ist erlaubt */
			return -2;
		uc=findstr(dirs, s, MAXDIRNAMES);
	}
	if (uc != -1)
		return dir_xref[uc];
	return uc;
}


#define getdirection()	finddirection(getstr())

int
getoption(void) {
	int i=findoption(getstr());
	if (empiria && i>5)	/* Option ist Eressea only */
		return -1;
	return i;
}


#define findkeyword(s)	findtoken(s, UT_KEYWORD)
#define igetkeyword(s)	findkeyword(igetstr(s))

char *
getbuf(void) {
	char lbuf[MAXLINE];
	bool cont = false;
	bool quote = false;
	bool report = false;
	char * cp = warn_buf;

	lbuf[MAXLINE-1] = '@';

	do {
		bool eatwhite = true;
		bool start = true;
		char *end;
		char *bp = fgets(lbuf, MAXLINE, F);

		if (!bp) return NULL;

		end=bp+strlen(bp);
		if (*(end-1)=='\n') {
			line_no++;
			*(--end)=0;
		} else {
				/* wenn die Zeile länger als erlaubt war, wird der Rest weggeworfen: */
			while (bp && !lbuf[MAXLINE-1] && lbuf[MAXLINE-2]!='\n')
				bp=fgets(warn_buf, 1024, F);
			sprintf(warn_buf, "%.30s", lbuf);
			Error("Zeile zu lang", line_no, warn_buf);
			bp=lbuf;
		}
		cont=false;
		while (cp!=warn_buf+MAXLINE && bp!=lbuf+MAXLINE && *bp) {
			if (a_isspace(*bp)) {
				if (eatwhite) {
					do { ++bp; } while (bp!=lbuf+MAXLINE && a_isspace(*bp));
					if (!quote && !start) *(cp++)=' ';
				} else {
					do {
						*(cp++)=SPACE_REPLACEMENT;
						++bp;
					} while (cp!=warn_buf+MAXLINE && bp!=lbuf+MAXLINE && a_isspace(*bp));
				}
			} else {
				cont=false;
				if (*bp=='"') {
					quote=(bool)!quote;
					eatwhite=true;
				} else {
					if (*bp=='\\') cont=true;
					else
#if !defined(AMIGA) && !defined(UMLAUTE)
					if (!iscntrl(*bp)) {
#else
					if ((unsigned char)(*bp)>32) {
#endif

						*(cp++) = *bp;
						eatwhite = (bool)!quote;
					}
				}
				++bp;
			}
			start=false;
		}
		if (cp==warn_buf+MAXLINE) {
			--cp;
			if (!report) {
				report=true;
				sprintf(lbuf, "%.30s", warn_buf);
				Error("Zeile zu lang", line_no, lbuf);
			}
		}
		*cp=0;
	} while (cont || cp==warn_buf);

	if (quote)
		Error("Fehlende \"",line_no,lbuf);

	return warn_buf;
}


void
get_order(void) {
	char *buf;
	bool ok=false;

	while (!befehle_ende && !ok) {
		buf=getbuf();

		if (buf) {
			if (buf[0]==COMMENT_CHAR && buf[1]==COMMENT_CHAR) {
 				if (ignore_NameMe) line_no--;
				continue;
			}
			strcpy(order_buf,buf);
			ok=true;
		} else
			befehle_ende=1;
	}
}


void
getafaction(char *s) {
	int i;

	if (empiria) i=atoi(s);
	else i=btoi(s);
	if (!s[0])
		anerror("Partei fehlt");
	else {
		if (!i)
			awarning("Partei 0 verwendet",1);
		icat(i);
	}
}


int
getmoreunits(bool partei) {
	char *s;
	int i, count, temp;
	unit *u;

	count=0;
	for (;;) {
		s=getstr();
		if (!(*s))
			break;

		if (partei) {
			if (empiria)
				i=atoi(s);
			else
				i=btoi(s);
			if (i<1) {
				sprintf(warn_buf,"Partei '%s' ungültig",s);
				anerror(warn_buf);
			} else
				bcat(i);
		} else {
			if (findparam(s)==P_TEMP) {
				scat(" TEMP");
				temp=1;
				s=getstr();
			} else
				temp=0;
			i=btoi(s);

			if (!i) {
				sprintf(warn_buf,"Einheit '%s' geht hier nicht",s);
				anerror(warn_buf);
			} else {
				bcat(i);
				if (!does_default) {
					u=newunit(i,temp);
					addteach(order_unit, u);
				}
			}
		}
		count++;
	}
	return count;
}


int
getaunit(int type) {
	char *s, is_temp=0;
	int i;

	current_temp_no=0;
	cmd_unit=NULL;

	s=getstr();
	switch (findparam(s)) {
		case P_PEASANT:
			Scat(parameters[P_PEASANT]);
			this_unit=0;
			return 2;

		case P_TEMP:
			scat(" TEMP");
			s=getstr();
			current_temp_no=i=btoi(s);
			is_temp=1;
			break;

		default:
			i=btoi(s);
			break;
	}

	if (type==42) {	/* Nur Test, ob eine Einheit kommt, weil das ein Fehler ist */
		if (i)
			return 42;
		return 0;
	}

	this_unit=i;

	if (s[0]==0) {
		if (type==NECESSARY)
			anerror("Einheit fehlt");
		return 0;
	}
	/* wenn in s etwas drinnen steht, aber i==0 ist, gilt es! */
	if (s[0] && i==0) {
		if (empiria) awarning("Einheit 0 verwendet",2);
			/* Bei Empiria kann man sich leichter vertippen; base36 erlaubt mehr */
		scat("0");
		return 2; /* Einheit 0==Bauern! */
	}

	cmd_unit=newunit(i,is_temp); /* Die Unit schon machen, wegen TEMP-Check */
	bcat(i);
	if (is_temp)
		return 3;
	return 1;
}


void
copy_unit(unit *from, unit *to) {
	to->no=from->no;
	to->people=from->people;
	to->money=from->money;
	to->line_no=from->line_no;
	to->temp=from->temp;
	to->lives=from->lives;
	to->start_of_orders_line=from->start_of_orders_line;
	to->long_order_line=from->long_order_line;
	if (from->start_of_orders_line)
		to->start_of_orders=strdup(from->start_of_orders);
	if (from->long_order_line)
		to->long_order=strdup(from->long_order);
	if (from->order)
		to->order=strdup(from->order);
	to->lernt=from->lernt;
}


char *
netaddress(char *s) {
	char *s1, *s2;

	if (strncasecmp(s, "internet:", 9)!=0)
		return 0;

	s1=strchr(s, '@');
	if (!s1)
		return 0;

	while (s1 > s && *s1 && (isalnum(*s1) || *s1=='@' || *s1=='-' ||
					*s1=='.' || *s1=='_'))
		s1--;

	s1++;

	s2=s1;
	while (*s2 && (isalnum(*s2) || *s2=='@' || *s2=='-' || *s2=='.' || *s2=='_'))
		s2++;

	*s2=0;

	if (s2-s1>DISPLAYSIZE) {
		anerror("Adresse zu lang");
		return 0;
	}
	return s1;
}


void
checkemail(void) {
	char *s, *addr;

	scat(keywords[K_EMAIL]);
	addr=getstr();
	checkstring(addr, DISPLAYSIZE, NECESSARY);

	if (!addr) {
		awarning("eMail bitte mit EMAIL setzen",3);
		scat("; eMail bitte mit EMAIL setzen!");
		return;
	}
	s=strchr(addr, '@');
	if (!s) {
		anerror("Ungültige eMail-Adresse");
		return;
	}
	scat("; Zustellung an ");
	scat(addr);
}


void
checkaddr(void) {
	char *s, *addr;

	scat(keywords[K_ADDRESS]);
	s=getstr();
	checkstring(s, DISPLAYSIZE, NECESSARY);

	addr=netaddress(s);
	if (!addr) {
		strcpy(warn_buf,"Adresse enthält keine eMail mehr, bitte mit EMAIL setzen");
		awarning(warn_buf,1);
		scat("; eMail bitte mit EMAIL setzen");
	} else {
		scat("; Zustellung an ");
		scat(addr);
	}
}


/* Check auf langen Befehl usw. - aus ACheck.c 4.1 */

void
end_unit_orders(void) {
	if (!order_unit) /* Für die ersten Befehle der ersten Einheit. */
		return;

	if (order_unit->lives>0 && !order_unit->long_order_line && order_unit->people>0) {
		sprintf(warn_buf, "Einheit %s hat keinen langen Befehl bekommen",
							uid(order_unit));
		warning(warn_buf, order_unit->start_of_orders_line,
			 				order_unit->start_of_orders,2);
	}
}


void
orders_for_unit(int i, unit *u) {
	unit *t;
	char *k, *j, *e;
	int s;

	end_unit_orders();
	mother_unit=order_unit=u;

	if (u->start_of_orders_line) {
		sprintf(warn_buf, "Einheit %s hat schon in Zeile %d Befehle bekommen. ",
					uid(u), u->start_of_orders_line);
		do {
			i++; if (i<1) i=1;
			u=newunit(i, 0);
		} while (u->start_of_orders_line);
		strcat(warn_buf,"Benutze stattdessen Einheit ");
		strcat(warn_buf, itob(i));
		awarning(warn_buf,1);
		order_unit=u;
	}

	u->start_of_orders=strdup(order_buf);
	u->start_of_orders_line=line_no;
	u->lives=1;

	if (no_comment>0)
		return;

	k=strchr(order_buf,'[');
	if (!k) {
		awarning("Kann Kommentar zu Personen nicht auswerten",4);
		no_comment++;
		return;
	}
	k++;
	while (!atoi(k)) {	/* Hier ist eine [ im Namen;
												 0 Personen ist nicht in der Zugvorlage */
		k=strchr(k,'[');
		if (!k) {
			awarning("Kann Kommentar zu Personen nicht auswerten",4);
			no_comment++;
			return;
		}
		k++;
	}
	e=strchr(k,']');
	u->people+=atoi(k);
	j=strchr(k,',');
	if (!j) j=strchr(k,';');
	if (!j || j>e) {
		awarning("Kann Kommentar zu Silber nicht auswerten",4);
		no_comment++;
		return;
	}
	while (!isdigit(j[0])) j++;
	u->money+=atoi(j);
	j=strchr(k,'U');
	if (j && j<e) {	/* Muß ein Gebäude unterhalten */
		j++;
		if (isdigit(j[0]))
			u->unterhalt=atoi(j);
	}

	if (!empiria) {
		j=strchr(k,'I');
		if (j && j<e) {	/* I wie Illusion, auch Untote - eben alles, was nix frißt und
											 keinen langen Befehl braucht */
			u->lives=-1;
		}
		j=strchr(k,'s');
		if (j && j<e) {	/* Ist auf einem Schiff */
			j++;
			if (u->ship>=0) 	/* hat sonst schon ein Schiffskommando! */
				u->ship=btoi(j);
		}
		j=strchr(k,'S');
		if (j && j<e) {	/* Ist Kapitän auf einem Schiff */
			j++;
			s=-btoi(j);
			for (t=units; t; t=t->next)		/* vielleicht hat schon eine Einheit durch */
				if (t->ship==s) {						/* BETRETE das Kommando; das ist ja falsch */
					t->ship=-s;
					break;
				}
			u->ship=s;
		}
	}
	addregion(Rx, Ry, u->people);
}


void
orders_for_temp_unit(unit *u) {
	if (u->start_of_orders_line && u->region==Regionen) {
			/* in Regionen steht die aktuelle Region drin */
		sprintf (warn_buf, "`TEMP %s' wurde in Zeile %d schon gebraucht",
				 			itob(u->no), u->start_of_orders_line);
		anerror (warn_buf);
		/* return;	Trotzdem kein return, damit die Befehle ordnungsgemäß
								zugeordnet werden! */
	}
	u->line_no=line_no;
	u->lives=1;
	if (u->order)
		free(u->order);
	u->order=strdup(order_buf);
	if (u->start_of_orders)
		free(u->start_of_orders);
	u->start_of_orders=strdup(order_buf);
	u->start_of_orders_line=line_no;
	mother_unit=order_unit;
	order_unit=u;
}


void
long_order(void) {
	char *s, *q;
	int i;

	if (order_unit->long_order_line) {
		if (!empiria) {
			s=strdup(order_unit->long_order);
			q=strchr(s,' ');
			if (q) *q=0;	/* Den Befehl extrahieren */
			i=findkeyword(s);
			switch (i) {
				case K_ATTACK:
				case K_CAST:
					if (this_command==i)
						return;
					/* ATTACKIERE und ZAUBERE sind zwar keine langen Befehle, aber man darf
					 * auch keine anderen langen haben - darum ist bei denen long_order() */
				case K_SELL:
				case K_BUY:
					if (this_command==K_SELL || this_command==K_BUY)
						return;
					/* Es sind mehrere VERKAUFE (und rein theoretisch auch KAUFE)
					 * pro Einheit möglich */
			}
			if ((i==K_FOLLOW && this_command!=K_FOLLOW) ||
					(i!=K_FOLLOW && this_command==K_FOLLOW))
				return;		/* FOLGE ist nur Trigger */
		}

		if (strlen(order_unit->long_order) > DISPLAYSIZE+NAMESIZE)
			order_unit->long_order[DISPLAYSIZE+NAMESIZE]=0;
				/* zu lange Befehle kappen */
		sprintf(warn_buf, "Einheit %s hat schon einen langen Befehl in"
				" Zeile %d (`%s')", uid(order_unit),
				order_unit->long_order_line, order_unit->long_order);
		awarning(warn_buf,1);
	} else {
		order_unit->long_order=strdup(order_buf);
		order_unit->long_order_line=line_no;
	}
}


void
checknaming(void) {
	int i;
	char *s;

	scat(keywords[K_NAME]);
	i=findparam(getstr());
	s=getstr();
	if (!empiria && i==P_FREMD) {
		if (i==P_REGION) {
			sprintf(warn_buf,"BENENNE FREMD nicht mit %s",parameters[i]);
			anerror(warn_buf);
		} else
			Scat(parameters[P_FREMD]);
		i=findparam(s);
		s=getstr();
	}
	if (strchr(s,'('))
		awarning("Namen dürfen keine Klammern enthalten",1);

	switch (i) {
		case -1:
			anerror("Objekt nicht erkannt");
			break;

		case P_UNIT:
		case P_FACTION:
		case P_GEBAEUDE:
		case P_CASTLE:
		case P_SHIP:
		case P_REGION:
			Scat(parameters[i]);
			checkstring(s, NAMESIZE, NECESSARY);
			break;

		default:
			anerror("Objekt kann nicht umbenannt werden");
	}
}


void
checkdisplay(void) {
	int i;

	scat(keywords[K_DISPLAY]);
	i=findparam(getstr());

	switch (i) {
		case -1:
			anerror("Objekt nicht erkannt");
			break;

		case P_REGION:
		case P_UNIT:
		case P_GEBAEUDE:
		case P_CASTLE:
		case P_SHIP:
		case P_PRIVAT:
			Scat(parameters[i]);
			checkstring(getstr(), 0, POSSIBLE);
			break;

		default:
			anerror("Objekt kann nicht beschrieben werden");
	}
}


void
check_leave(void) {
	unit *t,*T=NULL;
	int s;

	s=order_unit->ship;
	order_unit->ship=0;
	if (s<0) {	/* wir waren Kapitän, neuen suchen */
		for (t=units; t; t=t->next)
			if (t->ship==-s)	/* eine Unit auf dem selben Schiff */
				T=t;							/* in ECheck sind die Units down->top, darum */
		if (T)								/* die letzte der Liste==erste Unit im Programm */
			T->ship=s;
	}
}


void
checkenter(void) {
	int i,n;
	unit *u;

	scat(keywords[K_ENTER]);
	i=findparam(getstr());

	switch (i) {
		case -1:
			anerror("Objekt nicht erkannt");
			return;
		case P_GEBAEUDE:
		case P_CASTLE:
		case P_SHIP:
			Scat(parameters[i]);
			n=getI();
			if (!n) {
				anerror("Nummer des Objektes fehlt");
				return;
			} else
				bcat(n);
			break;
		default:
			anerror("Objekt kann nicht betreten werden");
			return;
	}
	check_leave();
	if (i==P_SHIP) {
		order_unit->ship=n;
		for (u=units; u; u=u->next) 	/* ggf. Kommando geben */
			if (u->ship==-n)	/* da hat einer schon das Kommando */
				return;
		order_unit->ship=-n;
	}
}


int
getaspell(char *s, char spell_typ, unit *u, int turmzauber) {
	t_spell *sp;
	int p;

	if (!empiria) {
		if (*s=='[' || *s=='<') {
			anerror("[ ] und  < >  dürfen nicht mit eingegeben werden!");
			return 0;
		}
		if (findparam(s) == P_REGION) {
			scat(parameters[P_REGION]);
			s=getstr();
			if (*s) {
				p=atoi(s);
				icat(p);
				s=getstr();
				if (*s) {
					p=atoi(s);
					icat(p);
				} else {
					anerror("Fehler bei Regions-Koordinaten");
					return 0;
				}
			} else {
				anerror("Fehler bei REGION-Parametern");
				return 0;
			}
			s=getstr();
		}
		if (findparam(s) == P_STUFE) {
			scat(parameters[P_STUFE]);
			s=getstr();
			if (!*s || atoi(s)<1) {
				anerror("Fehler bei STUFE-Parameter");
				return 0;
			}
			p=atoi(s);
			icat(p);
			s=getstr();
		}
	}
	sp=findspell(s);
	if (!sp) {
		if (u) {	/* sonst ist das der Test von GIB */
			if (show_warnings > 0)	/* nicht bei -w0 */
				anerror("Zauberspruch nicht erkannt");
			if (*s>='0' && *s<='9')
				anerror("Parameter STUFE oder REGION fehlt");
			qcat(s);
		}
		return 0;
	}
	qcat(sp->name);
	if (!(sp->typ & spell_typ)) {
		sprintf(warn_buf, "\"%s\" ist %sein Kampfzauber", sp->name,
						(sp->typ & SP_ZAUBER) ? "k" : "");
		if (show_warnings > 0)	/* nicht bei -w0 */
			anerror(warn_buf);
	} else {
		if (u && (sp->typ & SP_BATTLE) && (u->spell & sp->typ)) {
			sprintf(warn_buf, "Einheit %s hat schon einen ",uid(u));
			switch (sp->typ) {
				case SP_POST:
					strcat(warn_buf,"Post-");
					break;
				case SP_PRAE:
					strcat(warn_buf,"Prä-");
					break;
			}
			strcat(warn_buf,"Kampfzauber gesetzt");
			if (show_warnings > 0)	/* nicht bei -w0 */
				awarning(warn_buf,1);
		}
		if (u) {
			p=sp->typ*turmzauber;
			u->spell |= p;
			u->money-=sp->kosten;
			u->reserviert-=sp->kosten;
		}
	}
	do {
		s=getstr();
		if (*s) Scat(s);
	} while (*s);		/* restliche Parameter ohne Check ausgeben */
	return 1;
}


void
checkgiving(int key) {
	char *s;
	int i, n;

	scat(keywords[key]);
	getaunit(NECESSARY);
	s=getstr();
	if (!getaspell(s, SP_ALL, NULL, 0)
			&& !isparam(s, P_CONTROL,1)
			&& !isparam(s, P_HERBS,1)
			&& !isparam(s, P_SPELLBOOK,1)
			&& !isparam(s, P_UNIT,1)) {
		n=atoi(s);
		if (n<1) {
			if (!empiria && findparam(s)==P_ALLES) {	/* GIB xx ALLES wasauchimmer */
				n=-1;
				Scat(parameters[P_ALLES]);
			} else {
				anerror("Anzahl Gegenstände/Personen/Silber fehlt");
				n=1;
			}
		}
		if (n>0) icat(n);

		s=getstr();
		if (!(*s) && n<0 && !empiria) {	/* GIB xx ALLES */
			if (cmd_unit) {
				n=order_unit->money-order_unit->reserviert;
				cmd_unit->money+=n;
				cmd_unit->reserviert+=n;
			}
			order_unit->money=order_unit->reserviert;
			return;
		}

		if (!(*s)) {
			anerror("Was übergeben?");
			return;
		}
		i=findparam(s);
		switch (i) {
			case P_PERSON:
				Scat(parameters[i]);
				if (n<0) n=order_unit->people;
				if (cmd_unit)
					cmd_unit->people+=n;
				order_unit->people-=n;
				if (order_unit->people<0 && no_comment<1 && !does_default) {
					sprintf(warn_buf,"Einheit %s hat evtl. zu wenig Personen", uid(order_unit));
					awarning(warn_buf,4);
				}
				break;

			case P_SILVER:
				Scat(parameters[i]);
				if (n<0) n=order_unit->money-order_unit->reserviert;
				if (cmd_unit) {
					cmd_unit->money+=n;
					cmd_unit->reserviert+=n;
				}
				order_unit->money-=n;
				if (order_unit->money<0 && no_comment<1 && !does_default) {
						sprintf(warn_buf,"Einheit %s hat evtl. zu wenig Silber",
										uid(order_unit));
					awarning(warn_buf,4);
				}
				break;

			default:
				i=finditem(s);
				if (i < 0) {
					i=findherb(s);
					if (i < 0) {
						i=findpotion(s);
						if (i >= 0) {
							if (piping) {
								strcpy(warn_buf,potionnames[n!=1][i]);
								s=strchr(warn_buf, ' ');
								if (s) *s=0;
								Scat(warn_buf);
							} else {
								qcat(potionnames[n!=1][i]);
							}
						} else {
							awarning("Objekt nicht erkannt",1);
						}
					} else {
						if (piping) {
							strcpy(warn_buf,herbdata[n!=1][i]);
							s=strchr(warn_buf, ' ');
							if (s) *s=0;
							Scat(warn_buf);
						} else {
							qcat(herbdata[n!=1][i]);
						}
					}
				} else {
					if (piping) {
						strcpy(warn_buf,ItemName(i,n!=1));
						s=strchr(warn_buf, ' ');
						if (s) *s=0;
						Scat(warn_buf);
					} else {
						qcat(ItemName(i,n!=1));
					}
				}
				break;
		}
	} else if (findparam(s)==P_CONTROL) {
		if (order_unit->ship && !does_default) {
			if (order_unit->ship>0) {
				sprintf(warn_buf,"Einheit %s hat evtl. nicht das Kommando"
						" über Schiff %d",uid(order_unit),order_unit->ship);
				awarning(warn_buf,4);
			} else if (cmd_unit) {
				if (cmd_unit->ship!=0 && abs(cmd_unit->ship) != -order_unit->ship) {
					sprintf(warn_buf,"Einheit %s ist evtl. nicht auf Schiff %d sondern"
							" auf Schiff %d", uid(cmd_unit), -order_unit->ship, abs(cmd_unit->ship));
					awarning(warn_buf,4);
				}
				cmd_unit->ship=order_unit->ship;
			}
			order_unit->ship=-order_unit->ship;
		} else if (order_unit->unterhalt) {
			if (cmd_unit)
				cmd_unit->unterhalt=order_unit->unterhalt;
			order_unit->unterhalt=0;
		}
	}
}


void getluxuries(int cmd) {
	char *s;
	int n, i;

	s=getstr();

	n=atoi(s);
	if (n<1) {
		if (!empiria && findparam(s)==P_ALLES) {
			if (cmd==K_BUY) {
				anerror("KAUFE geht nicht mit ALLES");
				return;
			} else
				scat(" ALLE");
		} else {
			anerror("Anzahl Luxusgüter fehlt");
		}
		n=1;
	}

	icat(n);
	s=getstr();
	i=finditem(s);

	if (!isluxury(i))
		anerror("Kein Luxusgut");
	else {
		Scat(ItemName(i,n!=1));
		if (cmd==K_BUY) {	/* Silber abziehen; nur Grundpreis! */
			switch (i) {
				case I_BALM:
					i=4; break;
				case I_SPICES:
					i=5; break;
				case I_JEWELERY:
					i=7; break;
				case I_MYRRH:
					i=5; break;
				case I_OIL:
					i=3; break;
				case I_SILK:
					i=6; break;
				case I_INCENSE:
					i=4; break;
			}
			order_unit->money-=i*n;
			order_unit->reserviert-=i*n;
		}
	}
}


void
checkmake(void) {
	int i, j=0, k=0;
	char *s;

	scat(keywords[K_MAKE]);
	s=getstr();

	if (s[0]>='0' && s[0]<='9') { /* MACHE anzahl "Gegenstand" */
		j=atoi(s);
		if (j==0)
			awarning("Anzahl 0 macht keinen Sinn",2);
		s=getstr();
	}

	i=findparam(s);

	switch (i) {
		case P_HERBS:
			if (j) {
				if (empiria)
					anerror("Anzahl hier nicht möglich");
				else
					icat(j);
			}
			scat(" Kräuter");
			long_order();
			return;
		case P_TEMP:
			if (j)
				anerror("Anzahl hier nicht möglich");
			next_indent=INDENT_NEW_ORDERS;
			scat(" TEMP");
			j=getb();
			if (!j)
				anerror("Keine TEMP-Nummer");
			else {
				unit *u;
				bcat(j);
				if (!empiria) {
					s=getstr();
					if (*s) qcat(s);
				}
				from_temp_unit_no=j;
				u=newunit(j,42);
				if (u->ship==0)
					u->ship=abs(order_unit->ship);
				orders_for_temp_unit(u);
			}
			return;

		case P_SHIP:
			if (j>0) icat(j);
			k=getI();
			Scat(parameters[i]);
			if (k) bcat(k);
			long_order();
			return;

		case P_ROAD:
			if (j>0) icat(j);
			Scat(parameters[i]);
			k=getdirection();
			if (k<0)
				anerror("MACHE STRASSE <richtung>");
			else
				Scat(directions[k]);
			long_order();
			return;
	}

	i=findshiptype(s);
	if (i != -1) {
		qcat(shiptypes[i]);
		long_order();
		getI();
		return;
	}

	i=finditem(s);
	if (!isluxury (i)) {
		if (j)
			icat(j);
		if (piping) {
			strcpy(warn_buf,ItemName(i,1));
			s=strchr(warn_buf, ' ');
			if (s) *s=0;
			Scat(warn_buf);
		} else {
			qcat(ItemName(i,1));
		}
		long_order();
		return;
	}

	i=findpotion(s);
	if (i != -1) {
		if (j)
			awarning("Anzahl hier nicht möglich",2);
		if (piping) {
			strcpy(warn_buf,potionnames[1][i]);
			s=strchr(warn_buf, ' ');
			if (s) *s=0;
			Scat(warn_buf);
		} else {
			qcat(potionnames[1][i]);
		}
		long_order();
		return;
	}

	i=findbuildingtype(s);
	if (i != -1) {
		if (j) icat(j);
		qcat(buildingtypes[i]);
		long_order();
		k=getI();
		if (k) bcat(k);
		return;
	}

	/* Man kann MACHE ohne Parameter verwenden, wenn man in einer Burg oder
 		einem Schiff ist. Ist aber trotzdem eine Warnung wert. */
	if (s[0])
		anerror("Sowas kann man nicht machen");
	else
		awarning("Einheit muß in einer Burg, in einem Gebäude"
							" oder auf einem Schiff sein",4);
	long_order();
	/* es kam ja eine Meldung - evtl. kennt ECheck das nur nicht? */
}


void
checkdirections(int key) {
	int i, count=0, x, y, sx, sy, rx, ry;

	rx=sx=x=Rx; ry=sy=y=Ry;
	scat(keywords[key]);

	do {
		i=getdirection();
		if (i<=-1 || (empiria && i >= D_PAUSE) || (!empiria && i < D_EAST)) {
			if (i==-2) break;
			anerror("Richtung nicht erkannt");
		} else {
			if (key==K_ROUTE && i==D_PAUSE && count==0)
				awarning("ROUTE beginnt mit PAUSE",2);
			if (key==K_MOVE && i==D_PAUSE)
				anerror("NACH geht nicht mit PAUSE");
			else {
				Scat(directions[i]);
				count++;
				if (!noship && order_unit->ship==0 && key!=K_ROUTE && count==4) {
					sprintf(warn_buf,"Einheit %s bewegt sich evtl. zu weit",
							uid(order_unit));
					awarning(warn_buf,4);
				}
				switch (i) {
					case D_NORTHEAST:
						y++;
						break;
					case D_NORTHWEST:
						y++; x--;
						break;
					case D_SOUTHEAST:
						y--; x++;
						break;
					case D_SOUTHWEST:
						y--;
						break;
					case D_EAST:
						x++;
						break;
					case D_WEST:
						x--;
						break;
					case D_PAUSE:
						if (rx==sx && ry==sy) {
							rx=x; ry=y;	/* ROUTE: ersten Abschnitt merken für Silber verschieben */
						}
						break;
				}
			}
		}
	} while (i >= 0);

	if (!count)
		anerror("Richtung nicht erkannt");
	if (key==K_ROUTE && !noroute && (sx!=x || sy!=y)) {
		sprintf(warn_buf, "ROUTE ist kein Kreis; (%d,%d) -> (%d,%d)",sx,sy,x,y);
		awarning(warn_buf,4);
	}
	if (!does_default) {
		if (order_unit->hasmoved) {
			sprintf(warn_buf, "Einheit %s hat sich schon bewegt", uid(order_unit));
			anerror(warn_buf);
			return;
		}
		order_unit->hasmoved=2;	/* 2: selber bewegt */
		if (key==K_ROUTE) {
			order_unit->newx=rx;
			order_unit->newy=ry;
		} else {
			order_unit->newx=x;
			order_unit->newy=y;
		}
	}
}


void
check_sabotage(void) {
	if (getparam() != P_SHIP) {
		anerror("Bislang gibt es nur SABOTIERE SCHIFF");
		return;
	}
	Scat(parameters[P_SHIP]);
	return;
}


void
checkmail(void) {
	char *s;

	scat(keywords[K_MAIL]);
	s=getstr();
	if (strcasecmp(s, "an")==0)
		s=getstr();

	switch (findparam(s)) {
		case P_FACTION:
			Scat(parameters[P_FACTION]);
			break;

		case P_REGION:
			Scat(parameters[P_REGION]);
			break;

		/* implizit case P_UNIT: */
		default:
			Scat(parameters[P_UNIT]);
			bcat(getb());
			return;
	}
	s=getstr();
	checkstring(s, 0, NECESSARY);
}


void
reserve(void) {
	char *s;
	int i,n;

	scat(keywords[K_RESERVE]);

	if (from_temp_unit_no) {
		anerror("TEMP-Einheiten können nichts reservieren! Bitte GIB benutzen");
		return;
	}
	n=geti();
	if (n==0) {
		anerror("RESERVIERE 0 xxx macht keinen Sinn");
		return;
	}
	icat(n);
	s=getstr();

	if (!(*s)) {
		anerror("RESERVIERE was?");
		return;
	}
	i=finditem(s);
	if (i>=0) {
		Scat(ItemName(i,n!=1));
		return;
	}

	if (findparam(s)==P_SILVER) {
		Scat("Silber");
		order_unit->region->reserviert+=n;
		order_unit->reserviert+=n;
		return;
	}

	i=findpotion(s);
	if (i>=0) {
		Scat(potionnames[n!=1][i]);
		return;
	}

	i=findherb(s);
	if (i>=0) {
		Scat(herbdata[n=!1][i]);
		return;
	}
}


void
check_ally(void) {
	int i;
	char *s;

	scat(keywords[K_ALLY]);
	getafaction(getstr());
	s=getstr();

	i=findparam(s);
	switch (i) {
		case P_GIVE:
		case P_KAEMPFE:
		case P_SILVER:
		case P_OBSERVE:
		case P_GUARD:
		case P_ALLES:
		case P_NOT:
			Scat(parameters[i]);
			break;
		default:
			anerror("Helfe-Status falsch");
			return;
	}

	s=getstr();
	if (findparam(s)==P_NOT) {
		Scat(parameters[P_NOT]);
	}
}


int
studycost(t_skills *talent) {
	if (does_default)
		return 0;
	if (talent->kosten<0) {
		int i;
		/* Lerne Magie [gebiet] 2000 -> Lernkosten=2000 Silber */
		char *s=getstr();
		i=findstr(magiegebiet,s,5);
		if (i>=0) {
			fprintf(ERR,"Magiegebiet '%s' gewählt",magiegebiet[i]);
			i=geti();
		} else
			i=atoi(s);
		if (i<100) {
			i=200;
			awarning("Lernkosten mit 200 Silber angenommen",2);
		}
		return i;
	}
	return talent->kosten;
}


void
check_meinung(void) {
	int i;

	scat(keywords[K_MEINUNG]);
	i=geti();
	if (i <= 0)
		anerror("Nummer der Umfrage muß größer 0 sein");
	icat(i);
	i=geti();
	if (i <= 0)
		anerror("Nummer der Meinung muß größer 0 sein");
	icat(i);
	checkstring(getstr(),DISPLAYSIZE,POSSIBLE);
}


void
check_comment(void) {
	char *s;
	int m;

	s=getstr();
	if (strncasecmp(s,"ECHECK",6))
		return;
	s=getstr();

	if (strncasecmp(s,"VERSION",7)==0) {
		fprintf(ERR, "%s\n", order_buf);
		return;
	}
	if (strncasecmp(s,"NOWARN", 6)==0) {	/* Warnungen für nächste Zeile aus */
		warn_off=2;
		return;
	}
	if (strncasecmp(s,"LOHN",4)==0) {	/* LOHN für Arbeit */
		m=geti();
		lohn=(char)max(10,m);
		return;
	}
	if (strncasecmp(s,"ROUT",4)==0) {	/* ROUTe */
		noroute=(char)(1-noroute);
		return;
	}
	if (strncasecmp(s,"KOMM",4)==0) {	/* KOMMando */
		m=geti();
		if (!m) {
			m=order_unit->ship;
			if (!m)
				m=rand();
		}
		order_unit->ship=-abs(m);
		return;
	}
	if (strncasecmp(s,"EMPTY",5)==0) {
		order_unit->money=0;
		order_unit->reserviert=0;
		return;
	}
	if (strncasecmp(s,"NACH",4)==0) {
		order_unit->hasmoved=1;
		order_unit->newx=geti();
		order_unit->newy=geti();
		return;
	}
	m=atoi(s);
	if (m) {
		if (order_unit) {
			order_unit->money+=m;
			order_unit->reserviert+=m;
		}
		return;
	}
}


void
check_money(bool do_move) {	/* do_move=true: vor der Bewegung, anschließend */
	unit *u, *t;							/* Bewegung ausführen, damit das Silber bewegt wird */
	t_region *r;
	int i,x,y,um;

	u=find_unit(-1,0);
	if (u) {	/* Unit -1 leeren */
		u->people=u->money=u->unterhalt=u->reserviert=0;
		u->lives=-1;
	}

	if (do_move) {
		for (u=units; u; u=u->next) {		/* Check TEMP und leere Einheiten */
			if (u->lives<1) 	/* fremde Einheit oder Untot/Illusion */
				continue;				/* auslassen, weil deren Geldbedarf nicht zählt */

			if (u->temp && abs(u->temp)!=42) {
				sprintf(warn_buf, "Einheit TEMP %s wurde nicht mit "
								"MACHE TEMP generiert", itob(u->no));
				Error(warn_buf, u->line_no, u->order);
			}
			if (u->people<0) {
				sprintf(warn_buf,"Einheit %s hat %d Personen!", uid(u), u->people);
				warn(warn_buf, u->line_no, 3);
			}

			if (u->people==0 && ((!nolost && !u->temp && u->money>0) || u->temp)) {
				if (u->temp) {
					if (u->money>0)
						sprintf(warn_buf,"Einheit TEMP %s hat nicht rekrutiert und keine Personen"
								" bekommen! Sie verliert Silber und evtl. Gegenstände", itob(u->no));
					else
						sprintf(warn_buf,"Einheit TEMP %s hat nicht rekrutiert und keine Personen"
								" bekommen", itob(u->no));
					warn(warn_buf,u->line_no,2);
				} else if (no_comment<=0) {
					sprintf(warn_buf,"Einheit %s verliert Silber und evtl. Gegenstände", uid(u));
					warn(warn_buf,u->line_no,3);
				}
			}
		}
	}

	if (Regionen && no_comment<1) {
		if (silberpool && do_move) {
			for (u=units; u; u=u->next) {
				u->region->geld+=u->money;
				/* Reservierung < Silber der Unit? Silber von anderen Units holen */
				if (u->reserviert > u->money) {
					i=u->reserviert-u->money;
					for (t=units; t && i>0; t=t->next) {
						if (t->region!=u->region || t==u) continue;
						um=min(i,t->money-t->reserviert);
						if (um>0) {
							u->money+=um;
							i-=um;
							t->money-=um;
						}
					}
				}
			}
		}

		if (do_move)
			for (r=Regionen; r; r=r->next) {
				if (r->reserviert>0 && r->reserviert>r->geld) {	/* nur explizit mit RESERVIERE */
					sprintf(warn_buf, "In %s (%d,%d) wurde mehr Silber reserviert (%d)"
									" als vorhanden (%d).", r->name, r->x, r->y, r->reserviert, r->geld);
					warn(warn_buf, r->line_no, 3);
				}
			}

		for (u=units; u; u=u->next) {		/* fehlendes Silber aus dem Silberpool nehmen */
			if (do_move && u->unterhalt) {
				u->money-=u->unterhalt;
				u->reserviert-=u->unterhalt;
			}
			if (u->money<0 && silberpool) {
				for (t=units; t && u->money<0; t=t->next) {
					if (t->region!=u->region || t==u) continue;
					um=min(-u->money,t->money-t->reserviert);
					if (um>0) {
						u->money+=um;
						u->reserviert+=um;	/* das so erworbene Silber muß auch reserviert sein */
						t->money-=um;
					}
				}
			}
			if (u->money<0) {
				sprintf(warn_buf,"Einheit %s hat %s%d Silber!",
							uid(u),do_move?"vor den Einnahmen ":"", u->money);
				warn(warn_buf, u->line_no, 3);
				if (u->unterhalt) {
					if (do_move)
						sprintf(warn_buf,"Einheit %s braucht noch %d Silber, "
									"um ein Gebäude zu unterhalten.", uid(u), -u->money);
					else
						sprintf(warn_buf,"Einheit %s kann ein Gebäude nicht erhalten,"
									" es fehlen noch %d Silber!", uid(u), -u->money);
					warn(warn_buf, u->line_no, 1);
				}
			}
		}
	}

	if (Regionen)
		for (r=Regionen; r; r=r->next)
			r->geld=0;	/* gleich wird Geld bei den Einheiten bewegt, darum den Pool
									 * leeren und nach der Bewegung nochmal füllen */
	if (!do_move) 	/* Ebenso wird es in check_living nochmal neu eingezahlt */
		return;

	if (!Regionen)	/* ohne Regionen Bewegung nicht sinnvoll */
		return;

	for (u=units; u; u=u->next) {			/* Bewegen vormerken */
		if (u->lives<1) 	/* fremde Einheit oder Untot/Illusion */
			continue;
		if (u->hasmoved>1) {
			if (!noship && u->ship>0) {
				sprintf(warn_buf,"Einheit %s will Schiff %d bewegen und"
						" hat evtl. kein Kommando", uid(u), u->ship);
				warn(warn_buf,u->line_no,4);
			}
			i=-u->ship;
			if (i>0) {	/* wir sind Kapitän; alle Einheiten auf dem Schiff auch bewegen */
				x=u->newx; y=u->newy;
				for (t=units; t; t=t->next) {
					if (t->ship==i) {
						if (t->hasmoved>1) {	/* schon bewegt! */
							sprintf(warn_buf, "Einheit %s auf Schiff %d hat sich schon bewegt",
									uid(t),i);
							Error(warn_buf,t->line_no,t->long_order);
						}
						t->hasmoved=1;
						t->newx=x;
						t->newy=y;
					}
				}
			}
		}
	}

	for (u=units; u; u=u->next) {			/* Bewegen ausführen */
		if (u->lives<1) 	/* fremde Einheit oder Untot/Illusion */
			continue;

		if (u->transport && u->drive && u->drive != u->transport) {
			sprintf(checked_buf,"Einheit %s wird von Einheit %s transportiert, "
					"fährt aber mit ", uid(u), Uid(u->transport));
			scat(Uid(u->drive));
			warning(checked_buf,u->line_no,u->long_order,1);
			continue;
		}
		if (u->drive) {	/* FAHRE; in u->transport steht die transportierende Einheit */
			if (u->hasmoved) {
				sprintf(warn_buf,"Einheit %s hat sich bereits bewegt", uid(u));
				Error(warn_buf, u->line_no, u->long_order);
			}
			if (u->transport==0) {
				t=find_unit(u->drive,0);
				if (!t) t=find_unit(u->drive,1);
				if (t && t->lives) {
					sprintf(warn_buf,"Einheit %s fährt mit Einheit %s, "
							"diese transportiert aber nicht",uid(u),Uid(u->drive));
					Error(warn_buf,u->line_no,u->long_order);
				} else {	/* unbekannte Einheit -> unbekanntes Ziel */
					u->hasmoved=1;
					u->newx=-9999;
					u->newy=-9999;
				}
			} else {
				t=find_unit(u->transport,0);
				if (!t) t=find_unit(u->transport,1);
					/* muß es geben, hat ja schließlich u->transport gesetzt */
				u->hasmoved=1;
				u->newx=t->newx;
				u->newy=t->newy;
			}
		} else if (u->transport) {
			t=find_unit(u->transport,0);
			if (!t) t=find_unit(u->transport,1);
			if (t && t->lives && t->drive != u->no) {
				sprintf(warn_buf, "Einheit %s transportiert Einheit %s, "
						"diese fährt aber nicht",Uid(u->transport),uid(u));
				Error(warn_buf,u->line_no,u->long_order);
			}
		}

		if (u->hasmoved) {	/* NACH, gefahren oder auf Schiff */
			addregion(u->region->x, u->region->y, -(u->people));
			u->region=addregion(u->newx, u->newy, u->people);
			if (u->region->line_no==line_no)	/* line_no => NÄCHSTER */
				u->region->line_no=u->line_no;
		}
	}
}


void
check_living(void) {
	unit *u;
	t_region *r;

	/* Für die Nahrungsversorgung ist auch ohne Silberpool alles Silber der
	 * Region zuständig */

	for (u=units; u; u=u->next) {	/* Silber der Einheiten in den Silberpool "einzahlen" */
		if (u->lives<1)							/* jetzt nach der Umverteilung von Silber */
			continue;
		u->region->geld+=u->money;	/* jetzt wird reserviertes Silber nicht festgehalten */
	}

	for (r=Regionen; r; r=r->next) {
		for (u=units; u; u=u->next)
			if (u->region==r && u->lives>0)
				r->geld-=u->people*10;
		if (r->geld<0) {
			sprintf(warn_buf, "Das Silber in %s (%d,%d) reicht evtl. nicht"
							" zum Leben; es fehlen %d Silber.", r->name, r->x, r->y, -(r->geld));
			warn(warn_buf, r->line_no, 4);
		}
	}
}


void
remove_temp(void) {
	/* Markiert das TEMP-Flag von Einheiten der letzten Region
	 * -> falsche TEMP-Nummern bei GIB oder so fallen auf */
	unit *u;

	for (u=units; u; u=u->next)
		u->temp=-u->temp;
}


void
check_teachings(void) {
	teach *t;
	unit *u;
	int n;

	for (t=teachings; t; t=t->next) {
		t->student->schueler=t->student->people;
		if (t->teacher) {
			t->teacher->lehrer=t->teacher->people*10;
			if (t->teacher->lehrer==0) {
			  sprintf(warn_buf, "Einheit %s hat 0 Personen und lehrt Einheit ",
						uid(t->teacher));
			  strcat(warn_buf, uid(t->student));
			  strcat(warn_buf, ".");
			  warn(warn_buf, t->teacher->line_no, 4);
			}
			if (t->student->schueler==0 && t->student->lives>0) {
				sprintf(warn_buf, "Einheit %s hat 0 Personen und wird von Einheit ",
						uid(t->student));
				strcat(warn_buf, uid(t->teacher));
				strcat(warn_buf, " gelehrt.");
				warn(warn_buf, t->student->line_no, 4);
			}
		}
	}

	for (t=teachings; t; t=t->next) {
		if (t->teacher==NULL || t->student->lives<1) {	/* lernt ohne Lehrer bzw. */
			t->student->schueler=0;												/* keine eigene Einheit */
			continue;
		}

		if (!t->student->lernt) {
			if (t->student->temp )	/* unbekannte TEMP-Einheit, wird eh schon angemeckert */
				continue;
			sprintf(warn_buf,"Einheit %s wird von Einheit ", uid(t->student));
			strcat(warn_buf,uid(t->teacher));			/* uid() hat ein static char*, das
																			 im sprintf() dann zweimal das selbe ergibt */
			strcat(warn_buf," gelehrt, lernt aber nicht.");
			warn(warn_buf, t->student->line_no, 2);
			t->student->schueler=0;
			continue;
		}

		n=min(t->teacher->lehrer, t->student->schueler);
		t->teacher->lehrer-=n;
		t->student->schueler-=n;
	}

	for (u=units; u; u=u->next) {
		if (u->lives<1) continue;
		if (u->lehrer>0) {
			sprintf(warn_buf,"Einheit %s kann noch %d Schüler lehren.",
					uid(u), u->lehrer);
			warn(warn_buf, u->line_no, 5);
		}
		if (u->schueler>0) {
			sprintf(warn_buf,"Einheit %s kann noch %d Lehrer gebrauchen.",
					uid(u), (u->schueler+9)/10);
			warn(warn_buf, u->line_no, 5);
		}
	}
}


void
checkanorder(char *Orders) {
	int i, x;
	char *s;
	t_skills *sk;
	unit *u;

	s=strchr(Orders,';');
	if (s)
		*s=0;	/* Ggf. Kommentar kappen */

	if (Orders[0]==0)
		return; 	/* Dies war eine Kommentarzeile */

	strcpy(order_buf, Orders);

	i=igetkeyword(order_buf);
	this_command=i;
	switch (i) {
		case -1:
			anerror("Befehl nicht erkannt");
			return;

		case K_ADDRESS:
			if (empiria)
				checkaddr();
			else
				anerror("Statt ADRESSE bitte EMAIL und BANNER benutzen");
			break;

		case K_NUMMER:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else {
				scat(keywords[K_NUMMER]);
				i=getparam();
				if (!(i==P_UNIT || i==P_SHIP || i==P_GEBAEUDE || i==P_CASTLE
							|| i==P_FACTION)) {
					anerror("NUMMER SCHIFF, NUMMER BURG, NUMMER PARTEI oder NUMMER EINHEIT");
					break;
				}
				Scat(parameters[i]);
				s=getstr();
				if (strncasecmp(s,"TEMP",strlen(s))==0)
					anerror("Nummer nicht erlaubt");
				else
					Scat(s);
			}
			break;

		case K_MEINUNG:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else
				check_meinung();
			break;

		case K_BANNER:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else {
				scat(keywords[K_BANNER]);
				checkstring(getstr(),DISPLAYSIZE,NECESSARY);
			}
			break;

		case K_EMAIL:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else
				checkemail();
			break;

		case K_URSPRUNG:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else {
				scat(keywords[K_URSPRUNG]);
				s=getstr();
				if (*s) {
					x=atoi(s);
					icat(x);
					s=getstr();
					if (*s) {
						x=atoi(s);
						icat(x);
					} else
						anerror("Beide Koordinaten müssen angegeben werden");
				}
			}
			break;

		case K_USE:
			scat(keywords[K_USE]);
			s=getstr();
			i=findpotion(s);
			if (i<0)
				anerror("Unbekannter Trank");
			else {
				Scat(potionnames[1][i]);
			}
			break;

		case K_MAIL:
			checkmail();
			break;

		case K_WORK:
			scat(keywords[K_WORK]);
			long_order();
			if (!does_default)
				order_unit->money+=lohn*order_unit->people;
			break;

		case K_ATTACK:
			scat(keywords[K_ATTACK]);
			if (getaunit(NECESSARY)==3)
				anerror("TEMP-Einheiten kann man nicht ATTACKIEREn");
			long_order();
					/* ATTACKIERE ist zwar kein langer Befehl, aber man darf auch keine
						 anderen langen haben - darum ist bei ATTACKIERE long_order();
						 Check in order_for_unit() */
			if (getaunit(42)==42) {
				strcpy(warn_buf,"Pro Einheit muß ein ATTACKIERE-Befehl gegeben werden");
				anerror(warn_buf);
			}
			break;

		case K_BESIEGE:
			scat(keywords[K_BESIEGE]);
			i=getI();
			if (!i)
				anerror("Nummer der Burg fehlt");
			else
				bcat(i);
			long_order();
			break;

		case K_NAME:
			checknaming();
			break;

		case K_ZUECHTE:
			scat(keywords[K_ZUECHTE]);
			if (!empiria) {
				i=getparam();
				if (i==P_HERBS || i==P_HORSE)
					scat(parameters[i]);
				else
					anerror("ZÜCHTE PFERDE oder ZÜCHTE KRÄUTER");
			}
			long_order();
			break;

		case K_STEAL:
			scat(keywords[K_STEAL]);
			getaunit(NECESSARY);
			long_order();
			break;

		case K_DISPLAY:
			checkdisplay();
			break;

		case K_GUARD:
			scat(keywords[K_GUARD]);
			s=getstr();
			if (findparam(s)==P_NOT) {
				Scat(parameters[P_NOT]);
			}
			break;

		case K_END:
			if (from_temp_unit_no==0)
				awarning("ENDE ohne MACHE TEMP",2);
			scat(keywords[K_END]);
			indent=next_indent=INDENT_ORDERS;
			end_unit_orders();
			from_temp_unit_no=current_temp_no=0;
			if (mother_unit) {
				order_unit=mother_unit;
				mother_unit=NULL;
			}
			break;

		case K_FIND:
			if (!empiria)
				awarning("FINDE wurde gegen OPTION ADRESSEN ersetzt",1);
			else {
				scat(keywords[K_FIND]);
				s=getstr();
				if (findparam(s)==P_ALLES) {
					Scat(parameters[P_ALLES]);
				} else
					getafaction(s);
			}
			break;

		case K_RESEARCH:
			scat(keywords[K_RESEARCH]);
			i=getparam();
			if (i==P_HERBS) {
				scat(parameters[P_HERBS]);	/* momentan nur FORSCHE KRÄUTER */
			} else
				anerror("Es gibt nur FORSCHE KRÄUTER");
			long_order();
			break;

		case K_SETSTEALTH:
			scat(keywords[K_SETSTEALTH]);
			s=getstr();
			i=findstr(Rassen, s, R_MAX);
			if (i >= 0) {
				Scat(Rassen[i]);
				break;
			}
			i=findparam(s);
			if (i==P_FACTION) {
				Scat(parameters[i]);
				s=getstr();
				if (*s) {
					if (findparam(s)!=P_NOT)
						anerror("Falscher Parameter");
					else
						Scat(parameters[P_NOT]);
				}
				break;
			}
			if (isdigit(s[0])) {
				i=atoip(s);
				icat(i);
				break;
			}
			awarning("TARNE ohne Parameter",5);
			break;

		case K_GIVE:
		case K_LIEFERE:
			checkgiving(i);
			break;

		case K_BIETE:
			if (empiria) {
				scat(keywords[K_BIETE]);
				if (getaunit(NECESSARY) > 1 || current_temp_no) {
					sprintf(warn_buf,
									"%s geht nur mit normalen Einheiten", keywords[K_BIETE]);
					anerror(warn_buf);
					break;
				}
				i=geti();
				if (i<1)
					anerror("Geldgebot fehlt");
				icat(i);
				long_order();
			} else
				anerror("Befehl nicht erkannt");
			break;

		case K_ALLY:
			check_ally();
			break;

		case K_STATUS:
			scat(keywords[K_STATUS]);
			s=getstr();
			i=findparam(s);
			switch (i) {
				case P_NOT:
				case P_BEHIND:
				case P_FLEE:
				case P_VORNE:
					Scat(parameters[i]);
					break;
				default:
					if (*s) {
						if (findkeyword(s)==K_ALLY) {
							Scat(keywords[K_ALLY]);
							s=getstr();
							if (findparam(s)==P_NOT) {
								Scat(parameters[P_NOT]);
								break;
							}
						}
						Scat(s);
						anerror("Falscher Kampfstatus");
					} else
						Scat(parameters[P_VORNE]);
			}
			break;

		case K_COMBAT:
			scat(keywords[K_COMBAT]);
			s=getstr();
			getaspell(s, SP_KAMPF | SP_POST | SP_PRAE, order_unit, 1);
			break;

		case K_BUY:
		case K_SELL:
			scat(keywords[i]);
			getluxuries(i);
			long_order();
			break;

		case K_CONTACT:
			scat(keywords[K_CONTACT]);
			getaunit(NECESSARY);
			break;

		case K_SPY:
			scat(keywords[K_SPY]);
			i=getb();
			if (!i)
				anerror("Einheit fehlt");
			else
				bcat(i);
			long_order();
			break;

		case K_TEACH:
			scat(keywords[K_TEACH]);
			if (!getmoreunits(false))
				anerror("Wen lehren?");
			long_order();
			break;

		case K_VERGESSE:
			scat(keywords[K_VERGESSE]);
			sk=getskill();
			if (!sk)
				anerror("Talent nicht erkannt");
			else {
				Scat(sk->name);
			}
			break;

		case K_STUDY:
			scat(keywords[K_STUDY]);
			sk=getskill();
			if (!sk)
				anerror("Talent nicht erkannt");
			else {
				Scat(sk->name);
				if (!empiria && strcasecmp(sk->name,"Magie")==0)
					if (order_unit->people>1)
						anerror("Magiereinheiten dürfen nur eine Person haben");
			}
			if (!does_default) {
				x=studycost(sk)*order_unit->people;
				if (x) {
					order_unit->money-=x;
					order_unit->reserviert-=x;
				}
				addteach(NULL, order_unit);
				order_unit->lernt=1;
			}
			long_order();
			break;

		case K_MAKE:
			checkmake();
			break;

		case K_SABOTIERE:
			scat(keywords[K_SABOTIERE]);
			check_sabotage();
			long_order();
			break;

		case K_PASSWORD:
			scat(keywords[K_PASSWORD]);
			s=getstr();
			if (!s[0])
				awarning("Passwort gelöscht",0);
			else
				checkstring(s, NAMESIZE, POSSIBLE);
			break;

		case K_RECRUIT:
			scat(keywords[K_RECRUIT]);
			i=geti();
			if (i) {
				icat(i);
				if (from_temp_unit_no)
					u=newunit(from_temp_unit_no, 1);
				else
					u=order_unit;
				if (does_default)
					break;
				u->money-=i*rec_cost;
				u->reserviert-=i*rec_cost;
				u->people+=i;
				addregion(Rx, Ry, i);
			} else
				anerror("Anzahl Rekruten fehlt");
			break;

		case K_QUIT:
			scat(keywords[K_QUIT]);
			s=getstr();
			if (!s[0])
				anerror("Kein Passwort angeben");
			else
				checkstring(s, NAMESIZE, POSSIBLE);
			awarning("Achtung! STIRB gegeben! Die Partei wird aufgelöst!",0);
			break;

		case K_TAX:
			scat(keywords[K_TAX]);
			i=(geti()/10)*10;
			/* Steuern werden in Kontingenten von 10 Silber eingetrieben. */
			if (i)
				icat(i);
			else
				i=20*order_unit->people;
			while (*igetstr(NULL));
			long_order();
			if (!does_default)
				order_unit->money+=i;
			break;

		case K_ENTERTAIN:
			scat(keywords[K_ENTERTAIN]);
			i=geti();
			if (!does_default) {
				if (!i)
					i=20*order_unit->people;
				order_unit->money+=i;
			}
			long_order();
			break;

		case K_ENTER:
			checkenter();
			break;

		case K_LEAVE:
		  scat(keywords[K_LEAVE]);
			check_leave();
			break;

		case K_ROUTE:
			if (empiria) {
				anerror("Befehl nicht erkannt");
				break;
			}
		case K_MOVE:
			checkdirections(i);
			long_order();
			break;

		case K_FOLLOW:
			scat(keywords[K_FOLLOW]);
			if (empiria) {
				getaunit(NECESSARY);
				long_order();
			} else {
				s=getstr();
				if (s) {
					i=findparam(s);
					if (i==P_UNIT) {
						Scat(parameters[i]);
						getaunit(NECESSARY);
					} else if (i==P_SHIP) {
						Scat(parameters[i]);
						s=getstr();
						x=atoi(s);
						if (x<0 || x>9999) {
							sprintf(warn_buf,"Schiffsnr. %s ungültig",s);
							anerror(warn_buf);
						} else
							Scat(s);
					} else
						anerror("FOLGE EINHEIT xx, FOLGE SCHIFF xx oder FOLGE");
				}
			}
			break;

		case K_REPORT:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else {
				scat(keywords[K_REPORT]);
				s=getstr();
				i=findreport(s);
				if (i==-1) {
					if (strncasecmp(s, "ZEIGEN", strlen(s)))
						anerror("Unbekannte Report-Option");
					else
						scat(" ZEIGEN");
					break;
				}
				Scat(reports[i]);
				s=getstr();
				i=findstr(message_levels, s, ML_MAX);
				if (i==-1) {
					anerror("falscher Ausgabe-Level");
					break;
				}
				Scat(message_levels[i]);
			}
			break;

		case K_SEND:
			if (!empiria)
				awarning("SENDE wurde in OPTION umbenannt",4);
		case K_OPTION:
			if (i==K_OPTION && empiria) {
				anerror("Befehl nicht erkannt");
				break;
			} else {
				if (empiria)
					scat(keywords[K_SEND]);
				else
					scat(keywords[K_OPTION]);
			}
			i=getoption();
			if (i<0) {
				anerror("Option unbekannt");
				break;
			}
			Scat(options[i]);
			i=getparam();
			if (i==P_WARN) {
				Scat(parameters[i]);
				i=getparam();
			}
			if (i==P_NOT) {
				Scat(parameters[i]);
			}
			break;

		case K_CAST:
			scat(keywords[K_CAST]);
			s=getstr();
			getaspell(s, SP_ZAUBER, order_unit, 1);
			long_order();
			break;

		case K_TURMZAUBER:
			if (empiria) {
				scat(keywords[K_TURMZAUBER]);
				s=getstr();
				getaspell(s, SP_ZAUBER, order_unit, 256);
			} else
				anerror("Befehl nicht erkannt");
			break;

		case K_RESHOW:
			scat(keywords[K_RESHOW]);
			s=getstr();
			Scat(s);
			break;

		case K_DESTROY:
			scat(keywords[K_DESTROY]);
			break;

		case K_DRIVE:
			scat(keywords[K_DRIVE]);
			if (getaunit(NECESSARY)==2)
				anerror("Einheit 0 bzw. Bauern geht hier nicht");
			else if (!does_default)
				order_unit->drive=this_unit;
			long_order();
			break;

		case K_TRANSPORT:
			scat(keywords[K_TRANSPORT]);
			getaunit(NECESSARY);
			if (!does_default) {
				if (cmd_unit) cmd_unit->transport=order_unit->no;
				else awarning("Kann zu transportierende Einheit nicht ermitteln",3);
			}
			if (getaunit(42)==42)
				anerror("Pro Einheit muß ein TRANSPORTIERE-Befehl gegeben werden");
			break;

		case K_PIRATERIE:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else
				getmoreunits(true);
			break;

		case K_MAGIEGEBIET:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else {
				s=getstr();
				i=findstr(magiegebiet,s,5);
				if (i<0) {
					sprintf(warn_buf,"Magiegebiet '%s' gibt es nicht",s);
					anerror(warn_buf);
				} else {
					scat(keywords[K_MAGIEGEBIET]);
					Scat(magiegebiet[i]);
				}
			}
			break;

		case K_DEFAULT:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else {
				scat(keywords[K_DEFAULT]);
				scat(" \"");
				u=newunit(-1,0);
				copy_unit(order_unit, u);
				if (order_unit->start_of_orders)
					free(order_unit->start_of_orders);
				if (order_unit->long_order)
					free(order_unit->long_order);
				if (order_unit->order)
					free(order_unit->order);
				order_unit->long_order_line=0;
				order_unit->start_of_orders_line=0;
				order_unit->temp=0;
					/* der DEFAULT gilt ja erst nächste Runde! */
				s=getstr();
				does_default=1;
				porder();
				checkanorder(s);
				does_default=2;
				while (getstr()[0]);
				copy_unit(u, order_unit);
				u->no=-1;
				u->long_order_line=0;
				u->start_of_orders_line=0;
				u->temp=0;
			}
			break;

		case K_COMMENT:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else {
				check_comment();
				scat(Orders);
			}
			break;

		case K_RESERVE:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else
				reserve();
			break;

		case K_NEUSTART:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else {
				i=findstr(Rassen, getstr(), R_MAX);
				if (i<0) {
					anerror("Unbekannte Rasse");
					break;
				} else
					Scat(Rassen[i]);
				s=getstr();
				if (!*s) {
					anerror("Passwort fehlt");
					break;
				} else
					qcat(s);
				awarning("NEUSTART befohlen!",0);
			}
			break;

		case K_GRUPPE:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else {
				s=getstr();
				if (*s)
					Scat(s);
			}
			break;

		case K_SORTIERE:
			if (empiria)
				anerror("Befehl nicht erkannt");
			else {
				s=getstr();
				if (*s) {
					if (strncasecmp(s, "VOR", strlen(s))==0 ||
							strncasecmp(s, "HINTER", strlen(s))==0) {
						Scat(s);
						i=getaunit(NECESSARY);
						if (i==1 || i==3)	/* normale oder TEMP-Einheit: ok */
							break;
					}
				}
				anerror("SORTIERE VOR oder HINTER <einheit>");
			}
			break;

		default:
			anerror("Befehl nicht erkannt");
	}
	if (does_default != 1) {
		porder();
		does_default=0;
	}
}


void
readaunit(void) {
	int i;
	unit *u;

	i=getb();
	if (i==0) {
		anerror("Keine Einheitsnummer");
		get_order();
		return;
	}
	u=newunit(i, 0);
	u->line_no=line_no;
		/* Sonst ist die Zeilennummer die Zeile, wo die Einheit zuerst von
		 * einer anderen Einheit angesprochen wurde... */
	orders_for_unit(i,u);
	bcat(i);

	indent=INDENT_UNIT;
	next_indent=INDENT_ORDERS;
	porder();
	from_temp_unit_no=0;

	for (;;) {
		get_order();

		if (befehle_ende)
			return;

		/* Erst wenn wir sicher sind, dass kein Befehl eingegeben wurde, checken
		 * wir, ob nun eine neue Einheit oder ein neuer Spieler drankommt */

		if (igetkeyword(order_buf)==-1) {
			if (order_buf[0]==';') {
				check_comment();
				continue;
			}
			else switch (igetparam(order_buf)) {
				case P_UNIT:
				case P_FACTION:
				case P_NEXT:
				case P_REGION:
					if (from_temp_unit_no != 0) {
						sprintf(warn_buf,"TEMP-Einheit %s wird nicht durch ENDE abgeschlossen",
								itob(from_temp_unit_no));
						awarning(warn_buf, 2);
						from_temp_unit_no=0;
					}
					return;
			}
		}
		if (order_buf[0])
			checkanorder(order_buf);
	}
}


int
readafaction(void) {
	int i;
	char *s;

	if (line_start==1)
		line_no=1;
	i=getI();
	if (i) {
		bcat(i);
		s=getstr();
		if (s[0]==0) {
			anerror("Kein Passwort");
		} else if (strcmp(s, "hier_passwort_eintragen") == 0) {
			anerror("Nicht das korrekte Passwort eingesetzt");
		}
		qcat(s);
	} else
		anerror("Keine Parteinummer");

	indent=next_indent=INDENT_FACTION;
	porder();
	return i;
}


void
help(const char *s) {
	fprintf(ERR, "ECheck (Version "VERSION", "__DATE__
			"), Zug-Checker für Eressea - Freeware!\n\n"
			" Benutzung: %s [Optionen] Befehlsdatei\n"
		/* Vorsicht, TABs! Neue Texte mit Tab-Width=8 rein */
			"  -	Verwendet stdin anstelle einer Eingabedatei.\n"
			"  -b	unterdrückt Warnungen und Fehler (brief)\n"
			"  -q	erwartet keine Angaben zu Personen/Silber in [] bei EINHEIT\n"
			"  -rnnn	Legt Rekrutierungskosten auf nnn Silber fest\n"
			"  -c	schreibt die Warnungen und Fehler in einer Compiler-ähnlichen Form\n"
			"  -e	schreibt die geprüfte Datei auf stdout, Fehler nach stderr\n"
			"  -E	schreibt die geprüfte Datei auf stdout, Fehler nach stdout\n"
			"  -ofile	schreibt die geprüfte Datei in die Datei 'file'\n"
			"  -Ofile	schreibt Fehler in die Datei 'file'\n"
			"  -h	zeigt diese kleine Hilfe an\n"
			"  -s	verwendet stderr für Warnungen, Fehler etc., nicht stdout\n"
			"  -p	verkürzt einige Ausgaben für piping\n"
			"  -l	simuliert Silberpool-Funktion (nicht bei Empiria)\n"
			"  -n	zählt NameMe-Kommentare (;;) nicht als Zeile\n"
			"  -m	Empiria-Checking\n"
			"  -noxxx  Keine xxx-Warnungen. xxx kann sein:\n"
			"	   ship   Einheit steuert Schiff und hat evtl. kein Kommando\n"
			"	   route  kein Check auf zyklisches ROUTE\n"
			"	   lost   Einheit verliert Silber und Gegenstände\n"
			"  -w[n]	Warnungen der Stufe n (default: 4=alle Warnungen)\n"
			"  -x	Zeilenzählung ab PARTEI statt Dateianfang\n"
			"  -Ppfad   Pfadangabe für items.txt und zauber.txt\n"
			"  -vm.l   Mainversion.Level - für Test, ob richtige ECheck-Version\n"
		, s);
}


void
check_options(int argc, char *argv[], char dostop, char command_line) {
	int i;
	char *x;

	for (i=1; i != argc; i++) {
		if (argv[i][0]=='-'
#ifdef WIN32
		|| argv[i][0]=='/'
#endif
		) {
			switch (argv[i][1]) {
				case 'P':
					if (dostop && strlen(argv[i])>2) {
						/* bei Optionen via "; ECHECK" nicht mehr machen */
						if (argv[i][2]==0) {	/* -P path */
							i++;
							if (argv[i])
								path=strdup((char *)(argv[i]+2));
							else {
								fputs("Leere Pfad-Angabe ungültig\n",stderr);
								exit(0);
							}
						} else
							path=strdup((char *)(argv[i]+2));
					}
					break;

				case 'v':
					if (argv[i][2]==0) {	/* -v version */
						i++;
						if (!argv[i])
							break;
					}
					has_version=1;
					x=strchr(argv[i],'.');
					if (x) {
						*x=0;
						if (strcmp(MAINVERSION,(char *)(argv[i]+2))==0) {
							*x='.';
							x++;
							if (show_warnings>1 && strcmp(MINVERSION,x)!=0)
								fprintf(stderr,"Warnung, falsche ECheck-Version: %s\n",argv[i]+2);
							break;
						}
					}
					break;

				case 'b':
					brief=1;
					break;

				case 'l':
					silberpool=1;
					break;

				case 'm':
					empiria=1;
					break;

				case 'q':
					no_comment=1;
					noship=1; nolost=1; noroute=1;
					break;

				case 'r':
					if (argv[i][2]==0) { 	/* -r nnn */
						i++;
						if (argv[i])
							rec_cost=atoi(argv[i]);
						else
							fprintf(stderr,"Fehlende Rekrutierungskosten, auf %d gesetzt",rec_cost);
					} else
						rec_cost=atoi(argv[i]+2);
					break;

				case 'c':
					compile=1;
					break;

				case 'E':
					if (dostop)	{ /* bei Optionen via "; ECHECK" nicht mehr machen */
						echo_it=1;
						OUT=stdout;
						ERR=stdout;
					}
					break;

				case 'e':
					if (dostop)	{ /* bei Optionen via "; ECHECK" nicht mehr machen */
						echo_it=1;
						OUT=stdout;
						ERR=stderr;
					}
					break;

				case 'O':
					if (dostop)	{ /* bei Optionen via "; ECHECK" nicht mehr machen */
						if (argv[i][2]==0) {	/* "-o file" */
							i++;
							x=argv[i];
						} else							/* "-ofile" */
							x=argv[i]+2;
						if (!x) {
							fputs("Keine Datei für Fehler-Texte, stderr benutzt\n",stderr);
							ERR=stderr;
							break;
						}
						ERR=fopen(x,"w");
						if (!ERR) {
							fprintf(stderr,"Kann Datei '%s' nicht schreiben:\n	%s",
									x,strerror(errno));
							exit(0);
						}
					}
					break;

				case 'o':
					if (dostop)	{ /* bei Optionen via "; ECHECK" nicht mehr machen */
						if (argv[i][2]==0) {	/* "-o file" */
							i++;
							x=argv[i];
						} else							/* "-ofile" */
							x=argv[i]+2;
						echo_it=1;
						if (!x) {
							fputs("Leere Datei für geprüfte Datei, stdout benutzt\n",stderr);
							OUT=stdout;
							break;
						}
						OUT=fopen(x,"w");
						if (!OUT) {
							fprintf(stderr,"Kann Datei '%s' nicht schreiben:\n	%s",
									x,strerror(errno));
							exit(0);
						}
					}
					break;

				case 'p':
					piping=1;
					break;

				case 'x':
					line_start=1;
					break;

				case 'w':
					if (command_line == 1 || warnings_cl == 0) {
						if (argv[i][2])
							show_warnings=(char)atoi(argv[i]+2);
						else {
							if (argv[i+1] && isdigit(*argv[i+1])) {
								i++;
								show_warnings=atoi(argv[i]);
							} else
								show_warnings=0;
						}
					}
					if (command_line==1)
						warnings_cl=1;
					break;

				case 's':
					if (dostop)	/* bei Optionen via "; ECHECK" nicht mehr machen */
						ERR=stderr;
					break;

				case 'n':
					if (strlen(argv[i])>2) {
						if (argv[i][3]==0) {	/* -no xxx */
							i++;
							x=argv[i];
						} else
							x=argv[i]+3;
						if (!x) {
							fputs("-no ???\n",stderr);
							break;
						}
						switch (*x) {
							case 's':
								noship=1;
								break;
							case 'r':
								noroute=1;
								break;
							case 'l':
								nolost=1;
								break;
						}
					} else
						ignore_NameMe=1;
					break;

				case '?':
				case 'h':
					if (dostop)	{ /* bei Optionen via "; ECHECK" nicht mehr machen */
						help(argv[0]);
						exit(1);
					}
					break;

				default:
					if (argv[i][1]) {
						fprintf(ERR, "Option '%s' unbekannt.\n", argv[i]);
						if (dostop)		/* Nicht stoppen, wenn dies die Parameter aus der
														 Datei selbst sind! */
							exit(10);
				}
			}
		}
	}
	if (empiria && silberpool) {
		fprintf(ERR,"Empiria hat keinen Silberpool\n");
		silberpool=0;
	}
}


void
parse_options(char *p, char dostop) {
	char *argv[10], **ap=argv, *vl, argc=0;

	vl=strtok(p, " \t,");
	do {
		*ap++=vl;
		argc++;
	} while ((vl=strtok(NULL, " \t,"))!=NULL);
	*ap=0;
	check_options(argc, argv, dostop, 0);
}


void
check_OPTION(void) {
	get_order();
	if (befehle_ende) return;
	if (strncmp(order_buf, "From ", 5)==0) {	/* es ist eine Mail */
		do {	/* Bis zur Leerzeile suchen -> Mailheader zu Ende */
			fgets(order_buf, BUFSIZE, F);
		} while (order_buf[0] != '\n' && !feof(F));
		if (feof(F)) {
			befehle_ende=1;
			return;
		}
		get_order();
	}
	if (befehle_ende) return;
	if (order_buf[0]==COMMENT_CHAR)
		do {
			if (strlen(order_buf)>9) {
				if (strncasecmp(order_buf, "; OPTION", 8)==0 ||
					 strncasecmp(order_buf, "; ECHECK", 8)==0) {
					parse_options((char *)(order_buf+2),0);
						/* "; " überspringen; zeigt dann auf "OPTION" */
				} else if (strncasecmp(order_buf, "; VERSION", 9)==0)
					fprintf(ERR, "%s\n", order_buf);
			}
			get_order();
			if (befehle_ende) return;
		} while (order_buf[0]==COMMENT_CHAR);
}


void
process_order_file(int *faction_count, int *unit_count) {
	int f=0, next=0;
	t_region *r;
	unit *u;
	char *x;

	line_no=befehle_ende=0;

	check_OPTION();

	if (befehle_ende)		/* dies war wohl eine Datei ohne Befehle */
		return;

	Rx=Ry=-10000;

	/* Auffinden der ersten Partei, und danach abarbeiten bis zur letzten Partei. */

	while (!befehle_ende) {
		switch (igetparam(order_buf)) {
			case P_REGION:
				if (Regionen)
					remove_temp();
				if (echo_it)
				{
					fputs(order_buf,OUT);
					putc('\n',OUT);
				}
				x=getstr();
				if (*x) {
					Rx=atoi(x);
					x=strchr(order_buf, ',');
					if (!x) {
						x=strchr(order_buf, ' ');
						if (x) x=strchr(++x, ' ');		/* 2. Space ist Trenner? */
						else x=getstr();
					}
					if (x && *x)
						Ry=atoi(++x);
					else
						Ry=-10000;
				} else
					Rx=-10000;
				if (Rx<-9999 || Ry<-9999)
					awarning("REGION fehlerhaft",0);
				r=addregion(Rx, Ry, 0);
				r->line_no=line_no;
				x=strchr(order_buf, ';');
				if (x) {
					x++;
					while (a_isspace(*x)) x++;
					if(r->name) free(r->name);
					r->name=strdup(x);
					x=strchr(r->name,'\n');
					if (x)
						*x=0;
				} else {
					if(r->name) free(r->name);
					r->name=strdup("");
				}
				get_order();
				break;

			case P_FACTION:
				if (f && !next)
					awarning("NÄCHSTER fehlt",0);
				scat(parameters[P_FACTION]);
				befehle_ende=0;
				f=readafaction();
				fprintf(ERR, "Befehle für Partei %s gefunden.\n", itob(f));
				check_OPTION();	/* Nach PARTEI auf "; OPTION" / "; ECHECK" testen */
				if (befehle_ende) return;
				fprintf(ERR, "Rekrutierungskosten auf %d Silber gesetzt, "
								"Warning Level %d.\n", rec_cost, show_warnings);
				if (silberpool)
					fputs("Silberpool", ERR);
				else if (empiria)	/* Empipria hat keinen Silberpool */
					fputs("Empiria-Modus", ERR);
				if (empiria || silberpool)
					fputs(" aktiviert.", ERR);
				fputs("\n\n", ERR);
				if (!has_version)
					fputs("Hinweis: es wurde keine ECheck-Version angegeben (-v"
							MAINVERSION"."MINVERSION")\n",ERR);
				(*faction_count)++;
				next=0;
				break;

			case P_UNIT:
				if (f) {
					scat(parameters[P_UNIT]);
					readaunit();
					(*unit_count)++;
				} else
					get_order();
				break;

				/* Falls in readunit abgebrochen wird, steht dort entweder eine neue
					 Partei, eine neue Einheit oder das File-Ende. Das switch wird erneut
					 durchlaufen, und die entsprechende Funktion aufgerufen. Man darf
					 order_buf auf alle Fälle nicht überschreiben! Bei allen anderen
					 Einträgen hier muß order_buf erneut gefüllt werden, da die
					 betreffende Information in nur einer Zeile steht, und nun die
					 nächste gelesen werden muß. */

			case P_NEXT:
				f=0;
				scat(parameters[P_NEXT]);
				indent=next_indent=INDENT_FACTION;
				porder();
				next=1;

				check_money(true);	/* Check für Lerngeld, Handel usw.;
														 * true: dann Bewegung ausführen */
				if (Regionen) {
					check_money(false);	/* Silber nochmal in den Pool, fehlendes aus Pool */
					check_living();			/* Ernährung mit allem Silber der Region */
				}
				check_teachings();
				while (Regionen) {
					r=Regionen->next;
					if (Regionen->name)
						free(Regionen->name);
					free(Regionen);
					Regionen=r;
				}
				while (units) {
					u=units->next;
					if (units->start_of_orders)
						free(units->start_of_orders);
					if (units->long_order)
						free(units->long_order);
					if (units->order)
						free(units->order);
					free(units);
					units=u;
				}
				Regionen=(t_region *)NULL;
				units=(unit *)NULL;

			default:
				if (order_buf[0]==';') {
					check_comment();
				} else {
					if (f && order_buf[0])
						awarning("Wird von keiner Einheit ausgeführt",1);
				}
				get_order();
		}
	} /* end while !befehle_ende */

	if (igetparam(order_buf)==P_NEXT)	/* diese Zeile wurde ggf. gelesen und dann kam */
		next=1;													/* EOF -> kein Check mehr, next=0... */
	if (f && !next)
		anerror("NÄCHSTER fehlt");
}


void
addtoken(tnode *root, const char *str, int id) {
	static char buf[1024];
	static struct replace {
		char c;
		char *str;
	} replace[] = {
		{'ä', "ae"},
		{'Ä', "ae"},
		{'ö', "oe"},
		{'Ö', "oe"},
		{'ü', "ue"},
		{'Ü', "ue"},
		{'ß', "ss"},
		{ 0, 0}
	};
	if (root->id>=0 && root->id!=id && !root->leaf) root->id=-1;
	if (!*str) {
		root->id = id;
		root->leaf=1;
	} else {
		char c = (char)tolower(*str);
		int index = ((unsigned char)c) % 32;
		int i=0;
		tnode * tk = root->next[index];
		if (root->id<0) root->id = id;
		while (tk && tk->c != c) tk = tk->nexthash;
		if (!tk) {
			tk = calloc(1, sizeof(tnode));
			tk->id = -1;
			tk->c = c;
			tk->nexthash=root->next[index];
			root->next[index] = tk;
		}
		addtoken(tk, str+1, id);
		while (replace[i].str) {
			if (*str==replace[i].c) {
				strcat(strcpy(buf, replace[i].str), str+1);
				addtoken(root, buf, id);
				break;
			}
			++i;
		}
	}
}


void
inittokens(void) {
	int i, k;
	t_item *it;
	t_names *n;
	t_skills *s;

	for (i=0,it=itemdata; it; it=it->next,++i)
		for (n=it->name; n; n=n->next)
			addtoken(&tokens[UT_ITEM], n->txt, i);
	for (i=0; i!=MAXPARAMS; ++i)
		addtoken(&tokens[UT_PARAM], parameters[i], i);
	for (i=0, s=skilldata; s; s=s->next,++i)
		addtoken(&tokens[UT_SKILL], s->name, i);
	for (i=0; i!=MAXKEYWORDS; ++i)
		addtoken(&tokens[UT_KEYWORD], keywords[i], i);
	k=0;
	for (i=0; buildingtypes[i]; ++i) {
		if (!empiria && strcmp(buildingtypes[i],"Universität")==0) {
			k++;
			continue;
		}
		if (empiria && strcmp(buildingtypes[i],"Akademie")==0) {
			k++;
			continue;
		}
		addtoken(&tokens[UT_BUILDING], buildingtypes[i], i-k);
	}
	for (i=0; i!=MAXHERBS; ++i)
		for (k=0; k!=2; ++k)
			addtoken(&tokens[UT_HERB], herbdata[k][i], i);
	for (i=0; i!=MAXPOTIONS; ++i)
		for (k=0;k!=2;++k)
			addtoken(&tokens[UT_POTION], potionnames[k][i], i);
}


int
main(int argc, char *argv[]) {
	int i, faction_count=0, unit_count=0;

#if !defined(AMIGA) && !defined(UMLAUTE)
	setlocale(LC_CTYPE, "");
#endif

#if macintosh
	argc=ccommand(&argv);  /* consolenabruf der parameter fuer macintosh
														added 15.6.00 chartus*/
#endif

	/* Path-Handling */
	path = getenv("ECHECKPATH");
	if(path == NULL) {
		path = DEFAULT_PATH;
	}

	ERR=stdout;

	if (argc <= 1) {
		help(argv[0]);
		return 0;
	}

	if (argc>1)
		check_options(argc, argv, 1, 1);

	fprintf(ERR, "ECheck (Version "VERSION
				", "__DATE__"), Zug-Checker für Eressea - Freeware!\n\n");

	filename=getenv("ECHECKOPTS");
	if (filename) parse_options(filename,1);

	readitems();
	readspells();
	readskills();
	inittokens();
	F=stdin;

	for (i=1; i < argc; i++)
		if (argv[i][0] != '-'
#ifdef WIN32
				&& argv[i][0] != '/'
#endif
				) {
			F=fopen(argv[i], "r");
			if (!F) {
				fprintf(ERR, "Kann Datei `%s' nicht lesen.\n", argv[i]);
#ifdef AMIGA
				return 10;
#else
				return 2;
#endif
			} else {
				filename=argv[i];
				fprintf(ERR, "Verarbeite Datei `%s'.\n", argv[i]);
				process_order_file(&faction_count, &unit_count);
			}
		}
	/* Falls es keine input Dateien gab, ist F immer noch auf stdin gesetzt */
	if (F==stdin)
		process_order_file(&faction_count, &unit_count);

	fprintf(ERR, "\nEs wurden Befehle für %d %s und %d %s gelesen.\n",
						faction_count, faction_count != 1 ? "Parteien" : "Partei",
						unit_count, unit_count != 1 ? "Einheiten" : "Einheit");

	if (unit_count==0) {
		fputs("\nBitte überprüfe, ob Du die Befehle korrekt eingesandt hast.\n"
			"Beachte dabei besonders, dass die Befehle nicht als HTML, Word-Dokument\n"
			"oder als Attachment (Anlage) eingeschickt werden dürfen.\n", ERR);
		return -42;
	}

	if (!error_count && !warning_count && faction_count && unit_count)
		fputs("Die Befehle scheinen in Ordnung zu sein.\n", ERR);

	if (error_count > 1)
		fprintf(ERR, "Es wurden %d Fehler", error_count);
	else if (error_count==1)
		fputs("Es wurde ein Fehler", ERR);

	if (warning_count) {
		if (error_count)
			fputs(" und", ERR);
		else
			fputs("Es wurde", ERR);
	}

	if (warning_count > 1) {
		if (!error_count)
			fputs("n", ERR);
		fprintf(ERR," %d Warnungen", warning_count);
	} else if (warning_count==1)
		fputs(" eine Warnung", ERR);

	if (warning_count || error_count)
		fputs(" entdeckt.\n", ERR);
#ifdef AMIGA
	if (error_count>0)
		return 10;	/* FAIL beim AMIGA */
	if (warning_count>0)
		return 5;	/* WARN beim AMIGA */
#endif
	return 0;
}

