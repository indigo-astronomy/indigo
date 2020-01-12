/*
 * This file is Copyright (c) 2010-2018 by the GPSD project
 * SPDX-License-Identifier: BSD-2-clause
 */

#include "gpsd_config.h"  /* must be before all includes */

#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "gpsd.h"

/*
 * gpsd_packetdump()
 *
 * if binbuf is printable, just return a copy of it
 * otherwise) convert a binary string to a hex string and return that
 *
 * *scbuf   -- the buffer to fill with hex
 * scbuflen -- sizeof(scbuf)
 * *binbuf  -- the binary to convert to hex and place in scbuf
 * binbuflen -- sizeof(binbuf)
 *
 * scbuflen needs to be 2x binbuflen to hold the hex conversion
 */

const char *gpsd_packetdump(char *scbuf, size_t scbuflen,
			    char *binbuf, size_t binbuflen)
{
    char *cp;
    bool printable = true;

    assert(binbuf != NULL);
    for (cp = binbuf; cp < binbuf + binbuflen; cp++)
	if (!isprint((unsigned char) *cp) && !isspace((unsigned char) *cp)) {
	    printable = false;
	    break;	/* no need to keep iterating */
        }
    if (printable)
	return binbuf;
    else
	return gpsd_hexdump(scbuf, scbuflen, binbuf, binbuflen);
}

const char *gpsd_hexdump(char *scbuf, size_t scbuflen,
					  char *binbuf, size_t binbuflen)
{
#ifndef SQUELCH_ENABLE
    size_t i, j = 0;
    size_t len =
	(size_t) ((binbuflen >
		   MAX_PACKET_LENGTH) ? MAX_PACKET_LENGTH : binbuflen);
    const char *ibuf = (const char *)binbuf;
    const char *hexchar = "0123456789abcdef";

    if (NULL == binbuf || 0 == binbuflen)
	return "";

    for (i = 0; i < len && j < (scbuflen - 3); i++) {
	scbuf[j++] = hexchar[(ibuf[i] & 0xf0) >> 4];
	scbuf[j++] = hexchar[ibuf[i] & 0x0f];
    }
    scbuf[j] = '\0';
#else /* SQUELCH defined */
    scbuf[0] = '\0';
    (void)scbuflen;
    (void)binbuf;
    (void)binbuflen;
#endif /* SQUELCH_ENABLE */
    return scbuf;
}

static int hex2bin(const char *s)
{
    int a, b;

    a = s[0] & 0xff;
    b = s[1] & 0xff;

    if ((a >= 'a') && (a <= 'f'))
	a = a + 10 - 'a';
    else if ((a >= 'A') && (a <= 'F'))
	a = a + 10 - 'A';
    else if ((a >= '0') && (a <= '9'))
	a -= '0';
    else
	return -1;

    if ((b >= 'a') && (b <= 'f'))
	b = b + 10 - 'a';
    else if ((b >= 'A') && (b <= 'F'))
	b = b + 10 - 'A';
    else if ((b >= '0') && (b <= '9'))
	b -= '0';
    else
	return -1;

    return ((a << 4) + b);
}

int gpsd_hexpack(const char *src, char *dst, size_t len)
/* hex2bin source string to destination - destination can be same as source */
{
    int i, j;

    j = (int)(strlen(src) / 2);
    if ((j < 1) || ((size_t) j > len))
	return -2;

    for (i = 0; i < j; i++) {
	int k;
	if ((k = hex2bin(src + i * 2)) != -1)
	    dst[i] = (char)(k & 0xff);
	else
	    return -1;
    }
    (void)memset(dst + i, '\0', (size_t) (len - i));
    return j;
}


ssize_t hex_escapes(char *cooked, const char *raw)
/* interpret C-style hex escapes */
{
    char c, *cookend;

    for (cookend = cooked; *raw != '\0'; raw++)
	if (*raw != '\\')
	    *cookend++ = *raw;
	else {
	    switch (*++raw) {
	    case 'b':
		*cookend++ = '\b';
		break;
	    case 'e':
		*cookend++ = '\x1b';
		break;
	    case 'f':
		*cookend++ = '\f';
		break;
	    case 'n':
		*cookend++ = '\n';
		break;
	    case 'r':
		*cookend++ = '\r';
		break;
	    case 't':
		*cookend++ = '\r';
		break;
	    case 'v':
		*cookend++ = '\v';
		break;
	    case 'x':
		switch (*++raw) {
		case '0':
		    c = (char)0x00;
		    break;
		case '1':
		    c = (char)0x10;
		    break;
		case '2':
		    c = (char)0x20;
		    break;
		case '3':
		    c = (char)0x30;
		    break;
		case '4':
		    c = (char)0x40;
		    break;
		case '5':
		    c = (char)0x50;
		    break;
		case '6':
		    c = (char)0x60;
		    break;
		case '7':
		    c = (char)0x70;
		    break;
		case '8':
		    c = (char)0x80;
		    break;
		case '9':
		    c = (char)0x90;
		    break;
		case 'A':
		case 'a':
		    c = (char)0xa0;
		    break;
		case 'B':
		case 'b':
		    c = (char)0xb0;
		    break;
		case 'C':
		case 'c':
		    c = (char)0xc0;
		    break;
		case 'D':
		case 'd':
		    c = (char)0xd0;
		    break;
		case 'E':
		case 'e':
		    c = (char)0xe0;
		    break;
		case 'F':
		case 'f':
		    c = (char)0xf0;
		    break;
		default:
		    return -1;
		}
		switch (*++raw) {
		case '0':
		    c += 0x00;
		    break;
		case '1':
		    c += 0x01;
		    break;
		case '2':
		    c += 0x02;
		    break;
		case '3':
		    c += 0x03;
		    break;
		case '4':
		    c += 0x04;
		    break;
		case '5':
		    c += 0x05;
		    break;
		case '6':
		    c += 0x06;
		    break;
		case '7':
		    c += 0x07;
		    break;
		case '8':
		    c += 0x08;
		    break;
		case '9':
		    c += 0x09;
		    break;
		case 'A':
		case 'a':
		    c += 0x0a;
		    break;
		case 'B':
		case 'b':
		    c += 0x0b;
		    break;
		case 'C':
		case 'c':
		    c += 0x0c;
		    break;
		case 'D':
		case 'd':
		    c += 0x0d;
		    break;
		case 'E':
		case 'e':
		    c += 0x0e;
		    break;
		case 'F':
		case 'f':
		    c += 0x0f;
		    break;
		default:
		    return -2;
		}
		*cookend++ = c;
		break;
	    case '\\':
		*cookend++ = '\\';
		break;
	    default:
		return -3;
	    }
	}
    return (ssize_t) (cookend - cooked);
}
