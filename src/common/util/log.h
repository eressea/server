extern void log_open(const char * filename);
extern void log_printf(const char * str, ...);
extern void log_puts(const char * str);
extern void log_close(void);

#define log_warning(x) _log_warn x
#define log_error(x) _log_error x

/* use macros above instead of these: */
extern void _log_warn(const char * format, ...);
extern void _log_error(const char * format, ...);
