#ifndef _UNICODE_H
#define _UNICODE_H

#include <stddef.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USE_UNICODE
    int unicode_utf8_to_cp437(unsigned char *result, const char * utf8_string,
        size_t * length);
    int unicode_utf8_to_cp1252(unsigned char *result, const char * utf8_string,
        size_t * length);
    int unicode_utf8_decode(wint_t * result, const char * utf8_string,
        size_t * length);
    int unicode_utf8_encode(char * result, size_t * size,
        wint_t ucs4_character);
    int unicode_utf8_to_ascii(unsigned char *cp_character, const char * utf8_string,
        size_t *length);
    int unicode_utf8_strcasecmp(const char * a, const char * b);
    int unicode_latin1_to_utf8(char * out, size_t * outlen,
        const char *in, size_t * inlen);
    int unicode_utf8_tolower(char *op, size_t outlen, const char *ip);
    size_t unicode_utf8_trim(char *ip);

#ifdef __cplusplus
}
#endif
#endif
