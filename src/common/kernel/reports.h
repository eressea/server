/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#ifndef H_KRNL_REPORTS
#define H_KRNL_REPORTS
#ifdef __cplusplus
extern "C" {
#endif

/* Alter, ab dem der Score angezeigt werden soll: */
#define DISPLAYSCORE 12
/* Breite einer Reportzeile: */
#define REPORTWIDTH 78

extern const char *directions[];
extern const char *coasts[];

/* kann_finden speedups */
extern boolean kann_finden(struct faction * f1, struct faction * f2);
extern struct unit * can_find(struct faction *, struct faction *);

/* funktionen zum schreiben eines reports */
void sparagraph(struct strlist ** SP, const char *s, int indent, char mark);
void lparagraph(struct strlist ** SP, char *s, int indent, char mark);
const char *hp_status(const struct unit * u);
extern size_t spskill(char * pbuf, size_t siz, const struct locale * lang, const struct unit * u, struct skill * sv, int *dh, int days); /* mapper */
extern void spunit(struct strlist ** SP, const struct faction * f, const struct unit * u, int indent, int mode);

extern int reports(void);
extern int write_reports(struct faction * f, time_t ltime);
extern int init_reports(void);

extern const struct unit *ucansee(const struct faction *f, const struct unit *u, const struct unit *x);

int hat_in_region(item_t itm, struct region * r, struct faction * f);

/* für fast_region und neuen CR: */

enum {
  see_none,
  see_neighbour,
  see_lighthouse,
  see_travel,
  see_far,
  see_unit,
  see_battle
};
extern int stealth_modifier(int seen_mode);

typedef struct seen_region {
  struct seen_region * nextHash;
  struct seen_region * next;
  struct region *r;
  unsigned char mode;
  boolean disbelieves;
} seen_region;

extern struct seen_region * find_seen(struct seen_region * seehash[], const struct region * r);
extern boolean add_seen(struct seen_region * seehash[], struct region * r, unsigned char mode, boolean dis);
extern struct seen_region ** seen_init(void);
extern void seen_done(struct seen_region * seehash[]);
extern void free_seen(void);
extern void link_seen(seen_region * seehash[], const struct region * first, const struct region * last);
extern const char * visibility[];

typedef struct report_context {
  struct faction * f;
  struct faction_list * addresses;
  struct seen_region ** seen;
  struct region * first, * last;
  void * userdata;
  time_t report_time;
} report_context;

typedef int (*report_fun)(const char * filename, report_context * ctx);
extern void register_reporttype(const char * extension, report_fun write, int flag);
  
extern void report_item(const struct unit * owner, const struct item * i, const struct faction * viewer, const char ** name, const char ** basename, int * number, boolean singular);
extern void report_building(FILE *F, const struct region * r, const struct building * b, const struct faction * f, int mode);
extern int bufunit(const struct faction * f, const struct unit * u, int indent, int mode, char * buf, size_t size);

extern const char * reportpath(void);
extern const xmlChar * trailinto(const struct region * r, const struct locale * lang);
extern const xmlChar * report_kampfstatus(const struct unit * u, const struct locale * lang);
  extern void writeaddresses(void);

  extern void reports_init(void);

  extern int update_nmrs(void);
  extern int * nmrs;

  extern struct message * msg_curse(const struct curse * c, const void * obj, typ_t typ, int slef);

  typedef struct arg_regions {
    int nregions;
    struct region ** regions;
  } arg_regions;


#ifdef __cplusplus
}
#endif
#endif
