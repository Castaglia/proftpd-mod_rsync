/*
 * ProFTPD - mod_rsync file manifest entries
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

#ifndef MOD_RSYNC_ENTRY_H
#define MOD_RSYNC_ENTRY_H

struct rsync_entry {
  const char *dirname;
  const char *filename;
  time_t mtime;

  /* Lowest 32 bits of the file length. */
  uint32_t file_len;

  /* File type and permissions (i.e. st.st_mode). */
  uint16_t mode;

  /* Flags indicating which bits of data to serialize out to the client. */
  uint16_t xflags;

  /* XXX For holding "extra" data */
};

#define RSYNC_ENTRY_FL_TOP_DIR		0x0001
#define RSYNC_ENTRY_FL_CONTENT_DIR	0x0002

struct rsync_entry *rsync_entry_create(pool *p, struct rsync_session *sess,
  const char *path, int flags);

#endif
