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

#ifndef HEADER_MUSEUM_H
#define HEADER_MUSEUM_H

#ifdef __cplusplus
extern "C" {
#endif

    extern struct attrib_type at_warden;
    extern struct attrib_type at_museumexit;
    extern struct attrib_type at_museumgivebackcookie;
    extern struct attrib_type at_museumgiveback;

    typedef struct {
        int warden_no;
        int cookie;
    } museumgivebackcookie;

    typedef struct {
        int cookie;
        struct item *items;
    } museumgiveback;

    extern void register_museum(void);

#ifdef __cplusplus
}
#endif
#endif
