/*
 * Capture program prototype for the spherical 6-CCD Ladybug
 *     camera from Point Grey
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
 * Notes:
 *
 * - Uses the first camera on the bus, then setup the camera
 *   in JPEG mode (mode 7), and capture NFRAMES frames to the HDD.
 * 
 * - the image size has to be set so that the 6 sub-images,
 *   encoded in JPEG, will fit in the total RAW image size.
 *   If less than 6 frames are written you should use a larger
 *   HEIGHT. Also, each color field is saved in an individual
 *   frame. This results in 24 images (512x384) being written
 *   for each (future) hemispherical image.
 *
 * Easy adaptation include:
 * - using 1394a instead of 1394b
 * - using RAW instead of JPEG
 * 
 * For more information have a look at the format specs in
 * "Ladybug Stream File Specification" that is included in the
 * Ladybug SDK.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include <dc1394/dc1394.h>

// change this to switch from RAW to JPEG
#define VIDEO_MODE DC1394_VIDEO_MODE_FORMAT7_7
// number of frames to be recorded. The total number of files written to disk will be 24xNFRAMES.
#define NFRAMES 10
// file basename
#define BASENAME "~/test"

int
main(int argn, char **argv)
{

    dc1394error_t err;
    dc1394camera_t *camera;
    dc1394video_frame_t *frame;
    char filename[256];

    FILE *fd;

    dc1394_t * d;
    dc1394camera_list_t * list;

    d = dc1394_new ();
    if (!d)
        return 1;
    err=dc1394_camera_enumerate (d, &list);
    DC1394_ERR_RTN(err,"Failed to enumerate cameras");

    if (list->num == 0) {
        dc1394_log_error("No cameras found");
        return 1;
    }

    camera = dc1394_camera_new (d, list->ids[0].guid);
    if (!camera) {
        dc1394_log_error("Failed to initialize camera with guid %llx",list->ids[0].guid);
        return 1;
    }
    dc1394_camera_free_list (list);
    printf("Using camera \"%s %s\"\n",camera->vendor,camera->model);

    // setup video mode, etc...
    err=dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_1394B);
    DC1394_ERR_RTN(err,"Could not set B mode");
    err=dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_800);
    DC1394_ERR_RTN(err,"Could not set 800Mbps speed");
    err=dc1394_video_set_mode(camera, VIDEO_MODE);
    DC1394_ERR_RTN(err,"Could not set video mode");
    err=dc1394_format7_set_roi(camera, VIDEO_MODE, DC1394_COLOR_CODING_MONO8, 2000, 0,0, 512, 2015);
    DC1394_ERR_RTN(err,"Could not set ROI");

    // setup capture
    err=dc1394_capture_setup(camera, 10, DC1394_CAPTURE_FLAGS_DEFAULT);
    DC1394_ERR_RTN(err,"Could not setup capture");
    err=dc1394_video_set_transmission(camera, DC1394_ON);
    DC1394_ERR_RTN(err,"Could not start transmission");

    int cam, k, i=0;
    unsigned int jpgadr, jpgsize, adr;

    while (i<NFRAMES) {
        // capture frame
        err=dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
        DC1394_ERR_RTN(err,"Could not dequeue a frame");
        // do something with the image
        for (cam=0;cam<6;cam++) {
            for (k=0;k<4;k++) {
                adr=0x340+(5-cam)*32+(3-k)*8;
                jpgadr=(((unsigned int)*(frame->image+adr))<<24)+
                    (((unsigned int)*(frame->image+adr+1))<<16)+
                    (((unsigned int)*(frame->image+adr+2))<<8)+
                    (((unsigned int)*(frame->image+adr+3)));
                adr+=4;
                jpgsize=(((unsigned int)*(frame->image+adr))<<24)+
                    (((unsigned int)*(frame->image+adr+1))<<16)+
                    (((unsigned int)*(frame->image+adr+2))<<8)+
                    (((unsigned int)*(frame->image+adr+3)));

                if (jpgsize!=0) {
                    sprintf(filename,"%s-%05d-%d-%d.jpg",BASENAME,i,cam,k);
                    fd=fopen(filename,"w");
                    fwrite((unsigned char *)(jpgadr+frame->image),jpgsize,1,fd);
                    fclose(fd);
                }
            }
        }
        sprintf(filename,"%s-%05d.raw",BASENAME,i);
        fd=fopen(filename,"w");
        fwrite(frame->image,frame->total_bytes,1,fd);
        fclose(fd);
        // release frame
        err=dc1394_capture_enqueue(camera, frame);
        DC1394_ERR_RTN(err,"Could not enqueue a frame");
        fprintf(stderr,"%d\r",i);
        i++;
    }

    // stop capture
    err=dc1394_video_set_transmission(camera, DC1394_OFF);
    DC1394_ERR_RTN(err,"Could not stop transmission");
    err=dc1394_capture_stop(camera);
    DC1394_ERR_RTN(err,"Could not stop capture");
    dc1394_camera_free (camera);
    dc1394_free (d);
    return 0;
}
