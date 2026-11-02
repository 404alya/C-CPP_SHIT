#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int width;
} Item;

int main() {
  Item *items;

  items = malloc(sizeof(Item) * 10);

  for (char i = 0; i < 10; ++i) {
    items[i] = (Item){.width = i + 1};
  }

  printf("%d\n", items->width);

  return 0;
}
