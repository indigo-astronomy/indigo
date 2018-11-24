/**************************************************************
	Celestron NexStar compatible telescope control library
	
	(C)2013-2016 by Rumen G.Bogdanovski
***************************************************************/
#if !defined(__NEXSTAR_H)
#define __NEXSTAR_H

#define NX_TC_TRACK_OFF      0
#define NX_TC_TRACK_ALT_AZ   1
#define NX_TC_TRACK_EQ_NORTH 2
#define NX_TC_TRACK_EQ_SOUTH 3

#define SW_TC_TRACK_OFF      0
#define SW_TC_TRACK_ALT_AZ   1
#define SW_TC_TRACK_EQ       2
#define SW_TC_TRACK_EQ_PEC   3

#define TC_TRACK_OFF         0
#define TC_TRACK_ALT_AZ      1
#define TC_TRACK_EQ_NORTH    2
#define TC_TRACK_EQ_SOUTH    3
#define TC_TRACK_EQ          4
#define TC_TRACK_EQ_PEC      5
	
#define TC_DIR_POSITIVE 1
#define TC_DIR_NEGATIVE 0
	
#define TC_AXIS_RA_AZM 1
#define TC_AXIS_DE_ALT 0

#define TC_AXIS_RA TC_AXIS_RA_AZM
#define TC_AXIS_DE TC_AXIS_DE_ALT

#define TC_AXIS_AZM TC_AXIS_RA_AZM
#define TC_AXIS_ALT TC_AXIS_DE_ALT

#define _TC_DIR_POSITIVE 6
#define _TC_DIR_NEGATIVE 7
	
#define _TC_AXIS_RA_AZM 16
#define _TC_AXIS_DE_ALT 17

/* return codes */
#define RC_OK            0	/* success */
#define RC_FAILED      (-1)	/* general error */
#define RC_PARAMS      (-2)	/* invalid parameters */
#define RC_DEVICE      (-3)	/* no responce from the device */
#define RC_DATA        (-4)	/* invalid data */
#define RC_UNSUPPORTED (-5) /* Unsupported command */

#define DEG2RAD (3.1415926535897932384626433832795/180.0)
#define RAD2DEG (180.0/3.1415926535897932384626433832795)

/* Supported NexStar protocol versions */
#define VER_1_2  0x10200
#define VER_1_6  0x10600
#define VER_2_2  0x20200
#define VER_2_3  0x20300
#define VER_3_1  0x30100
#define VER_4_10 0x40A00
#define VER_3_37_8 0x32508
#define VER_4_37_8 0x42508
/* All protocol versions */
#define VER_AUX  0xFFFFFF
#define VER_AUTO 0x0

#define VNDR_CELESTRON  0x1
#define VNDR_SKYWATCHER 0x2
#define VNDR_ALL_SUPPORTED (VNDR_CELESTRON | VNDR_SKYWATCHER)

/* There is no way to tell SkyWatcher from Celestron. Unfortunately both share the
   same IDs and some Celestron mounts have RTC wile SW does not. That is why the user
   should decide. Set nexstar_use_rtc to 1 to enable using the RTC on the mounts that have RTC. */
extern int nexstar_use_rtc;

extern int nexstar_proto_version;
/* version check macros */
#define RELEASE_MASK  0xFF0000
#define REVISION_MASK 0x00FF00
#define PATCH_MASK    0x0000FF

#define GET_RELEASE(ver)  ((ver & RELEASE_MASK)>>16)
#define GET_REVISION(ver) ((ver & REVISION_MASK)>>8)
#define GET_PATCH(ver)    (ver & PATCH_MASK)

#define REQUIRE_VER(req_ver)      { if(req_ver > nexstar_proto_version) return RC_UNSUPPORTED; }
#define REQUIRE_RELEASE(req_ver)  { if ((req_ver) > GET_RELEASE(nexstar_proto_version)) return RC_UNSUPPORTED; }
#define REQUIRE_REVISION(req_ver) { if ((req_ver) > GET_REVISION(nexstar_proto_version)) return RC_UNSUPPORTED; }
#define REQUIRE_PATCH(req_ver)    { if ((req_ver) > GET_PATCH(nexstar_proto_version)) return RC_UNSUPPORTED; }

extern int nexstar_mount_vendor;
/* vendor check macros */
#define REQUIRE_VENDOR(req_vendor) { if(!(nexstar_mount_vendor & req_vendor)) return RC_UNSUPPORTED; }
#define VENDOR(vendor) (nexstar_mount_vendor & vendor)
#define VENDOR_IS(vendor) (nexstar_mount_vendor == vendor)

#include <time.h>
#include <unistd.h>

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

/* Telescope communication */
int open_telescope(char *dev_file);
int close_telescope(int dev_fd);
int enforce_protocol_version(int devfd, int ver);
#define write_telescope(dev_fd, buf, size) (write(dev_fd, buf, size))

int _read_telescope(int devfd, char *reply, int len, char vl);
#define read_telescope(devfd, reply, len) (_read_telescope(devfd, reply, len, 0))
#define read_telescope_vl(devfd, reply, len) (_read_telescope(devfd, reply, len, 1))

int guess_mount_vendor(int dev);
int enforce_vendor_protocol(int vendor);

/* Telescope commands */
int tc_check_align(int dev);
int tc_get_orientation(int dev);
int tc_goto_in_progress(int dev);
int tc_goto_cancel(int dev);
int tc_echo(int dev, char ch);
int tc_get_model(int dev);
int tc_get_version(int dev, char *major, char *minor);
int tc_get_tracking_mode(int dev);
int tc_set_tracking_mode(int dev, char mode);

int _tc_get_rade(int dev, double *ra, double *de, char precise);
#define tc_get_rade(dev, ra, de) (_tc_get_rade(dev, ra, de, 0))
#define tc_get_rade_p(dev, ra, de) (_tc_get_rade(dev, ra, de, 1))

int _tc_get_azalt(int dev, double *az, double *alt, char precise);
#define tc_get_azalt(dev, az, alt) (_tc_get_azalt(dev, az, alt, 0))
#define tc_get_azalt_p(dev, az, alt) (_tc_get_azalt(dev, az, alt, 1))

int _tc_goto_rade(int dev, double ra, double de, char precise);
#define tc_goto_rade(dev, ra, de) (_tc_goto_rade(dev, ra, de, 0))
#define tc_goto_rade_p(dev, ra, de) (_tc_goto_rade(dev, ra, de, 1))

int _tc_goto_azalt(int dev, double az, double alt, char precise);
#define tc_goto_azalt(dev, az, alt) (_tc_goto_azalt(dev, az, alt, 0))
#define tc_goto_azalt_p(dev, az, alt) (_tc_goto_azalt(dev, az, alt, 1))

int _tc_sync_rade(int dev, double ra, double de, char precise);
#define tc_sync_rade(dev, ra, de) (_tc_sync_rade(dev, ra, de, 0))
#define tc_sync_rade_p(dev, ra, de) (_tc_sync_rade(dev, ra, de, 1))

int tc_get_location(int dev, double *lon, double *lat);
int tc_set_location(int dev, double lon, double lat);

time_t tc_get_time(int dev, time_t *ttime, int *tz, int *dst);
int tc_set_time(char dev, time_t ttime, int tz, int dst);

int tc_slew_fixed(int dev, char axis, char direction, char rate);
int tc_slew_variable(int dev, char axis, char direction, float rate);

char *get_model_name(int id, char *name, int len);

/* Reverse engineered commands */
/*
int tc_get_guide_rate();
int tc_set_guide_rate_fixed();
int tc_set_guide_rate();
*/
int tc_get_autoguide_rate(int dev, char axis);
int tc_set_autoguide_rate(int dev, char axis, char rate);
int tc_get_backlash(int dev, char axis, char direction);
int tc_set_backlash(int dev, char axis, char direction, char backlash);
int tc_pass_through_cmd(int dev, char msg_len, char dest_id, char cmd_id,
                        char data1, char data2, char data3, char res_len, char *response);
/* End of reverse engineered commands */

/* nextar turns <=> degrees conversion */
int pnex2dd(char *nex, double *d1, double *d2);
int nex2dd(char *nex, double *d1, double *d2);
int dd2nex(double d1, double d2, char *nex);
int dd2pnex(double d1, double d2, char *nex);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __NEXSTAR_H */
