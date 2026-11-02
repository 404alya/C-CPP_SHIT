// Source - https://stackoverflow.com/a/4513437
// Posted by Hortinstein, modified by community. See post 'Timeline' for change history
// Retrieved 2026-07-28, License - CC BY-SA 3.0

/* strtok example */
#include <stdio.h>
#include <string.h>

int main ()
{
  char str[] ="- This, a sample string.";
  char * pch;
  printf ("Splitting string \"%s\" into tokens:\n",str);
  pch = strtok (str,",.-");
  while (pch != NULL)
  {
    printf ("%s\n",pch);
    pch = strtok (NULL, " ,.-");
  }
  return 0;
}
