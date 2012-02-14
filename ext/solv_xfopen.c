/*
 * Copyright (c) 2011, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <fcntl.h>

#include "solv_xfopen.h"

static ssize_t cookie_gzread(void *cookie, char *buf, size_t nbytes)
{
  return gzread((gzFile *)cookie, buf, nbytes);
}

static ssize_t cookie_gzwrite(void *cookie, const char *buf, size_t nbytes)
{
  return gzwrite((gzFile *)cookie, buf, nbytes);
}

static int
cookie_gzclose(void *cookie)
{
  return gzclose((gzFile *)cookie);
}

static FILE *mygzfopen(gzFile* gzf, const char *mode)
{
#ifdef HAVE_FUNOPEN
  return funopen(
      gzf, (int (*)(void *, char *, int))(*mode == 'r' ? cookie_gzread : NULL), /* readfn */
      (int (*)(void *, const char *, int))(*mode == 'w' ? cookie_gzwrite : NULL), /* writefn */
      (fpos_t (*)(void *, fpos_t, int))NULL, /* seekfn */
      cookie_gzclose
      );
#elif defined(HAVE_FOPENCOOKIE)
  cookie_io_functions_t cio;
  memset(&cio, 0, sizeof(cio));
  if (*mode == 'r')
    cio.read = cookie_gzread;
  else if (*mode == 'w')
    cio.write = cookie_gzwrite;
  cio.close = cookie_gzclose;
  return  fopencookie(gzf, *mode == 'w' ? "w" : "r", cio);
#else
# error Need to implement custom I/O
#endif
}

FILE *
solv_xfopen(const char *fn, const char *mode)
{
  char *suf;
  gzFile *gzf;

  if (!fn)
    return 0;
  if (!mode)
    mode = "r";
  suf = strrchr(fn, '.');
  if (!suf || strcmp(suf, ".gz") != 0)
    return fopen(fn, mode);
  gzf = gzopen(fn, mode);
  if (!gzf)
    return 0;
  return mygzfopen(gzf, mode);
}

FILE *
solv_xfopen_fd(const char *fn, int fd, const char *mode)
{
  char *suf;
  gzFile *gzf;

  suf = fn ? strrchr(fn, '.') : 0;
  if (!mode)
    {
      int fl = fcntl(fd, F_GETFL, 0);
      if (fl == -1)
	return 0;
      fl &= O_RDONLY|O_WRONLY|O_RDWR;
      if (fl == O_WRONLY)
	mode = "w";
      else if (fl == O_RDWR)
	{
	  if (!suf || strcmp(suf, ".gz") != 0)
	    mode = "r+";
	  else
	    mode = "r";
	}
      else
	mode = "r";
    }
  if (!suf || strcmp(suf, ".gz") != 0)
    return fdopen(fd, mode);
  gzf = gzdopen(fd, mode);
  if (!gzf)
    return 0;
  return mygzfopen(gzf, mode);
}

