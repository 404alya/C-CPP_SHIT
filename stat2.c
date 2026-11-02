#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Not enough arguments");
    exit(EXIT_FAILURE);
  }

  struct stat statBuff;
  stat(argv[1], &statBuff);

  printf("ID of containing device:  [%x,%x]\n",
        major(statBuff.st_dev),
        minor(statBuff.st_dev));

  printf("File type:                ");

  switch (statBuff.st_mode & S_IFMT) {
  case S_IFBLK:  printf("block device\n");            break;
  case S_IFCHR:  printf("character device\n");        break;
  case S_IFDIR:  printf("directory\n");               break;
  case S_IFIFO:  printf("FIFO/pipe\n");               break;
  case S_IFLNK:  printf("symlink\n");                 break;
  case S_IFREG:  printf("regular file\n");            break;
  case S_IFSOCK: printf("socket\n");                  break;
  default:       printf("unknown?\n");                break;
  }

  printf("I-node number:            %ju\n", (uintmax_t) statBuff.st_ino);

  printf("Mode:                     %jo (octal)\n",
        (uintmax_t) statBuff.st_mode);

  printf("Link count:               %ju\n", (uintmax_t) statBuff.st_nlink);
  printf("Ownership:                UID=%ju   GID=%ju\n",
        (uintmax_t) statBuff.st_uid, (uintmax_t) statBuff.st_gid);

  printf("Preferred I/O block size: %jd bytes\n",
        (intmax_t) statBuff.st_blksize);
  printf("File size:                %jd bytes\n",
        (intmax_t) statBuff.st_size);
  printf("Blocks allocated:         %jd\n",
        (intmax_t) statBuff.st_blocks);

  printf("Last status change:       %s", ctime(&statBuff.st_ctime));
  printf("Last file access:         %s", ctime(&statBuff.st_atime));
  printf("Last file modification:   %s", ctime(&statBuff.st_mtime));

  exit(EXIT_SUCCESS);
  return 0;
}
