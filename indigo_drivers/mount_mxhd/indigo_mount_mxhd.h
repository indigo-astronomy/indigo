#ifndef indigo_mount_mxhd_h
#define indigo_mount_mxhd_h

#include <indigo/indigo_driver.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_guider_driver.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOUNT_MXHD_NAME "MX-HD Mount"
#define MOUNT_MXHD_GUIDER_NAME "MX-HD Mount (guider)"

extern indigo_result indigo_mount_mxhd(indigo_driver_action action, indigo_driver_info *info);

#ifdef __cplusplus
}
#endif

#endif
