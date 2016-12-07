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
#include "options.h"
#include "names.h"
#include "msg.h"

struct name_id {
  struct name_id *next;
  id_t id;
  const char *name;
  unsigned char name_len;
};

static pool *names_pool = NULL;
static struct name_id *user_nids = NULL;
static struct name_id *group_nids = NULL;

static const char *trace_channel = "rsync.names";

/* Note: see the rsync source code's uidlist.c file for the implementation;
 * the prototypes are declared in proto.h.
 */

static void add_nid(struct name_id **head, id_t id, const char *name) {
  struct name_id *nid;

  nid = pcalloc(names_pool, sizeof(struct name_id));
  nid->id = id;
  nid->name = pstrdup(names_pool, name);
  if (nid->name != NULL) {
    nid->name_len = (unsigned char) strlen(nid->name);
  }

  nid->next = *head;
  *head = nid;
}

static struct name_id *find_by_id(struct name_id *nids, id_t id) {
  struct name_id *nid;

  for (nid = nids; nid; nid = nid->next) {
    if (nid->id == id) {
      return nid;
    }
  }

  return NULL;
}

const char *rsync_names_add_uid(pool *p, uid_t uid) {
  const char *name;
  struct name_id *nid;
  int xerrno;

  if (p == NULL) {
    errno = EINVAL;
    return NULL;
  }

  if (uid == PR_ROOT_UID) {
    /* Don't map root. */
    errno = EPERM;
    return NULL;
  }

  nid = find_by_id(user_nids, uid);
  if (nid != NULL) {
    errno = EEXIST;
    return NULL;
  }

  name = pr_auth_uid2name(p, uid);
  xerrno = errno;

  add_nid(&user_nids, uid, name);

  errno = xerrno;
  return name;
}

const char *rsync_names_add_gid(pool *p, gid_t gid) {
  const char *name;
  struct name_id *nid;
  int xerrno;

  if (p == NULL) {
    errno = EINVAL;
    return NULL;
  }

  if (gid == 0) {
    /* Don't map root. */
    errno = EPERM;
    return NULL;
  }

  nid = find_by_id(group_nids, gid);
  if (nid != NULL) {
    errno = EEXIST;
    return NULL;
  }

  name = pr_auth_gid2name(p, gid);
  xerrno = errno;

  add_nid(&group_nids, gid, name);

  errno = xerrno;
  return name;
}

uint32_t rsync_names_encode(pool *p, unsigned char **buf, uint32_t *buflen,
    struct rsync_session *sess) {
  struct rsync_options *opts;
  uint32_t len = 0;

  if (p == NULL ||
      buf == NULL ||
      buflen == NULL ||
      sess == NULL) {
    errno = EINVAL;
    return 0;
  }

  opts = sess->options;

  if (opts->preserve_uid == TRUE ||
      opts->preserve_acls == TRUE) {
    struct name_id *nid;

    for (nid = user_nids; nid; nid = nid->next) {
      if (nid->name == NULL) {
        continue;
      }

      if (sess->protocol_version < 30) {
        len += rsync_msg_write_int(buf, buflen, nid->id);

      } else {
        len += rsync_msg_write_varint(buf, buflen, nid->id);
      }

      len += rsync_msg_write_byte(buf, buflen, nid->name_len);
      len += rsync_msg_write_data(buf, buflen, nid->name, nid->name_len);
    }

    /* Note that we terminate the UID list with a zero UID.  We explicitly
     * exclude UID 0 from the list.
     */
    if (sess->protocol_version < 30) {
      len += rsync_msg_write_int(buf, buflen, 0);

    } else {
      len += rsync_msg_write_varint(buf, buflen, 0);
    }
  }

  if (opts->preserve_gid == TRUE ||
      opts->preserve_acls == TRUE) {
    struct name_id *nid;

    for (nid = group_nids; nid; nid = nid->next) {
      if (nid->name == NULL) {
        continue;
      }

      if (sess->protocol_version < 30) {
        len += rsync_msg_write_int(buf, buflen, nid->id);

      } else {
        len += rsync_msg_write_varint(buf, buflen, nid->id);
      }

      len += rsync_msg_write_byte(buf, buflen, nid->name_len);
      len += rsync_msg_write_data(buf, buflen, nid->name, nid->name_len);
    }

    /* Note that we terminate the GID list with a zero GID.  We explicitly
     * exclude GID 0 from the list.
     */
    if (sess->protocol_version < 30) {
      len += rsync_msg_write_int(buf, buflen, 0);

    } else {
      len += rsync_msg_write_varint(buf, buflen, 0);
    }
  }

  return len;
}

int rsync_names_alloc(pool *p) {
  if (p == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (names_pool != NULL) {
    destroy_pool(names_pool);
  }

  names_pool = make_sub_pool(p);
  pr_pool_tag(names_pool, "RSync Names Pool");

  return 0;
}

int rsync_names_destroy(void) {
  if (names_pool != NULL) {
    destroy_pool(names_pool);
    names_pool = NULL;
    user_nids = group_nids = NULL;
  }

  return 0;
}
