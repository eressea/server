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

struct locale;
struct message;
struct message_type;

typedef void (*tostring_f)(const void * data, char * buffer);
extern void tsf_register(const char * name, tostring_f fun);
	/* registers a new type->string-function */

extern void cr_string(const void * v, char * buffer);
extern void cr_int(const void * v, char * buffer);

extern void crt_register(const struct message_type * mtype, const struct locale * lang);
extern int cr_render(const struct message * msg, const struct locale * lang, char * buffer);

