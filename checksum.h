/*
 * ProFTPD - mod_rsync checksums
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
 */

#ifndef MOD_RSYNC_CHECKSUM_H
#define MOD_RSYNC_CHECKSUM_H

#include "mod_rsync.h"
#include "session.h"

/* XXX Protocol version 30 and later use MD5; prior to that, MD4.
 *
 * Since MD4 is considered weak, recent versions of OpenSSL have to be 
 * compiled with MD4 support explicitly enabled; for these versions, we
 * will need to supply our own MD4.  Damn.
 */

int rsync_checksum_handle_data(pool *p, struct rsync_session *sess,
  unsigned char **data, uint32_t *datalen);

#endif /* MOD_RSYNC_CHECKSUM_H */
