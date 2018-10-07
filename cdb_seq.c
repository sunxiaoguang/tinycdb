/* cdb_seq.c: sequential record retrieval routines
 *
 * This file is a part of tinycdb package by Michael Tokarev, mjt@corpit.ru.
 * Public domain.
 */

#include "cdb_int.h"

int
cdb_seqnext(unsigned *cptr, struct cdb *cdbp) {
  unsigned klen, vlen;
  unsigned pos = *cptr;
  unsigned dend = cdbp->cdb_dend;
  if (pos > dend - 8)
    return 0;
  klen = _cdb_unpack(cdbp, pos, cdb_buf_data);
  vlen = _cdb_unpack(cdbp, pos + 4, cdb_buf_data);
  pos += 8;
  if (dend - klen < pos || dend - vlen < pos + klen)
    return errno = EPROTO, -1;
  cdbp->cdb_kpos = pos;
  cdbp->cdb_klen = klen;
  cdbp->cdb_vpos = pos + klen;
  cdbp->cdb_vlen = vlen;
  *cptr = pos + klen + vlen;
  return 1;
}
