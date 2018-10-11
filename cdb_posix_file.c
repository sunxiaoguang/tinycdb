#include "cdb_int.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <sys/mman.h>
# ifndef MAP_FAILED
#  define MAP_FAILED ((void*)-1)
# endif
#endif

static int
_cdb_posix_file_open(struct cdb_file *cdbfp);
static int
_cdb_posix_file_create(struct cdb_file *cdbfp);
static const void *
_cdb_posix_file_get(struct cdb_file *cdbfp, unsigned len, unsigned pos, unsigned bufid);
static int
_cdb_posix_file_read(struct cdb_file *cdbfp, void *buf, unsigned len);
static int
_cdb_posix_file_pread(struct cdb_file *cdbfp, void *buf, unsigned len, unsigned pos);
static int
_cdb_posix_file_seek(struct cdb_file *cdbfp, unsigned pos);
static int
_cdb_posix_file_write(struct cdb_file *cdbfp, const unsigned char *buf, unsigned len);
static void
_cdb_posix_file_close(struct cdb_file *cdbfp);

struct cdb_posix_file_opaque {
  int fd;
  unsigned offset;
  const unsigned char *cdb_mem; /* mmap'ed file memory */
};

static const struct cdb_file _cdb_posix_file = {
  _cdb_posix_file_open,
  _cdb_posix_file_create,
  _cdb_posix_file_get,
  _cdb_posix_file_read,
  _cdb_posix_file_pread,
  _cdb_posix_file_seek,
  _cdb_posix_file_write,
  _cdb_posix_file_close,
  NULL,
};

struct cdb_file *
_cdb_posix_file_create_from_fd(int fd)
{
  struct cdb_file *file = malloc(sizeof(struct cdb_file));
  memcpy(file, &_cdb_posix_file, sizeof(struct cdb_file));
  file->opaque = (void *) (intptr_t) fd;
  return file;
}

int
_cdb_posix_file_open(struct cdb_file *cdbfp)
{
  struct stat st;
  unsigned char *mem;
  unsigned fsize;
#ifdef _WIN32
  HANDLE hFile, hMapping;
#endif
  struct cdb_posix_file_opaque *opaque;
  int cdb_fd = (int) (intptr_t) cdbfp->opaque;
  cdbfp->opaque = malloc(sizeof(struct cdb_posix_file_opaque));
  /* get file size */
  if (fstat(cdb_fd, &st) < 0)
    return -1;
  /* trivial sanity check: at least toc should be here */
  if (st.st_size < 2048)
    return errno = EPROTO, -1;
  fsize = st.st_size < 0xffffffffu ? st.st_size : 0xffffffffu;
  /* memory-map file */
#ifdef _WIN32
  hFile = (HANDLE) _get_osfhandle(cdb_fd);
  if (hFile == (HANDLE) -1)
    return -1;
  hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
  if (!hMapping)
    return -1;
  mem = (unsigned char *)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
  CloseHandle(hMapping);
  if (!mem)
    return -1;
#else
  mem = (unsigned char*)mmap(NULL, fsize, PROT_READ, MAP_SHARED, cdb_fd, 0);
  if (mem == MAP_FAILED)
    return -1;
#endif /* _WIN32 */

  opaque = cdbfp->opaque;
  cdbfp->fsize = fsize;
  opaque->cdb_mem = mem;
  opaque->fd = cdb_fd;

#if 0
  /* XXX don't know well about madvise syscall -- is it legal
     to set different options for parts of one mmap() region?
     There is also posix_madvise() exist, with POSIX_MADV_RANDOM etc...
  */
#ifdef MADV_RANDOM
  /* set madvise() parameters. Ignore errors for now if system
     doesn't support it */
  madvise(mem, 2048, MADV_WILLNEED);
  madvise(mem + 2048, cdbp->cdb_fsize - 2048, MADV_RANDOM);
#endif
#endif

  return 0;
}

int
_cdb_posix_file_create(struct cdb_file *cdbfp)
{
  struct cdb_posix_file_opaque *opaque;
  int cdb_fd = (int) (intptr_t) cdbfp->opaque;
  cdbfp->fsize = 0;
  cdbfp->opaque = calloc(1, sizeof(struct cdb_posix_file_opaque));
  opaque = cdbfp->opaque;
  opaque->fd = cdb_fd;
  return 0;
}

const void *
_cdb_posix_file_get(struct cdb_file *cdbfp, unsigned len, unsigned pos, unsigned bufid)
{
  struct cdb_posix_file_opaque *opaque = cdbfp->opaque;
  return opaque->cdb_mem + pos;
}

int
_cdb_posix_file_read(struct cdb_file *cdbfp, void *buf, unsigned len)
{
  struct cdb_posix_file_opaque *opaque = cdbfp->opaque;
  const void *data = _cdb_posix_file_get(cdbfp, len, opaque->offset, cdb_buf_default);
  if (!data) return -1;
  memcpy(buf, data, len);
  opaque->offset += len;
  return 0;
}

int
_cdb_posix_file_pread(struct cdb_file *cdbfp, void *buf, unsigned len, unsigned pos)
{
  const void *data = _cdb_posix_file_get(cdbfp, len, pos, cdb_buf_default);
  if (!data) return -1;
  memcpy(buf, data, len);
  return 0;
}

void
_cdb_posix_file_close(struct cdb_file *cdbfp)
{
  struct cdb_posix_file_opaque *opaque = cdbfp->opaque;
  if (opaque == NULL) {
    return;
  }
  if (opaque->cdb_mem) {
#ifdef _WIN32
    UnmapViewOfFile((void*) cdbp->cdb_mem);
#else
    munmap((void*)opaque->cdb_mem, cdbfp->fsize);
#endif /* _WIN32 */
    opaque->cdb_mem = NULL;
  }
  free(opaque);
  return;
}

int
_cdb_posix_file_seek(struct cdb_file *cdbfp, unsigned pos)
{
  struct cdb_posix_file_opaque *opaque = cdbfp->opaque;
  opaque->offset = pos;
  return lseek(opaque->fd, pos, SEEK_SET);
}

int
_cdb_posix_file_write(struct cdb_file *cdbfp, const unsigned char *buf, unsigned len)
{
  struct cdb_posix_file_opaque *opaque = cdbfp->opaque;
  int rc = write(opaque->fd, buf, len);
  if (rc > 0) {
    opaque->offset += rc;
  }
  return rc;
}
