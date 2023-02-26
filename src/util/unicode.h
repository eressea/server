#pragma once

#include <stddef.h>
#include <wchar.h>

int utf8_to_cp437(unsigned char *result, const char * utf8_string,
    size_t * length);
int utf8_to_cp1252(unsigned char *result, const char * utf8_string,
    size_t * length);
int utf8_decode(wchar_t * result, const char * utf8_string,
    size_t * length);
int utf8_to_ascii(unsigned char *cp_character, const char * utf8_string,
    size_t *length);
int utf8_from_latin1(char * out, size_t * outlen,
    const char *in, size_t * inlen);

int utf8_strcasecmp(const char* a, const char* b);
size_t utf8_trim(char * str);
void utf8_clean(char* str);
const char * utf8_ltrim(const char *str);

