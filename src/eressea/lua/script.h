/* vi: set ts=2:
 +-------------------+  
 |                   |  Christian Schlittchen <corwin@amber.kn-bremen.de>
 | Eressea PBEM host |  Enno Rehling <enno@eressea.de>
 | (c) 1998 - 2004   |  Katja Zedel <katze@felidae.kn-bremen.de>
 |                   |
 +-------------------+  

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/
#ifndef KRNL_SCRIPT_H
#define KRNL_SCRIPT_H

extern int call_script(struct unit * u);
extern void setscript(struct attrib ** ap, void * fptr);

extern void reset_scripts();

#endif
