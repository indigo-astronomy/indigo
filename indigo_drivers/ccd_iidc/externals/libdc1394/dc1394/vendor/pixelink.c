/*
 * 1394-Based Digital Camera Control Library
 *
 * Pixelink (PxL) specific extensions for Multi-camera control.
 * 
 * Written by
 *     Aravind Sundaresan <a.sundaresan@gmail.com>
 *     James Sherman <shermanj@umd.edu>
 *
 * Copyright (C) 2006 Tobii Technology AB, Stockholm Sweden
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

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_WINDOWS
#include <windows.h>
#else
#include <arpa/inet.h>
#endif

#include "vendor/pixelink.h"
#include "log.h"

/******************************************************************************
 * Function to convert float to quadlet (32 bit floating point representation)
 */
dc1394error_t
dc1394_pxl_convert_float32_to_quadlet(double d, uint32_t *i)
{
    dc1394error_t retval;

    float32_t f = d;
    memcpy((void *)i, (void *)&f, sizeof(uint32_t));

    if (sizeof(uint32_t) == sizeof(float32_t)) {
        retval = DC1394_SUCCESS;
    }
    else {
        retval = DC1394_FAILURE;
    }

    return retval;
}

/******************************************************************************
 * Function to convert uint32_t (32 bit float representation) to float
 */
dc1394error_t
dc1394_pxl_convert_uint32_to_float32(uint32_t i, double *d)
{
    dc1394error_t retval;

    float32_t f;
    memcpy((void *)&f, (void *)&i, sizeof(float32_t));

    *d = (double) f;
    if (sizeof(uint32_t) == sizeof(float32_t)) {
        retval = DC1394_SUCCESS;
    }
    else {
        retval = DC1394_FAILURE;
    }

    return retval;
}

/******************************************************************************
 * Function to read N bytes from a camera location
 * Internal function.
 */
dc1394error_t
dc1394_pxl_read_n_bytes(dc1394camera_t *camera, uint32_t offset, char *str, uint32_t n)
{
    if (camera == NULL)
        return DC1394_FAILURE;

    uint32_t quadlet;
    uint32_t i;

    for (i = 0; i < n; i+=4) {
        dc1394_get_register(camera, (uint64_t)offset*4+i, &quadlet);
        quadlet = ntohl(quadlet);
        memcpy((void *)(str+i), (void *)&quadlet, sizeof(quadlet));
    }

    return DC1394_SUCCESS;
}

/******************************************************************************
 * Function to get the camera serial number as unsigned integer.
 * As opposed to getting all parameters, this is a quick way to just identify
 * the camera.
 */
dc1394error_t
dc1394_pxl_get_camera_serial_number(dc1394camera_t *camera, uint32_t *serial_num_int)
{
    if (camera == NULL)
        return DC1394_FAILURE;

    uint32_t serial_num_offset, serial_num_length;
    dc1394_get_adv_control_register(camera, PxL_ACR_SERIAL_NUM_OFFSET,
                                    &serial_num_offset);
    dc1394_get_adv_control_register(camera, PxL_ACR_SERIAL_NUM_LENGTH,
                                    &serial_num_length);

    char *serial_num = (char *) malloc((serial_num_length/4+1)*4);
    dc1394_pxl_read_n_bytes(camera, serial_num_offset, serial_num, serial_num_length);
    *serial_num_int = atoi(serial_num);
    free(serial_num);

    return DC1394_SUCCESS;
}

/******************************************************************************
 * Function to get the camera info.
 * All strings that are read are hard limited to PxL_MAX_STRING_LENGTH.
 */
dc1394error_t
dc1394_pxl_get_camera_info(dc1394camera_t *camera, dc1394_pxl_camera_info_t *camera_info)
{
    if (camera == NULL)
        return DC1394_FAILURE;

    uint32_t serial_num_offset, serial_num_length,
        camera_desc_offset, camera_desc_length;

    dc1394_get_adv_control_register(camera, PxL_ACR_FPGA_VERSION,
                                    &(camera_info->fpga_version));
    dc1394_get_adv_control_register(camera, PxL_ACR_FW_VERSION,
                                    &(camera_info->fw_version));

    dc1394_get_adv_control_register(camera, PxL_ACR_SERIAL_NUM_OFFSET,
                                    &serial_num_offset);
    dc1394_get_adv_control_register(camera, PxL_ACR_SERIAL_NUM_LENGTH,
                                    &serial_num_length);
    dc1394_get_adv_control_register(camera, PxL_ACR_CAMERA_DESC_OFFSET,
                                    &camera_desc_offset);
    dc1394_get_adv_control_register(camera, PxL_ACR_CAMERA_DESC_LENGTH,
                                    &camera_desc_length);

#ifdef PIXELINK_DEBUG_LOWEST_LEVEL
    fprintf(stdout, "%-26s: %08x\n", "SERIAL_NUM", serial_num_offset);
    fprintf(stdout, "%-26s: %08x\n", "SERIAL_NUM_LENGTH", serial_num_length);
    fprintf(stdout, "%-26s: %08x\n", "CAMERA_DESC", camera_desc_offset);
    fprintf(stdout, "%-26s: %08x\n", "CAMERA_DESC_LENGTH", camera_desc_length);
#endif

    serial_num_length = (serial_num_length < PxL_MAX_STRING_LENGTH)?
        serial_num_length: PxL_MAX_STRING_LENGTH;
    dc1394_pxl_read_n_bytes(camera, serial_num_offset, camera_info->serial_number, serial_num_length);
    camera_info->serial_number[PxL_MAX_STRING_LENGTH-1] = '\0';

    camera_desc_length = (camera_desc_length < PxL_MAX_STRING_LENGTH)?
        camera_desc_length: PxL_MAX_STRING_LENGTH;
    dc1394_pxl_read_n_bytes(camera, camera_desc_offset, camera_info->description, camera_desc_length);
    camera_info->description[PxL_MAX_STRING_LENGTH-1] = '\0';

    return DC1394_SUCCESS;
}

/******************************************************************************
 * Function to get the camera info.
 * All strings that are read are hard limited to PxL_MAX_STRING_LENGTH.
 */
dc1394error_t
dc1394_pxl_get_adv_feature_info(dc1394camera_t *camera, dc1394_pxl_adv_feature_info_t *adv_feature_info)
{
    if (camera == NULL)
        return DC1394_FAILURE;

    uint32_t name_inquiry, name_offset, name_length;

    dc1394_get_adv_control_register(camera, PxL_ACR_NAME_INQUIRY, &name_inquiry);
    dc1394_get_adv_control_register(camera, PxL_ACR_NAME_OFFSET, &name_offset);
    dc1394_get_adv_control_register(camera, PxL_ACR_NAME_LENGTH, &name_length);

    if (name_inquiry & 0x80000000) {
        adv_feature_info->name_presence = DC1394_FALSE;
        name_length = (name_length < PxL_MAX_STRING_LENGTH)?
            name_length: PxL_MAX_STRING_LENGTH;
        dc1394_pxl_read_n_bytes(camera, name_offset, adv_feature_info->name, name_length);
        adv_feature_info->name[PxL_MAX_STRING_LENGTH-1] = '\0';
    }
    else
        {
            adv_feature_info->name_presence = DC1394_FALSE;
            adv_feature_info->name[0] = '\0';
        }
    adv_feature_info->name_offset = name_offset;

    return DC1394_SUCCESS;
}

/*****************************************************************************
 * Function to get the GPIO (General Purpose Input Output) information.
 * The GPIO functionality comes under Advanced Features, but are functionally
 * different enought to warrant their own structure.
 */
dc1394error_t
dc1394_pxl_get_gpio_inq(dc1394camera_t *camera, dc1394_pxl_gpio_info_t *gpio_info)
{
    uint32_t gpio_inq;

    if (camera == NULL)
        return DC1394_FAILURE;

    uint32_t address = PxL_ACR_GPIO_INQ;
    dc1394_get_adv_control_register(camera, address, &gpio_inq);
#ifdef PIXELINK_DEBUG_DISPLAY
    printf("  0x%08x : r 0x%08x < GPIO_INQ\n", address, gpio_inq);
#endif

    uint32_t bit = 0x1UL << 31;
    gpio_info->number      = (gpio_inq>>24) & 0x0FUL;

    gpio_info->presence    = gpio_inq & (bit>> 0)? DC1394_TRUE: DC1394_FALSE;
    gpio_info->polarity    = gpio_inq & (bit>> 1)? DC1394_TRUE: DC1394_FALSE;
    gpio_info->mode_strobe = gpio_inq & (bit>> 8)? DC1394_TRUE: DC1394_FALSE;
    gpio_info->mode_normal = gpio_inq & (bit>> 9)? DC1394_TRUE: DC1394_FALSE;
    gpio_info->mode_pulse  = gpio_inq & (bit>>10)? DC1394_TRUE: DC1394_FALSE;
    gpio_info->mode_busy   = gpio_inq & (bit>>11)? DC1394_TRUE: DC1394_FALSE;
    gpio_info->mode_flash  = gpio_inq & (bit>>12)? DC1394_TRUE: DC1394_FALSE;

    if (gpio_info->presence == DC1394_FALSE) {
        gpio_info->number      = 0;
        gpio_info->polarity    = DC1394_FALSE;
        gpio_info->mode_strobe = DC1394_FALSE;
        gpio_info->mode_normal = DC1394_FALSE;
        gpio_info->mode_pulse  = DC1394_FALSE;
        gpio_info->mode_busy   = DC1394_FALSE;
        gpio_info->mode_flash  = DC1394_FALSE;
    }

#ifdef PIXELINK_DEBUG_LOWEST_LEVEL
    printf("\n");
    printf("  Presence ---------------------------------+\n");
    printf("  Polarity inq ----------------------------+|\n");
    printf("  Number of supported GPIOs ---------+--+  ||\n");
    printf("  Strobe mode ----------------------+|  |  ||\n");
    printf("  Normal mode ---------------------+||  |  ||\n");
    printf("  Pulse mode ---------------------+|||  |  ||\n");
    printf("  Busy mode ---------------------+||||  |  ||\n");
    printf("                                 |||||  |  ||\n");
    printf("  GPIO_INQ : ");

    uint32_t i;
    for (i = 0; i < 32; i++) {
        (gpio_inq & (1<<i))? printf("1"): printf("0");
    }
    printf("\n\n");
#endif

    return DC1394_SUCCESS;
}

/*****************************************************************************
 * Function to get the GPO parameters Parameter1, Parameter2, Parameter3
 */
dc1394error_t
dc1394_pxl_get_gpo_param(dc1394camera_t *camera, uint32_t gpio_id,
        uint32_t *p1_val, uint32_t *p2_val, uint32_t *p3_val)
{
    dc1394error_t err;
    dc1394_pxl_gpio_info_t gpio_info;

    err = dc1394_pxl_get_gpio_inq(camera, &gpio_info);
    if (err == DC1394_FAILURE) {
        return DC1394_FAILURE;
    }

    if (!(gpio_info.presence && gpio_id < gpio_info.number)) {
        return DC1394_FAILURE;
    }

    uint32_t gpio_parm1_abs, gpio_parm2_abs, gpio_parm3_abs;

    dc1394_get_adv_control_register(camera, PxL_ACR_GPIO_PARM1_ABS, &gpio_parm1_abs);
    dc1394_get_adv_control_register(camera, PxL_ACR_GPIO_PARM2_ABS, &gpio_parm2_abs);
    dc1394_get_adv_control_register(camera, PxL_ACR_GPIO_PARM3_ABS, &gpio_parm3_abs);

    uint32_t gpio_parm1_add, gpio_parm2_add, gpio_parm3_add;

    gpio_parm1_add = 4*gpio_parm1_abs + gpio_id*0x0c + 0x08;
    gpio_parm2_add = 4*gpio_parm2_abs + gpio_id*0x0c + 0x08;
    gpio_parm3_add = 4*gpio_parm3_abs + gpio_id*0x0c + 0x08;

    dc1394_get_register(camera, (uint64_t)(gpio_parm1_add), p1_val);
    dc1394_get_register(camera, (uint64_t)(gpio_parm2_add), p2_val);
    dc1394_get_register(camera, (uint64_t)(gpio_parm3_add), p3_val);

#ifdef PIXELINK_DEBUG_DISPLAY
    printf("  0x%08x : r 0x%08x < GPIO_PARM1_VALUE\n", gpio_parm1_add, *p1_val);
    printf("  0x%08x : r 0x%08x < GPIO_PARM2_VALUE\n", gpio_parm2_add, *p2_val);
    printf("  0x%08x : r 0x%08x < GPIO_PARM3_VALUE\n", gpio_parm3_add, *p3_val);
#endif

    return DC1394_SUCCESS;
}

/*****************************************************************************
 * Function to get the GPO parameters Parameter1, Parameter2, Parameter3
 * as well as the corresponding minimum and maximum values.
 */
dc1394error_t
dc1394_pxl_get_gpo_param_min_max(dc1394camera_t *camera, uint32_t gpio_id,
        uint32_t *p1_val, uint32_t *p2_val, uint32_t *p3_val,
        uint32_t *p1_min, uint32_t *p2_min, uint32_t *p3_min,
        uint32_t *p1_max, uint32_t *p2_max, uint32_t *p3_max)
{
    dc1394error_t err;
    dc1394_pxl_gpio_info_t gpio_info;

    err = dc1394_pxl_get_gpio_inq(camera, &gpio_info);
    if (err == DC1394_FAILURE) {
        return DC1394_FAILURE;
    }

    if (!(gpio_info.presence && gpio_id < gpio_info.number)) {
        return DC1394_FAILURE;
    }

    uint32_t gpio_parm1_abs, gpio_parm2_abs, gpio_parm3_abs;

    dc1394_get_adv_control_register(camera, PxL_ACR_GPIO_PARM1_ABS, &gpio_parm1_abs);
    dc1394_get_adv_control_register(camera, PxL_ACR_GPIO_PARM2_ABS, &gpio_parm2_abs);
    dc1394_get_adv_control_register(camera, PxL_ACR_GPIO_PARM3_ABS, &gpio_parm3_abs);

    uint32_t gpio_parm1_add, gpio_parm2_add, gpio_parm3_add;

    gpio_parm1_add = 4*gpio_parm1_abs + gpio_id*0x0c + 0x08;
    gpio_parm2_add = 4*gpio_parm2_abs + gpio_id*0x0c + 0x08;
    gpio_parm3_add = 4*gpio_parm3_abs + gpio_id*0x0c + 0x08;

    dc1394_get_register(camera, (uint64_t)(gpio_parm1_add), p1_val);
    dc1394_get_register(camera, (uint64_t)(gpio_parm2_add), p2_val);
    dc1394_get_register(camera, (uint64_t)(gpio_parm3_add), p3_val);

#ifdef PIXELINK_DEBUG_DISPLAY
    printf("  0x%08x : r 0x%08x < GPIO_PARM1_VALUE\n", gpio_parm1_add, *p1_val);
    printf("  0x%08x : r 0x%08x < GPIO_PARM2_VALUE\n", gpio_parm2_add, *p2_val);
    printf("  0x%08x : r 0x%08x < GPIO_PARM3_VALUE\n", gpio_parm3_add, *p3_val);
#endif

    gpio_parm1_add = 4*gpio_parm1_abs+ gpio_id*0x0c + 0x00;
    gpio_parm2_add = 4*gpio_parm2_abs+ gpio_id*0x0c + 0x00;
    gpio_parm3_add = 4*gpio_parm3_abs+ gpio_id*0x0c + 0x00;

    dc1394_get_register(camera, (uint64_t)(gpio_parm1_add), p1_min);
    dc1394_get_register(camera, (uint64_t)(gpio_parm2_add), p2_min);
    dc1394_get_register(camera, (uint64_t)(gpio_parm3_add), p3_min);

#ifdef PIXELINK_DEBUG_DISPLAY
    printf("  0x%08x : r 0x%08x < GPIO_PARM1_MIN\n", gpio_parm1_add, *p1_min);
    printf("  0x%08x : r 0x%08x < GPIO_PARM2_MIN\n", gpio_parm2_add, *p2_min);
    printf("  0x%08x : r 0x%08x < GPIO_PARM3_MIN\n", gpio_parm3_add, *p3_min);
#endif

    gpio_parm1_add = 4*gpio_parm1_abs+ gpio_id*0x0c + 0x04;
    gpio_parm2_add = 4*gpio_parm2_abs+ gpio_id*0x0c + 0x04;
    gpio_parm3_add = 4*gpio_parm3_abs+ gpio_id*0x0c + 0x04;

    dc1394_get_register(camera, (uint64_t)(gpio_parm1_add), p1_max);
    dc1394_get_register(camera, (uint64_t)(gpio_parm2_add), p2_max);
    dc1394_get_register(camera, (uint64_t)(gpio_parm3_add), p3_max);

#ifdef PIXELINK_DEBUG_DISPLAY
    printf("  0x%08x : r 0x%08x < GPIO_PARM1_MAX\n", gpio_parm1_add, *p1_max);
    printf("  0x%08x : r 0x%08x < GPIO_PARM2_MAX\n", gpio_parm2_add, *p2_max);
    printf("  0x%08x : r 0x%08x < GPIO_PARM3_MAX\n", gpio_parm3_add, *p3_max);
#endif

    return DC1394_SUCCESS;
}

/******************************************************************************
 * Function to set the GPO parameters P1, P2, P3
 * Internal Function.
 *
 */
dc1394error_t
dc1394_pxl_set_gpo_param(dc1394camera_t *camera, uint32_t gpio_id,
        uint32_t p1_val, uint32_t p2_val, uint32_t p3_val)
{

    dc1394error_t err;
    dc1394_pxl_gpio_info_t gpio_info;

    err = dc1394_pxl_get_gpio_inq(camera, &gpio_info);
    if (err == DC1394_FAILURE) {
        return DC1394_FAILURE;
    }

    uint32_t gpio_parm1_abs, gpio_parm2_abs, gpio_parm3_abs;

    dc1394_get_adv_control_register(camera, PxL_ACR_GPIO_PARM1_ABS, &gpio_parm1_abs);
    dc1394_get_adv_control_register(camera, PxL_ACR_GPIO_PARM2_ABS, &gpio_parm2_abs);
    dc1394_get_adv_control_register(camera, PxL_ACR_GPIO_PARM3_ABS, &gpio_parm3_abs);

    uint32_t gpio_parm1_add, gpio_parm2_add, gpio_parm3_add;
    gpio_parm1_add = 4*gpio_parm1_abs+ gpio_id*0x0c + 0x08;
    gpio_parm2_add = 4*gpio_parm2_abs+ gpio_id*0x0c + 0x08;
    gpio_parm3_add = 4*gpio_parm3_abs+ gpio_id*0x0c + 0x08;

    dc1394_set_register(camera, (uint64_t)(gpio_parm1_add), p1_val);
    dc1394_set_register(camera, (uint64_t)(gpio_parm2_add), p2_val);
    dc1394_set_register(camera, (uint64_t)(gpio_parm3_add), p3_val);

#ifdef PIXELINK_DEBUG_DISPLAY
    printf("  0x%08x : w 0x%08x < GPIO_PARM1_VALUE\n", gpio_parm1_add, p1_val);
    printf("  0x%08x : w 0x%08x < GPIO_PARM2_VALUE\n", gpio_parm2_add, p2_val);
    printf("  0x%08x : w 0x%08x < GPIO_PARM3_VALUE\n", gpio_parm3_add, p3_val);
#endif

    return DC1394_SUCCESS;
}

/******************************************************************************
 * Function to read GPIO config. Internal function?
 */
dc1394error_t
dc1394_pxl_get_gpo_config(dc1394camera_t *camera, uint32_t gpio_id, uint32_t *gpio_cfg)
{
    dc1394error_t err;
    if (camera == NULL)
        return DC1394_FAILURE;

    uint32_t gpio_cfg_add = PxL_ACR_GPIO_0_CFG + gpio_id*4;
    err = dc1394_get_adv_control_register(camera, (uint64_t) gpio_cfg_add, gpio_cfg);

#ifdef PIXELINK_DEBUG_DISPLAY
    printf("  0x%08x : r 0x%08x < GPO_0_CFG\n", gpio_cfg_add, *gpio_cfg);
#endif

    return err;
}

/******************************************************************************
 * Function to set GPIO config. Internal function?
 */
dc1394error_t
dc1394_pxl_set_gpo_config(dc1394camera_t *camera, uint32_t gpio_id, uint32_t gpio_cfg)
{

    dc1394error_t err;
    if (camera == NULL)
        return DC1394_FAILURE;

    uint32_t gpio_cfg_add = PxL_ACR_GPIO_0_CFG + gpio_id*4;
    err = dc1394_set_adv_control_register(camera, (uint64_t) gpio_cfg_add, gpio_cfg);

#ifdef PIXELINK_DEBUG_DISPLAY
    printf("  0x%08x : w 0x%08x < GPO_0_CFG\n", gpio_cfg_add, gpio_cfg);
#endif

    return err;
}

/******************************************************************************
 * Function to print list of camera features.
 * It can be used to test many functions in this file.
 */
dc1394error_t
dc1394_pxl_print_camera_info(dc1394camera_t *camera, FILE *fd)
{
    dc1394_pxl_camera_info_t camera_info;
    dc1394_pxl_get_camera_info(camera, &camera_info);

    fprintf(fd,"Camera information.\n");
    fprintf(fd,"  %-16s: %08x\n", "FPGA Version", camera_info.fpga_version);
    fprintf(fd,"  %-16s: %08x\n", "FW Version", camera_info.fw_version);
    fprintf(fd,"  %-16s: %s\n", "Serial Number", camera_info.serial_number);
    fprintf(fd,"  %-16s: %s\n", "Description", camera_info.description);

    dc1394_pxl_adv_feature_info_t adv_info;
    dc1394_pxl_get_adv_feature_info(camera, &adv_info);

    fprintf(fd,"Advanced Feature Information.\n");
    fprintf(fd,"  %-16s: %s\n", "Name", adv_info.name);
    fprintf(fd,"\n");

    return DC1394_SUCCESS;
}

/******************************************************************************
 * Function to activate GPIO mode with the parameters
 */
dc1394error_t
dc1394_pxl_set_gpio_mode_param(dc1394camera_t *camera, uint32_t gpio_id,
        dc1394pxl_gpio_polarity_t gpio_polarity, dc1394pxl_gpio_mode_t gpio_mode,
        double f1_val, double f2_val, double f3_val)
{

    dc1394error_t err;
    dc1394_pxl_gpio_info_t gpio_info;

    err = dc1394_pxl_get_gpio_inq(camera, &gpio_info);
    if (err == DC1394_FAILURE) {
        return DC1394_FAILURE;
    }
    if (err == DC1394_FAILURE) {
        dc1394_log_error("error reading gpio inq register\n");
        return DC1394_FAILURE;
    }

    /* compose the cfg_register value */
    uint32_t gpio_cfg = PxL_GPO_CFG_ENABLE;

    /* check if mode is valid */
    switch (gpio_mode) {
    case DC1394_PxL_GPIO_MODE_STROBE:
        gpio_cfg = gpio_cfg | PxL_GPO_CFG_MODE_STROBE;
        if (gpio_info.mode_strobe == DC1394_FALSE) err = DC1394_FAILURE;
        break;

    case DC1394_PxL_GPIO_MODE_NORMAL:
        gpio_cfg = gpio_cfg | PxL_GPO_CFG_MODE_NORMAL;
        if (gpio_info.mode_normal == DC1394_FALSE) err = DC1394_FAILURE;
        break;

    case DC1394_PxL_GPIO_MODE_PULSE:
        gpio_cfg = gpio_cfg | PxL_GPO_CFG_MODE_PULSE;
        if (gpio_info.mode_pulse == DC1394_FALSE) err = DC1394_FAILURE;
        break;

    case DC1394_PxL_GPIO_MODE_BUSY:
        gpio_cfg = gpio_cfg | PxL_GPO_CFG_MODE_BUSY;
        if (gpio_info.mode_busy == DC1394_FALSE) err = DC1394_FAILURE;
        break;

    case DC1394_PxL_GPIO_MODE_FLASH:
        gpio_cfg = gpio_cfg | PxL_GPO_CFG_MODE_FLASH;
        if (gpio_info.mode_flash == DC1394_FALSE) err = DC1394_FAILURE;
        break;

    default:
        err = DC1394_FAILURE;
    }
    if (err == DC1394_FAILURE) {
        dc1394_log_error("error with gpio mode setting\n");
        return DC1394_FAILURE;
    }

    /* check if polarity is valid */
    switch (gpio_mode) {
    case DC1394_PxL_GPIO_POLARITY_NONE:
        /* No change in the mode, use whatever is available */
        break;

    case DC1394_PxL_GPIO_POLARITY_HIGH:
        gpio_cfg = gpio_cfg | PxL_GPO_CFG_POLARITY_HIGH;
        if (gpio_info.polarity == DC1394_FALSE) err = DC1394_FAILURE;
        break;

    case DC1394_PxL_GPIO_POLARITY_LOW:
        gpio_cfg = gpio_cfg | PxL_GPO_CFG_POLARITY_LOW;
        if (gpio_info.polarity == DC1394_FALSE) err = DC1394_FAILURE;
        break;
    default:
        err = DC1394_FAILURE;
    }

    if (err == DC1394_FAILURE) {
        dc1394_log_error("error with gpio polarity setting\n");
        return DC1394_FAILURE;
    }

    /* Set the GPIO_CFG parameter */
    dc1394_pxl_set_gpo_config(camera, gpio_id, gpio_cfg);

    /* Check the minimum and maximum values */
    uint32_t p1_val, p2_val, p3_val;
    uint32_t p1_min, p2_min, p3_min, p1_max, p2_max, p3_max;

    double f1_min, f2_min, f3_min, f1_max, f2_max, f3_max;

    dc1394_pxl_get_gpo_param_min_max(camera, gpio_id, &p1_val, &p2_val, &p3_val,
                                     &p1_min, &p2_min, &p3_min, &p1_max, &p2_max, &p3_max);
    dc1394_pxl_convert_uint32_to_float32(p1_min, &f1_min);
    dc1394_pxl_convert_uint32_to_float32(p2_min, &f2_min);
    dc1394_pxl_convert_uint32_to_float32(p3_min, &f3_min);
    dc1394_pxl_convert_uint32_to_float32(p1_max, &f1_max);
    dc1394_pxl_convert_uint32_to_float32(p2_max, &f2_max);
    dc1394_pxl_convert_uint32_to_float32(p3_max, &f3_max);

    if ((f1_val < f1_min)| (f1_val > f1_max)) {
        dc1394_log_warning("WARNING: parameter1 out of bounds! corrected.\n");
    }
    if ((f2_val < f2_min)| (f2_val > f2_max)) {
        dc1394_log_warning("\nWARNING: parameter2 out of bounds! corrected.\n");
    }
    if ((f3_val < f3_min)| (f3_val > f3_max)) {
        dc1394_log_warning("\nWARNING: parameter3 out of bounds! corrected.\n");
    }

    f1_val = (f1_val < f1_min)? f1_min: ((f1_val > f1_max)? f1_max: f1_val);
    f2_val = (f2_val < f2_min)? f2_min: ((f2_val > f2_max)? f2_max: f2_val);
    f3_val = (f3_val < f3_min)? f3_min: ((f3_val > f3_max)? f3_max: f3_val);

    dc1394_pxl_convert_float32_to_quadlet(f1_val, &p1_val);
    dc1394_pxl_convert_float32_to_quadlet(f2_val, &p2_val);
    dc1394_pxl_convert_float32_to_quadlet(f3_val, &p3_val);

    dc1394_pxl_set_gpo_param(camera, gpio_id, p1_val, p2_val, p3_val);

    return DC1394_SUCCESS;
}

