#include <stdio.h>

int main(int argc, char **argv) {
  char *firstValue = *(argv)++;

  for (int i = 0; i < argc; i++) {
    printf("%s", argv[i]);
  }

  printf("\n firstValue: ?? %s", firstValue);


  return 0;
}
