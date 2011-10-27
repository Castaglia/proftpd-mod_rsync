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
#include "session.h"

static struct rsync_session *rsync_sessions = NULL;

struct rsync_session *rsync_session_get(uint32_t channel_id) {
  struct rsync_session *sess;

  sess = rsync_sessions;
  while (sess) {
    pr_signals_handle();

    if (sess->channel_id == channel_id) {
      return sess;
    }

    sess = sess->next;
  }

  errno = ENOENT;
  return NULL;
}

int rsync_session_open(uint32_t channel_id) {
  pool *sub_pool;
  struct rsync_session *sess, *last;

  /* Check to see if we already have an rsync session opened for the given
   * channel ID.
   */
  sess = last = rsync_sessions;
  while (sess) {
    pr_signals_handle();

    if (sess->channel_id == channel_id) {
      errno = EEXIST;
      return -1;
    }

    if (sess->next == NULL) {
      /* This is the last item in the list. */
      last = sess;
    }

    sess = sess->next;
  }

  /* Looks like we get to allocate a new one. */
  sub_pool = make_sub_pool(rsync_pool);
  pr_pool_tag(sub_pool, "Rsync session pool");

  sess = pcalloc(sub_pool, sizeof(struct rsync_session));
  sess->pool = sub_pool;
  sess->channel_id = channel_id;

  if (last) {
    last->next = sess;
    sess->prev = last;

  } else {
    rsync_sessions = sess;
  }

  pr_session_set_protocol("rsync");
  return 0;
}

int rsync_session_close(uint32_t channel_id) {
  struct rsync_session *sess;

  /* Check to see if we have an rsync session opened for this channel ID. */
  sess = rsync_sessions;
  while (sess) {
    pr_signals_handle();

    if (sess->channel_id == channel_id) {
      if (sess->next)
        sess->next->prev = sess->prev;

      if (sess->prev) {
        sess->prev->next = sess->next;

      } else {
        /* This is the start of the session list. */
        rsync_sessions = sess->next;
      }

      /* XXX Perform any necessarily cleanup/checks here */

      pr_session_set_protocol("ssh2");
      return 0;
    }

    sess = sess->next;
  }

  errno = ENOENT;
  return -1;
}

