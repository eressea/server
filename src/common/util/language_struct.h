#ifndef CLASS_LANGUAGE_STRUCT
#define CLASS_LANGUAGE_STRUCT

/* This file should not be included by anything in the server. If you 
 * feel that you need to include it, it's a sure sign that you're trying to
 * do something BAD. */

#define SMAXHASH 512

typedef struct locale {
	struct locale * next;
	unsigned int hashkey;
	const char * name;
	struct locale_string {
		unsigned int hashkey;
		struct locale_string * nexthash;
		char * str;
		char * key;
	} * strings[SMAXHASH];
} locale;

extern locale * default_locale;
extern locale * locales;

#endif
