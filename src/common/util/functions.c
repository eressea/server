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

#include <platform.h>
#include "functions.h"

/* libc includes */
#include <stdlib.h>
#include <string.h>



typedef struct function_list {
	struct function_list * next;
	pf_generic fun;
	const char * name;
} function_list;

static function_list * functionlist;

pf_generic
get_function(const char * name)
{
	function_list * fl = functionlist;
	if (name==NULL) return NULL;
	while (fl && strcmp(fl->name, name)!=0) fl=fl->next;
	if (fl) return fl->fun;
	return NULL;
}

const char *
get_functionname(pf_generic fun)
{
	function_list * fl = functionlist;
	while (fl && fl->fun!=fun) fl=fl->next;
	if (fl) return fl->name;
	return NULL;
}

void
register_function(pf_generic fun, const char * name)
{
	function_list * fl = calloc(sizeof(function_list), 1);
	fl->next = functionlist;
	fl->fun = fun;
	fl->name = strdup(name);
	functionlist = fl;
}

void
list_registered_functions(void)
{
  function_list * fl = functionlist;

  while(fl) {
    printf("%s\n", fl->name);
    fl = fl->next;
  }
}
