#ifndef _LOGGING_H
#define _LOGGING_H

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

extern void log_read(const char * logname);
extern void log_region(const struct region * r);
extern void log_unit(const struct unit * r);
extern void log_faction(const struct faction * f);
extern void log_start(const char * filename);
extern void log_stop(void);

#endif /* _LOGGING_H */
