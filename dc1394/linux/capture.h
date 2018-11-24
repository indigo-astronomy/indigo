/*
 * 1394-Based Digital Camera Control Library
 *
 * Camera Capture headers for Linux
 *  
 * Written by
 *   Chris Urmson <curmson@ri.cmu.edu>
 *   Damien Douxchamps <ddouxchamps@users.sf.net>
 *   Dan Dennedy <ddennedy@users.sf.net>
 *   David Moore <dcm@acm.org>
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

#ifndef __DC1394_CAPTURE_LINUX_H__
#define __DC1394_CAPTURE_LINUX_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Set the DMA device filename manually. In most cases this is not necessary because the capture
   functions probe common filenames such as /dev/video1394/x or /dev/video1394. */
dc1394error_t dc1394_capture_set_device_filename(dc1394camera_t* camera, char *filename);

#ifdef __cplusplus
}
#endif

#endif
