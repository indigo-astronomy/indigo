/*
 *     Performance evaluation demo program for libdc1394
 *
 *     (c) 2011 Cyberdyne Inc., by Damien Douxchamps
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <dc1394/dc1394.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <sys/time.h>

#define HALF_RESOLUTION

#ifdef HALF_RESOLUTION
#define SX 376
#define SY 240
#define PKT 1504
#define MODE DC1394_VIDEO_MODE_FORMAT7_1
#define DEF_BIN_MAX  125.0
#define DEF_BIN_MIN  109.0
#else
#define SX 752
#define SY 480
#define PKT 3008
#define MODE DC1394_VIDEO_MODE_FORMAT7_0
#define DEF_BIN_MAX  66.0
#define DEF_BIN_MIN  50.0
#endif

#define DEF_BIN_N    8

#define STABILIZING_TIME 1.0


double fps_min, fps_max;
unsigned int *bins = NULL;
unsigned int nframes=0;
float bin_max, bin_min;
int bin_n;
dc1394_t *d;
dc1394camera_t *camera;
SDL_Surface *video;

typedef struct {
    struct timeval rawtime_new;
    struct timeval rawtime_old;
} TimeLapse_t;

void
DisplayStatsAndExit(int sig)
{
    int i;
    fprintf(stderr,"\nFramerate statistics:\n");
    fprintf(stderr,"bin centers:\t[");
    for (i=0;i<bin_n;i++) {
        if (i==0)
            fprintf(stderr,"<");
        fprintf(stderr,"%3.1f",(float)bin_min+(float)(bin_max-bin_min)/(float)bin_n*((float)i+.5));
        if (i==bin_n-1)
            fprintf(stderr,">");
        if (i<bin_n)
            fprintf(stderr,"\t");
    }
    fprintf(stderr,"]\n");
    fprintf(stderr,"histogram:\t[");
    for (i=0;i<bin_n;i++) {
        if (bins[i]!=0)
            fprintf(stderr,"%d",bins[i]);
        else
            fprintf(stderr,"    ");
        if (i<bin_n)
            fprintf(stderr,"\t");
    }
    fprintf(stderr,"]\n");
    fprintf(stderr,"percentage:\t[");
    for (i=0;i<bin_n;i++) {
        if (bins[i]!=0)
            fprintf(stderr,"%3.1f",(float)bins[i]/nframes*100);
        else
            fprintf(stderr,"    ");
        if (i<bin_n)
            fprintf(stderr,"\t");
    }
    fprintf(stderr,"]\n");
    free(bins);

    SDL_Quit();

    dc1394_video_set_transmission(camera, DC1394_OFF);
    dc1394_capture_stop(camera);
    dc1394_camera_free (camera);
    dc1394_free(d);

    exit(0);
}

SDL_Surface*
SDL_Start_GL(int width, int height)
{
    // Slightly different SDL initialization
    if ( SDL_Init(SDL_INIT_VIDEO) != 0 ) {
        printf("SDL error: %s\n", SDL_GetError());
        return NULL;
    }

    // set window title:
    SDL_WM_SetCaption("Camera live output","");

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 0);

    video = SDL_SetVideoMode( width, height, 16, SDL_OPENGL | SDL_HWSURFACE); // *changed*
    if ( !video ) {
        printf("SDL error: %s\n", SDL_GetError());
        return NULL;
    }

    // Setup OpenGL
    glClearColor( 0, 0, 0, 0 );
    glEnable( GL_TEXTURE_2D ); // Need this to display a texture
    glViewport( 0, 0, width, height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( 0, width, height, 0, -10000, 10000);
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    SDL_ShowCursor(0);

    return(video);
}

// Reset the interval timer
void
ResetTimer(TimeLapse_t *timer)
{
    gettimeofday(&timer->rawtime_old, NULL);
}

// Return the time lapse in seconds between the last call to the same function or to ResetTimer
double
GetTimeLapseSec(TimeLapse_t *timer)
{
    double interval;

    gettimeofday(&timer->rawtime_new, NULL);

    interval = timer->rawtime_new.tv_sec-timer->rawtime_old.tv_sec + (float)(timer->rawtime_new.tv_usec-timer->rawtime_old.tv_usec)/1000000.0;

    timer->rawtime_old.tv_sec  = timer->rawtime_new.tv_sec;
    timer->rawtime_old.tv_usec = timer->rawtime_new.tv_usec;

    return interval;
}

int main(int argc, char *argv[])
{
    double lapse=0;
    double fps;
    double ntime=0, ptime=0, tmp;
    int index;
    dc1394video_frame_t *frame;
    dc1394error_t err;
    GLuint texture;
    TimeLapse_t timer;
    int cam_id=0;

    if ( argc!=1 && argc!=2) {
        fprintf(stderr,"Usage:\n\tdc1394_iso <optional camera_ID>\n");
        fprintf(stderr,"\t\tDefault values for histogram are min=%.0f, max=%.0f and %d bins\n\n",DEF_BIN_MIN, DEF_BIN_MAX, (int)DEF_BIN_N);
        exit(0);
    }

    signal(SIGINT, DisplayStatsAndExit);

    if (argc==2)
        cam_id=atoi(argv[1]);

    bin_n=DEF_BIN_N;
    bin_max=DEF_BIN_MAX;
    bin_min=DEF_BIN_MIN;
    bins = calloc(bin_n,sizeof(unsigned int));

    // init 1394
    d = dc1394_new();
    if (!d) return 1;
    // find cameras
    dc1394camera_list_t * list;
    err=dc1394_camera_enumerate (d, &list);
    DC1394_ERR_RTN(err,"Failed to enumerate cameras");
    if (list->num == 0) {
        fprintf(stderr,"No cameras found\n");
        return 1;
    }
    else {
        if (list->num<cam_id+1) {
            fprintf(stderr,"Not enough cameras found for id\n");
            return 1;
        }
    }
    // use selected camera
    camera = dc1394_camera_new (d, list->ids[cam_id].guid);
    if (!camera) {
        dc1394_log_error("Failed to initialize camera with guid %llx", list->ids[cam_id].guid);
        return 1;
    }
    dc1394_camera_free_list (list);
    // setup video format
    dc1394_video_set_transmission(camera, DC1394_OFF); // just in case
    dc1394_video_set_mode(camera, MODE);
    dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
    err=dc1394_format7_set_roi(camera, MODE, DC1394_COLOR_CODING_MONO8, PKT, 0, 0, SX, SY);
    if (err!=DC1394_SUCCESS) {
        fprintf(stderr,"Could not set ROI!\n");
        return 0;
    }
    // setup catpure
    err=dc1394_capture_setup(camera, 10, DC1394_CAPTURE_FLAGS_DEFAULT);
    if (err!=DC1394_SUCCESS) {
        fprintf(stderr,"Could not set capture!\n");
        return 0;
    }

    fps_min=9e6;
    fps_max=0;

    fprintf(stderr,"\t\t\t\t------------- FPS --------------        \n");
    fprintf(stderr,"Time:\t\tFrames:\t\tInst:\tMin:\tMax:\tAvg:\n");
    fprintf(stderr,"Stabilizing...\r");
    unsigned int stabilized=0;

    video=SDL_Start_GL(SX, SY);

    // start image transmission
    dc1394_video_set_transmission(camera, DC1394_ON);

    ResetTimer(&timer);

    // ----- ready to roll on all images! -----
    int quit = 0;
    while (!quit) {
        // capture image
        err=dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);/* Capture */
        DC1394_ERR_RTN(err,"Problem getting an image");

        // display image
        glGenTextures( 1, &texture);
        glBindTexture( GL_TEXTURE_2D, texture);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE8, SX, SY, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->image );

        // return buffer
        dc1394_capture_enqueue(camera, frame);

        glBegin( GL_QUADS );
        {
            glTexCoord2f(0, 0);
            glVertex3f(0, 0, 1 );
            glTexCoord2f(0, 1);
            glVertex3f(0, SY, 1 );
            glTexCoord2f(1, 1);
            glVertex3f(SX, SY, 1 );
            glTexCoord2f(1, 0);
            glVertex3f(SX, 0, 1 );
        }
        glEnd();
        SDL_GL_SwapBuffers();
        glDeleteTextures(1, &texture);

        if (ntime>0.1) // if ntime is valid...
            ptime=ntime;
        ntime=(double)frame->timestamp/1000000.0;

        // display the framerate
        nframes++;
        fps=1.0/(ntime-ptime+1e-20);
        tmp=GetTimeLapseSec(&timer);
        lapse+=tmp;
        if (lapse>STABILIZING_TIME && stabilized==0) {
            stabilized=1;
            lapse=tmp;
            nframes=1;
            fprintf(stderr,"                                       \r");
        }

        if (stabilized>0) {

            if (nframes>1) {
                fprintf(stderr,"%9.3f\t%7d   \t%-3.2f \t%3.2f \t%3.2f \t%3.3f     \r", lapse, nframes, fps, fps_min, fps_max, ((float)nframes)/(lapse+1e-10));
            }
            index=(int)((fps-bin_min)/((double)(bin_max-bin_min)/(double)bin_n));

            if (index>bin_n-1)
                index=bin_n-1;
            if (index<0)
                index=0;
            bins[index]++;
            if (fps_min>fps)
                fps_min=fps;
            if (fps_max<fps)
                fps_max=fps;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                quit = 1;
                break;
            }
        }
    } // --------------- END OF BIG IMAGE LOOP -------------------

    DisplayStatsAndExit(0);

    return 0;
}
