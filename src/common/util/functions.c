/* vi: set ts=2:
 *
 *	$Id: functions.c,v 1.2 2001/01/26 16:19:41 enno Exp $
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

#include <config.h>
#include "functions.h"

/* libc includes */
#include <stdlib.h>
#include <string.h>



typedef struct function_list {
	struct function_list * next;
	pf_generic fun;
	const char * name;
} function_list;

static function_list * functions;

pf_generic
get_function(const char * name)
{
	function_list * fl = functions;
	while (fl && strcmp(fl->name, name)!=0) fl=fl->next;
	if (fl) return fl->fun;
	return NULL;
}

const char *
get_functionname(pf_generic fun)
{
	function_list * fl = functions;
	while (fl && fl->fun!=fun) fl=fl->next;
	if (fl) return fl->name;
	return NULL;
}

void
register_function(pf_generic fun, const char * name)
{
	function_list * fl = calloc(sizeof(function_list), 1);
	fl->next = functions;
	fl->fun = fun;
	fl->name = strdup(name);
	functions = fl;
}
