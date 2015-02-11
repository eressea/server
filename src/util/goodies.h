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

#ifndef GOODIES_H
#define GOODIES_H
#ifdef __cplusplus
extern "C" {
#endif

    extern char *set_string(char **s, const char *neu);
    extern int set_email(char **pemail, const char *newmail);

    extern int *intlist_init(void);
    extern int *intlist_add(int *i_p, int i);
    extern int *intlist_find(int *i_p, int i);

    extern unsigned int hashstring(const char *s);
    extern const char *escape_string(const char *str, char *buffer,
        unsigned int len);
    extern unsigned int jenkins_hash(unsigned int a);
    extern unsigned int wang_hash(unsigned int a);

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

#ifdef __cplusplus
}
#endif
#endif                          /* GOODIES_H */
