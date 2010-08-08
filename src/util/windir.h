/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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

#ifndef WINDIR_H
#define WINDIR_H

#ifdef _MSC_VER
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <io.h>
typedef struct DIR {
	char name[_MAX_PATH];
	int first;
} DIR;

typedef struct dirent {
	char d_path[_MAX_PATH];
	char d_name[_MAX_FNAME];
	char d_drive[_MAX_DRIVE];
	char d_dir[_MAX_DIR];
	char d_ext[_MAX_EXT];
	intptr_t hnd;
} dirent;

DIR *opendir(const char *name);
struct dirent *readdir(DIR * thedir);

#ifdef __cplusplus
}
#endif
#endif
#endif
