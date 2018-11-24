/*
 * Grab an image using libdc1394
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
 * Description:
 *
 *  An extension to the original grab image sample program.  This demonstrates
 *  how to collect more detailed information on the various modes of the
 *  camera, convert one image format to another, and waiting so that the
 *  camera has time to capture the image before trying to write it out.
 */


#include <stdio.h>
#include <stdint.h>
#include <dc1394/dc1394.h>
#include <stdlib.h>
#include <inttypes.h>


#define IMAGE_FILE_NAME "ImageRGB.ppm"

/*-----------------------------------------------------------------------
 *  Prints the type of format to standard out
 *-----------------------------------------------------------------------*/
void print_format( uint32_t format )
{
#define print_case(A) case A: printf(#A ""); break;

    switch( format ) {
        print_case(DC1394_VIDEO_MODE_160x120_YUV444);
        print_case(DC1394_VIDEO_MODE_320x240_YUV422);
        print_case(DC1394_VIDEO_MODE_640x480_YUV411);
        print_case(DC1394_VIDEO_MODE_640x480_YUV422);
        print_case(DC1394_VIDEO_MODE_640x480_RGB8);
        print_case(DC1394_VIDEO_MODE_640x480_MONO8);
        print_case(DC1394_VIDEO_MODE_640x480_MONO16);
        print_case(DC1394_VIDEO_MODE_800x600_YUV422);
        print_case(DC1394_VIDEO_MODE_800x600_RGB8);
        print_case(DC1394_VIDEO_MODE_800x600_MONO8);
        print_case(DC1394_VIDEO_MODE_1024x768_YUV422);
        print_case(DC1394_VIDEO_MODE_1024x768_RGB8);
        print_case(DC1394_VIDEO_MODE_1024x768_MONO8);
        print_case(DC1394_VIDEO_MODE_800x600_MONO16);
        print_case(DC1394_VIDEO_MODE_1024x768_MONO16);
        print_case(DC1394_VIDEO_MODE_1280x960_YUV422);
        print_case(DC1394_VIDEO_MODE_1280x960_RGB8);
        print_case(DC1394_VIDEO_MODE_1280x960_MONO8);
        print_case(DC1394_VIDEO_MODE_1600x1200_YUV422);
        print_case(DC1394_VIDEO_MODE_1600x1200_RGB8);
        print_case(DC1394_VIDEO_MODE_1600x1200_MONO8);
        print_case(DC1394_VIDEO_MODE_1280x960_MONO16);
        print_case(DC1394_VIDEO_MODE_1600x1200_MONO16);

    default:
        dc1394_log_error("Unknown format\n");
        exit(1);
    }

}

/*-----------------------------------------------------------------------
 *  Returns the number of pixels in the image based upon the format
 *-----------------------------------------------------------------------*/
uint32_t get_num_pixels(dc1394camera_t *camera, uint32_t format ) {
    uint32_t w,h;

    dc1394_get_image_size_from_video_mode(camera, format,&w,&h);

    return w*h;
}

/*-----------------------------------------------------------------------
 *  Prints the type of color encoding
 *-----------------------------------------------------------------------*/
void print_color_coding( uint32_t color_id )
{
    switch( color_id ) {
    case DC1394_COLOR_CODING_MONO8:
        printf("MONO8");
        break;
    case DC1394_COLOR_CODING_YUV411:
        printf("YUV411");
        break;
    case DC1394_COLOR_CODING_YUV422:
        printf("YUV422");
        break;
    case DC1394_COLOR_CODING_YUV444:
        printf("YUV444");
        break;
    case DC1394_COLOR_CODING_RGB8:
        printf("RGB8");
        break;
    case DC1394_COLOR_CODING_MONO16:
        printf("MONO16");
        break;
    case DC1394_COLOR_CODING_RGB16:
        printf("RGB16");
        break;
    case DC1394_COLOR_CODING_MONO16S:
        printf("MONO16S");
        break;
    case DC1394_COLOR_CODING_RGB16S:
        printf("RGB16S");
        break;
    case DC1394_COLOR_CODING_RAW8:
        printf("RAW8");
        break;
    case DC1394_COLOR_CODING_RAW16:
        printf("RAW16");
        break;
    default:
        dc1394_log_error("Unknown color coding = %d\n",color_id);
        exit(1);
    }
}

/*-----------------------------------------------------------------------
 *  Prints various information about the mode the camera is in
 *-----------------------------------------------------------------------*/
void print_mode_info( dc1394camera_t *camera , uint32_t mode )
{
    int j;

    printf("Mode: ");
    print_format(mode);
    printf("\n");

    dc1394framerates_t framerates;
    dc1394error_t err;
    err=dc1394_video_get_supported_framerates(camera,mode,&framerates);
    DC1394_ERR(err,"Could not get frame rates");

    printf("Frame Rates:\n");
    for( j = 0; j < framerates.num; j++ ) {
        uint32_t rate = framerates.framerates[j];
        float f_rate;
        dc1394_framerate_as_float(rate,&f_rate);
        printf("  [%d] rate = %f\n",j,f_rate );
    }

}

/*-----------------------------------------------------------------------
 *  Releases the cameras and exits
 *-----------------------------------------------------------------------*/
void cleanup_and_exit(dc1394camera_t *camera)
{
    dc1394_video_set_transmission(camera, DC1394_OFF);
    dc1394_capture_stop(camera);
    dc1394_camera_free(camera);
    exit(1);
}

int main(int argc, char *argv[])
{
    FILE* imagefile;
    dc1394camera_t *camera;
    unsigned int width, height;
    dc1394video_frame_t *frame=NULL;
    dc1394_t * d;
    dc1394camera_list_t * list;

    dc1394error_t err;

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
        dc1394_log_error("Failed to initialize camera with guid %llx", list->ids[0].guid);
        return 1;
    }
    dc1394_camera_free_list (list);

    printf("Using camera with GUID %"PRIx64"\n", camera->guid);

    dc1394video_modes_t modes;

    /*-----------------------------------------------------------------------
     *  list Capture Modes
     *-----------------------------------------------------------------------*/
    err=dc1394_video_get_supported_modes(camera, &modes);
    DC1394_ERR_RTN(err,"Could not get list of modes");

    uint32_t selected_mode = modes.modes[modes.num-1];

    /*-----------------------------------------------------------------------
     *  setup capture
     *-----------------------------------------------------------------------*/

    err=dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set iso speed");

    err=dc1394_video_set_mode(camera, selected_mode);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set video mode\n");

    err=dc1394_video_set_framerate(camera, DC1394_FRAMERATE_7_5);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set framerate\n");

    err=dc1394_capture_setup(camera,4, DC1394_CAPTURE_FLAGS_DEFAULT);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not setup camera-\nmake sure that the video mode and framerate are\nsupported by your camera\n");

    /*-----------------------------------------------------------------------
     *  have the camera start sending us data
     *-----------------------------------------------------------------------*/
    err=dc1394_video_set_transmission(camera, DC1394_ON);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not start camera iso transmission\n");

    /*-----------------------------------------------------------------------
     *  capture one frame
     *-----------------------------------------------------------------------*/
    err=dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not capture a frame\n");

    /*-----------------------------------------------------------------------
     *  stop data transmission
     *-----------------------------------------------------------------------*/
    err=dc1394_video_set_transmission(camera,DC1394_OFF);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not stop the camera?\n");

    /*-----------------------------------------------------------------------
     *  convert the image from what ever format it is to its RGB8
     *-----------------------------------------------------------------------*/

    dc1394_get_image_size_from_video_mode(camera, selected_mode, &width, &height);
    uint64_t numPixels = height*width;

    dc1394video_frame_t *new_frame=calloc(1,sizeof(dc1394video_frame_t));
    new_frame->color_coding=DC1394_COLOR_CODING_RGB8;
    dc1394_convert_frames(frame, new_frame);

    /*-----------------------------------------------------------------------
     *  save image as 'Image.pgm'
     *-----------------------------------------------------------------------*/
    imagefile=fopen(IMAGE_FILE_NAME, "wb");

    if( imagefile == NULL) {
        perror( "Can't create '" IMAGE_FILE_NAME "'");
        cleanup_and_exit(camera);
    }


    fprintf(imagefile,"P6\n%u %u\n255\n", width, height);
    fwrite((const char *)new_frame->image, 1,numPixels*3, imagefile);
    fclose(imagefile);
    printf("wrote: " IMAGE_FILE_NAME "\n");

    /*-----------------------------------------------------------------------
     *  close camera
     *-----------------------------------------------------------------------*/
    free(new_frame->image);
    free(new_frame);
    dc1394_video_set_transmission(camera, DC1394_OFF);
    dc1394_capture_stop(camera);
    dc1394_camera_free(camera);
    dc1394_free (d);

    return 0;
}
