/* cdb_int.h: internal cdb library declarations
 *
 * This file is a part of tinycdb package by Michael Tokarev, mjt@corpit.ru.
 * Public domain.
 */

#include "cdb.h"
#include <errno.h>
#include <string.h>

#ifndef EPROTO
# define EPROTO EINVAL
#endif

#ifndef internal_function
# ifdef __GNUC__
#  define internal_function __attribute__((visibility("hidden")))
# else
#  define internal_function
# endif
#endif

struct cdb_rec {
  unsigned hval;
  unsigned rpos;
};

struct cdb_rl {
  struct cdb_rl *next;
  unsigned cnt;
  struct cdb_rec rec[254];
};

int _cdb_make_write(struct cdb_make *cdbmp,
        const unsigned char *ptr, unsigned len);
int _cdb_make_fullwrite(struct cdb_make *cdbmp, const unsigned char *buf, unsigned len);
int _cdb_make_flush(struct cdb_make *cdbmp);
int _cdb_make_add(struct cdb_make *cdbmp, unsigned hval,
                  const void *key, unsigned klen,
                  const void *val, unsigned vlen);

#define cdb_buf_default 0
#define cdb_buf_htab 1
#define cdb_buf_data 2

const void *_cdb_get(const struct cdb *cdbp, unsigned len, unsigned pos, unsigned bufid);
unsigned _cdb_unpack(const struct cdb *cdbp, unsigned at, unsigned bufid);

struct cdb_file *_cdb_posix_file_create_from_fd(int fd);
