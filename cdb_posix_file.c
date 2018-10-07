#include "cdb_int.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <sys/mman.h>
# ifndef MAP_FAILED
#  define MAP_FAILED ((void*)-1)
# endif
#endif

static int
_cdb_posix_file_reader_init(struct cdb *cdbp);
static const void *
_cdb_posix_file_reader_get(const struct cdb *cdbp, unsigned len, unsigned pos, unsigned bufid);
static int
_cdb_posix_file_reader_read(const struct cdb *cdbp, void *buf, unsigned len, unsigned pos);
static void
_cdb_posix_file_reader_fini(struct cdb *cdbp);

struct cdb_posix_file_reader_opaque {
  const unsigned char *cdb_mem; /* mmap'ed file memory */
};

struct cdb_file_reader _cdb_posix_file_reader = {
  _cdb_posix_file_reader_init,
  _cdb_posix_file_reader_get,
  _cdb_posix_file_reader_read,
  _cdb_posix_file_reader_fini,
  NULL,
};

int
_cdb_posix_file_reader_init(struct cdb *cdbp) {
  struct stat st;
  unsigned char *mem;
  unsigned fsize;
#ifdef _WIN32
  HANDLE hFile, hMapping;
#endif
  struct cdb_posix_file_reader_opaque *opaque;
  struct cdb_file_reader *cdbf = cdbp->reader;
  int cdb_fd = cdbp->cdb_fd;
  cdbf->opaque = malloc(sizeof(struct cdb_posix_file_reader_opaque));
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

  opaque = cdbf->opaque;
  cdbp->cdb_fsize = fsize;
  opaque->cdb_mem = mem;

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

const void *
_cdb_posix_file_reader_get(const struct cdb *cdbp, unsigned len, unsigned pos, unsigned bufid)
{
  struct cdb_file_reader *cdbf = cdbp->reader;
  struct cdb_posix_file_reader_opaque *opaque = cdbf->opaque;
  return opaque->cdb_mem + pos;
}

int
_cdb_posix_file_reader_read(const struct cdb *cdbp, void *buf, unsigned len, unsigned pos) {
  const void *data = _cdb_posix_file_reader_get(cdbp, len, pos, cdb_buf_default);
  if (!data) return -1;
  memcpy(buf, data, len);
  return 0;
}

void
_cdb_posix_file_reader_fini(struct cdb *cdbp) {
  struct cdb_file_reader *cdbf = cdbp->reader;
  struct cdb_posix_file_reader_opaque *opaque = cdbf->opaque;
  if (opaque == NULL) {
    return;
  }
  if (opaque->cdb_mem) {
#ifdef _WIN32
    UnmapViewOfFile((void*) cdbp->cdb_mem);
#else
    munmap((void*)opaque->cdb_mem, cdbp->cdb_fsize);
#endif /* _WIN32 */
    opaque->cdb_mem = NULL;
  }
  cdbp->cdb_fsize = 0;
  free(opaque);
  return;
}

static int
_cdb_posix_file_writer_init(struct cdb_make *cdbp);
static int
_cdb_posix_file_writer_write(const struct cdb_make *cdbp, const unsigned char *buf, unsigned len);
static int
_cdb_posix_file_writer_seek(const struct cdb_make *cdbp, unsigned pos);
static void
_cdb_posix_file_writer_fini(struct cdb_make *cdbp);

struct cdb_file_writer _cdb_posix_file_writer = {
  _cdb_posix_file_writer_init,
  _cdb_posix_file_writer_seek,
  _cdb_posix_file_writer_write,
  _cdb_posix_file_writer_fini,
  NULL,
};

int
_cdb_posix_file_writer_init(struct cdb_make *cdbp) {
  return 0;
}

int
_cdb_posix_file_writer_seek(const struct cdb_make *cdbp, unsigned pos) {
  return lseek(cdbp->cdb_fd, pos, SEEK_SET);
}

int
_cdb_posix_file_writer_write(const struct cdb_make *cdbp, const unsigned char *buf, unsigned len) {
  return write(cdbp->cdb_fd, buf, len);
}

void
_cdb_posix_file_writer_fini(struct cdb_make *cdbp) {
}
