/*
 * ProFTPD: mod_rsync -- a module supporting the rsync protocol
 *
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
 * This is mod_rsync, contrib software for proftpd 1.3.x and above.
 * For more information contact TJ Saunders <tj@castaglia.org>.
 *
 * --- DO NOT EDIT BELOW THIS LINE ---
 * $Archive: mod_rsync.a $
 * $Libraries: -lrsync -lpopt -lz$
 * $Id: mod_deflate.c,v 1.3 2007/03/05 17:55:51 tj Exp tj $
 */

#include "mod_rsync.h"
#include "msg.h"
#include "session.h"
#include "options.h"
#include "disconnect.h"
#include "version.h"
#include "checksum.h"
#include "filters.h"
#include "manifest.h"

module rsync_module;

int rsync_logfd = -1;
pool *rsync_pool = NULL;

/* This looks weird, I know.  Its purpose is to be a placeholder pointer;
 * when we call sftp_channel_register_exec_handler(), we give that function
 * a pointer to this stub.  The mod_sftp module then writes in the address
 * of the mod_sftp function to use for writing data.  That function is not
 * exposed in the public mod_sftp.h header file, hence this function pointer
 * passing fun.
 */
int (*rsync_write_data)(pool *, uint32_t, char *, uint32_t);

static int rsync_engine = FALSE;

static const char *trace_channel = "rsync";

static int rsync_set_params(pool *p, uint32_t channel_id, array_header *req) {
  struct rsync_session *sess;

  if (rsync_session_open(channel_id) < 0) {
    int xerrno = errno;

    pr_trace_msg(trace_channel, 3,
      "error opening rsync session for channel ID %lu: %s",
      (unsigned long) channel_id, strerror(xerrno));

    errno = xerrno;
    return -1;
  }

  sess = rsync_session_get(channel_id);

  if (rsync_options_handle(p, req, sess) < 0) {
    int xerrno = errno;

    (void) rsync_session_close(channel_id);

    errno = xerrno;
    return -1;
  }

  return 0;
}

static int rsync_init_channel(uint32_t channel_id) {
  struct rsync_session *sess;

  /* An rsync session should have been opened in the 'set_params' callback.
   * All we need to do is make sure that a session exists for this channel ID.
   */

  sess = rsync_session_get(channel_id);
  if (sess == NULL) {
    return -1;
  }

  return 0;
}

static int rsync_free_channel(uint32_t channel_id) {
  return rsync_session_close(channel_id);
}

static int rsync_handle_data_recv(pool *p, struct rsync_session *sess,
    char *data, uint32_t datalen) {
  struct rsync_options *opts;
  char *buf, *ptr;
  uint32_t buflen, bufsz;

  opts = sess->options;

  errno = ENOSYS;
  return -1;
}

static int rsync_handle_data_send(pool *p, struct rsync_session *sess,
    char *data, uint32_t datalen) {
  struct rsync_options *opts;
  char *buf, *ptr;
  uint32_t buflen, bufsz;

  opts = sess->options;

  /* XXX Move this function a list.c file, rsync_list_send(), rsync_list_recv(),
   * and their friends.
   */

  bufsz = buflen = sizeof(char);

  /* A byte is used to indicate end-of-list. */
  rsync_msg_write_byte(&buf, &buflen, 0);

  if (rsync_write_data(sess->pool, sess->channel_id, ptr, (bufsz - buflen)) < 0) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "error sending file list EOL marker: %s", strerror(errno));
    return -1;
  } 

  /* Write the ID/name list. */

  if (sess->protocol_version < 30) {
    /* Write the IO error flag. */
    rsync_msg_write_int(&buf, &buflen, 0);
  }
 
  return 0;
}

static int rsync_handle_packet(pool *p, void *ssh2, uint32_t channel_id,
    char *data, uint32_t datalen) {
  struct rsync_session *sess;
  struct rsync_options *opts;

  sess = rsync_session_get(channel_id);
  if (sess == NULL) {
    pr_trace_msg(trace_channel, 9,
      "no open rsync session found for channel ID %lu",
      (unsigned long) channel_id);
    return -1;
  }

(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "handling packet");

  /* The rsync protocol is more like scp than it is like sftp or ftp.
   * It is a series of states, rather than a series of interactive commands
   * and responses sent back and forth.
   *
   * This means that this handle_packet() callback will probably not be
   * invoked very often; when it does, the data will be intended for processing
   * far down the call stack, depending on the state machine.
   */

  if (!(sess->state & RSYNC_SESS_FL_PROTO_VERSION)) {
pr_trace_msg(trace_channel, 17, "handling protocol version");
    if (rsync_version_handle(p, sess, &data, &datalen) < 0) {
      return -1;
    }

    sess->state |= RSYNC_SESS_FL_PROTO_VERSION;
  }

  /* If we're the server, we need to send the checksum seed.  If we're the
   * receiver, we need to read the checksum seed.
   */
  if (!(sess->state & RSYNC_SESS_FL_CHECKSUM_SEED)) {
pr_trace_msg(trace_channel, 17, "handling checksum seed");
    if (rsync_checksum_handle(p, sess, &data, &datalen) < 0) {
      return -1;
    }

    sess->state |= RSYNC_SESS_FL_CHECKSUM_SEED;
  }

  /* Have we consumed all of the data? */
  if (datalen == 0) {
    return 0;
  }

  if (!(sess->state & RSYNC_SESS_FL_FILTERS)) {
pr_trace_msg(trace_channel, 17, "handling filters");
    if (rsync_filters_handle(p, sess, &data, &datalen) < 0) {
      return -1;
    }

    sess->state |= RSYNC_SESS_FL_FILTERS;
  }

  opts = sess->options;

  if (opts->sender) {
    if (!(sess->state & RSYNC_SESS_FL_SENT_MANIFEST)) {
      if (rsync_manifest_handle(p, sess, &data, &datalen) < 0) {
        return -1;
      }

      sess->state |= RSYNC_SESS_FL_SENT_MANIFEST;
    }

  } else {
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "we're the receiver; need to receive file data now");
    if (!(sess->state & RSYNC_SESS_FL_RECVD_DATA)) {
      if (rsync_handle_data_recv(p, sess, data, datalen) < 0) {
        return -1;
      }
    }
  }

  /* XXX How do we know when to terminate the connection? */

(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "received %lu bytes of data", (unsigned long) datalen);
  return -1;
}

/* Configuration handlers
 */

/* usage: RsyncEngine on|off */
MODRET set_rsyncengine(cmd_rec *cmd) {
  int bool;
  config_rec *c;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  bool = get_boolean(cmd, 1);
  if (bool == -1)
    CONF_ERROR(cmd, "expected Boolean parameter");

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[0]) = bool;

  return PR_HANDLED(cmd);
}

/* usage: RsyncLog path|"none" */
MODRET set_rsynclog(cmd_rec *cmd) {
  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* Event handlers
 */

static void rsync_exit_ev(const void *event_data, void *user_data) {
}

#if defined(PR_SHARED_MODULE)
static void rsync_mod_unload_ev(const void *event_data, void *user_data) {
  if (strcmp("mod_rsync.c", (const char *) event_data) == 0) {
    /* Unregister ourselves from all events. */
    pr_event_unregister(&rsync_module, NULL, NULL);
  }
}
#endif /* !PR_SHARED_MODULE */

static void rsync_restart_ev(const void *event_data, void *user_data) {
}

static void rsync_shutdown_ev(const void *event_data, void *user_data) {
}

/* Initialization functions
 */

static int rsync_init(void) {

  /* XXX initialize librsync? */

  pr_event_register(&rsync_module, "core.exit", rsync_shutdown_ev, NULL);
#if defined(PR_SHARED_MODULE)
  pr_event_register(&rsync_module, "core.module-unload", rsync_mod_unload_ev,
    NULL);
#endif
  pr_event_register(&rsync_module, "core.restart", rsync_restart_ev, NULL);

  return 0;
}

static int rsync_sess_init(void) {
  config_rec *c;

  pr_event_unregister(&rsync_module, "core.exit", rsync_shutdown_ev);

  c = find_config(main_server->conf, CONF_PARAM, "RsyncEngine", FALSE);
  if (c) {
    rsync_engine = *((int *) c->argv[0]);
  }

  if (!rsync_engine)
    return 0;

  c = find_config(main_server->conf, CONF_PARAM, "RsyncLog", FALSE);
  if (c) {
    const char *log_path = c->argv[0];

    if (strcasecmp(log_path, "none") != 0) {
      int res;

      pr_signals_block();
      PRIVS_ROOT
      res = pr_log_openfile(log_path, &rsync_logfd, 0600);
      PRIVS_RELINQUISH
      pr_signals_unblock();

      if (res < 0) {
        if (res == -1) {
          pr_log_pri(PR_LOG_NOTICE, MOD_RSYNC_VERSION
            ": notice: unable to open RsyncLog '%s': %s", log_path,
            strerror(errno));

        } else if (res == PR_LOG_WRITABLE_DIR) {
          pr_log_pri(PR_LOG_NOTICE, MOD_RSYNC_VERSION
            ": notice: unable to open RsyncLog '%s': parent directory is "
            "world-writable", log_path);

        } else if (res == PR_LOG_SYMLINK) {
          pr_log_pri(PR_LOG_NOTICE, MOD_RSYNC_VERSION
            ": notice: unable to open RsyncLog '%s': cannot log to a symlink",
            log_path);
        }
      }
    }
  }

  pr_event_register(&rsync_module, "core.exit", rsync_exit_ev, NULL);

  rsync_pool = make_sub_pool(session.pool);
  pr_pool_tag(rsync_pool, MOD_RSYNC_VERSION);

  /* Note: The registered 'command' here, "rsync", is meant to be a literal
   * match for the path/command that the SSH client requests in its exec
   * command.
   *
   * This assumption can fail if e.g. the client specifies a fully qualified
   * path, e.g. "/path/to/rsync".  Perhaps there should be a configuration
   * directive for specifying the additional locations/names that should
   * be registered with mod_sftp?
   */

  if (sftp_channel_register_exec_handler(&rsync_module, "rsync",
    rsync_set_params,
    rsync_init_channel,
    rsync_handle_packet,
    rsync_free_channel,
    &rsync_write_data) < 0) {

    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
      "error registering 'rsync' SSH exec handler: %s", strerror(errno));  
    return -1;
  }

  return 0;
}

/* Module API tables
 */

static conftable rsync_conftab[] = {
  { "RsyncEngine",		set_rsyncengine,		NULL },
  { "RsyncLog",			set_rsynclog,			NULL },
  { NULL }
};

module rsync_module = {
  NULL, NULL,

  /* Module API version 2.0 */
  0x20,

  /* Module name */
  "rsync",

  /* Module configuration handler table */
  rsync_conftab,

  /* Module command handler table */
  NULL,

  /* Module authentication handler table */
  NULL,

  /* Module initialization function */
  rsync_init,

  /* Session initialization function */
  rsync_sess_init,

  /* Module version */
  MOD_RSYNC_VERSION
};
