#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv)
{
  char key[4];
  char code[4];
  char result[4];
  int a, i, rot;
  for (a = 1; a < argc; ++a) {
    const char *str = argv[a];
    size_t len = strlen(str);
    str = str + len - 6;
    memcpy(key, str, 3);
    memcpy(code, str + 3, 3);
    result[3] = key[3] = code[3] = 0;
    rot = atoi(key);
    for (i = 0; i != 3; ++i)
      result[(i + rot) % 3] = ((code[i] + 10 - key[i]) % 10) + '0';
    printf("%s %s\n", argv[a], result);
  }
  return 0;
}
