#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
  char *cmd = "ls";
  char *argv[] = {cmd, "-a", NULL};

  pid_t cpid = fork();

  if (cpid < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  printf("cpid %d \n", cpid);

  if (cpid == 0) {
    int xstat = execvp("ls", argv);
    if (xstat < 0) {
      perror("execvp");
      exit(EXIT_FAILURE);
    }
  } else {
  }

  return 0;
}
