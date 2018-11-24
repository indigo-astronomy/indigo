/*
 * Get one image using libdc1394 using the DMA interface (viedo1394).
 *    Nothing is done with the image.
 *
 * Written by Damien Douxchamps <ddouxchamps@users.sf.net>
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
 *
 * Description: This is a minimalist program that doesn't do everything
 *              you should do, like cleaning up in case of failure. The
 *              program uses the current setup of your camera and should
 *              (but may not) work right after a camera boot.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <dc1394/dc1394.h>

int main(int argc, char *argv[])
{
    dc1394camera_t * camera;
    dc1394error_t err;
    dc1394video_frame_t * frame;
    dc1394_t * d;
    dc1394camera_list_t * list;

    d = dc1394_new ();                                                     /* Initialize libdc1394 */
    if (!d)
        return 1;

    err=dc1394_camera_enumerate (d, &list);                                /* Find cameras */
    DC1394_ERR_RTN(err,"Failed to enumerate cameras");

    if (list->num == 0) {                                                  /* Verify that we have at least one camera */
        dc1394_log_error("No cameras found");
        return 1;
    }

    camera = dc1394_camera_new (d, list->ids[0].guid);                     /* Work with first camera */
    if (!camera) {
        dc1394_log_error("Failed to initialize camera with guid %llx", list->ids[0].guid);
        return 1;
    }
    dc1394_camera_free_list (list);

    err=dc1394_capture_setup(camera, 4, DC1394_CAPTURE_FLAGS_DEFAULT);     /* Setup capture */

    err=dc1394_video_set_transmission(camera, DC1394_ON);                  /* Start transmission */
    
    err=dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);/* Capture */
    DC1394_ERR_RTN(err,"Problem getting an image");

    err=dc1394_capture_enqueue(camera, frame);                             /* Release the buffer */

    err=dc1394_video_set_transmission(camera, DC1394_OFF);                 /* Stop transmission */

    err=dc1394_capture_stop(camera);                                       /* Stop capture */

    printf("Hello World\n");                                               /* Hey, this is a HELLO WORLD program!! */

    dc1394_camera_free (camera);                                           /* cleanup and exit */
    dc1394_free (d);
    return 0;
}
