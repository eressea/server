extern void log_open(const char * filename);
extern void log_printf(const char * str, ...);
extern void log_puts(const char * str);
extern void log_close(void);

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
