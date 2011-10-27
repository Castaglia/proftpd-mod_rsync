/*
 * ProFTPD - mod_rsync message/IO formatting
 * Copyright (c) 2010 TJ Saunders
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
 *
 *
 * $Id: msg.c,v 1.4 2010/03/04 23:14:33 castaglia Exp $
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

#if BYTE_ORDER == BIG_ENDIAN
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
# define rsync_ntole32(x)	((uint32_t) (x))
# define rsync_ntole64(x)	((uint64_t) (x))
#else
# error "Endianness of platform is unknown"
#endif

char rsync_msg_read_byte(pool *p, char **buf, uint32_t *buflen) {
  char byte = 0;

  (void) p;

  if (*buflen < sizeof(char)) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "data error: unable to read byte (buflen = %lu)",
      (unsigned long) *buflen);
    RSYNC_DISCONNECT("IO error");
  }

  memcpy(&byte, *buf, sizeof(char));
  (*buf) += sizeof(char);
  (*buflen) -= sizeof(char);

  return byte;
}

char *rsync_msg_read_data(pool *p, char **buf, uint32_t *buflen,
    size_t datalen) {
  char *data = NULL;

  if (*buflen < datalen) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to read %lu bytes of data from %lu byte buffer",
      (unsigned long) datalen, (unsigned long) *buflen);
    RSYNC_DISCONNECT("IO error");
  }

  data = palloc(p, datalen);
  memcpy(data, *buf, datalen);
  (*buf) += datalen;
  (*buflen) -= datalen;

  return data;
}

int32_t rsync_msg_read_int(pool *p, char **buf, uint32_t *buflen) {
  int32_t val = 0;

  (void) p;

  if (*buflen < sizeof(int32_t)) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to read int from %lu byte buffer",
      (unsigned long) *buflen);
    RSYNC_DISCONNECT("IO error");
  }

  memcpy(&val, *buf, sizeof(int32_t));
  (*buf) += sizeof(int32_t);
  (*buflen) -= sizeof(int32_t);

  val = rsync_ntole32(val);
  return val;
}

int64_t rsync_msg_read_long(pool *p, char **buf, uint32_t *buflen) {
  int64_t val = 0;

  (void) p;

  if (*buflen < sizeof(int64_t)) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "IO error: unable to read long from %lu byte buffer",
      (unsigned long) *buflen);
    RSYNC_DISCONNECT("IO error");
  }

  memcpy(&val, *buf, sizeof(int64_t));
  (*buf) += sizeof(int64_t);
  (*buflen) -= sizeof(int64_t);

  val = rsync_ntole64(val);
  return val;
}

char *rsync_msg_read_string(pool *p, char **buf, uint32_t *buflen,
    size_t datalen) {
  char *data = NULL;

  data = rsync_msg_read_data(p, buf, buflen, datalen);
  data[datalen] = '\0';

  return data;
}

void rsync_msg_write_byte(char **buf, uint32_t *buflen, char byte) {
  if (*buflen < sizeof(char)) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "data error: unable to write byte to %lu byte buffer",
      (unsigned long) *buflen);
    RSYNC_DISCONNECT("IO error");
  }

  memcpy(*buf, &byte, sizeof(char));
  (*buf) += sizeof(char);
  (*buflen) -= sizeof(char);
}

void rsync_msg_write_data(char **buf, uint32_t *buflen, const char *data,
    size_t datalen) {
  if (*buflen < datalen) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "data error: unable to write %lu bytes of data to %lu byte buffer",
      (unsigned long) datalen, (unsigned long) *buflen);
    RSYNC_DISCONNECT("IO error");
  }

  if (datalen > 0) {
    memcpy(*buf, data, datalen);
    (*buf) += datalen;
    (*buflen) -= datalen;
  }
}

void rsync_msg_write_int(char **buf, uint32_t *buflen, int32_t val) {
  if (*buflen < sizeof(int32_t)) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "data error: unable to write int to %lu byte buffer",
      (unsigned long) *buflen);
    RSYNC_DISCONNECT("IO error");
  }

  val = rsync_ntole32(val);
  memcpy(*buf, &val, sizeof(int32_t));
  (*buf) += sizeof(int32_t);
  (*buflen) -= sizeof(int32_t);
}

void rsync_msg_write_long(char **buf, uint32_t *buflen, int64_t val) {
  if (*buflen < sizeof(int64_t)) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "data error: unable to write long to %lu byte buffer",
      (unsigned long) *buflen);
    RSYNC_DISCONNECT("IO error");
  }

  val = rsync_ntole64(val);
  memcpy(*buf, &val, sizeof(int64_t));
  (*buf) += sizeof(int64_t);
  (*buflen) -= sizeof(int64_t);
}

void rsync_msg_write_string(char **buf, uint32_t *buflen, const char *str) {
  size_t len;

  len = strlen(str);
  rsync_msg_write_data(buf, buflen, str, len);
}
