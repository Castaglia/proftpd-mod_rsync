/*
 * ProFTPD - mod_rsync protocol versions
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
#include "session.h"

#ifndef MOD_RSYNC_VERSION_H
#define MOD_RSYNC_VERSION_H

/* The rsync protocol version we want to use. */
#define RSYNC_PROTOCOL_VERSION          30

/* Define a narrow range of acceptable supported rsync protocol versions until
 * support for older versions is added.
 */
#define RSYNC_PROTOCOL_VERSION_MIN      28
#define RSYNC_PROTOCOL_VERSION_MAX      31

/* Protocol version compatibility flags */
#define RSYNC_VERSION_COMPAT_FL_INCR_RECURSE		0x0001

int rsync_version_handle_data(pool *p, struct rsync_session *sess,
  unsigned char **data, uint32_t *datalen);

#endif /* MOD_RSYNC_VERSION_H */
