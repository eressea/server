/* vi: set ts=2:
 *
 *	$Id: message.h,v 1.4 2001/02/24 12:50:48 enno Exp $
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

#ifdef NEW_MESSAGES
struct message_type;

typedef struct message_list {
	struct mlist {
		struct mlist * next;
		struct message *msg;
	} * begin, **end;
} message_list;

#else /* NEW_MESSAGES */

typedef struct messagetype
{
	struct messagetype * next;
	const struct messageclass * section;
	int level;
	char * name;
	int argc;
	struct entry {
		struct entry * next;
		enum {
			IT_FACTION,
			IT_UNIT,
			IT_REGION,
			IT_SHIP,
			IT_BUILDING,
#ifdef NEW_ITEMS
			IT_RESOURCETYPE,
#endif
			IT_RESOURCE,
			IT_SKILL,
			IT_INT,
			IT_STRING,
			IT_DIRECTION,
			IT_FSPECIAL
		} type;
		char * name;
	} * entries;
	unsigned int hashkey;
} messagetype;

extern struct messagetype * find_messagetype(const char * name);
extern struct messagetype * new_messagetype(const char * name, int level, const char * section);

typedef struct message {
	struct message * next;
	struct messagetype * type;
	void ** data;
	void * receiver;
	int level;
} message;

extern int msg_level(const struct message * msg);

int get_msglevel(const struct warning * warnings, const struct msglevel * levels, const struct messagetype * mtype);

void debug_messagetypes(FILE * out);
#endif /* NEW_MESSAGES */

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

extern struct message * new_message(struct faction * receiver, const char * signature, ...);
extern struct message* add_message(struct message_list** pm, struct message* m);
extern void free_messages(struct message_list * m);
extern void read_messages(FILE * F, const struct locale * lang);

/* message sections */
extern struct messageclass * msgclasses;
extern const struct messageclass * mc_add(const char * name);
extern const struct messageclass * mc_find(const char * name);

#endif
