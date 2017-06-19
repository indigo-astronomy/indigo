// Copyright (C) 2016 Rumen G. Bogdanovski
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

// version history
// 2.0 by Rumen G. Bogdanovski

#include <ctype.h>
#include <stdint.h>
#include "indigo_base64.h"
#include "indigo_base64_luts.h"
#include <stdio.h>

/* out size should be at least 4*inlen/3 + 4.
 * returns length of out (without trailing NULL).
 */
long base64_encode(unsigned char *out, const unsigned char *in, long inlen) {
	uint16_t* b64lut = (uint16_t*)base64lut;
	long dlen = ((inlen+2)/3)*4; /* 4/3, rounded up */
	uint16_t* wbuf = (uint16_t*)out;

	for(; inlen > 2; inlen -= 3 ) {
		uint32_t n = in[0] << 16 | in[1] << 8 | in[2];

		wbuf[0] = b64lut[ n >> 12 ];
		wbuf[1] = b64lut[ n & 0x00000fff ];

		wbuf += 2;
		in += 3;
	}

	out = (unsigned char*)wbuf;
	if ( inlen > 0 ) {
		unsigned char fragment;
		*out++ = base64digits[in[0] >> 2];
		fragment = (in[0] << 4) & 0x30;
		if (inlen > 1) fragment |= in[1] >> 4;
		*out++ = base64digits[fragment];
		*out++ = (inlen < 2) ? '=' : base64digits[(in[1] << 2) & 0x3c];
		*out++ = '=';
	}
	*out = 0; // NULL terminate
	return dlen;
}


/* base64 should not contain whitespaces.*/
long base64_decode_fast(unsigned char* out, const unsigned char* in, long inlen) {
	long outlen = 0;
	uint8_t b1, b2, b3;
	uint16_t s1, s2;
	uint32_t n32;
	int j;
	long n = (inlen/4)-1;
	uint16_t* inp = (uint16_t*)in;

	for( j = 0; j < n; j++ ) {
		s1 = rbase64lut[ inp[0] ];
		s2 = rbase64lut[ inp[1] ];

		n32 = s1;
		n32 <<= 10;
		n32 |= s2 >> 2;

		b3 = ( n32 & 0x00ff );
		n32 >>= 8;
		b2 = ( n32 & 0x00ff );
		n32 >>= 8;
		b1 = ( n32 & 0x00ff );

		out[0] = b1;
		out[1] = b2;
		out[2] = b3;

		inp += 2;
		out += 3;
	}
	outlen = (inlen / 4 - 1) * 3;

	s1 = rbase64lut[ inp[0] ];
	s2 = rbase64lut[ inp[1] ];

	n32 = s1;
	n32 <<= 10;
	n32 |= s2 >> 2;

	b3 = ( n32 & 0x00ff );
	n32 >>= 8;
	b2 = ( n32 & 0x00ff );
	n32 >>= 8;
	b1 = ( n32 & 0x00ff );

	*out++ = b1;
	outlen++;
	if ((inp[1] & 0x00FF) != 0x003D)  {
		*out++ = b2;
		outlen++;
		if ((inp[1] & 0xFF00) != 0x3D00)  {
			*out++ = b3;
			outlen++;
		}
	}
	return outlen;
}


long base64_decode_fast_nl(unsigned char* out, const unsigned char* in, long inlen) {
	long outlen = 0;
	uint8_t b1, b2, b3;
	uint16_t s1, s2;
	uint32_t n32;
	int j;
	long n = (inlen/4)-1;
	uint16_t* inp = (uint16_t*)in;

	for( j = 0; j < n; j++ ) {
		if (in[0] == '\n') in++;
		inp = (uint16_t*)in;

		s1 = rbase64lut[ inp[0] ];
		s2 = rbase64lut[ inp[1] ];

		n32 = s1;
		n32 <<= 10;
		n32 |= s2 >> 2;

		b3 = ( n32 & 0x00ff );
		n32 >>= 8;
		b2 = ( n32 & 0x00ff );
		n32 >>= 8;
		b1 = ( n32 & 0x00ff );

		out[0] = b1;
		out[1] = b2;
		out[2] = b3;

		in += 4;
		out += 3;
	}
	outlen = (inlen / 4 - 1) * 3;
	if (in[0] == '\n') in++;
	inp = (uint16_t*)in;

	s1 = rbase64lut[ inp[0] ];
	s2 = rbase64lut[ inp[1] ];

	n32 = s1;
	n32 <<= 10;
	n32 |= s2 >> 2;

	b3 = ( n32 & 0x00ff );
	n32 >>= 8;
	b2 = ( n32 & 0x00ff );
	n32 >>= 8;
	b1 = ( n32 & 0x00ff );

	*out++ = b1;
	outlen++;
	if ((inp[1] & 0x00FF) != 0x003D)  {
		*out++ = b2;
		outlen++;
		if ((inp[1] & 0xFF00) != 0x3D00)  {
			*out++ = b3;
			outlen++;
		}
	}
	return outlen;
}


