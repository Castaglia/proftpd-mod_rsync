/* 
 * ProFTPD - mod_rsync name/ID mapping
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
#include "names.h"

static const char *trace_channel = "rsync.names";

/* Note: see the rsync source code's uidlist.c file for the implementation;
 * the prototypes are declared in proto.h.
 */

const char *rsync_names_add_uid(pool *p, struct rsync_session *sess,
    uid_t uid) {
  const char *name;

  if (p == NULL ||
      sess == NULL) {
    errno = EINVAL;
    return NULL;
  }

  if (uid == PR_ROOT_UID) {
    /* Don't map root. */
    errno = EPERM;
    return NULL;
  }

  name = pr_auth_uid2name(p, uid);

/* XXX Add name/ID to linked list, stored in session, for sending. */
  errno = ENOSYS;
  return NULL;
}

const char *rsync_names_add_gid(pool *p, struct rsync_session *sess,
    gid_t gid) {

  if (p == NULL ||
      sess == NULL) {
    errno = EINVAL;
    return NULL;
  }

  errno = ENOSYS;
  return NULL;
}

int rsync_names_encode(pool *p, unsigned char **buf, uint32_t *buflen,
    struct rsync_session *sess) {
  errno = ENOSYS;
  return -1;
}
