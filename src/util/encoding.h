#ifndef ENCODING_H
#define ENCODING_H
#ifdef __cplusplus
extern "C" {
#endif

enum {
	ENCODING_ERROR = -1,
	ENCODING_NONE = 0,
	ENCODING_8859_1 = 1,
	ENCODING_UTF8 = 10
};

int get_encoding_by_name(const char * name);

#ifdef __cplusplus
}
#endif
#endif
