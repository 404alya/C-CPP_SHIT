#include <stdio.h>
#include <stdlib.h>

typedef struct Vector2 {
    float x;                // Vector x component
    float y;                // Vector y component
} Vector2;

typedef struct {
  Vector2 *items;
  size_t count;
  size_t capacity;
} Points;

// Append an item to a dynamic array
#define da_append(da, item)                    \
    do {                                       \
        (da)->items[(da)->count++] = (item);   \
    } while (0) \

Points nodes = {0};


int main() {
  int *a = malloc(sizeof(int) * 4);


  a[2] = 2;
  return 0;
}
