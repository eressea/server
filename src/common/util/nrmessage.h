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
struct nrmessage_type;

extern int nr_render(const struct message * msg, const struct locale * lang,
                      char * buffer);
extern void nrt_register(const struct message_type * mtype, 
                         const struct locale * lang, const char * script, 
								 int level, const char * section);

extern int nr_level(const struct message *msg);
extern const char * nr_section(const struct message *msg);

/* before:
 * fogblock;movement:0;de;{unit} konnte von {region} nicht nach {$dir direction} ausreisen, der Nebel war zu dicht.
 * after:
 * fogblock:movement:0
 * $unit($unit) konnte von $region($region) nicht nach $direction($direction) ausreisen, der Nebel war zu dicht.
 * unit:unit region:region direction:int
 */