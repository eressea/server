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

#ifndef MESSAGE_H
#define MESSAGE_H

struct faction;
struct warning;

struct message;
struct messageclass;
struct warning;
struct msglevel;

struct message_type;

typedef struct message_list {
	struct mlist {
		struct mlist * next;
		struct message *msg;
	} * begin, **end;
} message_list;

typedef struct messageclass
{
	struct messageclass * next;
	const char * name;
} messageclass;

typedef struct msglevel {
	/* used to set specialized msg-levels */
	struct msglevel *next;
	const struct message_type *type;
	int level;
} msglevel;

void write_msglevels(struct warning * warnings, FILE * F);
void read_msglevels(struct warning ** w, FILE * F);
void set_msglevel(struct warning ** warnings, const char * type, int level);

extern struct message * make_message(const char * name, const char* sig, ...);
extern struct message * new_message(struct faction * receiver, const char * signature, ...);
extern struct message * add_message(struct message_list** pm, struct message * m);
extern void free_messages(struct message_list * m);
extern void read_messages(FILE * F, const struct locale * lang);

/* message sections */
extern struct messageclass * msgclasses;
extern const struct messageclass * mc_add(const char * name);
extern const struct messageclass * mc_find(const char * name);

#endif
