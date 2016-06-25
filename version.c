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
#include "version.h"
#include "options.h"
#include "msg.h"
#include "disconnect.h"

static const char *trace_channel = "rsync.version";

int rsync_version_handle(pool *p, struct rsync_session *sess,
    unsigned char **data, uint32_t *datalen) {
  int32_t client_version;
  unsigned char *buf, *ptr;
  uint32_t buflen, bufsz;
  struct rsync_options *opts;

  client_version = rsync_msg_read_int(p, data, datalen);

  pr_trace_msg(trace_channel, 9, "client sent protocol version %lu",
    (unsigned long) client_version);

  /* Send our version back. */
  bufsz = buflen = sizeof(int32_t);
  ptr = buf = palloc(p, bufsz);

  rsync_msg_write_int(&buf, &buflen, RSYNC_PROTOCOL_VERSION);

  if ((rsync_write_data)(p, sess->channel_id, ptr, (bufsz - buflen)) < 0) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "error sending server protocol version: %s", strerror(errno));
    return -1;
  }

  pr_trace_msg(trace_channel, 9, "sent protocol version %lu",
    (unsigned long) RSYNC_PROTOCOL_VERSION);

  sess->protocol_version = client_version;
  if (sess->protocol_version > RSYNC_PROTOCOL_VERSION) {
    sess->protocol_version = RSYNC_PROTOCOL_VERSION;
  }

  /* Is this protocol version too old for us? */
  if (sess->protocol_version < RSYNC_PROTOCOL_VERSION_MIN) {
    RSYNC_DISCONNECT("protocol version too old");
  }
 
  /* Is this protocol version too new for us? */
  if (sess->protocol_version > RSYNC_PROTOCOL_VERSION_MAX) {
    RSYNC_DISCONNECT("protocol version too new"); 
  }

  opts = sess->options;

  if (sess->protocol_version < 29) {
    if (opts->fuzzy_basis) {
      RSYNC_DISCONNECT("--fuzzy option supported by protocol version");
    }

    if (opts->prune_empty_dirs) {
      RSYNC_DISCONNECT("--prune-empty-dirs option supported by protocol version");
    }
  }

  if (sess->protocol_version < 30) {
    if (opts->append_mode == 1) {
      opts->append_mode = 2;
    }

    if (opts->preserve_acls) {
      RSYNC_DISCONNECT("--acls option supported by protocol version");
    }

    if (opts->preserve_xattrs) {
      RSYNC_DISCONNECT("--xattrs option supported by protocol version");
    }
  }

  if (opts->delete_mode &&
      !(opts->delete_before + opts->delete_during + opts->delete_after)) {

    if (sess->protocol_version < 30) {
      opts->delete_before = 1;

    } else {
      opts->delete_during = 1;
    }
  }

  if (sess->protocol_version >= 30) {
    int32_t compat_flags = 0;
    unsigned char *buf, *ptr;
    uint32_t buflen, bufsz;

    if (opts->allow_incr_recurse) {
      compat_flags |= RSYNC_VERSION_COMPAT_FL_INCR_RECURSE;
    }

    bufsz = buflen = sizeof(int32_t);
    ptr = buf = palloc(sess->pool, bufsz);

    rsync_msg_write_int(&buf, &buflen, compat_flags);

    pr_trace_msg(trace_channel, 9, "sending compatibility flags %lu",
      (unsigned long) compat_flags);

    if ((rsync_write_data)(sess->pool, sess->channel_id, ptr,
        (bufsz - buflen)) < 0) {
      (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
        "error sending compatibility flags: %s", strerror(errno));
      return -1;
    }
  }

  pr_trace_msg(trace_channel, 9, "negotiated protocol version %lu",
    (unsigned long) sess->protocol_version);
  return 0;
}
