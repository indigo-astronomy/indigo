// Copyright (c) 2022 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Refactored fromhttps://github.com/Zunawe/md5-c

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO MD5 implementation
 \file indigo_md5.c
 */

/*
 * Derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm
 * and modified slightly to be functionally identical but condensed into control structures.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <indigo/indigo_md5.h>

typedef struct {
	uint64_t size;        // Size of input in bytes
	uint32_t buffer[4];   // Current accumulation of hash
	uint8_t input[64];    // Input to be used in the next step
	uint8_t digest[16];   // Result of algorithm
} md5_context;

/*
 * Constants defined by the MD5 algorithm
 */
#define A 0x67452301
#define B 0xefcdab89
#define C 0x98badcfe
#define D 0x10325476

static uint32_t S[] = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

static uint32_t K[] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

/*
 * Bit-manipulation functions defined by the MD5 algorithm
 */
#define F(X, Y, Z) ((X & Y) | (~X & Z))
#define G(X, Y, Z) ((X & Z) | (Y & ~Z))
#define H(X, Y, Z) (X ^ Y ^ Z)
#define I(X, Y, Z) (Y ^ (X | ~Z))

/*
 * Padding used to make the size (in bits) of the input congruent to 448 mod 512
 */
static uint8_t PADDING[] = {
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/*
 * Rotates a 32-bit word left by n bits
 */
static uint32_t rotate_left(uint32_t x, uint32_t n) {
	return (x << n) | (x >> (32 - n));
}

/*
 * Step on 512 bits of input with the main MD5 algorithm.
 */
static void md5_step(uint32_t *buffer, uint32_t *input) {
	uint32_t AA = buffer[0];
	uint32_t BB = buffer[1];
	uint32_t CC = buffer[2];
	uint32_t DD = buffer[3];

	uint32_t E;

	unsigned int j;

	for (unsigned int i = 0; i < 64; ++i) {
		switch (i / 16) {
			case 0:
				E = F(BB, CC, DD);
				j = i;
				break;
			case 1:
				E = G(BB, CC, DD);
				j = ((i * 5) + 1) % 16;
				break;
			case 2:
				E = H(BB, CC, DD);
				j = ((i * 3) + 5) % 16;
				break;
			default:
				E = I(BB, CC, DD);
				j = (i * 7) % 16;
				break;
		}

		uint32_t temp = DD;
		DD = CC;
		CC = BB;
		BB = BB + rotate_left(AA + E + K[i] + input[j], S[i]);
		AA = temp;
	}

	buffer[0] += AA;
	buffer[1] += BB;
	buffer[2] += CC;
	buffer[3] += DD;
}

/*
 * Initialize a context
 */
static void md5_init(md5_context *ctx) {
	ctx->size = (uint64_t)0;

	ctx->buffer[0] = (uint32_t)A;
	ctx->buffer[1] = (uint32_t)B;
	ctx->buffer[2] = (uint32_t)C;
	ctx->buffer[3] = (uint32_t)D;
}

/*
 * Add some amount of input to the context
 *
 * If the input fills out a block of 512 bits, apply the algorithm (md5Step)
 * and save the result in the buffer. Also updates the overall size.
 */
static void md5_update(md5_context *ctx, uint8_t *input_buffer, size_t input_len) {
	uint32_t input[16];
	unsigned int offset = ctx->size % 64;
	ctx->size += (uint64_t)input_len;

	// Copy each byte in input_buffer into the next space in our context input
	for(unsigned int i = 0; i < input_len; ++i) {
		ctx->input[offset++] = (uint8_t)*(input_buffer + i);

		// If we've filled our context input, copy it into our local array input
		// then reset the offset to 0 and fill in a new buffer.
		// Every time we fill out a chunk, we run it through the algorithm
		// to enable some back and forth between cpu and i/o
		if (offset % 64 == 0) {
			for(unsigned int j = 0; j < 16; ++j) {
				// Convert to little-endian
				// The local variable `input` our 512-bit chunk separated into 32-bit words
				// we can use in calculations
				input[j] = (uint32_t)(ctx->input[(j * 4) + 3]) << 24 |
							 (uint32_t)(ctx->input[(j * 4) + 2]) << 16 |
							 (uint32_t)(ctx->input[(j * 4) + 1]) <<  8 |
							 (uint32_t)(ctx->input[(j * 4)]);
			}
			md5_step(ctx->buffer, input);
			offset = 0;
		}
	}
}

/*
 * Pad the current input to get to 448 bytes, append the size in bits to the very end,
 * and save the result of the final iteration into digest.
 */
static void md5_finalize(md5_context *ctx) {
	uint32_t input[16];
	unsigned int offset = ctx->size % 64;
	unsigned int padding_length = offset < 56 ? 56 - offset : (56 + 64) - offset;

	// Fill in the padding andndo the changes to size that resulted from the update
	md5_update(ctx, PADDING, padding_length);
	ctx->size -= (uint64_t)padding_length;

	// Do a final update (internal to this function)
	// Last two 32-bit words are the two halves of the size (converted from bytes to bits)
	for(unsigned int j = 0; j < 14; ++j) {
		input[j] = (uint32_t)(ctx->input[(j * 4) + 3]) << 24 |
							 (uint32_t)(ctx->input[(j * 4) + 2]) << 16 |
							 (uint32_t)(ctx->input[(j * 4) + 1]) <<  8 |
							 (uint32_t)(ctx->input[(j * 4)]);
	}
	input[14] = (uint32_t)(ctx->size * 8);
	input[15] = (uint32_t)((ctx->size * 8) >> 32);

	md5_step(ctx->buffer, input);

	// Move the result into digest (convert from little-endian)
	for(unsigned int i = 0; i < 4; ++i) {
		ctx->digest[(i * 4) + 0] = (uint8_t)((ctx->buffer[i] & 0x000000FF));
		ctx->digest[(i * 4) + 1] = (uint8_t)((ctx->buffer[i] & 0x0000FF00) >>  8);
		ctx->digest[(i * 4) + 2] = (uint8_t)((ctx->buffer[i] & 0x00FF0000) >> 16);
		ctx->digest[(i * 4) + 3] = (uint8_t)((ctx->buffer[i] & 0xFF000000) >> 24);
	}
}

/*
 * Functions that will return a pointer to the hash of the provided input
 */
void indigo_md5(char digest[33], const void *data, const long length) {
	md5_context ctx;
	md5_init(&ctx);
	md5_update(&ctx, (uint8_t *)data, length);
	md5_finalize(&ctx);
	for (int i = 0; i < 16; i++) {
		sprintf(digest + 2 * i, "%02x", ctx.digest[i]);
	}
}

void indigo_md5_partial(char digest[33], const void *data, const long data_length, const long use_length) {
	if (data_length < use_length) {
		indigo_md5(digest, data, data_length);
	} else {
		indigo_md5(digest, data, use_length);
	}
}

void indigo_md5_file_partial(char digest[33], FILE *file, const long use_length) {
	char *input_buffer = malloc(use_length);
	size_t input_size = 0;
	md5_context ctx;
	md5_init(&ctx);
	if ((input_size = fread(input_buffer, 1, use_length, file)) > 0) {
		md5_update(&ctx, (uint8_t *)input_buffer, input_size);
	}
	md5_finalize(&ctx);
	free(input_buffer);
	for (int i = 0; i < 16; i++) {
		sprintf(digest + 2 * i, "%02x", ctx.digest[i]);
	}
}
