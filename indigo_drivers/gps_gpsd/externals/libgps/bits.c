/* bits.c - bitfield extraction code
 *
 * This file is Copyright (c)2010-2018 by the GPSD project
 * SPDX-License-Identifier: BSD-2-clause
 *
 * Bitfield extraction functions.  In each, start is a bit index  - not
 * a byte index - and width is a bit width.  The width is bounded above by
 * 64 bits.
 *
 * The sbits() function assumes twos-complement arithmetic. ubits()
 * and sbits() assume no padding in integers.
 */
#include "gpsd_config.h"  /* must be before all includes */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>

#include "bits.h"

uint64_t ubits(unsigned char buf[], unsigned int start, unsigned int width, bool le)
/* extract a (zero-origin) bitfield from the buffer as an unsigned big-endian uint64_t */
{
    uint64_t fld = 0;
    unsigned int i;
    unsigned end;

    assert(width <= sizeof(uint64_t) * CHAR_BIT);
    for (i = start / CHAR_BIT;
	 i < (start + width + CHAR_BIT - 1) / CHAR_BIT; i++) {
	fld <<= CHAR_BIT;
	fld |= (unsigned char)buf[i];
    }

    end = (start + width) % CHAR_BIT;
    if (end != 0) {
	fld >>= (CHAR_BIT - end);
    }

    fld &= ~(~0ULL << width);

    /* was extraction as a little-endian requested? */
    if (le)
    {
	uint64_t reversed = 0;

	for (i = width; i; --i)
	{
	    reversed <<= 1;
	    if (fld & 1)
		reversed |= 1;
	    fld >>= 1;
	}
	fld = reversed;
    }

    return fld;
}

int64_t sbits(signed char buf[], unsigned int start, unsigned int width, bool le)
/* extract a bitfield from the buffer as a signed big-endian long */
{
    uint64_t fld = ubits((unsigned char *)buf, start, width, le);

    /* ensure width > 0 as the result of
       1ULL << (width - 1)
       is undefined for width <= 0 */
    assert(width > 0);

    if (fld & (1ULL << (width - 1))) {
	fld |= (~0ULL << (width - 1));
    }
    return (int64_t)fld;
}

union int_float {
    int32_t i;
    float f;
};

union long_double {
    int64_t l;
    double d;
};

float getlef32(const char *buf, int off)
{
    union int_float i_f;

    i_f.i = getles32(buf, off);
    return i_f.f;
}

double getled64(const char *buf, int off)
{
    union long_double l_d;

    l_d.l = getles64(buf, off);
    return l_d.d;
}

float getbef32(const char *buf, int off)
{
    union int_float i_f;

    i_f.i = getbes32(buf, off);
    return i_f.f;
}

double getbed64(const char *buf, int off)
{
    union long_double l_d;

    l_d.l = getbes64(buf, off);
    return l_d.d;
}

void putbef32(char *buf, int off, float val)
{
    union int_float i_f;

    i_f.f = val;
    putbe32(buf, off, i_f.i);
}


void shiftleft(unsigned char *data, int size, unsigned short left)
{
    unsigned char *byte;

    if (left >= CHAR_BIT) {
	size -= left/CHAR_BIT;
	memmove(data, data + left/CHAR_BIT, (size + CHAR_BIT - 1)/CHAR_BIT);
	left %= CHAR_BIT;
    }

    for (byte = data; size--; ++byte )
    {
      unsigned char bits;
      if (size)
	  bits = byte[1] >> (CHAR_BIT - left);
      else
	  bits = 0;
      *byte <<= left;
      *byte |= bits;
    }
}

#ifdef __UNUSED__
void putbed64(char *buf, int off, double val)
{
    union long_double l_d;

    l_d.d = val;
    putbe32(buf, (off), (l_d.l) >> 32);
    putbe32(buf, (off)+4, (l_d.l));
}

u_int16_t swap_u16(u_int16_t i)
/* byte-swap a 16-bit unsigned int */
{
    u_int8_t c1, c2;

    c1 = i & 255;
    c2 = (i >> 8) & 255;

    return (c1 << 8) + c2;
}

u_int32_t swap_u32(u_int32_t i)
/* byte-swap a 32-bit unsigned int */
{
    u_int8_t c1, c2, c3, c4;

    c1 = i & 255;
    c2 = (i >> 8) & 255;
    c3 = (i >> 16) & 255;
    c4 = (i >> 24) & 255;

    return ((u_int32_t)c1 << 24) + ((u_int32_t)c2 << 16) + ((u_int32_t)c3 << 8) + c4;
}

u_int64_t swap_u64(u_int64_t i)
/* byte-swap a 64-bit unsigned int */
{
    u_int8_t c1, c2, c3, c4, c5, c6, c7, c8;

    c1 = i & 255;
    c2 = (i >> 8) & 255;
    c3 = (i >> 16) & 255;
    c4 = (i >> 24) & 255;
    c5 = (i >> 32) & 255;
    c6 = (i >> 40) & 255;
    c7 = (i >> 48) & 255;
    c8 = (i >> 56) & 255;

    return ((u_int64_t)c1 << 56) +
            ((u_int64_t)c2 << 48) +
            ((u_int64_t)c3 << 40) +
            ((u_int64_t)c4 << 32) +
            ((u_int64_t)c5 << 24) +
            ((u_int64_t)c6 << 16) +
            ((u_int64_t)c7 << 8) +
            c8;
}
#endif /* __UNUSED__ */
