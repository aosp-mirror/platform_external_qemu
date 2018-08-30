#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>

int main (void)
{
  struct stat buf;

  if (stat (".", &buf) != 0
      || !S_ISDIR (buf.st_mode))
    abort ();
  printf ("pass\n");
  exit (0);
}
