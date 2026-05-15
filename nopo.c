// Number of processors online

#include <unistd.h>
#include <stdio.h>

int main() {
  printf("PROCESSORS: %lu", sysconf(_SC_NPROCESSORS_ONLN));
  return 0;
}
