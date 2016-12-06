/*
 * ProFTPD - mod_rsync testsuite
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

/* Session API tests. */

#include "tests.h"
#include "session.h"

static pool *p = NULL;

static void set_up(void) {
  if (p == NULL) {
    p = rsync_pool = make_sub_pool(NULL);
  }
}

static void tear_down(void) {
  if (p) {
    destroy_pool(p);
    p = rsync_pool = NULL;
  }
}

START_TEST (session_get_test) {
  struct rsync_session *sess;

  mark_point();
  sess = rsync_session_get(0);
  fail_unless(sess == NULL, "Failed to handle no sessions");
  fail_unless(errno == ENOENT, "Expected ENOENT (%d), got %s (%d)", ENOENT,
    strerror(errno), errno);
}
END_TEST

START_TEST (session_close_test) {
  int res;
  uint32_t channel_id;

  channel_id = 1;

  mark_point();
  res = rsync_session_close(channel_id);
  fail_unless(res < 0, "Failed to handle nonexistent session");
  fail_unless(errno == ENOENT, "Expected ENOENT (%d), got %s (%d)", ENOENT,
    strerror(errno), errno);
}
END_TEST

START_TEST (session_open_test) {
  int res;
  struct rsync_session *sess;
  uint32_t channel_id;

  channel_id = 0;

  mark_point();
  res = rsync_session_open(channel_id);
  fail_unless(res == 0, "Failed to open session: %s", strerror(errno));

  mark_point();
  sess = rsync_session_get(channel_id);
  fail_unless(sess != NULL, "Failed to get session: %s", strerror(errno));

  mark_point();
  res = rsync_session_open(channel_id);
  fail_unless(res < 0, "Failed to handle duplicate channel ID");
  fail_unless(errno == EEXIST, "Expected EEXIST (%d), got %s (%d)", EEXIST,
    strerror(errno), errno);

  mark_point();
  res = rsync_session_close(channel_id);
  fail_unless(res == 0, "Failed to close channel: %s", strerror(errno));

  mark_point();
  sess = rsync_session_get(channel_id);
  fail_unless(sess == NULL, "Failed to handle nonexistent/closed session");
  fail_unless(errno == ENOENT, "Expected ENEONT (%d), got %s (%d)", ENOENT,
    strerror(errno), errno);
}
END_TEST

Suite *tests_get_session_suite(void) {
  Suite *suite;
  TCase *testcase;

  suite = suite_create("session");
  testcase = tcase_create("base");

  tcase_add_checked_fixture(testcase, set_up, tear_down);

  tcase_add_test(testcase, session_get_test);
  tcase_add_test(testcase, session_close_test);
  tcase_add_test(testcase, session_open_test);

  suite_add_tcase(suite, testcase);
  return suite;
}
