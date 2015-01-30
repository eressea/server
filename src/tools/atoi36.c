/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <base36.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  int i = 1, reverse = 0;
  if (strstr(argv[0], "itoa36"))
    reverse = 1;
  if (argc > 1) {
    if (strcmp(argv[1], "-r") == 0) {
      i = 2;
      reverse = 1;
    }
  }
  for (; i != argc; ++i) {
    if (reverse) {
      printf("%s -> %s\n", argv[i], itoa36(atoi(argv[i])));
    } else
      printf("%s -> %d\n", argv[i], atoi36(argv[i]));
  }
  return 0;
}
