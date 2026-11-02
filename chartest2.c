#include <stdio.h>
#include <string.h>

int main() {

  char *a = "aopsd\0jsaop";

  printf("%zu", strlen(a));
  return 0;
}
