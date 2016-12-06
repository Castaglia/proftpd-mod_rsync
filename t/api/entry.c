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

/* Entry API tests. */

#include "tests.h"
#include "entry.h"

static pool *p = NULL;

static void set_up(void) {
  if (p == NULL) {
    p = make_sub_pool(NULL);
  }
}

static void tear_down(void) {
  if (p) {
    destroy_pool(p);
    p = NULL;
  }
}

START_TEST (entry_create_test) {
  struct rsync_entry *ent;
  const char *path;

  mark_point();
  ent = rsync_entry_create(NULL, NULL, 0);
  fail_unless(ent == NULL, "Failed to handle null pool");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  mark_point();
  ent = rsync_entry_create(p, NULL, 0);
  fail_unless(ent == NULL, "Failed to handle null path");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  path = "/foo/bar";

  mark_point();
  ent = rsync_entry_create(p, path, 0);
  fail_unless(ent == NULL, "Failed to handle nonexistent path");
  fail_unless(errno == ENOENT, "Expected ENOENT (%d), got %s (%d)", ENOENT,
    strerror(errno), errno);

  /* XXX Need test for path whose length exceeds RSYNC_LONG_NAME_LEN; check for
   * ent->flags & RSYNC_ENTRY_CODEC_FL_LONG_NAME when done.
   */

  path = "/";

  mark_point();
  ent = rsync_entry_create(p, path, 0);
  fail_unless(ent != NULL, "Failed to create entry for path '%s': %s", path,
    strerror(errno));
  fail_unless(ent->flags & RSYNC_ENTRY_CODEC_FL_TOP_DIR,
    "Expected TOP_DIR flag");
}
END_TEST

START_TEST (entry_codec_test) {
  /* encode, decode */
}
END_TEST

Suite *tests_get_entry_suite(void) {
  Suite *suite;
  TCase *testcase;

  suite = suite_create("entry");
  testcase = tcase_create("base");

  tcase_add_checked_fixture(testcase, set_up, tear_down);

  tcase_add_test(testcase, entry_create_test);
  tcase_add_test(testcase, entry_codec_test);

  suite_add_tcase(suite, testcase);
  return suite;
}
