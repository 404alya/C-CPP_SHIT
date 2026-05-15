#include <stdio.h>

typedef struct {
  int items;
  char selected;
} Panel;


int main() {
  Panel panel = {0,2};

  printf("%d", panel.selected);

  return 0;
};
