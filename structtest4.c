#include <stdio.h>

typedef struct {
  int id;
} Opt2;

typedef struct {
  int x;
  int y;
  Opt2 z;
} Opt;

int main() {
  Opt opt = {0};

  for (int i = 0; i < 9999; ++i) {
printf("%d %d %d \n", opt.x, opt.y, opt.z.id);
  }

  return 0;
}
