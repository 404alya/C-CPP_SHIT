#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#include <ftw.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int display_info(
  const char *fpath,
  const struct stat *sb,
  int tflag,
  struct FTW *ftwbuf
) {
  if (tflag != FTW_F && tflag != FTW_D) return FTW_CONTINUE;

  char *fpathm = malloc(strlen(fpath) + 1);
  strcpy(fpathm, fpath);
  fpathm[strlen(fpath)] = '\0';

  char *fpath_clean =  fpathm + ftwbuf->base;

  printf("   %s %d %s\n",
          fpath, ftwbuf->base, fpath + ftwbuf->base);



  if (tflag == FTW_D && fpath_clean[0] == '.' && strcmp(fpath_clean, ".") != 0) {
    free(fpathm);
    return FTW_SKIP_SUBTREE;
  }


  char *dot = strrchr(fpath + ftwbuf->base, '.');
  if (dot == NULL) return FTW_CONTINUE;
  if (strcmp(dot, ".html")) {
   free(fpathm);
    return FTW_CONTINUE;  // if file not ends with .html
  }



  printf("   %s %d %s\n",
          fpath, ftwbuf->base, fpath + ftwbuf->base);

  return 0;
}

int main(int argc, char *argv[])
{
  int flags = 0;
  flags |= FTW_PHYS;
  flags |= FTW_MOUNT;
  flags |= FTW_ACTIONRETVAL;

  if (nftw(".", display_info, 20, flags) == -1)
  {
    perror("nftw");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
