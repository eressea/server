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

#ifdef _MSC_VER
#include <config.h>

#include <stdlib.h>
#include <string.h>
#include "windir.h"

DIR *
opendir(const char *name)
{
	static DIR direct; /* STATIC_RESULT: used for return, not across calls */

	direct.first = 1;
	_searchenv(name, "ERESSEA_PATH", direct.name);
	if (*direct.name != '\0')
		return &direct;
	return NULL;
}

struct dirent *
readdir(DIR * thedir)
{
	static struct _finddata_t ft; /* STATIC_RESULT: used for return, not across calls */
	static struct dirent de; /* STATIC_RESULT: used for return, not across calls */
	char where[_MAX_PATH];

	strcat(strcpy(where, thedir->name), "/*");
	if (thedir->first) {
		thedir->first = 0;
		de.hnd = _findfirst(where, &ft);
	} else {
		if (_findnext(de.hnd, &ft) != 0)
			return NULL;
	}
	_splitpath(ft.name, de.d_drive, de.d_dir, de.d_name, de.d_ext);
	return &de;
}
#endif
