/* cdb_findnext.c: sequential cdb_find routines
 *
 * This file is a part of tinycdb package by Michael Tokarev, mjt@corpit.ru.
 * Public domain.
 */

/* see cdb_find.c for comments */

#include "cdb_int.h"

int
cdb_findinit(struct cdb_find *cdbfp, struct cdb *cdbp,
             const void *key, unsigned klen)
{
  unsigned n, pos;

  cdbfp->cdb_cdbp = cdbp;
  cdbfp->cdb_key = key;
  cdbfp->cdb_klen = klen;
  cdbfp->cdb_hval = cdb_hash(key, klen);

  cdbfp->cdb_htp = ((cdbfp->cdb_hval << 3) & 2047);
  n = _cdb_unpack(cdbp, cdbfp->cdb_htp + 4, cdb_buf_htab);
  cdbfp->cdb_httodo = n << 3;
  if (!n)
    return 0;
  pos = _cdb_unpack(cdbp, cdbfp->cdb_htp, cdb_buf_htab);
  if (n > (cdbp->file->fsize >> 3)
      || pos < cdbp->cdb_dend
      || pos > cdbp->file->fsize
      || cdbfp->cdb_httodo > cdbp->file->fsize - pos)
    return errno = EPROTO, -1;

  cdbfp->cdb_htab = pos;
  cdbfp->cdb_htend = cdbfp->cdb_htab + cdbfp->cdb_httodo;
  cdbfp->cdb_htp = cdbfp->cdb_htab + (((cdbfp->cdb_hval >> 8) % n) << 3);

  return 1;
}

int
cdb_findnext(struct cdb_find *cdbfp) {
  struct cdb *cdbp = cdbfp->cdb_cdbp;
  unsigned pos, n;
  unsigned klen = cdbfp->cdb_klen;

  while(cdbfp->cdb_httodo) {
    pos = _cdb_unpack(cdbp, cdbfp->cdb_htp + 4, cdb_buf_htab);
    if (!pos)
      return 0;
    n = _cdb_unpack(cdbp, cdbfp->cdb_htp, cdb_buf_htab) == cdbfp->cdb_hval;
    if ((cdbfp->cdb_htp += 8) >= cdbfp->cdb_htend)
      cdbfp->cdb_htp = cdbfp->cdb_htab;
    cdbfp->cdb_httodo -= 8;
    if (n) {
      if (pos > cdbp->file->fsize - 8)
        return errno = EPROTO, -1;
      if (_cdb_unpack(cdbp, pos, cdb_buf_data) == klen) {
        if (cdbp->file->fsize - klen < pos + 8)
          return errno = EPROTO, -1;
        if (memcmp(cdbfp->cdb_key,
            _cdb_get(cdbp, klen, pos + 8, cdb_buf_data), klen) == 0) {
          n = _cdb_unpack(cdbp, pos + 4, cdb_buf_data);
          pos += 8;
          if (cdbp->file->fsize < n ||
              cdbp->file->fsize - n < pos + klen)
            return errno = EPROTO, -1;
          cdbp->cdb_kpos = pos;
          cdbp->cdb_klen = klen;
          cdbp->cdb_vpos = pos + klen;
          cdbp->cdb_vlen = n;
          return 1;
        }
      }
    }
  }

  return 0;
}
