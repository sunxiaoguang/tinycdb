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
  return cdb_init_with_reader(cdbp, fd, &_cdb_posix_file_reader);
}

int cdb_init_with_reader(struct cdb *cdbp, int fd, struct cdb_file_reader *reader) {
  int rc;
  unsigned dend;
  memset(cdbp, 0, sizeof(*cdbp));
  cdbp->reader = reader;
  cdbp->cdb_fd = fd;
  if ((rc = cdbp->reader->init(cdbp)) == 0) {
    cdbp->cdb_vpos = cdbp->cdb_vlen = 0;
    cdbp->cdb_kpos = cdbp->cdb_klen = 0;
    dend = cdb_unpack(cdb_get(cdbp, 4, 0));
    if (dend < 2048) dend = 2048;
    else if (dend >= cdbp->cdb_fsize) dend = cdbp->cdb_fsize;
    cdbp->cdb_dend = dend;
  }
  return rc;
}

void
cdb_free(struct cdb *cdbp)
{
  cdbp->reader->fini(cdbp);
}

const void *
_cdb_get(const struct cdb *cdbp, unsigned len, unsigned pos, unsigned bufid)
{
  return cdbp->reader->get(cdbp, len, pos, bufid);
}

const void *
cdb_get(const struct cdb *cdbp, unsigned len, unsigned pos)
{
  if (pos > cdbp->cdb_fsize || cdbp->cdb_fsize - pos < len) {
    errno = EPROTO;
    return NULL;
  }
  return cdbp->reader->get(cdbp, len, pos, cdb_buf_default);
}

int
cdb_read(const struct cdb *cdbp, void *buf, unsigned len, unsigned pos)
{
  return cdbp->reader->read(cdbp, buf, len, pos);
}
