/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#ifndef H_UTIL_CRMESSAGE
#define H_UTIL_CRMESSAGE
#ifdef __cplusplus
extern "C" {
#endif

struct locale;
struct message;
struct message_type;

typedef int (*tostring_f)(const void * data, char * buffer, const void * userdata);
extern void tsf_register(const char * name, tostring_f fun);
	/* registers a new type->string-function */

extern int cr_string(const void * v, char * buffer, const void * userdata);
extern int cr_int(const void * v, char * buffer, const void * userdata);
extern int cr_ignore(const void * v, char * buffer, const void * userdata);

extern void crt_register(const struct message_type * mtype);
extern int cr_render(const struct message * msg, char * buffer, const void * userdata);

#ifdef __cplusplus
}
#endif
#endif
