#include <stdio.h>
#include <string.h>

int main() {
  char* c = "abcdef";
  char c2[] = "abcdef";
  printf("%zu %lu", sizeof(c2), sizeof(&c));
  return 0;
}
