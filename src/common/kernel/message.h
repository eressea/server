/* vi: set ts=2:
 *
 *	$Id: message.h,v 1.3 2001/02/10 19:24:05 enno Exp $
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

typedef struct messageclass
{
	struct messageclass * next;
	const char * name;
} messageclass;

extern messageclass * msgclasses;
extern const messageclass * mc_add(const char * name);
extern const messageclass * mc_find(const char * name);

typedef struct messagetype
{
	struct messagetype * next;
	const messageclass * section;
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

typedef struct msglevel {
	/* used to set specialized msg-levels */
	struct msglevel *next;
	const messagetype *type;
	int level;
} msglevel;

extern messagetype * find_messagetype(const char * name);
extern messagetype * new_messagetype(const char * name, int level, const char * section);

typedef struct message {
	struct message * next;
	messagetype * type;
	void ** data;
	void * receiver;
	int level;
} message;

struct faction;
struct warning;

extern int msg_level(const message * msg);

extern message * add_message(message** pm, message * m);
extern message * new_message(struct faction * receiver, const char * signature, ...);

extern void free_messages(message * m);

void write_msglevels(struct warning * warnings, FILE * F);
void read_msglevels(struct warning ** w, FILE * F);
void set_msglevel(struct warning ** warnings, const char * type, int level);
int get_msglevel(const struct warning * warnings, const msglevel * levels, const messagetype * mtype);

void debug_messagetypes(FILE * out);

#endif
