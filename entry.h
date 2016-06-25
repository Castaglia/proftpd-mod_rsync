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
  const char *path;
  size_t pathsz;

  time_t mtime;

  /* Lowest 32 bits of the file size. */
  uint32_t filesz;

  mode_t mode;
  uid_t uid;
  gid_t gid;
  dev_t rdev;

  /* Flags indicating which bits of data to serialize out to the client. */
  uint16_t flags;

  /* XXX For holding "extra" data */
};

/* These values are copied from rsync.h. */
#define RSYNC_ENTRY_CODEC_FL_TOP_DIR		0x0001
#define RSYNC_ENTRY_CODEC_FL_SAME_MODE		0x0002
#define RSYNC_ENTRY_CODEC_FL_EXTENDED_FLAGS	0x0004
#define RSYNC_ENTRY_CODEC_FL_SAME_UID		0x0008
#define RSYNC_ENTRY_CODEC_FL_SAME_GID		0x0010
#define RSYNC_ENTRY_CODEC_FL_SAME_NAME		0x0020
#define RSYNC_ENTRY_CODEC_FL_LONG_NAME		0x0040
#define RSYNC_ENTRY_CODEC_FL_SAME_TIME		0x0080
#define RSYNC_ENTRY_CODEC_FL_SAME_RDEV_MAJOR	0x0100
#define RSYNC_ENTRY_CODEC_FL_NO_CONTENT_DIR	0x0100
#define RSYNC_ENTRY_CODEC_FL_HARDLINKED		0x0200
#define RSYNC_ENTRY_CODEC_FL_SAME_DEV		0x0400
#define RSYNC_ENTRY_CODEC_FL_USER_NAME_NEXT	0x0400
#define RSYNC_ENTRY_CODEC_FL_RDEV_MINOR		0x0800
#define RSYNC_ENTRY_CODEC_FL_GROUP_NAME_NEXT	0x0800
#define RSYNC_ENTRY_CODEC_FL_HARDLINK_FIRST	0x1000
#define RSYNC_ENTRY_CODEC_FL_IO_ERR_ENDLIST	0x1000
#define RSYNC_ENTRY_CODEC_FL_MTIME_NSECS	0x2000

/* These flags also come from rsync.h.  They do not necessarily need to be
 * kept in sync with the rsync code, since they are not sent/used in the wire
 * codec.  But it does mean for easier maintenance/comparison when reading the
 * rsync code base.
 */
#define RSYNC_ENTRY_DATA_FL_TOP_DIR		0x0001
#define RSYNC_ENTRY_DATA_FL_OURS		0x0001
#define RSYNC_ENTRY_DATA_FL_FILE_SENT		0x0002
#define RSYNC_ENTRY_DATA_FL_DIR_CREATED		0x0002
#define RSYNC_ENTRY_DATA_FL_CONTENT_DIR		0x0004
#define RSYNC_ENTRY_DATA_FL_MOUNT_DIR		0x0008
#define RSYNC_ENTRY_DATA_FL_SKIP_HARDLINK	0x0008
#define RSYNC_ENTRY_DATA_FL_DUPLICATE		0x0010
#define RSYNC_ENTRY_DATA_FL_MISSING_DIR		0x0010
#define RSYNC_ENTRY_DATA_FL_HARDLINKED		0x0020
#define RSYNC_ENTRY_DATA_FL_HARDLINK_FIRST	0x0040
#define RSYNC_ENTRY_DATA_FL_IMPLIED_DIR		0x0040
#define RSYNC_ENTRY_DATA_FL_HARDLINK_LAST	0x0080
#define RSYNC_ENTRY_DATA_FL_HARDLINK_DONE	0x0100
#define RSYNC_ENTRY_DATA_FL_LEN64		0x0200
#define RSYNC_ENTRY_DATA_FL_SKIP_GROUP		0x0400
#define RSYNC_ENTRY_DATA_FL_TIME_FAILED		0x0800
#define RSYNC_ENTRY_DATA_FL_MTIME_NSECS		0x1000

struct rsync_entry *rsync_entry_create(pool *p, struct rsync_session *sess,
  const char *path, int flags);

int rsync_entry_encode(pool *p, unsigned char **buf, uint32_t *buflen,
  struct rsync_entry *entry, unsigned int protocol_version);

#endif
