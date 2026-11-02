#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void appendCharToStr(char **dest, char c) {
  size_t len = strlen(*dest);
  char *newBuff = malloc(len + 2);
  memcpy(newBuff, *dest, len);
  newBuff[len] = c;
  newBuff[len + 1] = '\0';

  free(*dest);

  *dest = newBuff;
}

void printChars(char *str) {
  printf("\n --- \n");
  for (int i = 0; i < strlen(str);++i) {
    if (str[i] == '\0') printf("null term \n");
    else printf("%c \n", str[i]);
  }
}

int main() {
  char *a = malloc(6);
  char *b = "12345\0\0\0";
  strcpy(a, b);
  printChars(a);


  printf("%zu\n", strlen(a));
  size_t cmd_len = strlen(a);



  char *newstr = malloc(cmd_len - 1); // 49 - 48
  strncpy(newstr, a, cmd_len - 1);
  newstr[cmd_len - 1] = '\0';

}
