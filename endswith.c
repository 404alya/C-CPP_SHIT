#include <stdio.h>
#include <string.h>


int main() {
  char *str = "adoihashtml";
  char *dot = strrchr(str, '.');

  if (dot && !strcmp(dot, ".html")) printf("ends with html\n");
  else printf("not ends with html\n");


  printf("%s", dot);

  return 0;
}
