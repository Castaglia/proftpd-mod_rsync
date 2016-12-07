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
#include "options.h"
#include "names.h"
#include "entry.h"
#include "msg.h"

/* Entries with names longer than this have a different wire format/flags. */
#define RSYNC_LONG_NAME_LEN		255

#ifndef S_ISDEVICE
# define S_ISDEVICE(m)	(0)
#endif

#ifndef S_ISSPECIAL
# define S_ISSPECIAL(m)	(0)
#endif

static const char *trace_channel = "rsync.entry";

struct rsync_entry *rsync_entry_create(pool *p, const char *path, int flags) {
  int res;
  struct rsync_entry *ent;
  struct stat st;
  char *best_path;
  size_t best_pathlen;

  if (p == NULL ||
      path == NULL) {
    errno = EINVAL;
    return NULL;
  }
 
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

/* Notes: see rsync-${version}/flist.c#send_file_entry()
 *
 * That rsync function makes tremendous use of static variables; these are
 * static so that they can be used as the "previous" values.
 */

static int get_codec_flags(pool *p, struct rsync_entry *ent,
    struct rsync_session *sess) {
  static time_t mtime;
  static mode_t mode;
#ifdef RSYNC_USE_HARD_LINKS
  static int64_t dev;
#endif /* RSYNC_USE_HARD_LINKS */
  static dev_t rdev;
  static uint32_t rdev_major;
  static uid_t uid;
  static gid_t gid;
  static const char *user_name, *group_name;
  static char prev_name[PR_TUNABLE_PATH_MAX+1];

  struct rsync_options *opts;
  int codec_flags = 0, len = 0;
  size_t name_len;

  opts = sess->options;

  if (S_ISREG(ent->mode)) {
    codec_flags = 0;

  } else if (S_ISDIR(ent->mode)) {
    if (sess->protocol_version >= 30) {
      if (ent->flags & RSYNC_ENTRY_DATA_FL_CONTENT_DIR) {
        codec_flags = ent->flags & RSYNC_ENTRY_DATA_FL_TOP_DIR;

      } else if (ent->flags & RSYNC_ENTRY_DATA_FL_IMPLIED_DIR) {
        codec_flags = RSYNC_ENTRY_CODEC_FL_TOP_DIR|RSYNC_ENTRY_CODEC_FL_NO_CONTENT_DIR;

      } else {
        codec_flags = RSYNC_ENTRY_CODEC_FL_NO_CONTENT_DIR;
      }

    } else {
      codec_flags = ent->flags & RSYNC_ENTRY_DATA_FL_TOP_DIR;
    }

  } else {
    codec_flags = 0;
  }

  if (ent->mode == mode) {
    codec_flags |= RSYNC_ENTRY_CODEC_FL_SAME_MODE;

  } else {
    mode = ent->mode;
  }

  if (opts->preserve_devices == TRUE &&
      S_ISDEVICE(mode)) {
  } else if (opts->preserve_specials == TRUE &&
             S_ISSPECIAL(mode) &&
             sess->protocol_version < 31) {
    /* XXX */

  } else if (sess->protocol_version < 28) {
#if 0
    /* XXX */
    rdev = MAKEDEV(0, 0);
#endif
  }

  if (opts->preserve_uid == FALSE ||
      (ent->uid == uid && *prev_name)) {
    codec_flags |= RSYNC_ENTRY_CODEC_FL_SAME_UID;

  } else {
    uid = ent->uid;

    if (opts->numeric_ids == FALSE) {
      user_name = rsync_names_add_uid(p, sess, uid);
      if (opts->allow_incr_recurse == TRUE &&
          user_name != NULL) {
        codec_flags |= RSYNC_ENTRY_CODEC_FL_USER_NAME_NEXT;
      }
    }
  }

  if (opts->preserve_gid == FALSE ||
      (ent->gid == gid && *prev_name)) {
    codec_flags |= RSYNC_ENTRY_CODEC_FL_SAME_GID;

  } else {
    gid = ent->gid;

    if (opts->numeric_ids == FALSE) {
      group_name = rsync_names_add_gid(p, sess, gid);
      if (opts->allow_incr_recurse == TRUE &&
          group_name != NULL) {
        codec_flags |= RSYNC_ENTRY_CODEC_FL_GROUP_NAME_NEXT;
      }
    }
  }

  if (ent->mtime == mtime) {
    codec_flags |= RSYNC_ENTRY_CODEC_FL_SAME_TIME;

  } else {
    mtime = ent->mtime;
  }

/* XXX MOD_NSEC? */

#ifdef RSYNC_USE_HARD_LINKS
#endif /* RSYNC_USE_HARD_LINKS */

#if 0
  for (len = 0;
       prev_name[len] && ... && (len < RSYNC_LONG_NAME_LEN);
       len++) {
  }
#endif

  name_len = strlen(ent->path + len);

  if (len > 0) {
    codec_flags |= RSYNC_ENTRY_CODEC_FL_SAME_NAME;
  }

  if (name_len > RSYNC_LONG_NAME_LEN) {
    codec_flags |= RSYNC_ENTRY_CODEC_FL_LONG_NAME;
  }

  if (sess->protocol_version >= 28) {
    if (codec_flags == 0 &&
        !S_ISDIR(mode)) {
      codec_flags |= RSYNC_ENTRY_CODEC_FL_TOP_DIR;
    }

    if ((codec_flags & 0xff00) ||
        codec_flags == 0) {
      codec_flags |= RSYNC_ENTRY_CODEC_FL_EXTENDED_FLAGS;
    }

  } else {
    if (!(codec_flags & 0xff)) {
      codec_flags |= S_ISDIR(mode) ?
        RSYNC_ENTRY_CODEC_FL_LONG_NAME :
        RSYNC_ENTRY_CODEC_FL_TOP_DIR;
    }
  }

  return codec_flags;
}

int rsync_entry_encode(pool *p, unsigned char **buf, uint32_t *buflen,
    struct rsync_entry *ent, struct rsync_session *sess) {
  struct rsync_options *opts;
  uint32_t len = 0;
  int codec_flags = 0;

  if (p == NULL ||
      buf == NULL ||
      buflen == NULL ||
      ent == NULL ||
      sess == NULL) {
    errno = EINVAL;
    return -1;
  }

  opts = sess->options;
  codec_flags = get_codec_flags(p, ent, sess);

  if (sess->protocol_version >= 28) {
    if (codec_flags & RSYNC_ENTRY_CODEC_FL_EXTENDED_FLAGS) {
      len += rsync_msg_write_short(buf, buflen, codec_flags);

    } else {
      len += rsync_msg_write_byte(buf, buflen, codec_flags);
    }

  } else {
  }

  len += rsync_msg_write_byte(buf, buflen, ent->flags);

  if (codec_flags & RSYNC_ENTRY_CODEC_FL_SAME_NAME) {
    len += rsync_msg_write_byte(buf, buflen, 0);
  }

  if (codec_flags & RSYNC_ENTRY_CODEC_FL_LONG_NAME) {
    /* XXX rsync_msg_write_varint */
  } else {
    len += rsync_msg_write_byte(buf, buflen, (char) ent->filesz);
  }

  /* XXX: write_buf(f, fname + l1, l2); */

#ifdef RSYNC_USE_HARD_LINKS
#endif /* RSYNC_USE_HARD_LINKS */

  /* XXX: write_varlong30(f, F_LENGTH(file), 3); */

  if (!(codec_flags & RSYNC_ENTRY_CODEC_FL_SAME_TIME)) {
    if (sess->protocol_version >= 30) {
      /* XXX rsync_msg_write_varlong */
    } else {
      len += rsync_msg_write_int(buf, buflen, ent->mtime);
    }
  }

/* XXX XMIT_MOD_NSEC? */

  if (!(codec_flags & RSYNC_ENTRY_CODEC_FL_SAME_MODE)) {
  }

#if 0
  if (opts->preserve_uid == TRUE &&
      !(codec_flags & RSYNC_ENTRY_CODEC_FL_SAME_UID)) {
  }

  if (opts->preserve_gid == TRUE &&
      !(codec_flags & RSYNC_ENTRY_CODEC_FL_SAME_GID)) {
  }
#endif

/* XXX Have arg for passing len back to caller? */

  errno = ENOSYS;
  return -1;
}
