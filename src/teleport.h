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

#ifndef TELEPORT_H
#define TELEPORT_H
#ifdef __cplusplus
extern "C" {
#endif

    struct region *r_standard_to_astral(const struct region *r);
    struct region *r_astral_to_standard(const struct region *);
    extern struct region_list *astralregions(const struct region *rastral,
        bool(*valid) (const struct region *));
    extern struct region_list *all_in_range(const struct region *r, int n,
        bool(*valid) (const struct region *));
    extern bool inhabitable(const struct region *r);
    extern bool is_astral(const struct region *r);
    extern struct plane *get_astralplane(void);
    extern struct plane *get_normalplane(void);

    void create_teleport_plane(void);
    void set_teleport_plane_regiontypes(void);
    void spawn_braineaters(float chance);

#ifdef __cplusplus
}
#endif
#endif
