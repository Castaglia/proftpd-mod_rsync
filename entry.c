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
#include "entry.h"
#include "msg.h"

/* Entries with names longer than this have a different wire format/flags. */
#define RSYNC_LONG_NAME_LEN		255

static const char *trace_channel = "rsync";

struct rsync_entry *rsync_entry_create(pool *p, struct rsync_session *sess,
    const char *path, int flags) {
  int res;
  struct rsync_entry *ent;
  struct stat st;
  char *best_path;
  size_t best_pathlen;
 
  best_path = dir_best_path(p, path);
  if (best_path == NULL) {
    int xerrno = errno;

    pr_trace_msg(trace_channel, 3, "error finding best path for '%s': %s",
      path, strerror(xerrno));

    errno = xerrno;
    return NULL;
  }

  pr_fs_clear_cache2(best_path);
  res = pr_fsio_lstat(best_path, &st);
  if (res < 0) {
    int xerrno = errno;

    pr_trace_msg(trace_channel, 3, "error stat'ing '%s': %s", best_path,
      strerror(xerrno));

    errno = xerrno;
    return NULL;
  }

  ent = pcalloc(p, sizeof(struct rsync_entry));
  ent->mtime = st.st_mtime;
  ent->mode = st.st_mode;
  ent->filesz = (uint32_t) st.st_size;
  ent->uid = st.st_uid;
  ent->gid = st.st_gid;
  ent->rdev = st.st_rdev;

  best_pathlen = strlen(best_path);
  if (best_pathlen > RSYNC_LONG_NAME_LEN) {
    flags |= RSYNC_ENTRY_CODEC_FL_LONG_NAME;
  }

  ent->path = best_path;
  ent->pathsz = best_pathlen;

  if (S_ISDIR(st.st_mode)) {
    ent->flags = flags|RSYNC_ENTRY_CODEC_FL_TOP_DIR;

  } else {
    ent->flags = flags;
  }

#if 0
#if SIZEOF_OFF_T >= 8
  if (st->st_size > 0xffffffffU &&
      S_ISREG(st->st_mode)) {
    ent->flags |= RSYNC_ENTRY_CODEC_FL_LEN64;
    ent->extras.uint = (uint32_t) (st.st_size >> 32);
  }
#endif
#endif

  return ent;
}

int rsync_entry_encode(pool *p, unsigned char **buf, uint32_t *buflen,
    struct rsync_entry *ent, unsigned int protocol_version) {
  uint32_t len = 0;

  len += rsync_msg_write_byte(buf, buflen, ent->flags);

  if (ent->flags & RSYNC_ENTRY_CODEC_FL_SAME_NAME) {
    len += rsync_msg_write_byte(buf, buflen, 0);
  }

  if (ent->flags & RSYNC_ENTRY_CODEC_FL_LONG_NAME) {
    /* XXX rsync_msg_write_varint */
  } else {
    len += rsync_msg_write_byte(buf, buflen, (char) ent->filesz);
  }

  errno = ENOSYS;
  return -1;
}
