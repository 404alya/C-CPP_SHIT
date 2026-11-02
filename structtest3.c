#include <stdio.h>

typedef struct {
  char *data;
  size_t len;      // current string length (not counting null terminator)
  size_t capacity;  // allocated size
} StrBuf;

int main() {
  StrBuf strbuf = { 0 };

  for (int i = 0; i < 110; ++i) {
    printf("a %zu \n", strbuf.capacity);
  }

  return 0;
}
