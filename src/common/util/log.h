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
#ifndef H_UTIL_LOG
#define H_UTIL_LOG
#ifdef __cplusplus
extern "C" {
#endif

  extern void log_open(const char * filename);
  extern void log_printf(const char * str, ...);
  extern void log_puts(const char * str);
  extern void log_close(void);
  extern void log_flush(void);

#define log_warning(x) _log_warn x
#define log_error(x) _log_error x
#define log_info(x) _log_info x

  /* use macros above instead of these: */
  extern void _log_warn(const char * format, ...);
  extern void _log_error(const char * format, ...);
  extern void _log_info(unsigned int flag, const char * format, ...);

#define LOG_FLUSH      (1<<0)
#define LOG_CPWARNING  (1<<1)
#define LOG_CPERROR    (1<<2)

#ifdef __cplusplus
}
#endif
#endif
