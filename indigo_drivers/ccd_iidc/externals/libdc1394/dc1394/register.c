/*
 * 1394-Based Digital Camera Control Library
 *
 * Low-level register access functions
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
 */

#include <inttypes.h>
#include "control.h"
#include "internal.h"
#include "offsets.h"
#include "register.h"
#include "utils.h"
#include "config.h"

/* Note: debug modes can be very verbose. */

/* To debug config rom structure: */
//#define DC1394_DEBUG_TAGGED_REGISTER_ACCESS

#define FEATURE_TO_ABS_VALUE_OFFSET(feature, offset)                  \
    {                                                                 \
    if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) )  \
    {                                                                 \
        return DC1394_FAILURE;                                        \
    }                                                                 \
    else if (feature < DC1394_FEATURE_ZOOM)                           \
    {                                                                 \
        offset= REG_CAMERA_FEATURE_ABS_HI_BASE;                       \
        feature-= DC1394_FEATURE_MIN;                                 \
    }                                                                 \
    else                                                              \
    {                                                                 \
        offset= REG_CAMERA_FEATURE_ABS_LO_BASE;                       \
        feature-= DC1394_FEATURE_ZOOM;                                \
                                                                      \
        if (feature >= DC1394_FEATURE_CAPTURE_SIZE)                   \
        {                                                             \
            feature+= 12;                                             \
        }                                                             \
                                                                      \
    }                                                                 \
                                                                      \
    offset+= feature * 0x04U;                                         \
    }


dc1394error_t
dc1394_get_registers (dc1394camera_t *camera, uint64_t offset,
                      uint32_t *value, uint32_t num_regs)
{
    dc1394camera_priv_t * cp = DC1394_CAMERA_PRIV (camera);

    if (camera == NULL)
        return DC1394_CAMERA_NOT_INITIALIZED;

    return cp->platform->dispatch->camera_read (cp->pcam, offset, value,
            num_regs);
}

dc1394error_t
dc1394_set_registers (dc1394camera_t *camera, uint64_t offset,
                      const uint32_t *value, uint32_t num_regs)
{
    dc1394camera_priv_t * cp = DC1394_CAMERA_PRIV (camera);

    if (camera == NULL)
        return DC1394_CAMERA_NOT_INITIALIZED;

    return cp->platform->dispatch->camera_write (cp->pcam, offset, value,
            num_regs);
}


/********************************************************************************/
/* Get/Set Command Registers                                                    */
/********************************************************************************/
dc1394error_t
dc1394_get_control_registers (dc1394camera_t *camera, uint64_t offset,
                              uint32_t *value, uint32_t num_regs)
{
    return dc1394_get_registers (camera,
        camera->command_registers_base + offset, value, num_regs);
}

dc1394error_t
dc1394_set_control_registers (dc1394camera_t *camera, uint64_t offset,
                              const uint32_t *value, uint32_t num_regs)
{
    return dc1394_set_registers (camera,
        camera->command_registers_base + offset, value, num_regs);
}

/********************************************************************************/
/* Get/Set Advanced Features Registers                                          */
/********************************************************************************/
dc1394error_t
dc1394_get_adv_control_registers (dc1394camera_t *camera, uint64_t offset,
                                  uint32_t *value, uint32_t num_regs)
{
    return dc1394_get_registers (camera,
        camera->advanced_features_csr + offset, value, num_regs);
}

dc1394error_t
dc1394_set_adv_control_registers (dc1394camera_t *camera, uint64_t offset,
                                  const uint32_t *value, uint32_t num_regs)
{
    return dc1394_set_registers (camera,
        camera->advanced_features_csr + offset, value, num_regs);
}

/********************************************************************************/
/* Get/Set Format_7 Registers                                                   */
/********************************************************************************/

dc1394error_t
QueryFormat7CSROffset(dc1394camera_t *camera, dc1394video_mode_t mode, uint64_t *offset)
{
    int retval;
    uint32_t temp;

    if (camera == NULL) {
        return DC1394_CAMERA_NOT_INITIALIZED;
    }

    if (!dc1394_is_video_mode_scalable(mode))
        return DC1394_INVALID_VIDEO_FORMAT;

    retval=dc1394_get_control_register(camera, REG_CAMERA_V_CSR_INQ_BASE + ((mode - DC1394_VIDEO_MODE_FORMAT7_MIN) * 0x04U), &temp);
    *offset=temp*4;
    return retval;
}


dc1394error_t
dc1394_get_format7_register(dc1394camera_t *camera, unsigned int mode, uint64_t offset, uint32_t *value)
{
    if (camera == NULL)
        return DC1394_CAMERA_NOT_INITIALIZED;

    if (!dc1394_is_video_mode_scalable(mode))
        return DC1394_INVALID_VIDEO_FORMAT;

    if (camera->format7_csr[mode-DC1394_VIDEO_MODE_FORMAT7_MIN]==0) {
        if (QueryFormat7CSROffset(camera, mode,
                    &camera->format7_csr[mode-DC1394_VIDEO_MODE_FORMAT7_MIN])
                != DC1394_SUCCESS) {
            return DC1394_FAILURE;
        }
    }

    return dc1394_get_registers (camera,
        camera->format7_csr[mode-DC1394_VIDEO_MODE_FORMAT7_MIN]+offset,
        value, 1);
}


dc1394error_t
dc1394_set_format7_register(dc1394camera_t *camera, unsigned int mode, uint64_t offset, uint32_t value)
{
    if (camera == NULL)
        return DC1394_CAMERA_NOT_INITIALIZED;

    if (!dc1394_is_video_mode_scalable(mode))
        return DC1394_INVALID_VIDEO_FORMAT;;

    if (camera->format7_csr[mode-DC1394_VIDEO_MODE_FORMAT7_MIN]==0) {
        QueryFormat7CSROffset(camera, mode,
                &camera->format7_csr[mode-DC1394_VIDEO_MODE_FORMAT7_MIN]);
    }

    return dc1394_set_registers (camera,
        camera->format7_csr[mode-DC1394_VIDEO_MODE_FORMAT7_MIN]+offset,
        &value, 1);
}

/********************************************************************************/
/* Get/Set Absolute Control Registers                                           */
/********************************************************************************/

dc1394error_t
QueryAbsoluteCSROffset(dc1394camera_t *camera, dc1394feature_t feature, uint64_t *offset)
{
    int absoffset, retval;
    uint32_t quadlet=0;

    if (camera == NULL)
        return DC1394_CAMERA_NOT_INITIALIZED;

    FEATURE_TO_ABS_VALUE_OFFSET(feature, absoffset);
    retval=dc1394_get_control_register(camera, absoffset, &quadlet);

    *offset=quadlet * 0x04;
    return retval;

}

dc1394error_t
dc1394_get_absolute_register(dc1394camera_t *camera, unsigned int feature,
        uint64_t offset, uint32_t *value)
{
    uint64_t absoffset;
    if (camera == NULL)
        return DC1394_CAMERA_NOT_INITIALIZED;

    QueryAbsoluteCSROffset(camera, feature, &absoffset);

    return dc1394_get_registers (camera, absoffset + offset, value, 1);
}

dc1394error_t
dc1394_set_absolute_register(dc1394camera_t *camera, unsigned int feature,
        uint64_t offset, uint32_t value)
{
    uint64_t absoffset;
    if (camera == NULL)
        return DC1394_CAMERA_NOT_INITIALIZED;

    QueryAbsoluteCSROffset(camera, feature, &absoffset);

    return dc1394_set_registers (camera, absoffset + offset, &value, 1);
}

/********************************************************************************/
/* Get/Set PIO Feature Registers                                                */
/********************************************************************************/

dc1394error_t
dc1394_get_PIO_register(dc1394camera_t *camera, uint64_t offset, uint32_t *value)
{
    return dc1394_get_registers (camera, camera->PIO_control_csr + offset,
            value, 1);
}

dc1394error_t
dc1394_set_PIO_register(dc1394camera_t *camera, uint64_t offset, uint32_t value)
{
    return dc1394_set_registers (camera, camera->PIO_control_csr + offset,
            &value, 1);
}


/********************************************************************************/
/* Get/Set SIO Feature Registers                                                */
/********************************************************************************/

dc1394error_t
dc1394_get_SIO_register(dc1394camera_t *camera, uint64_t offset, uint32_t *value)
{
    return dc1394_get_registers (camera, camera->SIO_control_csr + offset,
            value, 1);
}

dc1394error_t
dc1394_set_SIO_register(dc1394camera_t *camera, uint64_t offset, uint32_t value)
{
    return dc1394_set_registers (camera, camera->SIO_control_csr + offset,
            &value, 1);
}


/********************************************************************************/
/* Get/Set Strobe Feature Registers                                             */
/********************************************************************************/
dc1394error_t
dc1394_get_strobe_register(dc1394camera_t *camera, uint64_t offset,
        uint32_t *value)
{
    return dc1394_get_registers (camera, camera->strobe_control_csr + offset,
            value, 1);
}

dc1394error_t
dc1394_set_strobe_register(dc1394camera_t *camera, uint64_t offset,
        uint32_t value)
{
    return dc1394_set_registers (camera, camera->strobe_control_csr + offset,
            &value, 1);
}
