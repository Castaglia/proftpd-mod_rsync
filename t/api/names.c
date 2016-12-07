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

/* Names API tests. */

#include "tests.h"
#include "names.h"

static pool *p = NULL;

static void set_up(void) {
  if (p == NULL) {
    p = make_sub_pool(NULL);
  }
}

static void tear_down(void) {
  rsync_names_destroy();

  if (p) {
    destroy_pool(p);
    p = NULL;
  }
}

START_TEST (names_alloc_test) {
  int res;

  mark_point();
  res = rsync_names_destroy();
  fail_unless(res == 0, "Failed to destroy names: %s", strerror(errno));

  mark_point();
  res = rsync_names_alloc(NULL);
  fail_unless(res < 0, "Failed to handle null pool");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  mark_point();
  res = rsync_names_alloc(p);
  fail_unless(res == 0, "Failed to allocate names: %s", strerror(errno));

  mark_point();
  res = rsync_names_destroy();
  fail_unless(res == 0, "Failed to destroy names: %s", strerror(errno));
}
END_TEST

START_TEST (names_add_uid_test) {
  const char *res;
  uid_t uid;

  mark_point();
  res = rsync_names_add_uid(NULL, 0);
  fail_unless(res == NULL, "Failed to handle null pool");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  uid = PR_ROOT_UID;

  mark_point();
  res = rsync_names_add_uid(p, uid);
  fail_unless(res == NULL, "Failed to handle root uid");
  fail_unless(errno == EPERM, "Expected EPERM (%d), got %s (%d)", EPERM,
    strerror(errno), errno);

  (void) rsync_names_alloc(p);

  uid = 1;

  mark_point();
  res = rsync_names_add_uid(p, uid);
  fail_unless(res != NULL, "Failed to handle UID %lu: %s", (unsigned long) uid,
    strerror(errno));

  mark_point();
  res = rsync_names_add_uid(p, uid);
  fail_unless(res == NULL, "Failed to handle duplicate uid");
  fail_unless(errno == EEXIST, "Expected EEXIST (%d), got %s (%d)", EEXIST,
    strerror(errno), errno);

  (void) rsync_names_destroy();
}
END_TEST

START_TEST (names_add_gid_test) {
  const char *res;
  gid_t gid;

  mark_point();
  res = rsync_names_add_gid(NULL, 0);
  fail_unless(res == NULL, "Failed to handle null pool");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  gid = 0;

  mark_point();
  res = rsync_names_add_gid(p, gid);
  fail_unless(res == NULL, "Failed to handle root gid");
  fail_unless(errno == EPERM, "Expected EPERM (%d), got %s (%d)", EPERM,
    strerror(errno), errno);

  (void) rsync_names_alloc(p);

  gid = 1;

  mark_point();
  res = rsync_names_add_gid(p, gid);
  fail_unless(res != NULL, "Failed to handle GID %lu: %s", (unsigned long) gid,
    strerror(errno));

  mark_point();
  res = rsync_names_add_gid(p, gid);
  fail_unless(res == NULL, "Failed to handle duplicate gid");
  fail_unless(errno == EEXIST, "Expected EEXIST (%d), got %s (%d)", EEXIST,
    strerror(errno), errno);

  (void) rsync_names_destroy();
}
END_TEST

START_TEST (names_encode_test) {
}
END_TEST

Suite *tests_get_names_suite(void) {
  Suite *suite;
  TCase *testcase;

  suite = suite_create("names");
  testcase = tcase_create("base");

  tcase_add_checked_fixture(testcase, set_up, tear_down);

  tcase_add_test(testcase, names_alloc_test);
  tcase_add_test(testcase, names_add_uid_test);
  tcase_add_test(testcase, names_add_gid_test);
  tcase_add_test(testcase, names_encode_test);

  suite_add_tcase(suite, testcase);
  return suite;
}
