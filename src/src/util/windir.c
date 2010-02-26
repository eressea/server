/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

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
