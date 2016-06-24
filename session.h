/* 
 * ProFTPD - mod_rsync session data
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

#ifndef MOD_RSYNC_SESSION_H
#define MOD_RSYNC_SESSION_H

/* This struct is used to maintain the per-channel rsync-specific state. */
struct rsync_session {
  struct rsync_session *next, *prev;

  pool *pool;
  uint32_t channel_id;

  /* State */
  unsigned long state;

  unsigned int protocol_version;

  /* Opaque pointer to a struct rsync_options; will be filled in later. */
  void *options;

  /* Non-option paths/arguments */
  array_header *args;

  /* Checksum seed */
  int32_t checksum_seed;

  /* Filters */
  array_header *filters;
};

/* Session states */
#define RSYNC_SESS_FL_PROTO_VERSION	0x001
#define RSYNC_SESS_FL_CHECKSUM_SEED	0x002
#define RSYNC_SESS_FL_FILTERS		0x004
#define RSYNC_SESS_FL_SENT_MANIFEST	0x008

#define RSYNC_SESS_FL_RECVD_DATA	0x010

struct rsync_session *rsync_session_get(uint32_t channel_id);
int rsync_session_open(uint32_t channel_id);
int rsync_session_close(uint32_t channel_id);

#endif /* MOD_RSYNC_SESSION_H */
