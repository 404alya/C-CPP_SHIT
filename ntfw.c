#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
display_info(const char *fpath, const struct stat *sb,
             int tflag, struct FTW *ftwbuf)
{
  const char *flag_str;
  if (tflag == FTW_D)   flag_str = "d";
  else if (tflag == FTW_DNR) flag_str = "dnr";
  else if (tflag == FTW_DP)  flag_str = "dp";
  else if (tflag == FTW_F)   flag_str = "f";
  else if (tflag == FTW_NS)  flag_str = "ns";
  else if (tflag == FTW_SL)  flag_str = "sl";
  else if (tflag == FTW_SLN) flag_str = "sln";
  else flag_str = "???";

  printf("%-3s %2d ", flag_str, ftwbuf->level);

  if (tflag == FTW_NS)
      printf("-------");
  else
      printf("%7jd", (intmax_t) sb->st_size);

  printf("   %-40s %d %s\n",
          fpath, ftwbuf->base, fpath + ftwbuf->base);

  return 0;           /* To tell nftw() to continue */
}

int
main(int argc, char *argv[])
{
  int flags = 0;

  if (argc > 2 && strchr(argv[2], 'd') != NULL)
      flags |= FTW_DEPTH;
  if (argc > 2 && strchr(argv[2], 'p') != NULL)
      flags |= FTW_PHYS;

  if (nftw((argc < 2) ? "." : argv[1], display_info, 20, flags)
      == -1)
  {
      perror("nftw");
      exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
