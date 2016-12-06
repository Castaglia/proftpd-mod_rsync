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

/* Message API tests. */

#include "tests.h"
#include "msg.h"

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

START_TEST (msg_read_write_byte_test) {
  char b, expected;
  unsigned char *buf, *ptr;
  uint32_t buflen, bufsz, len;

  bufsz = buflen = 1024;
  ptr = buf = palloc(p, bufsz);
  memset(ptr, 'a', bufsz);

  /* Basic read_byte tests. */
  mark_point();
  b = rsync_msg_read_byte(NULL, NULL, NULL);
  fail_unless(b == 0, "Failed to handle null buf");

  mark_point();
  b = rsync_msg_read_byte(p, &buf, NULL);
  fail_unless(b == 0, "Failed to handle null buflen");

  buflen = 0;
  mark_point();
  b = rsync_msg_read_byte(p, &buf, &buflen);
  fail_unless(b == 0, "Failed to handle zero buflen");
  buflen = bufsz;

  mark_point();
  b = rsync_msg_read_byte(p, &buf, &buflen);
  expected = 'a';
  fail_unless(b == expected, "Expected '%c', got '%c'", expected, b);
  fail_unless(buflen == bufsz-1, "Expected %lu, got %lu",
    (unsigned long) (bufsz-1), (unsigned long) buflen);

  /* Basic write_byte tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_byte(NULL, NULL, 'b');
  fail_unless(len == 0, "Failed to handle null buf");

  mark_point();
  len = rsync_msg_write_byte(&buf, NULL, 'b');
  fail_unless(len == 0, "Failed to handle null buflen");

  mark_point();
  buflen = 0;
  len = rsync_msg_write_byte(&buf, &buflen, 'b');
  fail_unless(len == 0, "Failed to handle zero buflen");
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_byte(&buf, &buflen, 'b');
  fail_unless(len == 1, "Expected 1, got %lu", (unsigned long) len);
  fail_unless(buflen == bufsz-1, "Expected %lu, got %lu",
    (unsigned long) (bufsz-1), (unsigned long) buflen);
  expected = 'b';
  fail_unless(*ptr == expected, "Expected '%c', got '%c'", expected, *ptr);

  /* Read what I wrote tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  rsync_msg_write_byte(&buf, &buflen, 'F');
  rsync_msg_write_byte(&buf, &buflen, 'o');
  rsync_msg_write_byte(&buf, &buflen, 'o');
  rsync_msg_write_byte(&buf, &buflen, ' ');
  rsync_msg_write_byte(&buf, &buflen, 'B');
  rsync_msg_write_byte(&buf, &buflen, 'a');
  rsync_msg_write_byte(&buf, &buflen, 'r');

  buf = ptr;
  buflen = bufsz;

  mark_point();
  b = rsync_msg_read_byte(p, &buf, &buflen);
  expected = 'F';
  fail_unless(b == expected, "Expected '%c', got '%c'", expected, b);

  b = rsync_msg_read_byte(p, &buf, &buflen);
  expected = 'o';
  fail_unless(b == expected, "Expected '%c', got '%c'", expected, b);

  b = rsync_msg_read_byte(p, &buf, &buflen);
  expected = 'o';
  fail_unless(b == expected, "Expected '%c', got '%c'", expected, b);

  b = rsync_msg_read_byte(p, &buf, &buflen);
  expected = ' ';
  fail_unless(b == expected, "Expected '%c', got '%c'", expected, b);

  b = rsync_msg_read_byte(p, &buf, &buflen);
  expected = 'B';
  fail_unless(b == expected, "Expected '%c', got '%c'", expected, b);

  b = rsync_msg_read_byte(p, &buf, &buflen);
  expected = 'a';
  fail_unless(b == expected, "Expected '%c', got '%c'", expected, b);

  b = rsync_msg_read_byte(p, &buf, &buflen);
  expected = 'r';
  fail_unless(b == expected, "Expected '%c', got '%c'", expected, b);
}
END_TEST

START_TEST (msg_read_write_short_test) {
  int16_t n, expected;
  unsigned char *buf, *ptr;
  uint32_t buflen, bufsz, len;

  bufsz = buflen = 1024;
  ptr = buf = palloc(p, bufsz);
  memset(ptr, 'a', bufsz);

  /* Basic read_short tests. */
  mark_point();
  n = rsync_msg_read_short(NULL, NULL, NULL);
  fail_unless(n == 0, "Failed to handle null buf");

  mark_point();
  n = rsync_msg_read_short(p, &buf, NULL);
  fail_unless(n == 0, "Failed to handle null buflen");

  buflen = 0;
  mark_point();
  n = rsync_msg_read_short(p, &buf, &buflen);
  fail_unless(n == 0, "Failed to handle zero buflen");
  buflen = bufsz;

  mark_point();
  n = rsync_msg_read_short(p, &buf, &buflen);
  expected = 1633771873;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);
  fail_unless(buflen == bufsz-2, "Expected %lu, got %lu",
    (unsigned long) (bufsz-2), (unsigned long) buflen);

  /* Basic write_short tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_short(NULL, NULL, 32);
  fail_unless(len == 0, "Failed to handle null buf");

  mark_point();
  len = rsync_msg_write_short(&buf, NULL, 32);
  fail_unless(len == 0, "Failed to handle null buflen");

  mark_point();
  buflen = 1;
  len = rsync_msg_write_short(&buf, &buflen, 32);
  fail_unless(len == 0, "Failed to handle short buflen");
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_short(&buf, &buflen, 32);
  fail_unless(len == 2, "Expected 2, got %lu", (unsigned long) len);
  fail_unless(buflen == bufsz-2, "Expected %lu, got %lu",
    (unsigned long) (bufsz-2), (unsigned long) buflen);

  /* Read what I wrote tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  rsync_msg_write_short(&buf, &buflen, 24);
  rsync_msg_write_short(&buf, &buflen, 76);
  rsync_msg_write_short(&buf, &buflen, 1);
  rsync_msg_write_short(&buf, &buflen, -42);
  rsync_msg_write_short(&buf, &buflen, 48);

  buf = ptr;
  buflen = bufsz;

  mark_point();
  n = rsync_msg_read_short(p, &buf, &buflen);
  expected = 24;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);

  n = rsync_msg_read_short(p, &buf, &buflen);
  expected = 76;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);

  n = rsync_msg_read_short(p, &buf, &buflen);
  expected = 1;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);

  n = rsync_msg_read_short(p, &buf, &buflen);
  expected = -42;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);

  n = rsync_msg_read_short(p, &buf, &buflen);
  expected = 48;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);
}
END_TEST

START_TEST (msg_read_write_int_test) {
  int32_t n, expected;
  unsigned char *buf, *ptr;
  uint32_t buflen, bufsz, len;

  bufsz = buflen = 1024;
  ptr = buf = palloc(p, bufsz);
  memset(ptr, 'a', bufsz);

  /* Basic read_int tests. */
  mark_point();
  n = rsync_msg_read_int(NULL, NULL, NULL);
  fail_unless(n == 0, "Failed to handle null buf");

  mark_point();
  n = rsync_msg_read_int(p, &buf, NULL);
  fail_unless(n == 0, "Failed to handle null buflen");

  buflen = 0;
  mark_point();
  n = rsync_msg_read_int(p, &buf, &buflen);
  fail_unless(n == 0, "Failed to handle zero buflen");
  buflen = bufsz;

  mark_point();
  n = rsync_msg_read_int(p, &buf, &buflen);
  expected = 1633771873;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);
  fail_unless(buflen == bufsz-4, "Expected %lu, got %lu",
    (unsigned long) (bufsz-4), (unsigned long) buflen);

  /* Basic write_int tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_int(NULL, NULL, 32);
  fail_unless(len == 0, "Failed to handle null buf");

  mark_point();
  len = rsync_msg_write_int(&buf, NULL, 32);
  fail_unless(len == 0, "Failed to handle null buflen");

  mark_point();
  buflen = 2;
  len = rsync_msg_write_int(&buf, &buflen, 32);
  fail_unless(len == 0, "Failed to handle short buflen");
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_int(&buf, &buflen, 32);
  fail_unless(len == 4, "Expected 4, got %lu", (unsigned long) len);
  fail_unless(buflen == bufsz-4, "Expected %lu, got %lu",
    (unsigned long) (bufsz-4), (unsigned long) buflen);

  /* Read what I wrote tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  rsync_msg_write_int(&buf, &buflen, 24);
  rsync_msg_write_int(&buf, &buflen, 76);
  rsync_msg_write_int(&buf, &buflen, 1);
  rsync_msg_write_int(&buf, &buflen, -42);
  rsync_msg_write_int(&buf, &buflen, 48);

  buf = ptr;
  buflen = bufsz;

  mark_point();
  n = rsync_msg_read_int(p, &buf, &buflen);
  expected = 24;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);

  n = rsync_msg_read_int(p, &buf, &buflen);
  expected = 76;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);

  n = rsync_msg_read_int(p, &buf, &buflen);
  expected = 1;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);

  n = rsync_msg_read_int(p, &buf, &buflen);
  expected = -42;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);

  n = rsync_msg_read_int(p, &buf, &buflen);
  expected = 48;
  fail_unless(n == expected, "Expected %d, got %d", (int) expected, (int) n);
}
END_TEST

START_TEST (msg_read_write_long_test) {
  int64_t l, expected;
  unsigned char *buf, *ptr;
  uint32_t buflen, bufsz, len;

  bufsz = buflen = 1024;
  ptr = buf = palloc(p, bufsz);
  memset(ptr, 'a', bufsz);

  /* Basic read_long tests. */
  mark_point();
  l = rsync_msg_read_long(NULL, NULL, NULL);
  fail_unless(l == 0, "Failed to handle null buf");

  mark_point();
  l = rsync_msg_read_long(p, &buf, NULL);
  fail_unless(l == 0, "Failed to handle null buflen");

  buflen = 0;
  mark_point();
  l = rsync_msg_read_long(p, &buf, &buflen);
  fail_unless(l == 0, "Failed to handle zero buflen");
  buflen = bufsz;

  mark_point();
  l = rsync_msg_read_long(p, &buf, &buflen);
  expected = 7016996765293437281;
  fail_unless(l == expected, "Expected %ld, got %ld", (long) expected,
    (long) l);
  fail_unless(buflen == bufsz-8, "Expected %lu, got %lu",
    (unsigned long) (bufsz-8), (unsigned long) buflen);

  /* Basic write_long tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_long(NULL, NULL, 32);
  fail_unless(len == 0, "Failed to handle null buf");

  mark_point();
  len = rsync_msg_write_long(&buf, NULL, 32);
  fail_unless(len == 0, "Failed to handle null buflen");

  mark_point();
  buflen = 4;
  len = rsync_msg_write_long(&buf, &buflen, 32);
  fail_unless(len == 0, "Failed to handle short buflen");
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_long(&buf, &buflen, 32);
  fail_unless(len == 8, "Expected 8, got %lu", (unsigned long) len);
  fail_unless(buflen == bufsz-8, "Expected %lu, got %lu",
    (unsigned long) (bufsz-8), (unsigned long) buflen);

  /* Read what I wrote tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  rsync_msg_write_long(&buf, &buflen, 24);
  rsync_msg_write_long(&buf, &buflen, 76);
  rsync_msg_write_long(&buf, &buflen, 1);
  rsync_msg_write_long(&buf, &buflen, -42);
  rsync_msg_write_long(&buf, &buflen, 48);

  buf = ptr;
  buflen = bufsz;

  mark_point();
  l = rsync_msg_read_long(p, &buf, &buflen);
  expected = 24;
  fail_unless(l == expected, "Expected %ld, got %ld", (long) expected,
    (long) l);

  l = rsync_msg_read_long(p, &buf, &buflen);
  expected = 76;
  fail_unless(l == expected, "Expected %ld, got %ld", (long) expected,
    (long) l);

  l = rsync_msg_read_long(p, &buf, &buflen);
  expected = 1;
  fail_unless(l == expected, "Expected %ld, got %ld", (long) expected,
    (long) l);

  l = rsync_msg_read_long(p, &buf, &buflen);
  expected = -42;
  fail_unless(l == expected, "Expected %ld, got %ld", (long) expected,
    (long) l);

  l = rsync_msg_read_long(p, &buf, &buflen);
  expected = 48;
  fail_unless(l == expected, "Expected %ld, got %ld", (long) expected,
    (long) l);
}
END_TEST

START_TEST (msg_read_write_data_test) {
  unsigned char *buf, *ptr, *data, *expected;
  uint32_t buflen, bufsz, len;

  bufsz = buflen = 1024;
  ptr = buf = palloc(p, bufsz);
  memset(ptr, 'a', bufsz);

  /* Basic read_data tests. */
  mark_point();
  data = rsync_msg_read_data(NULL, NULL, NULL, 0);
  fail_unless(data == NULL, "Failed to handle null pool");

  mark_point();
  data = rsync_msg_read_data(p, NULL, NULL, 0);
  fail_unless(data == NULL, "Failed to handle null buf");

  mark_point();
  data = rsync_msg_read_data(p, &buf, NULL, 0);
  fail_unless(data == NULL, "Failed to handle null buflen");

  mark_point();
  buflen = 0;
  data = rsync_msg_read_data(p, &buf, &buflen, 1);
  fail_unless(data == NULL, "Failed to handle short buflen");
  buflen = bufsz;

  mark_point();
  data = rsync_msg_read_data(p, &buf, &buflen, 0);
  fail_unless(data == NULL, "Failed to handle zero datalen");
 
  mark_point();
  data = rsync_msg_read_data(p, &buf, &buflen, 3);
  fail_unless(data != NULL, "Failed to read 3 bytes of data");
  fail_unless(buflen == bufsz-3, "Expected %lu, got %lu",
    (unsigned long) (bufsz-3), (unsigned long) buflen);

  /* Basic write_data tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_data(NULL, NULL, NULL, 0);
  fail_unless(len == 0, "Failed to handle null buf");

  mark_point();
  len = rsync_msg_write_data(&buf, NULL, NULL, 0);
  fail_unless(len == 0, "Failed to handle null buflen");

  mark_point();
  len = rsync_msg_write_data(&buf, &buflen, NULL, 0);
  fail_unless(len == 0, "Failed to handle null data");

  mark_point();
  len = rsync_msg_write_data(&buf, &buflen, (unsigned char *) "foo bar", 0);
  fail_unless(len == 0, "Failed to handle zero datalen");

  mark_point();
  buflen = 3;
  len = rsync_msg_write_data(&buf, &buflen, (unsigned char *) "foo bar", 7);
  fail_unless(len == 0, "Failed to handle short buflen");
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_data(&buf, &buflen, (unsigned char *) "foo bar", 7);
  fail_unless(len == 7, "Failed to handle write data");

  /* Read what I wrote tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  rsync_msg_write_data(&buf, &buflen, (unsigned char *) "foo", 3);
  rsync_msg_write_data(&buf, &buflen, (unsigned char *) "bar", 3);
  rsync_msg_write_data(&buf, &buflen, (unsigned char *) "bazz", 4);

  buf = ptr;
  buflen = bufsz;

  mark_point();
  data = rsync_msg_read_data(p, &buf, &buflen, 10);
  fail_unless(data != NULL, "Failed to read data: %s", strerror(errno));
  expected = (unsigned char *) "foobarbazz";
  fail_unless(strcmp((char *) data, (char *) expected) == 0,
    "Expected '%s', got '%s'", expected, data);
}
END_TEST

START_TEST (msg_read_write_string_test) {
  char *s, *expected;
  unsigned char *buf, *ptr;
  uint32_t buflen, bufsz, len;

  bufsz = buflen = 1024;
  ptr = buf = palloc(p, bufsz);
  memset(ptr, 'a', bufsz);

  /* Basic read_string tests. */
  mark_point();
  s = rsync_msg_read_string(NULL, NULL, NULL, 0);
  fail_unless(s == NULL, "Failed to handle null pool");

  mark_point();
  s = rsync_msg_read_string(p, NULL, NULL, 0);
  fail_unless(s == NULL, "Failed to handle null buf");

  mark_point();
  s = rsync_msg_read_string(p, &buf, NULL, 0);
  fail_unless(s == NULL, "Failed to handle null buflen");

  mark_point();
  buflen = 0;
  s = rsync_msg_read_string(p, &buf, &buflen, 1);
  fail_unless(s == NULL, "Failed to handle short buflen");
  buflen = bufsz;

  mark_point();
  s = rsync_msg_read_string(p, &buf, &buflen, 0);
  fail_unless(s == NULL, "Failed to handle zero datalen");
 
  mark_point();
  s = rsync_msg_read_string(p, &buf, &buflen, 3);
  fail_unless(s != NULL, "Failed to read 3 bytes of data");
  fail_unless(buflen == bufsz-3, "Expected %lu, got %lu",
    (unsigned long) (bufsz-3), (unsigned long) buflen);

  /* Basic write_string tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_string(NULL, NULL, NULL);
  fail_unless(len == 0, "Failed to handle null buf");

  mark_point();
  len = rsync_msg_write_string(&buf, NULL, NULL);
  fail_unless(len == 0, "Failed to handle null buflen");

  mark_point();
  len = rsync_msg_write_string(&buf, &buflen, NULL);
  fail_unless(len == 0, "Failed to handle null data");

  mark_point();
  buflen = 3;
  len = rsync_msg_write_string(&buf, &buflen, "foo/bar");
  fail_unless(len == 0, "Failed to handle short buflen");
  buflen = bufsz;

  mark_point();
  len = rsync_msg_write_string(&buf, &buflen, "foo/bar");
  fail_unless(len == 7, "Failed to handle write data");

  /* Read what I wrote tests. */
  buf = ptr;
  buflen = bufsz;

  mark_point();
  rsync_msg_write_string(&buf, &buflen, "Quxx");
  rsync_msg_write_string(&buf, &buflen, "Foo");
  rsync_msg_write_string(&buf, &buflen, "Bar");
  rsync_msg_write_string(&buf, &buflen, "Bazz");

  buf = ptr;
  buflen = bufsz;

  mark_point();
  s = rsync_msg_read_string(p, &buf, &buflen, 4);
  fail_unless(s != NULL, "Failed to read string: %s", strerror(errno));
  expected = "Quxx";
  fail_unless(strcmp(s, expected) == 0, "Expected '%s', got '%s'", expected, s);

  mark_point();
  s = rsync_msg_read_string(p, &buf, &buflen, 3);
  fail_unless(s != NULL, "Failed to read string: %s", strerror(errno));
  expected = "Foo";
  fail_unless(strcmp(s, expected) == 0, "Expected '%s', got '%s'", expected, s);

  mark_point();
  s = rsync_msg_read_string(p, &buf, &buflen, 3);
  fail_unless(s != NULL, "Failed to read string: %s", strerror(errno));
  expected = "Bar";
  fail_unless(strcmp(s, expected) == 0, "Expected '%s', got '%s'", expected, s);

  mark_point();
  s = rsync_msg_read_string(p, &buf, &buflen, 4);
  fail_unless(s != NULL, "Failed to read string: %s", strerror(errno));
  expected = "Bazz";
  fail_unless(strcmp(s, expected) == 0, "Expected '%s', got '%s'", expected, s);
}
END_TEST

Suite *tests_get_msg_suite(void) {
  Suite *suite;
  TCase *testcase;

  suite = suite_create("msg");
  testcase = tcase_create("base");

  tcase_add_checked_fixture(testcase, set_up, tear_down);

  tcase_add_test(testcase, msg_read_write_byte_test);
  tcase_add_test(testcase, msg_read_write_short_test);
  tcase_add_test(testcase, msg_read_write_int_test);
  tcase_add_test(testcase, msg_read_write_long_test);
  tcase_add_test(testcase, msg_read_write_data_test);
  tcase_add_test(testcase, msg_read_write_string_test);

  suite_add_tcase(suite, testcase);
  return suite;
}
