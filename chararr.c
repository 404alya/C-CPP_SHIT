#include <stdio.h>
#include <stdlib.h>

int main() {
  char* arr[] = { "aa", "hj" };
  printf("arr size: %lu", sizeof(arr) / sizeof(arr[0]));

  return 0;
};
