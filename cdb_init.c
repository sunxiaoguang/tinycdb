/* cdb_init.c: cdb_init, cdb_free and cdb_read routines
 *
 * This file is a part of tinycdb package by Michael Tokarev, mjt@corpit.ru.
 * Public domain.
 */

#include <sys/types.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <sys/mman.h>
# ifndef MAP_FAILED
#  define MAP_FAILED ((void*)-1)
# endif
#endif
#include <sys/stat.h>
#include "cdb_int.h"

int
cdb_init(struct cdb *cdbp, int fd)
{
  return cdb_init_with_file(cdbp, _cdb_posix_file_create_from_fd(fd));
}

int
cdb_init_with_file(struct cdb *cdbp, struct cdb_file *file)
{
  int rc;
  unsigned dend;
  memset(cdbp, 0, sizeof(*cdbp));
  cdbp->file = file;
  if ((rc = cdbp->file->open(file)) == 0) {
    cdbp->cdb_vpos = cdbp->cdb_vlen = 0;
    cdbp->cdb_kpos = cdbp->cdb_klen = 0;
    dend = cdb_unpack(cdb_get(cdbp, 4, 0));
    if (dend < 2048) dend = 2048;
    else if (dend >= cdbp->file->fsize) dend = file->fsize;
    cdbp->cdb_dend = dend;
  }
  return rc;
}

void
cdb_free(struct cdb *cdbp)
{
  cdbp->file->close(cdbp->file);
}

const void *
_cdb_get(const struct cdb *cdbp, unsigned len, unsigned pos, unsigned bufid)
{
  return cdbp->file->get(cdbp->file, len, pos, bufid);
}

const void *
cdb_get(const struct cdb *cdbp, unsigned len, unsigned pos)
{
  if (pos > cdbp->file->fsize || cdbp->file->fsize - pos < len) {
    errno = EPROTO;
    return NULL;
  }
  return cdbp->file->get(cdbp->file, len, pos, cdb_buf_default);
}

int
cdb_read(const struct cdb *cdbp, void *buf, unsigned len, unsigned pos)
{
  return cdbp->file->pread(cdbp->file, buf, len, pos);
}
