#include <stdio.h>

#define R3D_INSTANCE_POSITION   (1u << 0) // 1
#define R3D_INSTANCE_ROTATION   (1u << 1) // 2
#define R3D_INSTANCE_SCALE      (1u << 2) // 4
#define R3D_INSTANCE_COLOR      (1u << 3) // 8
#define R3D_INSTANCE_CUSTOM     (1u << 4) // 16

int main() {
  printf("%d %d %d %d %d" , R3D_INSTANCE_POSITION, R3D_INSTANCE_ROTATION, R3D_INSTANCE_SCALE, R3D_INSTANCE_COLOR, R3D_INSTANCE_CUSTOM);
  return 0;
}
