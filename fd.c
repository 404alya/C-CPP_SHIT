#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

int main() {
  int fd = open("./smth", O_RDWR);
  if (fd == -1) perror("Couldn't open file");

  char* buff = calloc(100, 1);

  for (int i = 0; i < 50; ++i) {
    buff[i] = 'a';
  }

  write(fd, buff, 100);

  buff = calloc(100, 1);
  lseek(fd, 0, SEEK_SET);
  read(fd, buff, 100);


  printf("%s", buff);

  return 0;
}
