/* 
 * ProFTPD - mod_rsync filters
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
#include "options.h"
#include "msg.h"
#include "disconnect.h"

static const char *trace_channel = "rsync";

int rsync_filters_handle_data(pool *p, struct rsync_session *sess,
    unsigned char **data, uint32_t *datalen) {
  unsigned char *buf, *ptr;
  uint32_t buflen, bufsz;
  array_header *filters;
  struct rsync_options *opts;

  opts = sess->options;
  filters = sess->filters;

  if (filters == NULL) {
    filters = sess->filters = make_array(sess->pool, 1, sizeof(char *));
  }
 
  /* We expect to receive a filter list from the client if:
   *
   * a) we're the sender, OR
   * b) client requested --prune-empty-dirs OR
   *    client requested --delete AND the protocol version is >= 29
   */

  if (opts->sender == TRUE ||
      (opts->prune_empty_dirs == TRUE ||
       (opts->delete_mode == TRUE && sess->protocol_version >= 29))) {
#define RSYNC_MAX_STRLEN        4096
    int32_t filter_len;
    char *filter_data;

    filter_len = rsync_msg_read_int(p, data, datalen);

    while (filter_len != 0) {
      pr_signals_handle();

      if (filter_len >= RSYNC_MAX_STRLEN) {
        RSYNC_DISCONNECT("filter rule too long");
      }

      filter_data = rsync_msg_read_string(p, data, datalen, filter_len);
      pr_trace_msg(trace_channel, 15, "received filter '%s'", filter_data);
      *((char **) push_array(filters)) = pstrdup(sess->pool, filter_data);

      filter_len = rsync_msg_read_int(p, data, datalen);
    }

    pr_trace_msg(trace_channel, 9, "processed filters (%u)", filters->nelts);
  }

  return 0;
}
