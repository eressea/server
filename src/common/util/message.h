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
#ifndef UTIL_MESSAGE_H
#define UTIL_MESSAGE_H
struct locale;

typedef struct message_type {
	const char * name;
	int nparameters;
	const char ** pnames;
	const char ** types;
} message_type;

typedef struct message {
	const struct message_type * type;
	const void ** parameters;
	int refcount;
} message;

extern struct message_type * mt_new(const char * name, const char ** args);
extern struct message_type * mt_new_va(const char * name, ...);
	/* mt_new("simple_sentence", "subject:string", "predicate:string", 
    *        "object:string", "lang:locale", NULL); */

extern struct message * msg_create(const struct message_type * type, void * args[]);
extern struct message * msg_create_va(const struct message_type * type, ...);
	/* msg_create(&mt_simplesentence, "enno", "eats", "chocolate", &locale_de); 
	 * parameters must be in the same order as they were for mt_new! */

extern void msg_release(struct message * msg);
extern struct message * msg_addref(struct message * msg);

extern const char * mt_name(const struct message_type* mtype);

/** message_type registry (optional): **/
extern const struct message_type * mt_register(const struct message_type *);
extern const struct message_type * mt_find(const char *);
#endif
