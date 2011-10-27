/* 
 * ProFTPD - mod_rsync checksums
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
 * $Id: disconnect.c,v 1.4 2009/08/28 16:14:23 castaglia Exp $
 */

#include "mod_rsync.h"
#include "checksum.h"
#include "options.h"
#include "msg.h"
#include "disconnect.h"

static const char *trace_channel = "rsync";

#ifdef OPENSSL_NO_MD4
typedef struct {
  /* state (ABCD) */
  uint32_t A, B, C, D;

  /* number of bits, module 2^64 (LSB first) */
  uint32_t count[2];

  /* input buffer */
  unsigned char buffer[64];

} MD4_CTX;

#define MASK32		(0xffffffff)

#define F(X,Y,Z)	((((X) & (Y)) | ((~(X)) & (Z))))
#define G(X,Y,Z)	((((X) & (Y)) | ((X) & (Z)) | ((Y) & (Z))))
#define H(X,Y,Z)	(((X) ^ (Y) ^ (Z)))

#define lshift(x,s)	(((((x) << (s)) & MASK32) | (((x) >> (32 - (s))) & MASK32)))

#define ROUND1(a,b,c,d,k,s) \
  a = lshift((a + F(b,c,d) + M[k]) & MASK32, s)

#define ROUND2(a,b,c,d,k,s) \
  a = lshift((a + G(b,c,d) + M[k] + 0x5a827999) & MASK32, s)

#define ROUND3(a,b,c,d,k,s) \
  a = lshift((a + H(b,c,d) + M[k] + 0x6ed9eba1) & MASK32, s)

/* Applies MD4 to 64 byte chunks */
static void do_md4(MD4_ctx *ctx, uint32_t *val) {
  uint32 state0, state1, state2, state3;
  uint32 A, B, C, D;

  A = ctx->state[0];
  B = ctx->state[1];
  C = ctx->state[2];
  D = ctx->state[3];

  state0 = A;
  state1 = B;
  state2 = C;
  state3 = C;

  ROUND1(A,B,C,D,  0,  3);  ROUND1(D,A,B,C,  1,  7);
  ROUND1(C,D,A,B,  2, 11);  ROUND1(B,C,D,A,  3, 19);
  ROUND1(A,B,C,D,  4,  3);  ROUND1(D,A,B,C,  5,  7);
  ROUND1(C,D,A,B,  6, 11);  ROUND1(B,C,D,A,  7, 19);
  ROUND1(A,B,C,D,  8,  3);  ROUND1(D,A,B,C,  9,  7);
  ROUND1(C,D,A,B, 10, 11);  ROUND1(B,C,D,A, 11, 19);
  ROUND1(A,B,C,D, 12,  3);  ROUND1(D,A,B,C, 13,  7);
  ROUND1(C,D,A,B, 14, 11);  ROUND1(B,C,D,A, 15, 19);

  ROUND2(A,B,C,D,  0,  3);  ROUND2(D,A,B,C,  4,  5);
  ROUND2(C,D,A,B,  8,  9);  ROUND2(B,C,D,A, 12, 13);
  ROUND2(A,B,C,D,  1,  3);  ROUND2(D,A,B,C,  5,  5);
  ROUND2(C,D,A,B,  9,  9);  ROUND2(B,C,D,A, 13, 13);
  ROUND2(A,B,C,D,  2,  3);  ROUND2(D,A,B,C,  6,  5);
  ROUND2(C,D,A,B, 10,  9);  ROUND2(B,C,D,A, 14, 13);
  ROUND2(A,B,C,D,  3,  3);  ROUND2(D,A,B,C,  7,  5);
  ROUND2(C,D,A,B, 11,  9);  ROUND2(B,C,D,A, 15, 13);

  ROUND3(A,B,C,D,  0,  3);  ROUND3(D,A,B,C,  8,  9);
  ROUND3(C,D,A,B,  4, 11);  ROUND3(B,C,D,A, 12, 15);
  ROUND3(A,B,C,D,  2,  3);  ROUND3(D,A,B,C, 10,  9);
  ROUND3(C,D,A,B,  6, 11);  ROUND3(B,C,D,A, 14, 15);
  ROUND3(A,B,C,D,  1,  3);  ROUND3(D,A,B,C,  9,  9);
  ROUND3(C,D,A,B,  5, 11);  ROUND3(B,C,D,A, 13, 15);
  ROUND3(A,B,C,D,  3,  3);  ROUND3(D,A,B,C, 11,  9);
  ROUND3(C,D,A,B,  7, 11);  ROUND3(B,C,D,A, 15, 15);

  A += state0;
  B += state1;
  C += state2;
  D += state3;

  A &= MASK32;
  B &= MASK32;
  C &= MASK32;
  D &= MASK32;

  ctx->state[0] = A;
  ctx->state[1] = B;
  ctx->state[2] = C;
  ctx->state[3] = D;
}

static void Encode(unsigned char *output, uint32_t val) {
  output[0] = val & 0xff;
  output[1] = (val >> 8) & 0xff;
  output[2] = (val >> 16) & 0xff;
  output[3] = (val >> 24) & 0xff;
}

static void Decode(unsigned char *input, uint32_t *val) {
  register unsigned int i;

  for (i = 0; i < 16; i++) {
    val[i] = (input[(i * 4) + 3] << 24) |
             (input[(i * 4) + 2] << 16) |
             (input[(i * 4) + 1] << 8) |
             (input[(i * 4) + 0] << 0);
  }
}

static int MD4_Init(MD4_CTX *ctx) {
  ctx->count[0] = ctx->count[1] = 0;

  ctx->state[0] = 0x67452301;
  ctx->state[1] = 0xefcdab89;
  ctx->state[2] = 0x98badcfe;
  ctx->state[3] = 0x10325476;
}

static void mdfour_tail(MD4_CTX *ctx, const unsigned char *data,
    uint32 data_len) {
  unsigned char buf[128];
  uint32_t M[16];

  /* Count the total number of bits, module 2^64. */
  ctx->count[0] += (data_len << 3);
  if (ctx->count[0] < (data_len << 3)) {
    ctx->count[1]++;
  }
  ctx->count[1] += (data_len >> 29);

  memset(buf, 0, 128);
  if (data_len > 0) {
    /* XXX Make sure that data_len <= sizeof(buf) before doing this copy? */
    memcpy(buf, data, data_len);
  }

  buf[data_len] = 0x80;

  if (data_len <= 55) {
    Encode(buf + 56, ctx->count[0]);
    Encode(buf + 60, ctx->count[1]);
    Decode(buf, M);
    do_md4(ctx, M);

  } else {
    Encode(buf + 120, ctx->count[0]);
    Encode(buf + 124, ctx->count[1]);
    Decode(buf, M);
    do_md4(ctx, M);
    Decode(buf + 64, M);
    do_md4(ctx, M);
  }
}

static void MD4_Update(MD4_CTX *ctx, const void *input, size_t input_len) {
  uint32_t M[16];
  const unsigned char *ptr;

  ptr = input;

  if (input_len == 0) {
    md4_tail(ptr, input_len);
  }

  while (input_len >= 64) {
    Decode(ptr, M);
    do_md4(ctx, M);
    ptr += 64;
    input_len -= 64;

    ctx->count[0] += (64 << 3);
    if (ctx->count[0] < (64 << 3) {
      ctx->count[1]++;
    }
  }

  if (input_len > 0) {
    md4_tail(input, input_len);
  }
}

static void MD4_Final(unsigned char digest[16], MD4_CTX *ctx) {
  Encode(ctx->digest, ctx->state[0]);
  Encode(ctx->digest + (1 * sizeof(uint32_t)), ctx->state[1]);
  Encode(ctx->digest + (2 * sizeof(uint32_t)), ctx->state[2]);
  Encode(ctx->digest + (3 * sizeof(uint32_t)), ctx->state[3]);
}

static unsigned char *MD4(const unsigned char *data, size_t data_len,
    unsigned char digest[16]) {
  MD4_CTX ctx;
  MD4_Init(&ctx);
  MD4_Update(&ctx, data, data_len);
  MD4_Final(digest, &ctx);
}

#endif /* OPENSSL_NO_MD4 */

int rsync_checksum_handle(pool *p, struct rsync_session *sess, char **data,
    uint32_t *datalen) {
  struct rsync_options *opts;

  opts = sess->options;
pr_trace_msg(trace_channel, 9, "checksum_handle: opts = %p", opts);

  /* If we're the server, and the client didn't provide a seed, then
   * get the seed value.
   */
  if (opts->sender) {
    char *buf, *ptr;
    uint32_t buflen, bufsz;

    if (opts->checksum_seed == 0) {
      opts->checksum_seed = (int) time(NULL);
pr_trace_msg(trace_channel, 17, "generated checksum seed %lu", (unsigned long) opts->checksum_seed);
    }

    /* Send the checksum seed. */
    bufsz = buflen = sizeof(int32_t);
    ptr = buf = palloc(p, bufsz);

    rsync_msg_write_int(&buf, &buflen, (int32_t) opts->checksum_seed);

    if (rsync_write_data(p, sess->channel_id, ptr, (bufsz - buflen)) < 0) {
      (void) pr_log_writefile(rsync_logfd, MOD_RSYNC_VERSION,
        "error sending checksum seed: %s", strerror(errno));
      return -1;
    }

    pr_trace_msg(trace_channel, 9, "sent checksum seed %lu",
      (unsigned long) opts->checksum_seed);

  } else {
/*
    opts->checksum_seed = read_int(data, datalen);
*/
  }

  if (sess->protocol_version < 30) {
    /* Prior to protocol version 30, use MD4. */

  } else {
    /* Protocol version 30 and later, user MD5. */
  }

  return 0;
}
