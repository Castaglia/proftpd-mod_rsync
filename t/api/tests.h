/*
 * ProFTPD - mod_rsync API testsuite
 * Copyright (c) 2016 TJ Saunders <tj@castaglia.org>
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 * As a special exemption, TJ Saunders and other respective copyright holders
 * give permission to link this program with OpenSSL, and distribute the
 * resulting executable, without including the source code for OpenSSL in the
 * source distribution.
 */

/* Testsuite management */

#ifndef MOD_RSYNC_TESTS_H
#define MOD_RSYNC_TESTS_H

#include "mod_rsync.h"

#ifdef HAVE_CHECK_H
# include <check.h>
#else
# error "Missing Check installation; necessary for ProFTPD testsuite"
#endif

int tests_rmpath(pool *p, const char *path);
int tests_write_data(pool *p, uint32_t channel_id, unsigned char *buf,
  uint32_t buflen);
int tests_stubs_set_next_cmd(cmd_rec *cmd);

Suite *tests_get_msg_suite(void);
Suite *tests_get_checksum_suite(void);

unsigned int recvd_signal_flags;
extern pid_t mpid;
extern server_rec *main_server;

#endif /* MOD_RSYNC_TESTS_H */
