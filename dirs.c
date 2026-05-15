// Source - https://stackoverflow.com/a/12506
// Posted by Clayton, modified by community. See post 'Timeline' for change history
// Retrieved 2026-05-12, License - CC BY-SA 4.0
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

int main (void)
{
  DIR *dp;
  struct dirent *ep;
  dp = opendir("../");

  if (dp != NULL)
  {
    while ((ep = readdir(dp)) != NULL)
      puts(ep->d_name);

    (void) closedir (dp);
    return 0;
  }
  else
  {
    perror ("Couldn't open the directory");
    return -1;
  }
}
