/*
 * ProFTPD - mod_rsync message/IO formatting
 * Copyright (c) 2010-2016 TJ Saunders
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * As a special exemption, TJ Saunders and other respective copyright holders
 * give permission to link this program with OpenSSL, and distribute the
 * resulting executable, without including the source code for OpenSSL in the
 * source distribution.
 */

#include "mod_rsync.h"
#include "msg.h"
#include "disconnect.h"

#if defined(HAVE_ENDIAN_H)
# include <endian.h>
#elif defined(HAVE_MACHINE_ENDIAN_H)
# include <machine/endian.h>
#endif

/* Note: For some strange reason, the rsync authors chose to format/transmit
 * all of their numbers using LSB, rather than MSB.  This means that rather
 * using the standard htons()/htonl(), we have to use our own byte swapping
 * routines.  Whee.
 *
 * And to make things more fun, the determination of the byte order on a
 * platform, which should be made easy by system headers/macros, is an
 * exercise in portability (and thus in patience and endurance).
 */

#if !defined(BYTE_ORDER)
# if defined(__BYTE_ORDER)
#  define BYTE_ORDER	__BYTE_ORDER
# endif
#endif /* BYTE_ORDER */

#if !defined(BIG_ENDIAN)
# if defined(__BIG_ENDIAN)
#  define BIG_ENDIAN	__BIG_ENDIAN
# elif defined(__BIG_ENDIAN__)
#  define BIG_ENDIAN	__BIG_ENDIAN__
# endif
#endif /* BIG_ENDIAN */

#if !defined(LITTLE_ENDIAN)
# if defined(__LITTLE_ENDIAN)
#  define LITTLE_ENDIAN	__LITTLE_ENDIAN
# elif defined(__LITTLE_ENDIAN__)
#  define LITTLE_ENDIAN	__LITTLE_ENDIAN__
# endif
#endif /* LITTLE_ENDIAN */

#define AS_CVAL(b, i)		(((unsigned char *) (b))[i])
#define AS_UVAL(b, i)		((uint32_t) AS_CVAL(b, i))

#if BYTE_ORDER == BIG_ENDIAN
# define AS_PVAL(b, i)		(AS_UVAL(b, i) | AS_UVAL(b, (i)+1) << 8)
# define AS_IVAL(b, i)		(AS_PVAL(b, i) | AS_PVAL(b, (i)+2) << 16)
# define AS_IVAL64(b, i)	(AS_IVAL(b, i) | (int64_t) AS_IVAL(b, (i)+4) << 32)
# define SET_SVALX(b, i, v)	(AS_CVAL(b, i) = (v) & 0xff, AS_CVAL(b, i+1) = (v) >> 8)
# define SET_IVALX(b, i, v)	(SET_SVALX(b, i, v & 0xffff), SET_SVALX(b, i+2, v >> 16))
# define SET_IVAL(b, i)		SET_IVALX(b, i, (uint32_t) (v))

# define rsync_ntole16(x) \
    ((uint32_t) (x) & 0x0000ff00) << 8) | \
    ((uint32_t) (x) & 0x00ff0000) >> 8)
# define rsync_ntole32(x) \
    ((uint32_t) (x) & 0x000000ff) << 24) | \
    ((uint32_t) (x) & 0x0000ff00) << 8)  | \
    ((uint32_t) (x) & 0x00ff0000) >> 8)  | \
    ((uint32_t) (x) & 0xff000000) >> 24)
# define rsync_ntole64(x) \
    ((uint64_t) (x) & 0x00000000000000ff) << 56) | \
    ((uint64_t) (x) & 0x000000000000ff00) << 40) | \
    ((uint64_t) (x) & 0x0000000000ff0000) << 24) | \
    ((uint64_t) (x) & 0x00000000ff000000) << 8)  | \
    ((uint64_t) (x) & 0x000000ff00000000) >> 8)  | \
    ((uint64_t) (x) & 0x0000ff0000000000) >> 24) | \
    ((uint64_t) (x) & 0x00ff000000000000) >> 40) | \
    ((uint64_t) (x) & 0xff00000000000000) >> 56)
#elif BYTE_ORDER == LITTLE_ENDIAN
# define AS_IVAL(b, i)		(*(uint32_t *)((char *)(b) + (i)))
# define SET_IVAL(b, i, v)	AS_IVAL(b, i)=((uint32_t) (val))

# define rsync_ntole16(x)	((uint16_t) (x))
# define rsync_ntole32(x)	((uint32_t) (x))
# define rsync_ntole64(x)	((uint64_t) (x))

static inline void SET_IVAL64(char *b, int i, int64_t v) {
  union {
    char *b;
    int64_t *l;
  } n;

  n.b = b + i;
  *n.l = v;
}

#else
# error "Endianness of platform is unknown"
#endif

static unsigned char int_byte_extra[64] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* (00 - 3F)/4 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* (40 - 7F)/4 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* (80 - BF)/4 */
  2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6, /* (C0 - FF)/4 */
};

char rsync_msg_read_byte(pool *p, unsigned char **buf, uint32_t *buflen) {
  char byte = 0;

  (void) p;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

  if (*buflen < sizeof(char)) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to read byte (buflen = %lu)",
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return 0;
  }

  memcpy(&byte, *buf, sizeof(char));
  (*buf) += sizeof(char);
  (*buflen) -= sizeof(char);

  return byte;
}

int16_t rsync_msg_read_short(pool *p, unsigned char **buf, uint32_t *buflen) {
  int16_t val = 0;

  (void) p;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

  if (*buflen < sizeof(int16_t)) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to read short (buflen = %lu)",
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return 0;
  }

  memcpy(&val, *buf, sizeof(int16_t));
  (*buf) += sizeof(int16_t);
  (*buflen) -= sizeof(int16_t);

  val = rsync_ntole16(val);
  return val;
}

int32_t rsync_msg_read_int(pool *p, unsigned char **buf, uint32_t *buflen) {
  int32_t val = 0;

  (void) p;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

  if (*buflen < sizeof(int32_t)) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to read int (buflen = %lu)",
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return 0;
  }

  memcpy(&val, *buf, sizeof(int32_t));
  (*buf) += sizeof(int32_t);
  (*buflen) -= sizeof(int32_t);

  val = rsync_ntole32(val);
  return val;
}

int32_t rsync_msg_read_varint(pool *p, unsigned char **buf, uint32_t *buflen) {
  union {
    char b[5];
    int32_t i;
  } n;
  int extra;
  unsigned char b;

  (void) p;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

  n.i = 0;

  b = rsync_msg_read_byte(p, buf, buflen);
  extra = int_byte_extra[b/4];
  if (extra > 0) {
    unsigned char bit, *data;

    bit = ((unsigned char) 1 << (8 - extra));
    if (extra >= ((int) sizeof(n.b))) {
      (void) pr_log_pri(PR_LOG_NOTICE, MOD_RSYNC_VERSION
        ": overflow when reading varint from network");
      pr_log_stacktrace(-1, MOD_RSYNC_VERSION);
      pr_session_disconnect(&rsync_module, PR_SESS_DISCONNECT_BY_APPLICATION,
        "malformed data");
      return 0;
    }

    data = rsync_msg_read_data(p, buf, buflen, extra);
    if (data == NULL) {
      return 0;
    }

    memcpy(n.b, data, extra);
    n.b[extra] = (b & (bit-1));

  } else {
    n.b[0] = b;
  }

#if BYTE_ORDER == BIG_ENDIAN
  n.i = AS_IVAL(n.b, 0);
#endif

#if SIZEOF_INT32_T > 4
  if (n.i & ((int32_t) 0x80000000)) {
    n.i |= ~((int32_t) 0xffffffff);
  }
#endif

  return n.i;
}

int64_t rsync_msg_read_long(pool *p, unsigned char **buf, uint32_t *buflen) {
  int64_t val = 0;

  (void) p;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

  if (*buflen < sizeof(int64_t)) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to read long (buflen = %lu)",
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return 0;
  }

  memcpy(&val, *buf, sizeof(int64_t));
  (*buf) += sizeof(int64_t);
  (*buflen) -= sizeof(int64_t);

  val = rsync_ntole64(val);
  return val;
}

int64_t rsync_msg_read_varlong(pool *p, unsigned char **buf, uint32_t *buflen,
    unsigned char min) {
  union {
    char b[9];
    int64_t l;
  } n;
  int extra;
  char b[8];
  unsigned char *data;

  (void) p;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

#if SIZEOF_INT64_T < 8
  memset(n.b, 0, 8);
#else
  n.l = 0;
#endif

  data = rsync_msg_read_data(p, buf, buflen, min);
  if (data == NULL) {
    return 0;
  }

  memset(b, 0, sizeof(b));
  memmove(b, data, min);
  memmove(n.b, b + 1, min - 1);
  extra = int_byte_extra[AS_CVAL(b, 0) / 4];
  if (extra > 0) {
    unsigned char bit, *more;

    bit = ((unsigned char) 1 << (8 - extra));
    if ((min + extra) > ((int) sizeof(n.b))) {
      (void) pr_log_pri(PR_LOG_NOTICE, MOD_RSYNC_VERSION
        ": overflow when reading varlong from network");
      pr_log_stacktrace(-1, MOD_RSYNC_VERSION);
      pr_session_disconnect(&rsync_module, PR_SESS_DISCONNECT_BY_APPLICATION,
        "malformed data");
      return 0;
    }

    more = rsync_msg_read_data(p, buf, buflen, extra);
    if (more == NULL) {
      return 0;
    }

    memcpy(n.b + min - 1, more, extra);
    n.b[min + extra - 1] = (AS_CVAL(data, 0) & (bit-1));

#if SIZEOF_INT64_T < 8
    if (min + extra > 5 ||
        n.b[4] ||
        (((unsigned char) n.b[3]) & 0x80) {
      (void) pr_log_pri(PR_LOG_NOTICE, MOD_RSYNC_VERSION
        ": overflow when reading varlong from network: "
        "attempted 64-bit offset");
      pr_log_stacktrace(-1, MOD_RSYNC_VERSION);
      pr_session_disconnect(&rsync_module, PR_SESS_DISCONNECT_BY_APPLICATION,
        "malformed data");
      return 0;
    }
#endif

  } else {
    n.b[min + extra - 1] = AS_CVAL(data, 0);
  }

#if SIZEOF_INT64_T < 8
  n.l = AS_IVAL(n.b, 0);
#elif BYTE_ORDER == BIG_ENDIAN
  n.l = AS_IVAL64(n.b, 0);
#endif

  return n.l;
}

unsigned char *rsync_msg_read_data(pool *p, unsigned char **buf,
    uint32_t *buflen, size_t datalen) {
  unsigned char *data = NULL;

  if (p == NULL ||
      buf == NULL ||
      buflen == NULL) {
    errno = EINVAL;
    return NULL;
  }

  if (*buflen < datalen) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to read %lu %s of data (buflen = %lu)",
      (unsigned long) datalen, datalen != 1 ? "bytes" : "byte",
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return NULL;
  }

  if (datalen == 0) {
    return NULL;
  }

  data = palloc(p, datalen);
  memcpy(data, *buf, datalen);
  (*buf) += datalen;
  (*buflen) -= datalen;

  return data;
}

char *rsync_msg_read_string(pool *p, unsigned char **buf, uint32_t *buflen,
    size_t datalen) {
  unsigned char *data = NULL;
  char *s;

  data = rsync_msg_read_data(p, buf, buflen, datalen);
  if (data == NULL) {
    return NULL;
  }

  s = pcalloc(p, datalen+1);
  memmove(s, data, datalen);
  return s;
}

uint32_t rsync_msg_write_byte(unsigned char **buf, uint32_t *buflen,
    char byte) {
  uint32_t len;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

  len = sizeof(char);

  if (*buflen < len) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to write byte (buflen = %lu)",
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return 0;
  }

  memcpy(*buf, &byte, len);
  (*buf) += len;
  (*buflen) -= len;

  return len;
}

uint32_t rsync_msg_write_short(unsigned char **buf, uint32_t *buflen,
    int16_t val) {
  uint32_t len = 0;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

  len = sizeof(int16_t);

  if (*buflen < len) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to write short (buflen = %lu)",
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return 0;
  }

  val = rsync_ntole16(val);
  memcpy(*buf, &val, len);
  (*buf) += len;
  (*buflen) -= len;

  return len;
}

uint32_t rsync_msg_write_int(unsigned char **buf, uint32_t *buflen,
    int32_t val) {
  uint32_t len = 0;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

  len = sizeof(int32_t);

  if (*buflen < len) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to write int (buflen = %lu)",
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return 0;
  }

  val = rsync_ntole32(val);
  memcpy(*buf, &val, len);
  (*buf) += len;
  (*buflen) -= len;

  return len;
}

uint32_t rsync_msg_write_varint(unsigned char **buf, uint32_t *buflen,
    int32_t val) {
  uint32_t len = 0;
  char b[5];
  unsigned char bit;
  int l = 4;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

  SET_IVAL(b, 1, val);

  while (l > 1 &&
         b[l] == 0) {
    l--;
  }

  bit = ((unsigned char) 1 << (7 - l + 1));
  if (AS_CVAL(b, l) >= bit) {
    l++;
    b[0] = ~(bit - 1);

  } else if (l > 1) {
    b[0] = b[l] | ~((bit * 2) - 1);

  } else {
    b[0] = b[l];
  }

  if (*buflen < l) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to write varint (len %d, buflen = %lu)", l,
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return 0;
  }

  len = rsync_msg_write_data(buf, buflen, (const unsigned char *) b, l);
  return len;
}

uint32_t rsync_msg_write_long(unsigned char **buf, uint32_t *buflen,
    int64_t val) {
  uint32_t len = 0;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

  len = sizeof(int64_t);

  if (*buflen < len) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to write long (buflen = %lu)",
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return 0;
  }

  val = rsync_ntole64(val);
  memcpy(*buf, &val, len);
  (*buf) += len;
  (*buflen) -= len;

  return len;
}

uint32_t rsync_msg_write_varlong(unsigned char **buf, uint32_t *buflen,
    int64_t val, unsigned char min) {
  uint32_t len = 0;
  char b[9];
  unsigned char bit;
  int l = 8;

  if (buf == NULL ||
      buflen == NULL) {
    return 0;
  }

#if SIZEOF_INT64_T >= 8
  SET_IVAL64(b, 1, val);

#else
  SET_IVAL(b, 1, val);
  if (val <= 0x7fffffff &&
      val >= 0) {
    memset(b + 5, 0, 4);

  } else {
    (void) pr_log_pri(PR_LOG_NOTICE, MOD_RSYNC_VERSION
      ": overflow when writing varlong to network: "
      "attempted 64-bit offset");
    pr_log_stacktrace(-1, MOD_RSYNC_VERSION);
    pr_session_disconnect(&rsync_module, PR_SESS_DISCONNECT_BY_APPLICATION,
      "malformed data");
    return 0;
  }
#endif

  while (l > min &&
         b[l] == 0) {
    l--;
  }

  bit = ((unsigned char) 1 << (7 - l + min));
  if (AS_CVAL(b, l) >= bit) {
    l++;
    b[0] = ~(bit - 1);

  } else if (l > min) {
    b[0] = b[l] | ~((bit * 2) - 1);

  } else {
    b[0] = b[l];
  }

  if (*buflen < l) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to write varlong (len %d, buflen = %lu)", l,
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return 0;
  }

  len = rsync_msg_write_data(buf, buflen, (const unsigned char *) b, l);
  return len;
}

uint32_t rsync_msg_write_data(unsigned char **buf, uint32_t *buflen,
    const unsigned char *data, size_t datalen) {
  uint32_t len = 0;

  if (buf == NULL ||
      buflen == NULL ||
      data == NULL) {
    return 0;
  }

  if (*buflen < datalen) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to write %lu %s of data (buflen = %lu)",
      (unsigned long) datalen, datalen != 1 ? "bytes" : "byte",
      (unsigned long) *buflen);
    pr_log_stacktrace(rsync_logfd, MOD_RSYNC_VERSION);
    RSYNC_DISCONNECT("IO error");
    return 0;
  }

  if (datalen > 0) {
    memcpy(*buf, data, datalen);
    (*buf) += datalen;
    (*buflen) -= datalen;

    len += datalen;
  }

  return len;
}

uint32_t rsync_msg_write_string(unsigned char **buf, uint32_t *buflen,
    const char *s) {
  size_t len;

  if (buf == NULL ||
      buflen == NULL ||
      s == NULL) {
    return 0;
  }

  len = strlen(s);
  return rsync_msg_write_data(buf, buflen, (const unsigned char *) s, len);
}
