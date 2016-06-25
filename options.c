/* 
 * ProFTPD - mod_rsync options
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
#include "disconnect.h"

#include <popt.h>

static const char *trace_channel = "rsync";

/* We use the popt option parsing library, rather than getopt(3), because
 * getopt(3) can't handle the variety of options that rsync uses.
 */

struct rsync_options default_options;
static int use_server = 0;

enum {
  OPT_SENDER = 1000,
  OPT_SERVER,
  OPT_EXCLUDE,
  OPT_EXCLUDE_FROM,
  OPT_FILTER,
  OPT_COMPARE_DEST,
  OPT_COPY_DEST,
  OPT_LINK_DEST,
  OPT_INCLUDE,
  OPT_INCLUDE_FROM,
  OPT_MODIFY_WINDOW,
  OPT_MIN_SIZE,
  OPT_CHMOD,
  OPT_MAX_SIZE,
  OPT_NO_D,
  OPT_APPEND,
  OPT_REFUSED_BASE = 9000
};

static struct poptOption long_options[] = {
  /* longName, shortName, argInfo, argPtr, value, descrip, argDesc */

  { "verbose",         'v', POPT_ARG_NONE,
     NULL, 'v', NULL, NULL },

  { "no-verbose",       0,  POPT_ARG_VAL,
     &default_options.verbose, 0, NULL, NULL },

  { "quiet",           'q', POPT_ARG_NONE,
     NULL, 'q', NULL, NULL },

  { "stats",            0,  POPT_ARG_NONE,
    &default_options.show_stats, 0, NULL, NULL },

  { "human-readable",  'h', POPT_ARG_NONE,
     NULL, 'h', NULL, NULL },

  { "no-human-readable", 0,  POPT_ARG_VAL,
    &default_options.show_human_readable, 0, NULL, NULL },

  { "dry-run",         'n', POPT_ARG_NONE,
    &default_options.dry_run, 0, NULL, NULL },

  { "archive",         'a', POPT_ARG_NONE,
    NULL, 'a', NULL, NULL },

  { "recursive",       'r', POPT_ARG_VAL,
    &default_options.recurse, 2, NULL, NULL },

  { "no-recursive",     0,  POPT_ARG_VAL,
    &default_options.recurse, 0, NULL, NULL },

  { "inc-recursive",    0,  POPT_ARG_VAL,
    &default_options.allow_incr_recurse, 1, NULL, NULL },

  { "no-inc-recursive", 0,  POPT_ARG_VAL,
    &default_options.allow_incr_recurse, 0, NULL, NULL },

  { "dirs",            'd', POPT_ARG_VAL,
    &default_options.transfer_dirs, 2, NULL, NULL },

  { "no-dirs",          0,  POPT_ARG_VAL,
    &default_options.transfer_dirs, 0, NULL, NULL },

  { "old-dirs",         0,  POPT_ARG_VAL,
    &default_options.transfer_dirs, 4, NULL, NULL },

  { "old-d",            0,  POPT_ARG_VAL,
    &default_options.transfer_dirs, 4, NULL, NULL },

  { "perms",           'p', POPT_ARG_VAL,
    &default_options.preserve_perms, 1, NULL, NULL },

  { "no-perms",         0,  POPT_ARG_VAL,
    &default_options.preserve_perms, 0, NULL, NULL },

  { "executability",   'E', POPT_ARG_NONE,
    &default_options.preserve_exec, 0, NULL, NULL },

  { "acls",            'A', POPT_ARG_NONE,
     NULL, 'A', NULL, NULL },

  { "no-acls",          0,  POPT_ARG_VAL,
    &default_options.preserve_acls, 0, NULL, NULL },

  { "xattrs",          'X', POPT_ARG_NONE,
    NULL, 'X', NULL, NULL },

  { "no-xattrs",        0,  POPT_ARG_VAL,
    &default_options.preserve_xattrs, 0, NULL, NULL },

  { "times",           't', POPT_ARG_VAL,    &default_options.preserve_times, 2, NULL, NULL },
  { "no-times",         0,  POPT_ARG_VAL,    &default_options.preserve_times, 0, NULL, NULL },
  { "omit-dir-times",  'O', POPT_ARG_VAL,    &default_options.skip_dir_times, 1, NULL, NULL },
  { "no-omit-dir-times",0,  POPT_ARG_VAL,    &default_options.skip_dir_times, 0, NULL, NULL },
  { "modify-window",    0,  POPT_ARG_INT,    &default_options.modify_window, OPT_MODIFY_WINDOW, NULL, NULL },
  { "super",            0,  POPT_ARG_VAL,    &default_options.as_root, 2, NULL, NULL },
  { "no-super",         0,  POPT_ARG_VAL,    &default_options.as_root, 0, NULL, NULL },
  { "fake-super",       0,  POPT_ARG_VAL,    &default_options.as_root, -1, NULL, NULL },
  { "owner",           'o', POPT_ARG_VAL,    &default_options.preserve_uid, 1, NULL, NULL },
  { "no-owner",         0,  POPT_ARG_VAL,    &default_options.preserve_uid, 0, NULL, NULL },
  { "group",           'g', POPT_ARG_VAL,    &default_options.preserve_gid, 'g', NULL, NULL },
  { "no-group",         0,  POPT_ARG_VAL,    &default_options.preserve_gid, 0, NULL, NULL },
  { NULL,              'D', POPT_ARG_NONE,   NULL, 'D', NULL, NULL },
  { "no-D",             0,  POPT_ARG_NONE,   NULL, OPT_NO_D, NULL, NULL },
  { "devices",          0,  POPT_ARG_VAL,    &default_options.preserve_devices, 1, NULL, NULL },
  { "no-devices",       0,  POPT_ARG_VAL,    &default_options.preserve_devices, 0, NULL, NULL },
  { "specials",         0,  POPT_ARG_VAL,    &default_options.preserve_specials, 1, NULL, NULL },
  { "no-specials",      0,  POPT_ARG_VAL,    &default_options.preserve_specials, 0, NULL, NULL },
  { "links",           'l', POPT_ARG_VAL,    &default_options.preserve_links, 1, NULL, NULL },
  { "no-links",         0,  POPT_ARG_VAL,    &default_options.preserve_links, 0, NULL, NULL },
  { "copy-links",      'L', POPT_ARG_NONE,   &default_options.copy_links, 0, NULL, NULL },
  { "copy-unsafe-links",0,  POPT_ARG_NONE,   &default_options.copy_unsafe_links, 0, NULL, NULL },
  { "safe-links",       0,  POPT_ARG_NONE,   &default_options.safe_symlinks, 0, NULL, NULL },
  { "copy-dirlinks",   'k', POPT_ARG_NONE,   &default_options.copy_dirlinks, 0, NULL, NULL },
  { "keep-dirlinks",   'K', POPT_ARG_NONE,   &default_options.keep_dirlinks, 0, NULL, NULL },
  { "hard-links",      'H', POPT_ARG_NONE,   NULL, 'H', NULL, NULL },
  { "no-hard-links",    0,  POPT_ARG_VAL,    &default_options.preserve_hard_links, 0, NULL, NULL },
  { "relative",        'R', POPT_ARG_VAL,    &default_options.use_relative_paths, 1, NULL, NULL },
  { "no-relative",      0,  POPT_ARG_VAL,    &default_options.use_relative_paths, 0, NULL, NULL },
  { "implied-dirs",     0,  POPT_ARG_VAL,    &default_options.implied_dirs, 1, NULL, NULL },
  { "no-implied-dirs",  0,  POPT_ARG_VAL,    &default_options.implied_dirs, 0, NULL, NULL },
  { "chmod",            0,  POPT_ARG_STRING, NULL, OPT_CHMOD, NULL, NULL },

  { "ignore-times",    'I', POPT_ARG_NONE,
    &default_options.ignore_times, 0, NULL, NULL },

  { "size-only",        0,  POPT_ARG_NONE,   &default_options.size_only, 0, NULL, NULL },
  { "one-file-system", 'x', POPT_ARG_NONE,   NULL, 'x', NULL, NULL },
  { "no-one-file-system",'x',POPT_ARG_VAL,   &default_options.single_filesystem, 0, NULL, NULL },
  { "update",          'u', POPT_ARG_NONE,   &default_options.update_only, 0, NULL, NULL },
  { "existing",         0,  POPT_ARG_NONE,   &default_options.skip_nonexisting, 0, NULL, NULL },
  { "ignore-non-existing",0,POPT_ARG_NONE,   &default_options.skip_nonexisting, 0, NULL, NULL },
  { "ignore-existing",  0,  POPT_ARG_NONE,   &default_options.skip_existing, 0, NULL, NULL },
  { "max-size",         0,  POPT_ARG_STRING, &default_options.max_size, OPT_MAX_SIZE, NULL, NULL },
  { "min-size",         0,  POPT_ARG_STRING, &default_options.min_size, OPT_MIN_SIZE, NULL, NULL },
  { "sparse",          'S', POPT_ARG_VAL,    &default_options.sparse_files, 1, NULL, NULL },
  { "no-sparse",        0,  POPT_ARG_VAL,    &default_options.sparse_files, 0, NULL, NULL },
  { "inplace",          0,  POPT_ARG_VAL,    &default_options.inplace, 1, NULL, NULL },
  { "no-inplace",       0,  POPT_ARG_VAL,    &default_options.inplace, 0, NULL, NULL },
  { "append",           0,  POPT_ARG_NONE,   NULL, OPT_APPEND, NULL, NULL },
  { "append-verify",    0,  POPT_ARG_VAL,    &default_options.append_mode, 2, NULL, NULL },
  { "no-append",        0,  POPT_ARG_VAL,    &default_options.append_mode, 0, NULL, NULL },
  { "del",              0,  POPT_ARG_NONE,   &default_options.delete_during, 0, NULL, NULL },
  { "delete",           0,  POPT_ARG_NONE,   &default_options.delete_mode, 0, NULL, NULL },
  { "delete-before",    0,  POPT_ARG_NONE,   &default_options.delete_before, 0, NULL, NULL },
  { "delete-during",    0,  POPT_ARG_VAL,    &default_options.delete_during, 1, NULL, NULL },
  { "delete-delay",     0,  POPT_ARG_VAL,    &default_options.delete_during, 2, NULL, NULL },
  { "delete-after",     0,  POPT_ARG_NONE,   &default_options.delete_after, 0, NULL, NULL },
  { "delete-excluded",  0,  POPT_ARG_NONE,   &default_options.delete_excluded, 0, NULL, NULL },
  { "remove-source-files",0,POPT_ARG_VAL,    &default_options.remove_source_files, 1, NULL, NULL },
  { "force",            0,  POPT_ARG_VAL,    &default_options.delete_forced, 1, NULL, NULL },
  { "no-force",         0,  POPT_ARG_VAL,    &default_options.delete_forced, 0, NULL, NULL },
  { "ignore-errors",    0,  POPT_ARG_VAL,    &default_options.ignore_errors, 1, NULL, NULL },
  { "no-ignore-errors", 0,  POPT_ARG_VAL,    &default_options.ignore_errors, 0, NULL, NULL },
  { "max-delete",       0,  POPT_ARG_INT,    &default_options.delete_max, 0, NULL, NULL },
  { NULL,              'F', POPT_ARG_NONE,   0, 'F', NULL, NULL },

  { "filter",          'f', POPT_ARG_STRING, NULL, OPT_FILTER, NULL, NULL },
  { "exclude",          0,  POPT_ARG_STRING, NULL, OPT_EXCLUDE, NULL, NULL },
  { "exclude-from",     0,  POPT_ARG_STRING, NULL, OPT_EXCLUDE_FROM, NULL, NULL },
  { "include",          0,  POPT_ARG_STRING, NULL, OPT_INCLUDE, NULL, NULL },
  { "include-from",     0,  POPT_ARG_STRING, NULL, OPT_INCLUDE_FROM, NULL, NULL },
  { "cvs-exclude",     'C', POPT_ARG_NONE,   &default_options.exclude_cvs, 0, NULL, NULL },
  { "whole-file",      'W', POPT_ARG_VAL,    &default_options.whole_file, 1, NULL, NULL },
  { "no-whole-file",    0,  POPT_ARG_VAL,    &default_options.whole_file, 0, NULL, NULL },
  { "checksum",        'c', POPT_ARG_VAL,    &default_options.always_checksum, 1, NULL, NULL },
  { "no-checksum",      0,  POPT_ARG_VAL,    &default_options.always_checksum, 0, NULL, NULL },
  { "block-size",      'B', POPT_ARG_LONG,   &default_options.block_size, 0, NULL, NULL },
  { "compare-dest",     0,  POPT_ARG_STRING, NULL, OPT_COMPARE_DEST, NULL, NULL },
  { "copy-dest",        0,  POPT_ARG_STRING, NULL, OPT_COPY_DEST, NULL, NULL },
  { "link-dest",        0,  POPT_ARG_STRING, NULL, OPT_LINK_DEST, NULL, NULL },
  { "fuzzy",           'y', POPT_ARG_VAL,    &default_options.fuzzy_basis, 1, NULL, NULL },
  { "no-fuzzy",         0,  POPT_ARG_VAL,    &default_options.fuzzy_basis, 0, NULL, NULL },
  { "compress",        'z', POPT_ARG_NONE,   NULL, 'z', NULL, NULL },
  { "no-compress",      0,  POPT_ARG_VAL,    &default_options.use_compression, 0, NULL, NULL },
  { "skip-compress",    0,  POPT_ARG_STRING, &default_options.skip_compression, 0, NULL, NULL },
  { "compress-level",   0,  POPT_ARG_INT,    &default_options.compression_level, 'z', NULL, NULL },
  { NULL,              'P', POPT_ARG_NONE,   NULL, 'P', NULL, NULL },
  { "partial",          0,  POPT_ARG_VAL,    &default_options.keep_partial, 1, NULL, NULL },
  { "no-partial",       0,  POPT_ARG_VAL,    &default_options.keep_partial, 0, NULL, NULL },
  { "partial-dir",      0,  POPT_ARG_STRING, &default_options.partial_dir, 0, NULL, NULL },
  { "delay-updates",    0,  POPT_ARG_VAL,    &default_options.delay_updates, 1, NULL, NULL },
  { "no-delay-updates", 0,  POPT_ARG_VAL,    &default_options.delay_updates, 0, NULL, NULL },
  { "prune-empty-dirs",'m', POPT_ARG_VAL,    &default_options.prune_empty_dirs, 1, NULL, NULL },
  { "no-prune-empty-dirs",0,POPT_ARG_VAL,    &default_options.prune_empty_dirs, 0, NULL, NULL },
  { "itemize-changes", 'i', POPT_ARG_NONE,   NULL, 'i', NULL, NULL },
  { "no-itemize-changes",0, POPT_ARG_VAL,    &default_options.itemize_changes, 0, NULL, NULL },
  { "backup",          'b', POPT_ARG_VAL,    &default_options.use_backups, 1, NULL, NULL },
  { "no-backup",        0,  POPT_ARG_VAL,    &default_options.use_backups, 0, NULL, NULL },
  { "backup-dir",       0,  POPT_ARG_STRING, &default_options.backup_dir, 0, NULL, NULL },
  { "suffix",           0,  POPT_ARG_STRING, &default_options.backup_suffix, 0, NULL, NULL },
  { "list-only",        0,  POPT_ARG_VAL,    &default_options.list_only, 2, NULL, NULL },
  { "from0",           '0', POPT_ARG_VAL,    &default_options.eol_nulls, 1, NULL, NULL },
  { "no-from0",         0,  POPT_ARG_VAL,    &default_options.eol_nulls, 0, NULL, NULL },
  { "protect-args",    's', POPT_ARG_VAL,    &default_options.protect_args, 1, NULL, NULL },
  { "no-protect-args",  0,  POPT_ARG_VAL,    &default_options.protect_args, 0, NULL, NULL },
  { "numeric-ids",      0,  POPT_ARG_VAL,    &default_options.numeric_ids, 1, NULL, NULL },
  { "no-numeric-ids",   0,  POPT_ARG_VAL,    &default_options.numeric_ids, 0, NULL, NULL },
  { "timeout",          0,  POPT_ARG_INT,    &default_options.io_timeout, 0, NULL, NULL },
  { "no-timeout",       0,  POPT_ARG_VAL,    &default_options.io_timeout, 0, NULL, NULL },
  { "contimeout",       0,  POPT_ARG_INT,    NULL, 0, NULL, NULL },
  { "no-contimeout",    0,  POPT_ARG_VAL,    NULL, 0, NULL, NULL },

#ifdef PR_USE_NLS
  { "iconv",            0,  POPT_ARG_STRING, &default_options.iconv_opt, 0, NULL, NULL },

/* XXX Need to support the no-iconv option here! */

#endif /* PR_USE_NLS */

  { "8-bit-output",    '8', POPT_ARG_VAL,    &default_options.escape_chars, 1, NULL, NULL },
  { "no-8-bit-output",  0,  POPT_ARG_VAL,    &default_options.escape_chars, 0, NULL, NULL },
  { "qsort",            0,  POPT_ARG_NONE,   &default_options.use_qsort, 0, NULL, NULL },
  { "checksum-seed",    0,  POPT_ARG_INT,    &default_options.checksum_seed, 0, NULL, NULL },
  { "server",           0,  POPT_ARG_NONE,   &use_server, OPT_SERVER, NULL, NULL },
  { "sender",           0,  POPT_ARG_NONE,   &default_options.sender, OPT_SENDER, NULL, NULL },

  { NULL,		0,  0, NULL, 0, NULL, NULL }
};

static void dump_options(struct rsync_options *opts) {
  if (pr_trace_get_level(trace_channel) >= 15) {
    pr_trace_msg(trace_channel, 15, "opts.allow_incr_recurse = %s",
      opts->allow_incr_recurse ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.dry_run = %s",
      opts->dry_run ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.verbose = %s",
      opts->verbose ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.show_stats = %s",
      opts->show_stats ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.show_human_readable = %s",
      opts->show_human_readable ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.recurse = %s",
      opts->recurse ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.transfer_dirs = %s",
      opts->transfer_dirs ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.preserve_uid = %s",
      opts->preserve_uid ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.preserve_gid = %s",
      opts->preserve_gid ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.preserve_perms = %s",
      opts->preserve_perms ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.preserve_exec = %s",
      opts->preserve_exec ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.preserve_acls = %s",
      opts->preserve_acls ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.preserve_xattrs = %s",
      opts->preserve_xattrs ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.preserve_links = %s",
      opts->preserve_links ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.preserve_hard_links = %s",
      opts->preserve_hard_links ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.preserve_devices = %s",
      opts->preserve_devices ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.preserve_specials = %s",
      opts->preserve_specials ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.preserve_times = %s",
      opts->preserve_times ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.skip_dir_times = %s",
      opts->skip_dir_times ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.copy_links = %s",
      opts->copy_links ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.copy_dirlinks = %s",
      opts->copy_dirlinks ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.copy_unsafe_links = %s",
      opts->copy_unsafe_links ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.keep_dirlinks = %s",
      opts->keep_dirlinks ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.safe_symlinks = %s",
      opts->safe_symlinks ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.modify_window = %s",
      opts->modify_window ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.as_root = %s",
      opts->as_root ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.use_relative_paths = %s",
      opts->use_relative_paths ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.implied_dirs = %s",
      opts->implied_dirs ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.ignore_times = %s",
      opts->ignore_times ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.size_only = %s",
      opts->size_only ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.max_size = %s",
      opts->max_size ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.min_size = %s",
      opts->min_size ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.single_filesystem = %s",
      opts->single_filesystem ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.update_only = %s",
      opts->update_only ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.skip_existing = %s",
      opts->skip_existing ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.skip_nonexisting = %s",
      opts->skip_nonexisting ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.sparse_files = %s",
      opts->sparse_files ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.inplace = %s",
      opts->inplace ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.append_mode = %s",
      opts->append_mode ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.delete_mode = %s",
      opts->delete_mode ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.delete_before = %s",
      opts->delete_before ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.delete_during = %s",
      opts->delete_during ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.delete_after = %s",
      opts->delete_after ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.delete_excluded = %s",
      opts->delete_excluded ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.delete_forced = %s",
      opts->delete_forced ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.delete_max = %s",
      opts->delete_max ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.remove_source_files = %s",
      opts->remove_source_files ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.ignore_errors = %s",
      opts->ignore_errors ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.exclude_cvs = %s",
      opts->exclude_cvs ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.always_checksum = %s",
      opts->always_checksum ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.block_size = %ld",
      opts->block_size);

    pr_trace_msg(trace_channel, 15, "opts.whole_file = %s",
      opts->whole_file ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.fuzzy_basis = %s",
      opts->fuzzy_basis ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.use_compression = %s",
      opts->use_compression ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.skip_compression = %s",
      opts->skip_compression ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.compression_level = %d",
      opts->compression_level);

    pr_trace_msg(trace_channel, 15, "opts.keep_partial = %s",
      opts->keep_partial ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.partial_dir = %s",
      opts->partial_dir ? opts->partial_dir : "(none)");

    pr_trace_msg(trace_channel, 15, "opts.delay_updates = %s",
      opts->delay_updates ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.prune_empty_dirs = %s",
      opts->prune_empty_dirs ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.itemize_changes = %s",
      opts->itemize_changes ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.use_backups = %s",
      opts->use_backups ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.backup_dir = %s",
      opts->backup_dir ? opts->backup_dir : "(none)");

    pr_trace_msg(trace_channel, 15, "opts.backup_suffix = %s",
      opts->backup_suffix ? opts->backup_suffix : "(none)");

    pr_trace_msg(trace_channel, 15, "opts.list_only = %s",
      opts->list_only ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.eol_nulls = %s",
      opts->eol_nulls ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.protect_args = %s",
      opts->protect_args ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.numeric_ids = %s",
      opts->numeric_ids ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.io_timeout = %d",
      opts->io_timeout);

    pr_trace_msg(trace_channel, 15, "opts.escape_chars = %s",
      opts->escape_chars ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.use_qsort = %s",
      opts->use_qsort ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.sender = %s",
      opts->sender ? "true" : "false");

    pr_trace_msg(trace_channel, 15, "opts.checksum_seed = %lu",
      (unsigned long) opts->checksum_seed);
  }
}

int rsync_options_handle_data(pool *p, array_header *req,
    struct rsync_session *sess) {
  register unsigned int i;
  poptContext pc;
  unsigned int argc = 0;
  int opt;
  const char **argv;

  memset(&default_options, 0, sizeof(default_options));
  default_options.compression_level = Z_DEFAULT_COMPRESSION;
  default_options.use_relative_paths = -1;
  default_options.implied_dirs = 1;
  default_options.transfer_dirs = -1;
  default_options.delete_max = INT_MAX; /* XXX rsync has this as INT_MIN?? */
  /* default_options.allow_incr_recurse = 1; */

  argc = req->nelts - 1;
  argv = req->elts;

  for (i = 0; i < argc; i++) {
    if (argv[i]) {
      (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
        "req[%u]: %s", i, argv[i]);
    }
  }

  pc = poptGetContext(NULL, argc, argv, long_options, 0);

  while ((opt = poptGetNextOpt(pc)) != -1) {
    pr_signals_handle();

    if (opt < 0) {
      (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
        "error handling option '%s': %s",
        poptBadOption(pc, POPT_BADOPTION_NOALIAS), poptStrerror(opt));
      errno = EINVAL;
      return -1;
    }

    /* Most options are handled automatically by popt.  Only the special
     * cases are returned, and thus handled here.
     */

pr_trace_msg(trace_channel, 7, "options: poptGetNextOpt() returned %d", opt);

    switch (opt) {
      case OPT_SERVER:
        use_server = TRUE;
        default_options.iconv_opt = NULL;
        break;

      case OPT_SENDER:
        if (!use_server) {
          RSYNC_DISCONNECT("--sender option requires --server");
        }
        break;

      case OPT_MODIFY_WINDOW:
        /* XXX The value has already been modified by popt, but we need
         * to set some flag indicating that a non-default setting is being
         * used.
         */
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--modify-window option has been used!");
        break;

      case OPT_FILTER:
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--filter option has been used!");
        break;

      case OPT_EXCLUDE:
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--exclude option has been used!");
        break;

      case OPT_EXCLUDE_FROM:
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--exclude-from option has been used!");
        break;

      case OPT_INCLUDE:
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--include option has been used!");
        break;

      case OPT_INCLUDE_FROM:
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--include-from option has been used!");
        break;

      case 'a':
        if (!default_options.recurse) {
          default_options.recurse = 1;
        }

        default_options.preserve_perms = 1;
        default_options.preserve_times = 2;
        default_options.preserve_uid = 1;
        default_options.preserve_gid = 1;
        default_options.preserve_links = 1;
        default_options.preserve_devices = 1;
        default_options.preserve_specials = 1;
        break;

      case 'D':
        default_options.preserve_devices = 1;
        default_options.preserve_specials = 1;
        break;

      case OPT_NO_D:
        default_options.preserve_devices = 0;
        default_options.preserve_specials = 0;
        break;

      case 'h':
        default_options.show_human_readable++;
        break;

      case 'g':
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--group option has been used!");
        break;

      case 'H':
        default_options.preserve_hard_links++;
        break;

      case 'i':
        default_options.itemize_changes++;
        break;

      case 'v':
        default_options.verbose++;
        break;

      case 'x':
        default_options.single_filesystem++;
        break;

      case 'P':
        default_options.keep_partial = 1;
        break;

      case 'z':
        /* Yes, this looks weird.  But Z_DEFAULT_COMPRESSION is defined as
         * -1 in zlib.h, so of course any explicitly requested compression
         * level less than -1 will be invalid.
         */
        if (default_options.compression_level < Z_DEFAULT_COMPRESSION ||
            default_options.compression_level > Z_BEST_COMPRESSION) {
          pr_trace_msg(trace_channel, 2,
            "invalid --compress-level value %d requested",
            default_options.compression_level);
          return -1;
        }

        if (default_options.compression_level != Z_NO_COMPRESSION) {
          default_options.use_compression = 1;
        }

        break;

      case OPT_MAX_SIZE:
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--max-size option has been used!");
        break;

      case OPT_MIN_SIZE:
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--min-size option has been used!");
        break;

      case OPT_APPEND:
        default_options.append_mode++;
        break;

      case OPT_LINK_DEST:
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--link-dest option has been used!");
        break;

      case OPT_COPY_DEST:
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--copy-dest option has been used!");
        break;

      case OPT_COMPARE_DEST:
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--compare-dest option has been used!");
        break;

      case OPT_CHMOD:
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--chmod option has been used!");
        break;

      /* XXX NOTE: Should this be in an PR_USE_FACL ifdef? */
      case 'A':
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--acls option has been used!");
        default_options.preserve_acls = 1;
        default_options.preserve_perms = 1;
        break;

      case 'X':
(void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "--xattrs option has been used!");
        default_options.preserve_xattrs++;
        break;

      default:
        (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
          "unsupported/bad option '%s': %s",
          poptBadOption(pc, POPT_BADOPTION_NOALIAS), poptStrerror(opt));
        errno = EINVAL;
        return -1;
    }
  }

  if (opt < -1) {
    (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION, "opt = %d: %s", opt, poptStrerror(opt));
  }

/*
    if (default_options.protect_args == 1 &&
        use_server) {
      poptFreeContext(pc);
      return 0;
    }
 */

  dump_options(&default_options);

  /* Extract the args on which we're operating. */
  argv = poptGetArgs(pc);
  argc = 0;

  for (i = 0; argv && argv[i]; i++) {
    pr_trace_msg(trace_channel, 17, "received arg #%u: '%s'", argc+1,
      argv[i]);
    argc++;
  }

  pr_trace_msg(trace_channel, 13, "received args (%u)", argc);

  /* If the client didn't send the --server option, it's an error. */
  if (!use_server) {
    pr_trace_msg(trace_channel, 3,
      "client did not send required --server option");
    errno = EINVAL;
    return -1;
  }

  /* XXX A lot more sanity checking of various bad combinations, made
   * almost combinatorial by the myriad crazy options that rsync supports.
   */

  if (default_options.sender) {
    default_options.keep_dirlinks = FALSE;
  }

  sess->options = pcalloc(sess->pool, sizeof(struct rsync_options));
  memcpy(sess->options, &default_options, sizeof(struct rsync_options));

  sess->args = make_array(sess->pool, 1, sizeof(char *));

  for (i = 0; i < argc; i++) {
    *((char **) push_array(sess->args)) = pstrdup(sess->pool, argv[i]);
  }

  poptFreeContext(pc);
  return 0;
}

int rsync_options_init(void) {
  /* Some of the options may not be allowed due to the configuration. */
  return 0;
}
