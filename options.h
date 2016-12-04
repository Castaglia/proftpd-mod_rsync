/*
 * ProFTPD - mod_rsync option handling
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

#ifndef MOD_RSYNC_OPTIONS_H
#define MOD_RSYNC_OPTIONS_H

#include "mod_rsync.h"
#include "session.h"

struct rsync_options {
  int allow_incr_recurse;
  int dry_run;
  int verbose;
  int show_stats;
  int show_human_readable;
  int recurse;
  int transfer_dirs;
  int preserve_uid;
  int preserve_gid;
  int preserve_perms;
  int preserve_exec;
  int preserve_acls;
  int preserve_xattrs;
  int preserve_links;
  int preserve_hard_links;
  int preserve_devices;
  int preserve_specials;
  int preserve_times;
  int skip_dir_times;
  int copy_links;
  int copy_dirlinks;
  int copy_unsafe_links;
  int keep_dirlinks;
  int safe_symlinks;
  int modify_window;
  int as_root;
  int use_relative_paths;
  int implied_dirs;
  int ignore_times;
  int size_only;
  int max_size;
  int min_size;
  int single_filesystem;
  int update_only;
  int skip_existing;
  int skip_nonexisting;
  int sparse_files;
  int inplace;
  int append_mode;
  int delete_mode;
  int delete_before;
  int delete_during;
  int delete_after;
  int delete_excluded;
  int delete_forced;
  int delete_max;
  int remove_source_files;
  int ignore_errors;
  int exclude_cvs;
  int always_checksum;
  long block_size;
  int whole_file;
  int fuzzy_basis;
  int use_compression;
  int skip_compression;
  int compression_level;
  int keep_partial;
  char *partial_dir;
  int delay_updates;
  int prune_empty_dirs;
  int itemize_changes;
  int use_backups;
  char *backup_dir;
  char *backup_suffix;
  int list_only;
  int eol_nulls;
  int protect_args;
  int numeric_ids;
  int io_timeout;
  int escape_chars;
  int use_qsort;
  int sender;
  int32_t checksum_seed;
  char *iconv_opt;
};

int rsync_options_handle_data(pool *p, array_header *req,
  struct rsync_session *sess);
int rsync_options_init(void);

#endif /* MOD_RSYNC_OPTIONS_H */
