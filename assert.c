#include <assert.h>
#include <time.h>
#include <stdlib.h>

int main() {
  struct timespec waitspec  = {
    0,
    1000 * 1000 * 300
  };

  while (1) {
    assert(0 == 0 && "Something bad happened!");
    nanosleep(&waitspec, NULL);
  }

  return 0;
}
