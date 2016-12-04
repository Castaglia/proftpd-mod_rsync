/*
 * ProFTPD - mod_rsync disconnect msgs
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

#ifndef MOD_RSYNC_DISCONNECT_H
#define MOD_RSYNC_DISCONNECT_H

#include "mod_rsync.h"

void rsync_disconnect(const char *, const char *, int, const char *);

/* Deal with the fact that __FUNCTION__ is a gcc extension.  Sun's compilers
 * (e.g. SunStudio) like __func__.
 */

# if defined(__FUNCTION__)
#define RSYNC_DISCONNECT(r) \
  rsync_disconnect((r), __FILE__, __LINE__, __FUNCTION__)

# elif defined(__func__)
#define RSYNC_DISCONNECT(r) \
  rsync_disconnect((r), __FILE__, __LINE__, __func__)

# else
#define RSYNC_DISCONNECT(r) \
  rsync_disconnect((r), __FILE__, __LINE__, "")

# endif

#endif /* MOD_RSYNC_DISCONNECT_H */
