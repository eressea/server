/* vi: set ts=2:
 *
 *	$Id: windir.h,v 1.1 2001/01/25 09:37:58 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef WINDIR_H
#define WINDIR_H
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
	long hnd;
};

DIR *opendir(const char *name);
struct dirent *readdir(DIR * thedir);

#endif
