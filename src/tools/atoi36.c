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
