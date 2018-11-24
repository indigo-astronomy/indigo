/*
 * 1394-Based Digital Camera Control Library
 *
 * Color conversion functions
 *
 * Written by Damien Douxchamps and Frederic Devernay
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdlib.h>
#include "conversions.h"

// this should disappear...
extern void swab();

/**********************************************************************
 *
 *  CONVERSION FUNCTIONS TO YUV422
 *
 **********************************************************************/

dc1394error_t
dc1394_YUV422_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
    switch (byte_order) {
    case DC1394_BYTE_ORDER_YUYV:
        swab(src, dest, (width*height) << 1);
        return DC1394_SUCCESS;
    case DC1394_BYTE_ORDER_UYVY:
        memcpy(dest,src, (width*height)<<1);
        return DC1394_SUCCESS;
    default:
        return DC1394_INVALID_BYTE_ORDER;
    }
}

dc1394error_t
dc1394_YUV411_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
    register int i=(width*height) + ((width*height) >> 1) -1;
    register int j=((width*height) << 1)-1;
    register int y0, y1, y2, y3, u, v;

    switch (byte_order) {
    case DC1394_BYTE_ORDER_YUYV:
        while (i >= 0) {
            y3 = src[i--];
            y2 = src[i--];
            v  = src[i--];
            y1 = src[i--];
            y0 = src[i--];
            u  = src[i--];

            dest[j--] = v;
            dest[j--] = y3;
            dest[j--] = u;
            dest[j--] = y2;

            dest[j--] = v;
            dest[j--] = y1;
            dest[j--] = u;
            dest[j--] = y0;
        }
        return DC1394_SUCCESS;
    case DC1394_BYTE_ORDER_UYVY:
        while (i >= 0) {
            y3 = src[i--];
            y2 = src[i--];
            v  = src[i--];
            y1 = src[i--];
            y0 = src[i--];
            u  = src[i--];

            dest[j--] = y3;
            dest[j--] = v;
            dest[j--] = y2;
            dest[j--] = u;

            dest[j--] = y1;
            dest[j--] = v;
            dest[j--] = y0;
            dest[j--] = u;
        }
        return DC1394_SUCCESS;
    default:
        return DC1394_INVALID_BYTE_ORDER;
    }

}

dc1394error_t
dc1394_YUV444_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
    register int i = (width*height) + ((width*height) << 1)-1;
    register int j = ((width*height) << 1)-1;
    register int y0, y1, u0, u1, v0, v1;

    switch (byte_order) {
    case DC1394_BYTE_ORDER_YUYV:
        while (i >= 0) {
            v1 = src[i--];
            y1 = src[i--];
            u1 = src[i--];
            v0 = src[i--];
            y0 = src[i--];
            u0 = src[i--];

            dest[j--] = (v0+v1) >> 1;
            dest[j--] = y1;
            dest[j--] = (u0+u1) >> 1;
            dest[j--] = y0;
        }
        return DC1394_SUCCESS;
    case DC1394_BYTE_ORDER_UYVY:
        while (i >= 0) {
            v1 = src[i--];
            y1 = src[i--];
            u1 = src[i--];
            v0 = src[i--];
            y0 = src[i--];
            u0 = src[i--];

            dest[j--] = y1;
            dest[j--] = (v0+v1) >> 1;
            dest[j--] = y0;
      dest[j--] = (u0+u1) >> 1;
        }
        return DC1394_SUCCESS;
    default:
        return DC1394_INVALID_BYTE_ORDER;
    }
}

dc1394error_t
dc1394_MONO8_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
    if ((width%2)==0) {
        // do it the quick way
        register int i = width*height - 1;
        register int j = (width*height << 1) - 1;
        register int y0, y1;

        switch (byte_order) {
        case DC1394_BYTE_ORDER_YUYV:
            while (i >= 0) {
                y1 = src[i--];
                y0 = src[i--];
                dest[j--] = 128;
                dest[j--] = y1;
                dest[j--] = 128;
                dest[j--] = y0;
            }
            return DC1394_SUCCESS;
        case DC1394_BYTE_ORDER_UYVY:
            while (i >= 0) {
                y1 = src[i--];
                y0 = src[i--];
                dest[j--] = y1;
                dest[j--] = 128;
                dest[j--] = y0;
                dest[j--] = 128;
            }
            return DC1394_SUCCESS;
        default:
            return DC1394_INVALID_BYTE_ORDER;
        }
    } else { // width*2 != dest_pitch
        register int x, y;

        //assert ((dest_pitch - 2*width)==1);

        switch (byte_order) {
        case DC1394_BYTE_ORDER_YUYV:
            y=height;
            while (y--) {
                x=width;
                while (x--) {
                    *dest++ = *src++;
                    *dest++ = 128;
                }
                // padding required, duplicate last column
                *dest++ = *(src-1);
                *dest++ = 128;
            }
            return DC1394_SUCCESS;
        case DC1394_BYTE_ORDER_UYVY:
            y=height;
            while (y--) {
                x=width;
                while (x--) {
                    *dest++ = 128;
                    *dest++ = *src++;
                }
                // padding required, duplicate last column
                *dest++ = 128;
                *dest++ = *(src-1);
            }
            return DC1394_SUCCESS;
        default:
            return DC1394_INVALID_BYTE_ORDER;
        }
    }
}

dc1394error_t
dc1394_MONO16_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order, uint32_t bits)
{
    register int i = ((width*height) << 1)-1;
    register int j = ((width*height) << 1)-1;
    register int y0, y1;

    switch (byte_order) {
    case DC1394_BYTE_ORDER_YUYV:
        while (i >= 0) {
            y1 = src[i--];
            y1 = (y1 + (((int)src[i--])<<8))>>(bits-8);
            y0 = src[i--];
            y0 = (y0 + (((int)src[i--])<<8))>>(bits-8);
            dest[j--] = 128;
            dest[j--] = y1;
            dest[j--] = 128;
            dest[j--] = y0;
        }
        return DC1394_SUCCESS;
    case DC1394_BYTE_ORDER_UYVY:
        while (i >= 0) {
            y1 = src[i--];
            y1 = (y1 + (((int)src[i--])<<8))>>(bits-8);
            y0 = src[i--];
            y0 = (y0 + (((int)src[i--])<<8))>>(bits-8);
            dest[j--] = y1;
            dest[j--] = 128;
            dest[j--] = y0;
            dest[j--] = 128;
        }
        return DC1394_SUCCESS;
    default:
        return DC1394_INVALID_BYTE_ORDER;
    }

}

dc1394error_t
dc1394_MONO16_to_MONO8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t bits)
{
    register int i = ((width*height)<<1)-1;
    register int j = (width*height)-1;
    register int y;

    while (i >= 0) {
        y = src[i--];
        dest[j--] = (y + (src[i--]<<8))>>(bits-8);
    }
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_RGB8_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
    register int i = (width*height) + ( (width*height) << 1 )-1;
    register int j = ((width*height) << 1)-1;
    register int y0, y1, u0, u1, v0, v1 ;
    register int r, g, b;

    switch (byte_order) {
    case DC1394_BYTE_ORDER_YUYV:
        while (i >= 0) {
            b = (uint8_t) src[i--];
            g = (uint8_t) src[i--];
            r = (uint8_t) src[i--];
            RGB2YUV (r, g, b, y0, u0 , v0);
            b = (uint8_t) src[i--];
            g = (uint8_t) src[i--];
            r = (uint8_t) src[i--];
            RGB2YUV (r, g, b, y1, u1 , v1);
            dest[j--] = (v0+v1) >> 1;
            dest[j--] = y0;
            dest[j--] = (u0+u1) >> 1;
            dest[j--] = y1;
        }
        return DC1394_SUCCESS;
    case DC1394_BYTE_ORDER_UYVY:
        while (i >= 0) {
            b = (uint8_t) src[i--];
            g = (uint8_t) src[i--];
            r = (uint8_t) src[i--];
            RGB2YUV (r, g, b, y0, u0 , v0);
            b = (uint8_t) src[i--];
            g = (uint8_t) src[i--];
            r = (uint8_t) src[i--];
            RGB2YUV (r, g, b, y1, u1 , v1);
            dest[j--] = y0;
            dest[j--] = (v0+v1) >> 1;
            dest[j--] = y1;
            dest[j--] = (u0+u1) >> 1;
        }
        return DC1394_SUCCESS;
    default:
        return DC1394_INVALID_BYTE_ORDER;
    }
}

dc1394error_t
dc1394_RGB16_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order, uint32_t bits)
{
    register int i = ( ((width*height) + ( (width*height) << 1 )) << 1 ) -1;
    register int j = ((width*height) << 1)-1;
    register int y0, y1, u0, u1, v0, v1 ;
    register int r, g, b, t;

    switch (byte_order) {
    case DC1394_BYTE_ORDER_YUYV:
        while (i >= 0) {
            t =src[i--];
            b = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            t =src[i--];
            g = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            t =src[i--];
            r = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            RGB2YUV (r, g, b, y0, u0 , v0);
            t =src[i--];
            b = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            t =src[i--];
            g = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            t =src[i--];
            r = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            RGB2YUV (r, g, b, y1, u1 , v1);
            dest[j--] = (v0+v1) >> 1;
            dest[j--] = y0;
            dest[j--] = (u0+u1) >> 1;
            dest[j--] = y1;
        }
        return DC1394_SUCCESS;
    case DC1394_BYTE_ORDER_UYVY:
        while (i >= 0) {
            t =src[i--];
            b = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            t =src[i--];
            g = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            t =src[i--];
            r = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            RGB2YUV (r, g, b, y0, u0 , v0);
            t =src[i--];
            b = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            t =src[i--];
            g = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            t =src[i--];
            r = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
            RGB2YUV (r, g, b, y1, u1 , v1);
            dest[j--] = y0;
            dest[j--] = (v0+v1) >> 1;
            dest[j--] = y1;
            dest[j--] = (u0+u1) >> 1;
        }
        return DC1394_SUCCESS;
    default:
        return DC1394_INVALID_BYTE_ORDER;
    }
}

/**********************************************************************
 *
 *  CONVERSION FUNCTIONS TO RGB 24bpp
 *
 **********************************************************************/

dc1394error_t
dc1394_RGB16_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t bits)
{
    register int i = (((width*height) + ( (width*height) << 1 )) << 1)-1;
    register int j = (width*height) + ( (width*height) << 1 ) -1;
    register int t;

    while (i >= 0) {
        t = src[i--];
        t = (t + (src[i--]<<8))>>(bits-8);
        dest[j--]=t;
        t = src[i--];
        t = (t + (src[i--]<<8))>>(bits-8);
        dest[j--]=t;
        t = src[i--];
        t = (t + (src[i--]<<8))>>(bits-8);
        dest[j--]=t;
    }
    return DC1394_SUCCESS;
}


dc1394error_t
dc1394_YUV444_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height)
{
    register int i = (width*height) + ( (width*height) << 1 ) -1;
    register int j = (width*height) + ( (width*height) << 1 ) -1;
    register int y, u, v;
    register int r, g, b;

    while (i >= 0) {
        v = (uint8_t) src[i--] - 128;
        y = (uint8_t) src[i--];
        u = (uint8_t) src[i--] - 128;
        YUV2RGB (y, u, v, r, g, b);
        dest[j--] = b;
        dest[j--] = g;
        dest[j--] = r;
    }
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_YUV422_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
    register int i = ((width*height) << 1)-1;
    register int j = (width*height) + ( (width*height) << 1 ) -1;
    register int y0, y1, u, v;
    register int r, g, b;


    switch (byte_order) {
    case DC1394_BYTE_ORDER_YUYV:
        while (i >= 0) {
            v  = (uint8_t) src[i--] -128;
            y1 = (uint8_t) src[i--];
            u  = (uint8_t) src[i--] -128;
            y0  = (uint8_t) src[i--];
            YUV2RGB (y1, u, v, r, g, b);
            dest[j--] = b;
            dest[j--] = g;
            dest[j--] = r;
            YUV2RGB (y0, u, v, r, g, b);
            dest[j--] = b;
            dest[j--] = g;
            dest[j--] = r;
        }
        return DC1394_SUCCESS;
    case DC1394_BYTE_ORDER_UYVY:
        while (i >= 0) {
            y1 = (uint8_t) src[i--];
            v  = (uint8_t) src[i--] - 128;
            y0 = (uint8_t) src[i--];
            u  = (uint8_t) src[i--] - 128;
            YUV2RGB (y1, u, v, r, g, b);
            dest[j--] = b;
            dest[j--] = g;
            dest[j--] = r;
            YUV2RGB (y0, u, v, r, g, b);
            dest[j--] = b;
            dest[j--] = g;
            dest[j--] = r;
        }
        return DC1394_SUCCESS;
    default:
        return DC1394_INVALID_BYTE_ORDER;
    }

}


dc1394error_t
dc1394_YUV411_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height)
{
    register int i = (width*height) + ( (width*height) >> 1 )-1;
    register int j = (width*height) + ( (width*height) << 1 )-1;
    register int y0, y1, y2, y3, u, v;
    register int r, g, b;

    while (i >= 0) {
        y3 = (uint8_t) src[i--];
        y2 = (uint8_t) src[i--];
        v  = (uint8_t) src[i--] - 128;
        y1 = (uint8_t) src[i--];
        y0 = (uint8_t) src[i--];
        u  = (uint8_t) src[i--] - 128;
        YUV2RGB (y3, u, v, r, g, b);
        dest[j--] = b;
        dest[j--] = g;
        dest[j--] = r;
        YUV2RGB (y2, u, v, r, g, b);
        dest[j--] = b;
        dest[j--] = g;
        dest[j--] = r;
        YUV2RGB (y1, u, v, r, g, b);
        dest[j--] = b;
        dest[j--] = g;
        dest[j--] = r;
        YUV2RGB (y0, u, v, r, g, b);
        dest[j--] = b;
        dest[j--] = g;
        dest[j--] = r;
    }
    return DC1394_SUCCESS;
}


dc1394error_t
dc1394_MONO8_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height)
{
    register int i = (width*height)-1;
    register int j = (width*height) + ( (width*height) << 1 )-1;
    register int y;

    while (i >= 0) {
        y = (uint8_t) src[i--];
        dest[j--] = y;
        dest[j--] = y;
        dest[j--] = y;
    }
    return DC1394_SUCCESS;
}


dc1394error_t
dc1394_MONO16_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t bits)
{
    register int i = ((width*height) << 1)-1;
    register int j = (width*height) + ( (width*height) << 1 )-1;
    register int y;

    while (i > 0) {
        y = src[i--];
        y = (y + (src[i--]<<8))>>(bits-8);
        dest[j--] = y;
        dest[j--] = y;
        dest[j--] = y;
    }
    return DC1394_SUCCESS;
}


// change a 16bit stereo image (8bit/channel) into two 8bit images on top
// of each other
dc1394error_t
dc1394_deinterlace_stereo(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height)
{
    register int i = (width*height)-1;
    register int j = ((width*height)>>1)-1;
    register int k = (width*height)-1;

    while (i >= 0) {
        dest[k--] = src[i--];
        dest[j--] = src[i--];
    }
    return DC1394_SUCCESS;
}


dc1394error_t
dc1394_convert_to_YUV422(uint8_t *src, uint8_t *dest, uint32_t width, uint32_t height, uint32_t byte_order,
                         dc1394color_coding_t source_coding, uint32_t bits)
{
    switch(source_coding) {
    case DC1394_COLOR_CODING_YUV422:
        return dc1394_YUV422_to_YUV422(src, dest, width, height, byte_order);
        break;
    case DC1394_COLOR_CODING_YUV411:
        return dc1394_YUV411_to_YUV422(src, dest, width, height, byte_order);
        break;
    case DC1394_COLOR_CODING_YUV444:
        return dc1394_YUV444_to_YUV422(src, dest, width, height, byte_order);
        break;
    case DC1394_COLOR_CODING_RGB8:
        return dc1394_RGB8_to_YUV422(src, dest, width, height, byte_order);
        break;
    case DC1394_COLOR_CODING_MONO8:
    case DC1394_COLOR_CODING_RAW8:
        return dc1394_MONO8_to_YUV422(src, dest, width, height, byte_order);
        break;
    case DC1394_COLOR_CODING_MONO16:
    case DC1394_COLOR_CODING_RAW16:
        return dc1394_MONO16_to_YUV422(src, dest, width, height, byte_order, bits);
        break;
    case DC1394_COLOR_CODING_RGB16:
        return dc1394_RGB16_to_YUV422(src, dest, width, height, byte_order, bits);
        break;
    default:
        return DC1394_FUNCTION_NOT_SUPPORTED;
    }

    return DC1394_SUCCESS;

}


dc1394error_t
dc1394_convert_to_MONO8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order,
                        dc1394color_coding_t source_coding, uint32_t bits)
{
    switch(source_coding) {
    case DC1394_COLOR_CODING_MONO16:
        return dc1394_MONO16_to_MONO8(src, dest, width, height, bits);
        break;
    case DC1394_COLOR_CODING_MONO8:
        memcpy(dest, src, width*height);
        break;
    default:
        return DC1394_FUNCTION_NOT_SUPPORTED;
    }

    return DC1394_SUCCESS;
}


dc1394error_t
dc1394_convert_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order,
                       dc1394color_coding_t source_coding, uint32_t bits)
{
    switch(source_coding) {
    case DC1394_COLOR_CODING_RGB16:
        return dc1394_RGB16_to_RGB8 (src, dest, width, height, bits);
        break;
    case DC1394_COLOR_CODING_YUV444:
        return dc1394_YUV444_to_RGB8 (src, dest, width, height);
        break;
    case DC1394_COLOR_CODING_YUV422:
        return dc1394_YUV422_to_RGB8 (src, dest, width, height, byte_order);
        break;
    case DC1394_COLOR_CODING_YUV411:
        return dc1394_YUV411_to_RGB8 (src, dest, width, height);
        break;
    case DC1394_COLOR_CODING_MONO8:
    case DC1394_COLOR_CODING_RAW8:
        return dc1394_MONO8_to_RGB8 (src, dest, width, height);
        break;
    case DC1394_COLOR_CODING_MONO16:
    case DC1394_COLOR_CODING_RAW16:
        return dc1394_MONO16_to_RGB8 (src, dest, width, height,bits);
        break;
    case DC1394_COLOR_CODING_RGB8:
      memcpy(dest, src, width*height*3);
      break;
    default:
        return DC1394_FUNCTION_NOT_SUPPORTED;
    }

    return DC1394_SUCCESS;
}

dc1394error_t
Adapt_buffer_convert(dc1394video_frame_t *in, dc1394video_frame_t *out)
{
    uint32_t bpp;

    // conversions don't change the size of buffers or its position
    out->size[0]=in->size[0];
    out->size[1]=in->size[1];
    out->position[0]=in->position[0];
    out->position[1]=in->position[1];

    // color coding has already been set before conversion: don't touch it.

    // keep the color filter value in all cases. if the format is not raw it will not be further used anyway
    out->color_filter=in->color_filter;

    // the output YUV byte order must be already set if the buffer is YUV422 at the output
    // if the output is not YUV we don't care about this field.
    // Hence nothing to do.

    // we always convert to 8bits (at this point) we can safely set this value to 8.
    out->data_depth=8;

    // don't know what to do with stride... >>>> TODO: STRIDE SHOULD BE TAKEN INTO ACCOUNT... <<<<
    // out->stride=??

    // the video mode should not change. Color coding and other stuff can be accessed in specific fields of this struct
    out->video_mode = in->video_mode;

    // padding is kept:
    out->padding_bytes = in->padding_bytes;

    // image bytes changes:    >>>> TODO: STRIDE SHOULD BE TAKEN INTO ACCOUNT... <<<<
    dc1394_get_color_coding_bit_size(out->color_coding, &bpp);
    out->image_bytes=(out->size[0]*out->size[1]*bpp)/8;

    // total is image_bytes + padding_bytes
    out->total_bytes = out->image_bytes + out->padding_bytes;

    // bytes-per-packet and packets_per_frame are internal data that can be kept as is.
    out->packet_size  = in->packet_size;
    out->packets_per_frame = in->packets_per_frame;

    // timestamp, frame_behind, id and camera are copied too:
    out->timestamp = in->timestamp;
    out->frames_behind = in->frames_behind;
    out->camera = in->camera;
    out->id = in->id;

    // verify memory allocation:
    if (out->total_bytes>out->allocated_image_bytes) {
        free(out->image);
        out->image=(uint8_t*)malloc(out->total_bytes*sizeof(uint8_t));
        if (out->image)
            out->allocated_image_bytes = out->total_bytes*sizeof(uint8_t);
        else
            out->allocated_image_bytes = 0;
    }

    // Copy padding bytes:
    if(out->image)
        memcpy(&(out->image[out->image_bytes]),&(in->image[in->image_bytes]),out->padding_bytes);

    out->little_endian=0;   // not used before 1.32 is out.
    out->data_in_padding=0; // not used before 1.32 is out.
    
    if(out->image)
        return DC1394_SUCCESS;
        
    return DC1394_MEMORY_ALLOCATION_FAILURE;
}

dc1394error_t
dc1394_convert_frames(dc1394video_frame_t *in, dc1394video_frame_t *out)
{

    switch(out->color_coding) {
    case DC1394_COLOR_CODING_YUV422:
        switch(in->color_coding) {
        case DC1394_COLOR_CODING_YUV422:
            
            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_YUV422_to_YUV422(in->image, out->image, in->size[0], in->size[1], out->yuv_byte_order);
            break;
            
        case DC1394_COLOR_CODING_YUV411:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_YUV411_to_YUV422(in->image, out->image, in->size[0], in->size[1], out->yuv_byte_order);
            break;
            
        case DC1394_COLOR_CODING_YUV444:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_YUV444_to_YUV422(in->image, out->image, in->size[0], in->size[1], out->yuv_byte_order);
            break;
            
        case DC1394_COLOR_CODING_RGB8:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_RGB8_to_YUV422(in->image, out->image, in->size[0], in->size[1], out->yuv_byte_order);
            break;
            
        case DC1394_COLOR_CODING_MONO8:
        case DC1394_COLOR_CODING_RAW8:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_MONO8_to_YUV422(in->image, out->image, in->size[0], in->size[1], out->yuv_byte_order);
            break;
            
        case DC1394_COLOR_CODING_MONO16:
        case DC1394_COLOR_CODING_RAW16:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_MONO16_to_YUV422(in->image, out->image, in->size[0], in->size[1], out->yuv_byte_order, in->data_depth);
            break;
            
        case DC1394_COLOR_CODING_RGB16:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_RGB16_to_YUV422(in->image, out->image, in->size[0], in->size[1], out->yuv_byte_order, in->data_depth);
            break;
            
        default:
            return DC1394_FUNCTION_NOT_SUPPORTED;
        }
        break;
    case DC1394_COLOR_CODING_MONO8:
        switch(in->color_coding) {
        case DC1394_COLOR_CODING_MONO16:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_MONO16_to_MONO8(in->image, out->image, in->size[0], in->size[1], in->data_depth);
            break;
            
        case DC1394_COLOR_CODING_MONO8:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            memcpy(out->image, in->image, in->size[0]*in->size[1]);
            break;
            
        default:
            return DC1394_FUNCTION_NOT_SUPPORTED;
        }
        break;
    case DC1394_COLOR_CODING_RGB8:
        switch(in->color_coding) {
        case DC1394_COLOR_CODING_RGB16:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_RGB16_to_RGB8 (in->image, out->image, in->size[0], in->size[1], in->data_depth);
            break;
            
        case DC1394_COLOR_CODING_YUV444:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_YUV444_to_RGB8 (in->image, out->image, in->size[0], in->size[1]);
            break;
            
        case DC1394_COLOR_CODING_YUV422:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_YUV422_to_RGB8 (in->image, out->image, in->size[0], in->size[1], in->yuv_byte_order);
            break;
            
        case DC1394_COLOR_CODING_YUV411:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_YUV411_to_RGB8 (in->image, out->image, in->size[0], in->size[1]);
            break;
            
        case DC1394_COLOR_CODING_MONO8:
        case DC1394_COLOR_CODING_RAW8:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_MONO8_to_RGB8 (in->image, out->image, in->size[0], in->size[1]);
            break;
            
        case DC1394_COLOR_CODING_MONO16:
        case DC1394_COLOR_CODING_RAW16:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            return dc1394_MONO16_to_RGB8 (in->image, out->image, in->size[0], in->size[1],in->data_depth);
            break;
            
        case DC1394_COLOR_CODING_RGB8:

            if(DC1394_SUCCESS != Adapt_buffer_convert(in,out))
                return DC1394_MEMORY_ALLOCATION_FAILURE;
                
            memcpy(out->image, in->image, in->size[0]*in->size[1]*3);
            break;
            
        default:
            return DC1394_FUNCTION_NOT_SUPPORTED;
        }
        break;
    default:
        return DC1394_FUNCTION_NOT_SUPPORTED;
    }

    return DC1394_SUCCESS;
}


dc1394error_t
Adapt_buffer_stereo(dc1394video_frame_t *in, dc1394video_frame_t *out)
{
    uint32_t bpp;

    // buffer position is not changed. Size is boubled in Y
    out->size[0]=in->size[0];
    out->size[1]=in->size[1]*2;
    out->position[0]=in->position[0];
    out->position[1]=in->position[1];

    // color coding is set to mono8 or raw8.
    switch (in->color_coding) {
    case DC1394_COLOR_CODING_RAW16:
        out->color_coding=DC1394_COLOR_CODING_RAW8;
        break;
    case DC1394_COLOR_CODING_MONO16:
    case DC1394_COLOR_CODING_YUV422:
        out->color_coding=DC1394_COLOR_CODING_MONO8;
        break;
    default:
        return DC1394_INVALID_COLOR_CODING;
  }

    // keep the color filter value in all cases. if the format is not raw it will not be further used anyway
    out->color_filter=in->color_filter;

    // the output YUV byte order must be already set if the buffer is YUV422 at the output
    // if the output is not YUV we don't care about this field.
    // Hence nothing to do.

    // we always convert to 8bits (at this point) we can safely set this value to 8.
    out->data_depth=8;

    // don't know what to do with stride... >>>> TODO: STRIDE SHOULD BE TAKEN INTO ACCOUNT... <<<<
    // out->stride=??

    // the video mode should not change. Color coding and other stuff can be accessed in specific fields of this struct
    out->video_mode = in->video_mode;

    // padding is kept:
    out->padding_bytes = in->padding_bytes;

    // image bytes changes:    >>>> TODO: STRIDE SHOULD BE TAKEN INTO ACCOUNT... <<<<
    dc1394_get_color_coding_bit_size(out->color_coding, &bpp);
    out->image_bytes=(out->size[0]*out->size[1]*bpp)/8;

    // total is image_bytes + padding_bytes
    out->total_bytes = out->image_bytes + out->padding_bytes;

    // bytes-per-packet and packets_per_frame are internal data that can be kept as is.
    out->packet_size  = in->packet_size;
    out->packets_per_frame = in->packets_per_frame;

    // timestamp, frame_behind, id and camera are copied too:
    out->timestamp = in->timestamp;
    out->frames_behind = in->frames_behind;
    out->camera = in->camera;
    out->id = in->id;

    // verify memory allocation:
    if (out->total_bytes>out->allocated_image_bytes) {
        free(out->image);
        out->image=(uint8_t*)malloc(out->total_bytes*sizeof(uint8_t));
        if (out->image)
            out->allocated_image_bytes = out->total_bytes*sizeof(uint8_t);
        else
            out->allocated_image_bytes = 0;
    }

    // Copy padding bytes:
    if(out->image)
        memcpy(&(out->image[out->image_bytes]),&(in->image[in->image_bytes]),out->padding_bytes);

    out->little_endian=0;   // not used before 1.32 is out.
    out->data_in_padding=0; // not used before 1.32 is out.

    if(out->image)
        return DC1394_SUCCESS;
        
    return DC1394_MEMORY_ALLOCATION_FAILURE;

}

dc1394error_t
dc1394_deinterlace_stereo_frames(dc1394video_frame_t *in, dc1394video_frame_t *out, dc1394stereo_method_t method)
{
    dc1394error_t err;

    if ((in->color_coding==DC1394_COLOR_CODING_RAW16)||
        (in->color_coding==DC1394_COLOR_CODING_MONO16)||
        (in->color_coding==DC1394_COLOR_CODING_YUV422)) {
        switch (method) {
        case DC1394_STEREO_METHOD_INTERLACED:
            err=Adapt_buffer_stereo(in,out);
            if(err != DC1394_SUCCESS)
                return err;
                
            return dc1394_deinterlace_stereo(in->image, out->image, out->size[0], out->size[1]);
            break;
            
        case DC1394_STEREO_METHOD_FIELD:
            err=Adapt_buffer_stereo(in,out);
            if (err != DC1394_SUCCESS)
                return err;
                
            memcpy(out->image,in->image,out->image_bytes);
	    return DC1394_SUCCESS;
            break;
        default:
            return DC1394_INVALID_STEREO_METHOD;
        }
// Never reached:  return DC1394_INVALID_STEREO_METHOD;
    }
    else
        return DC1394_FUNCTION_NOT_SUPPORTED;
}
