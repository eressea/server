/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef STRINGS_H
#define STRINGS_H

#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

    void str_replace(char *buffer, size_t size, const char *tmpl, const char *var, const char *value);
    int str_hash(const char *s);
    size_t str_slprintf(char * dst, size_t size, const char * format, ...);
    size_t str_strlcpy(char *dst, const char *src, size_t len);
    size_t str_strlcat(char *dst, const char *src, size_t len);
    char *str_strdup(const char *s);

    const char *str_escape(const char *str, char *buffer, size_t size);
    const char *str_escape_ex(const char *str, char *buffer, size_t size, const char *chars);
    char *str_unescape(char *str);

    unsigned int jenkins_hash(unsigned int a);
    unsigned int wang_hash(unsigned int a);

    /* static buffered string */
    typedef struct sbstring {
        size_t size;
        char *begin;
        char *end;
    } sbstring;

    void sbs_init(struct sbstring *sbs, char *buffer, size_t size);
    void sbs_strcat(struct sbstring *sbs, const char *str);
    void sbs_strncat(struct sbstring *sbs, const char *str, size_t size);
    void sbs_strcpy(struct sbstring *sbs, const char *str);
    size_t sbs_length(const struct sbstring *sbs);

    /* benchmark for units:
     * JENKINS_HASH: 5.25 misses/hit (with good cache behavior)
     * WANG_HASH:    5.33 misses/hit (with good cache behavior)
     * KNUTH_HASH:   1.93 misses/hit (with bad cache behavior)
     * CF_HASH:      fucking awful!
     */
#define KNUTH_HASH1(a, m) ((a) % m)
#define KNUTH_HASH2(a, m) (m - 2 - a % (m-2))
#define CF_HASH1(a, m) ((a) % m)
#define CF_HASH2(a, m) (8 - ((a) & 7))
#define JENKINS_HASH1(a, m) (jenkins_hash(a) % m)
#define JENKINS_HASH2(a, m) 1
#define WANG_HASH1(a, m) (wang_hash(a) % m)
#define WANG_HASH2(a, m) 1

#define HASH1 JENKINS_HASH1
#define HASH2 JENKINS_HASH2
#define slprintf str_slprintf

#ifdef __cplusplus
}
#endif
#endif                          /* STRINGS_H */
