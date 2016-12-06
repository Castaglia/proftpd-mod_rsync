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

#ifndef MOD_RSYNC_MSG_H
#define MOD_RSYNC_MSG_H

char rsync_msg_read_byte(pool *p, unsigned char **buf, uint32_t *buflen);
uint16_ rsync_msg_read_short(pool *p, unsigned char **buf, uint32_t *buflen);
int32_t rsync_msg_read_int(pool *p, unsigned char **buf, uint32_t *buflen);
int64_t rsync_msg_read_long(pool *p, unsigned char **buf, uint32_t *buflen);
unsigned char *rsync_msg_read_data(pool *p, unsigned char **buf,
  uint32_t *buflen, size_t datalen);
char *rsync_msg_read_string(pool *p, unsigned char **buf, uint32_t *buflen,
  size_t datalen);

uint32_t rsync_msg_write_byte(unsigned char **buf, uint32_t *buflen, char val);
uint32_t rsync_msg_write_short(unsigned char **buf, uint32_t *buflen,
  int16_t val);
uint32_t rsync_msg_write_int(unsigned char **buf, uint32_t *buflen,
  int32_t val);
uint32_t rsync_msg_write_long(unsigned char **buf, uint32_t *buflen,
  int64_t val);
uint32_t rsync_msg_write_data(unsigned char **buf, uint32_t *buflen,
  const unsigned char *data, size_t datalen);
uint32_t rsync_msg_write_string(unsigned char **buf, uint32_t *buflen,
  const char *str);

#endif /* MOD_RSYNC_MSG_H */
