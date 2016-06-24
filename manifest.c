/* 
 * ProFTPD - mod_rsync file list/manifest
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
#include "entry.h"

static const char *trace_channel = "rsync.manifest";

/* Returns -1 to indicate that the given path is explicitly excluded, 1 to
 * indicate that the given path is explicitly included, and 0 to indicate that
 * the path did not match anything in the list.
 */
static int exclude_file(pool *p, array_header *filters, const char *path) {
  register unsigned int i;
  char **rules;

  rules = filters->elts;
  for (i = 0; i < filters->nelts; i++) {
    int res;

    pr_trace_msg(trace_channel, 17,
      "matching file '%s' against filter rule '%s'", path, rules[i]);

    res = pr_fnmatch(rules[i], path, PR_FNM_PERIOD|PR_FNM_PATHNAME);
    if (res == 0) {
      return -1;
    }
  }
 
  return 0;
}

int rsync_manifest_handle(pool *p, struct rsync_session *sess, char **data,
    uint32_t *datalen) {
  register unsigned int i;
  char *buf, *ptr;
  char **names;
  uint32_t buflen, bufsz;
  array_header *args, *filters;
  struct rsync_options *opts;
  int res;
  unsigned int nentries = 0;

  opts = sess->options;
  filters = sess->filters;
  args = sess->args;

  /* XXX Why do we need to build the entire list of files up in memory, rather
   * than streamily writing it out to the client?  Answer: because we need
   * to sort and clean the list before sending.  Sigh.
   */

  names = args->elts;
  for (i = 0; i < args->nelts; i++) {
    struct rsync_entry *ent;
    int flags = 0;

    res = exclude_file(p, filters, names[i]);
    if (res == -1) {
      pr_trace_msg(trace_channel, 9, "path '%s' excluded by filters", names[i]);
      continue;
    }

    ent = rsync_entry_create(p, sess, names[i], flags);

    /* XXX Handle real manifest entry */
    nentries++;
  }

  bufsz = buflen = sizeof(int32_t) * 2;
  ptr = buf = palloc(p, bufsz);

  /* Indicate the end-of-manifest */
  rsync_msg_write_int(&buf, &buflen, 0);

  /* XXX Send the id-to-name mapping list. */

  if (sess->protocol_version < 30) {
    /* Send the error flag */
    rsync_msg_write_int(&buf, &buflen, 0);

  } else {
    /* XXX ... */
  }

  if (rsync_write_data(p, sess->channel_id, ptr, (bufsz - buflen)) < 0) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "error sending file manifest: %s", strerror(errno));
    return -1;
  }

  pr_trace_msg(trace_channel, 9, "sent file manifest (%u entries)", nentries);
  return 0;
}
