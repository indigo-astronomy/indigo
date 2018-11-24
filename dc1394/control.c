/*
 * 1394-Based Digital Camera Control Library
 *
 * Generic camera control functions
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "internal.h"
#include "offsets.h"

dc1394error_t
dc1394_camera_set_broadcast(dc1394camera_t *camera, dc1394bool_t pwr)
{
    dc1394camera_priv_t * priv = DC1394_CAMERA_PRIV (camera);
    const platform_dispatch_t * d = priv->platform->dispatch;
    if (!d->set_broadcast)
        return DC1394_FUNCTION_NOT_SUPPORTED;
    return d->set_broadcast (priv->pcam, pwr);
}

dc1394error_t
dc1394_camera_get_broadcast(dc1394camera_t *camera, dc1394bool_t *pwr)
{
    dc1394camera_priv_t * priv = DC1394_CAMERA_PRIV (camera);
    const platform_dispatch_t * d = priv->platform->dispatch;
    if (!d->get_broadcast)
        return DC1394_FUNCTION_NOT_SUPPORTED;
    return d->get_broadcast (priv->pcam, pwr);
}

dc1394error_t
dc1394_reset_bus (dc1394camera_t * camera)
{
    dc1394camera_priv_t * priv = DC1394_CAMERA_PRIV (camera);
    const platform_dispatch_t * d = priv->platform->dispatch;
    if (!d->reset_bus)
        return DC1394_FUNCTION_NOT_SUPPORTED;
    return d->reset_bus (priv->pcam);
}

dc1394error_t
dc1394_read_cycle_timer (dc1394camera_t * camera,
                         uint32_t * cycle_timer, uint64_t * local_time)
{
    dc1394camera_priv_t * priv = DC1394_CAMERA_PRIV (camera);
    const platform_dispatch_t * d = priv->platform->dispatch;
    if (!d->read_cycle_timer)
        return DC1394_FUNCTION_NOT_SUPPORTED;
    return d->read_cycle_timer (priv->pcam, cycle_timer, local_time);
}

dc1394error_t
dc1394_camera_get_node (dc1394camera_t *camera, uint32_t *node,
        uint32_t * generation)
{
    dc1394camera_priv_t * priv = DC1394_CAMERA_PRIV (camera);
    const platform_dispatch_t * d = priv->platform->dispatch;
    if (!d->camera_get_node)
        return DC1394_FUNCTION_NOT_SUPPORTED;
    return d->camera_get_node (priv->pcam, node, generation);
}

static dc1394error_t
update_camera_info (dc1394camera_t *camera)
{
    uint32_t value=0, quadval = 0; // set to zero to avoid valgrind errors

    dc1394_get_control_register(camera, REG_CAMERA_BASIC_FUNC_INQ, &value);

    int adv_features_capable = value >> 31;
    camera->has_vmode_error_status = (value >> 30) & 1;
    camera->has_feature_error_status = (value >> 29) & 1;
    int opt_function_capable = (value >> 28) & 1;
    camera->bmode_capable = (value >> 23) & 1;
    camera->can_switch_on_off = (value >> 15) & 1;
    camera->one_shot_capable = (value >> 12) & 1;
    camera->multi_shot_capable = (value >> 11) & 1;
    camera->max_mem_channel    = value & 0xf;

    if (adv_features_capable)
        if (dc1394_get_control_register (camera, REG_CAMERA_ADV_FEATURE_INQ,
                                         &quadval) == DC1394_SUCCESS)
            camera->advanced_features_csr = (uint64_t) quadval * 4;

    if (opt_function_capable) {
        value = 0;
        dc1394_get_control_register (camera, REG_CAMERA_OPT_FUNC_INQ, &value);
        if ((value >> 30) & 1)
            if (dc1394_get_control_register (camera, REG_CAMERA_PIO_CONTROL_CSR_INQ, &quadval) == DC1394_SUCCESS)
                camera->PIO_control_csr = (uint64_t) quadval * 4;
        if ((value >> 29) & 1)
            if (dc1394_get_control_register (camera, REG_CAMERA_SIO_CONTROL_CSR_INQ, &quadval) == DC1394_SUCCESS)
                camera->SIO_control_csr = (uint64_t) quadval * 4;
        if ((value >> 28) & 1)
            if (dc1394_get_control_register (camera, REG_CAMERA_STROBE_CONTROL_CSR_INQ, &quadval) == DC1394_SUCCESS)
                camera->strobe_control_csr = (uint64_t) quadval * 4;
    }

    // verify that the iso speed, the video mode and the framerates are OK
    // at boot time

    /* get the current ISO speed, and verify it*/
    dc1394error_t err;
    dc1394speed_t iso_speed;
    err=dc1394_video_get_iso_speed(camera, &iso_speed);
    if (err==DC1394_INVALID_ISO_SPEED) {
        // default to the most probable speed: 400 Mbps
        dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
    }

    /* get the current video mode, and verify it*/
    dc1394video_modes_t modes;
    dc1394video_mode_t video_mode;
    err=dc1394_video_get_mode(camera, &video_mode);
    if (err==DC1394_INVALID_VIDEO_FORMAT) {
        // a proper video mode may not be present. Try to set a default video mode
        dc1394_video_get_supported_modes(camera,&modes);

        dc1394_video_set_mode(camera,modes.modes[0]);
    }

    /* get the current framerate, and verify it*/
    dc1394framerate_t framerate;
    dc1394framerates_t framerates;
    err=dc1394_video_get_framerate(camera, &framerate);
    if (err==DC1394_INVALID_FRAMERATE) {
        // a proper framerate may not be present. Try to set a default framerate
        dc1394_video_get_supported_framerates(camera,video_mode,&framerates);

        dc1394_video_set_framerate(camera,framerates.framerates[0]);
    }

    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_camera_print_info(dc1394camera_t *camera, FILE* fd)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    uint32_t value[2];

    value[0]= camera->guid & 0xffffffff;
    value[1]= (camera->guid >>32) & 0xffffffff;
    fprintf(fd,"------ Camera information ------\n");
    fprintf(fd,"Vendor                            :     %s\n", camera->vendor);
    fprintf(fd,"Model                             :     %s\n", camera->model);
    fprintf(fd,"Unit                              :     %d\n", camera->unit);
    fprintf(fd,"Specifications ID                 :     0x%x\n", camera->unit_spec_ID);
    fprintf(fd,"Software revision                 :     0x%x\n", camera->unit_sw_version);
    fprintf(fd,"IIDC version code                 :     %d\n", camera->iidc_version);
    fprintf(fd,"Unit directory offset             :     0x%x\n", camera->unit_directory);
    fprintf(fd,"Unit dependent directory offset   :     0x%x\n", camera->unit_dependent_directory);
    fprintf(fd,"Commands registers base           :     0x%x\n", camera->command_registers_base);
    fprintf(fd,"Unique ID                         :     0x%08x%08x\n", value[1], value[0]);
    fprintf(fd,"Vendor ID                         :     0x%x\n", camera->vendor_id);
    fprintf(fd,"Model ID                          :     0x%x\n", camera->model_id);
    if (camera->advanced_features_csr>0)
        fprintf(fd,"Advanced features found at offset :     0x%"PRIx64"\n", camera->advanced_features_csr);
    fprintf(fd,"1394b mode capable (>=800Mbit/s)  :     ");
    if (camera->bmode_capable==DC1394_TRUE)
        fprintf(fd,"Yes\n");
    else
        fprintf(fd,"No\n");

    fprintf(fd,"Platform backend                  :     %s\n",
            cpriv->platform->name);
    const platform_dispatch_t * d = cpriv->platform->dispatch;
    if (d->camera_print_info)
        d->camera_print_info (cpriv->pcam, fd);

    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_camera_get_platform_string(dc1394camera_t *camera, const char **platform)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
	*platform=cpriv->platform->name;

	return DC1394_SUCCESS;
}

/*****************************************************
 dc1394_get_camera_feature_set

 Collects the available features for the camera
 described by node and stores them in features.
*****************************************************/
dc1394error_t
dc1394_feature_get_all(dc1394camera_t *camera, dc1394featureset_t *features)
{
    uint32_t i, j;
    dc1394error_t err=DC1394_SUCCESS;

    for (i= DC1394_FEATURE_MIN, j= 0; i <= DC1394_FEATURE_MAX; i++, j++)  {
        features->feature[j].id= i;
        err=dc1394_feature_get(camera, &features->feature[j]);
        DC1394_ERR_RTN(err, "Could not get camera feature");
    }

    return err;
}

/*****************************************************
 dc1394_get_camera_feature

 Stores the bounds and options associated with the
 feature described by feature->id
*****************************************************/
dc1394error_t
dc1394_feature_get(dc1394camera_t *camera, dc1394feature_info_t *feature)
{
    uint64_t offset;
    uint32_t value;
    dc1394error_t err;

    if ( (feature->id < DC1394_FEATURE_MIN) || (feature->id > DC1394_FEATURE_MAX) ) {
        return DC1394_INVALID_FEATURE;
    }

    // check presence
    err=dc1394_feature_is_present(camera, feature->id, &(feature->available));
    DC1394_ERR_RTN(err, "Could not check feature presence");

    if (feature->available == DC1394_FALSE) {
        return DC1394_SUCCESS;
    }

    // get capabilities
    FEATURE_TO_INQUIRY_OFFSET(feature->id, offset);
    err=dc1394_get_control_register(camera, offset, &value);
    DC1394_ERR_RTN(err, "Could not check feature characteristics");

    dc1394_feature_get_modes(camera, feature->id, &feature->modes);
    dc1394_feature_get_mode(camera, feature->id, &feature->current_mode);

    switch (feature->id) {
    case DC1394_FEATURE_TRIGGER:
        feature->polarity_capable= (value & 0x02000000UL) ? DC1394_TRUE : DC1394_FALSE;
        int i, j;
        uint32_t value_tmp;

        feature->trigger_modes.num=0;
        value_tmp= (value & (0xFFFF));

        for (i=DC1394_TRIGGER_MODE_MIN;i<=DC1394_TRIGGER_MODE_MAX;i++) {
            j = i - DC1394_TRIGGER_MODE_MIN;
            if (value_tmp & (0x1 << (15-j-(j>5)*8))) { // (i>5)*8 to take the mode gap into account
                feature->trigger_modes.modes[feature->trigger_modes.num]=i;
                feature->trigger_modes.num++;
            }
        }

        err=dc1394_external_trigger_get_supported_sources(camera,&feature->trigger_sources);
        DC1394_ERR_RTN(err, "Could not get supported trigger sources");

        break;
    default:
        feature->polarity_capable = 0;
        feature->trigger_mode     = 0;

        feature->min= (value & 0xFFF000UL) >> 12;
        feature->max= (value & 0xFFFUL);
        break;
    }

    feature->absolute_capable = (value & 0x40000000UL) ? DC1394_TRUE : DC1394_FALSE;
    feature->readout_capable  = (value & 0x08000000UL) ? DC1394_TRUE : DC1394_FALSE;
    feature->on_off_capable   = (value & 0x04000000UL) ? DC1394_TRUE : DC1394_FALSE;

    // get current values
    FEATURE_TO_VALUE_OFFSET(feature->id, offset);

    err=dc1394_get_control_register(camera, offset, &value);
    DC1394_ERR_RTN(err, "Could not get feature register");

    switch (feature->id) {
    case DC1394_FEATURE_TRIGGER:
        feature->trigger_polarity= (value & 0x01000000UL) ? DC1394_TRUE : DC1394_FALSE;
        feature->trigger_mode= (uint32_t)((value >> 16) & 0xF);
        if (feature->trigger_mode >= 14)
            feature->trigger_mode += DC1394_TRIGGER_MODE_MIN - 8;
        else
            feature->trigger_mode += DC1394_TRIGGER_MODE_MIN;
        feature->trigger_source = (uint32_t)((value >> 21) & 0x7UL);
        if (feature->trigger_source > 3)
            feature->trigger_source -= 3;
        feature->trigger_source += DC1394_TRIGGER_SOURCE_MIN;
        break;
    default:
        break;
    }

    feature->is_on= (value & 0x02000000UL) ? DC1394_TRUE : DC1394_FALSE;

    switch (feature->id) {
    case DC1394_FEATURE_WHITE_BALANCE:
        feature->RV_value= value & 0xFFFUL;
        feature->BU_value= (value & 0xFFF000UL) >> 12;
        break;
    case DC1394_FEATURE_WHITE_SHADING:
        feature->R_value=value & 0xFFUL;
        feature->G_value=(value & 0xFF00UL)>>8;
        feature->B_value=(value & 0xFF0000UL)>>16;
        break;
    case DC1394_FEATURE_TEMPERATURE:
        feature->value= value & 0xFFFUL;
        feature->target_value= value & 0xFFF000UL;
        break;
    default:
        feature->value= value & 0xFFFUL;
        break;
    }

    if (feature->absolute_capable>0) {
        err=dc1394_feature_get_absolute_boundaries(camera, feature->id, &feature->abs_min, &feature->abs_max);
        DC1394_ERR_RTN(err, "Could not get feature absolute min/max");
        err=dc1394_feature_get_absolute_value(camera, feature->id, &feature->abs_value);
        DC1394_ERR_RTN(err, "Could not get feature absolute value");
        err=dc1394_feature_get_absolute_control(camera, feature->id, &feature->abs_control);
        DC1394_ERR_RTN(err, "Could not get feature absolute control");
    }

    return err;
}

/*****************************************************
 dc1394_print_feature

 Displays the bounds and options of the given feature
*****************************************************/
dc1394error_t
dc1394_feature_print(dc1394feature_info_t *f, FILE *fd)
{
    int fid= f->id;

    if ( (fid < DC1394_FEATURE_MIN) || (fid > DC1394_FEATURE_MAX) ) {
        return DC1394_INVALID_FEATURE;
    }
    const char *feature_string = dc1394_feature_get_string (fid);
    fprintf(fd,"%s:\n\t", feature_string);

    if (!f->available) {
        fprintf(fd,"NOT AVAILABLE\n");
        return DC1394_SUCCESS;
    }

    if (f->readout_capable)
        fprintf(fd,"RC  ");
    if (f->on_off_capable)
        fprintf(fd,"O/OC  ");
    int i;
    for (i=0;i<f->modes.num;i++) {
        switch (f->modes.modes[i]) {
        case DC1394_FEATURE_MODE_MANUAL:
            fprintf(fd,"MC  ");
            break;
        case DC1394_FEATURE_MODE_AUTO:
            fprintf(fd,"AC  ");
            break;
        case DC1394_FEATURE_MODE_ONE_PUSH_AUTO:
            fprintf(fd,"OP  ");
            break;
        }
        fprintf(fd,"(active is: ");
        switch (f->current_mode) {
        case DC1394_FEATURE_MODE_MANUAL:
            fprintf(fd,"MAN)  ");
            break;
        case DC1394_FEATURE_MODE_AUTO:
            fprintf(fd,"AUTO)  ");
            break;
        case DC1394_FEATURE_MODE_ONE_PUSH_AUTO:
            fprintf(fd,"ONE PUSH)  ");
            break;
        }

    }
    if (f->absolute_capable)
        fprintf(fd,"ABS  ");
    fprintf(fd,"\n");

    if (f->on_off_capable) {
        if (f->is_on)
            fprintf(fd,"\tFeature: ON  ");
        else
            fprintf(fd,"\tFeature: OFF  ");
    }
    else
        fprintf(fd,"\t");

    if (fid != DC1394_FEATURE_TRIGGER)
        fprintf(fd,"min: %d max %d\n", f->min, f->max);

    switch(fid) {
    case DC1394_FEATURE_TRIGGER:
        fprintf(fd,"\n\tAvailableTriggerModes: ");
        if (f->trigger_modes.num==0) {
            fprintf(fd,"none");
        }
        else {
            int i;
            for (i=0;i<f->trigger_modes.num;i++) {
                fprintf(fd,"%d ",f->trigger_modes.modes[i]);
            }
        }
        fprintf(fd,"\n\tAvailableTriggerSources: ");
        if (f->trigger_sources.num==0) {
            fprintf(fd,"none");
        }
        else {
            int i;
            for (i=0;i<f->trigger_sources.num;i++) {
                fprintf(fd,"%d ",f->trigger_sources.sources[i]);
            }
        }
        fprintf(fd,"\n\tPolarity Change Capable: ");

        if (f->polarity_capable)
            fprintf(fd,"True");
        else
            fprintf(fd,"False");

        fprintf(fd,"\n\tCurrent Polarity: ");

        if (f->trigger_polarity)
            fprintf(fd,"POS");
        else
            fprintf(fd,"NEG");

        fprintf(fd,"\n\tcurrent mode: %d\n", f->trigger_mode);
        if (f->trigger_sources.num>0) {
            fprintf(fd,"\n\tcurrent source: %d\n", f->trigger_source);
        }
        break;
    case DC1394_FEATURE_WHITE_BALANCE:
        fprintf(fd,"\tB/U value: %d R/V value: %d\n", f->BU_value, f->RV_value);
        break;
    case DC1394_FEATURE_TEMPERATURE:
        fprintf(fd,"\tTarget temp: %d Current Temp: %d\n", f->target_value, f->value);
        break;
    case DC1394_FEATURE_WHITE_SHADING:
        fprintf(fd,"\tR value: %d G value: %d B value: %d\n", f->R_value,
                f->G_value, f->B_value);
        break;
    default:
        fprintf(fd,"\tcurrent value is: %d\n",f->value);
        break;
    }
    if (f->absolute_capable)
        fprintf(fd,"\tabsolute settings:\n\t value: %f\n\t min: %f\n\t max: %f\n", f->abs_value,f->abs_min,f->abs_max);

    return DC1394_SUCCESS;
}

/*****************************************************
 dc1394_print_feature_set

 Displays the entire feature set stored in features
*****************************************************/
dc1394error_t
dc1394_feature_print_all(dc1394featureset_t *features, FILE *fd)
{
    uint32_t i, j;
    dc1394error_t err=DC1394_SUCCESS;

    fprintf(fd,"------ Features report ------\n");
    fprintf(fd,"OP   - one push capable\n");
    fprintf(fd,"RC   - readout capable\n");
    fprintf(fd,"O/OC - on/off capable\n");
    fprintf(fd,"AC   - auto capable\n");
    fprintf(fd,"MC   - manual capable\n");
    fprintf(fd,"ABS  - absolute capable\n");
    fprintf(fd,"-----------------------------\n");

    for (i= DC1394_FEATURE_MIN, j= 0; i <= DC1394_FEATURE_MAX; i++, j++)  {
        err=dc1394_feature_print(&features->feature[j], fd);
        DC1394_ERR_RTN(err, "Could not print feature");
    }

    return err;
}

dc1394error_t
dc1394_camera_reset(dc1394camera_t *camera)
{
    dc1394error_t err;
    err=dc1394_set_control_register(camera, REG_CAMERA_INITIALIZE, DC1394_FEATURE_ON);
    DC1394_ERR_RTN(err, "Could not reset the camera");
    return err;
}

dc1394error_t
dc1394_video_get_supported_modes(dc1394camera_t *camera, dc1394video_modes_t *modes)
{
    dc1394error_t err;
    uint32_t value, sup_formats;
    dc1394video_mode_t mode;

    // get supported formats
    err=dc1394_get_control_register(camera, REG_CAMERA_V_FORMAT_INQ, &sup_formats);
    DC1394_ERR_RTN(err, "Could not get supported formats");

    // for each format check supported modes and add them as we find them.

    modes->num=0;
    // Format_0
    if ((sup_formats & (0x1 << (31-(DC1394_FORMAT0-DC1394_FORMAT_MIN)))) > 0) {
        err=dc1394_get_control_register(camera, REG_CAMERA_V_MODE_INQ_BASE + ((DC1394_FORMAT0-DC1394_FORMAT_MIN) * 0x04U), &value);
        DC1394_ERR_RTN(err, "Could not get supported modes for Format_0");

        for (mode=DC1394_VIDEO_MODE_FORMAT0_MIN;mode<=DC1394_VIDEO_MODE_FORMAT0_MAX;mode++) {
            if ((value & (0x1<<(31-(mode-DC1394_VIDEO_MODE_FORMAT0_MIN)))) > 0) {
                modes->modes[modes->num]=mode;
                modes->num++;
            }
        }
    }
    // Format_1
    if ((sup_formats & (0x1 << (31-(DC1394_FORMAT1-DC1394_FORMAT_MIN)))) > 0) {
        err=dc1394_get_control_register(camera, REG_CAMERA_V_MODE_INQ_BASE + ((DC1394_FORMAT1-DC1394_FORMAT_MIN) * 0x04U), &value);
        DC1394_ERR_RTN(err, "Could not get supported modes for Format_1");

        for (mode=DC1394_VIDEO_MODE_FORMAT1_MIN;mode<=DC1394_VIDEO_MODE_FORMAT1_MAX;mode++) {
            if ((value & (0x1<<(31-(mode-DC1394_VIDEO_MODE_FORMAT1_MIN)))) > 0) {
                modes->modes[modes->num]=mode;
                modes->num++;
            }
        }
    }
    // Format_2
    if ((sup_formats & (0x1 << (31-(DC1394_FORMAT2-DC1394_FORMAT_MIN)))) > 0) {
        err=dc1394_get_control_register(camera, REG_CAMERA_V_MODE_INQ_BASE + ((DC1394_FORMAT2-DC1394_FORMAT_MIN) * 0x04U), &value);
        DC1394_ERR_RTN(err, "Could not get supported modes for Format_2");

        for (mode=DC1394_VIDEO_MODE_FORMAT2_MIN;mode<=DC1394_VIDEO_MODE_FORMAT2_MAX;mode++) {
            if ((value & (0x1<<(31-(mode-DC1394_VIDEO_MODE_FORMAT2_MIN)))) > 0) {
                modes->modes[modes->num]=mode;
                modes->num++;
            }
        }
    }
    // Format_6
    if ((sup_formats & (0x1 << (31-(DC1394_FORMAT6-DC1394_FORMAT_MIN)))) > 0) {
        err=dc1394_get_control_register(camera, REG_CAMERA_V_MODE_INQ_BASE + ((DC1394_FORMAT6-DC1394_FORMAT_MIN) * 0x04U), &value);
        DC1394_ERR_RTN(err, "Could not get supported modes for Format_3");

        for (mode=DC1394_VIDEO_MODE_FORMAT6_MIN;mode<=DC1394_VIDEO_MODE_FORMAT6_MAX;mode++) {
            if ((value & (0x1<<(31-(mode-DC1394_VIDEO_MODE_FORMAT6_MIN))))>0) {
                modes->modes[modes->num]=mode;
                modes->num++;
            }
        }
    }
    // Format_7
    if ((sup_formats & (0x1 << (31-(DC1394_FORMAT7-DC1394_FORMAT_MIN)))) > 0) {
        err=dc1394_get_control_register(camera, REG_CAMERA_V_MODE_INQ_BASE + ((DC1394_FORMAT7-DC1394_FORMAT_MIN) * 0x04U), &value);
        DC1394_ERR_RTN(err, "Could not get supported modes for Format_4");

        for (mode=DC1394_VIDEO_MODE_FORMAT7_MIN;mode<=DC1394_VIDEO_MODE_FORMAT7_MAX;mode++) {
            if ((value & (0x1<<(31-(mode-DC1394_VIDEO_MODE_FORMAT7_MIN))))>0) {
                modes->modes[modes->num]=mode;
                modes->num++;
            }
        }
    }

    return err;
}

dc1394error_t
dc1394_video_get_supported_framerates(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394framerates_t *framerates)
{
    dc1394framerate_t framerate;
    dc1394error_t err;
    uint32_t format;
    uint32_t value;

    err=get_format_from_mode(video_mode, &format);
    DC1394_ERR_RTN(err, "Invalid mode code");

    if ((format==DC1394_FORMAT6)||(format==DC1394_FORMAT7)) {
        err=DC1394_INVALID_VIDEO_FORMAT;
        DC1394_ERR_RTN(err, "Modes corresponding for format6 and format7 do not have framerates!");
    }

    switch (format) {
    case DC1394_FORMAT0:
        video_mode-=DC1394_VIDEO_MODE_FORMAT0_MIN;
        break;
    case DC1394_FORMAT1:
        video_mode-=DC1394_VIDEO_MODE_FORMAT1_MIN;
        break;
    case DC1394_FORMAT2:
        video_mode-=DC1394_VIDEO_MODE_FORMAT2_MIN;
        break;
    }
    format-=DC1394_FORMAT_MIN;


    err=dc1394_get_control_register(camera,REG_CAMERA_V_RATE_INQ_BASE + (format * 0x20U) + (video_mode * 0x04U), &value);
    DC1394_ERR_RTN(err, "Could not get supported framerates");

    framerates->num=0;
    for (framerate=DC1394_FRAMERATE_MIN;framerate<=DC1394_FRAMERATE_MAX;framerate++) {
        if ((value & (0x1<<(31-(framerate-DC1394_FRAMERATE_MIN))))>0) {
            framerates->framerates[framerates->num]=framerate;
            framerates->num++;
        }
    }

    return err;
}


dc1394error_t
dc1394_video_get_framerate(dc1394camera_t *camera, dc1394framerate_t *framerate)
{
    uint32_t value;
    dc1394error_t err;

    err=dc1394_get_control_register(camera, REG_CAMERA_FRAME_RATE, &value);
    DC1394_ERR_RTN(err, "Could not get video framerate");

    *framerate= (uint32_t)((value >> 29) & 0x7UL) + DC1394_FRAMERATE_MIN;

    return err;
}

dc1394error_t
dc1394_video_set_framerate(dc1394camera_t *camera, dc1394framerate_t framerate)
{
    dc1394error_t err;
    if ( (framerate < DC1394_FRAMERATE_MIN) || (framerate > DC1394_FRAMERATE_MAX) ) {
        return DC1394_INVALID_FRAMERATE;
    }

    err=dc1394_set_control_register(camera, REG_CAMERA_FRAME_RATE, (uint32_t)(((framerate - DC1394_FRAMERATE_MIN) & 0x7UL) << 29));
    DC1394_ERR_RTN(err, "Could not set video framerate");

    return err;
}

dc1394error_t
dc1394_video_get_mode(dc1394camera_t *camera, dc1394video_mode_t *mode)
{
    dc1394error_t err;
    uint32_t value = 0; // set to zero to avoid valgrind errors
    uint32_t format = 0; // set to zero to avoid valgrind errors

    err= dc1394_get_control_register(camera, REG_CAMERA_VIDEO_FORMAT, &value);
    DC1394_ERR_RTN(err, "Could not get video format");

    format= (uint32_t)((value >> 29) & 0x7UL) + DC1394_FORMAT_MIN;

    err= dc1394_get_control_register(camera, REG_CAMERA_VIDEO_MODE, &value);
    DC1394_ERR_RTN(err, "Could not get video mode");

    switch(format) {
    case DC1394_FORMAT0:
        *mode= (uint32_t)((value >> 29) & 0x7UL) + DC1394_VIDEO_MODE_FORMAT0_MIN;
        break;
    case DC1394_FORMAT1:
        *mode= (uint32_t)((value >> 29) & 0x7UL) + DC1394_VIDEO_MODE_FORMAT1_MIN;
        break;
    case DC1394_FORMAT2:
        *mode= (uint32_t)((value >> 29) & 0x7UL) + DC1394_VIDEO_MODE_FORMAT2_MIN;
        break;
    case DC1394_FORMAT6:
        *mode= (uint32_t)((value >> 29) & 0x7UL) + DC1394_VIDEO_MODE_FORMAT6_MIN;
        break;
    case DC1394_FORMAT7:
        *mode= (uint32_t)((value >> 29) & 0x7UL) + DC1394_VIDEO_MODE_FORMAT7_MIN;
        break;
    default:
        return DC1394_INVALID_VIDEO_FORMAT;
        break;
    }

    return err;
}

dc1394error_t
dc1394_video_set_mode(dc1394camera_t *camera, dc1394video_mode_t  mode)
{
    uint32_t format, min;
    dc1394error_t err;

    if ( (mode<DC1394_VIDEO_MODE_MIN) || (mode>DC1394_VIDEO_MODE_MAX) )
        return DC1394_INVALID_VIDEO_MODE;

    err=get_format_from_mode(mode, &format);
    DC1394_ERR_RTN(err, "Invalid video mode code");

    switch(format) {
    case DC1394_FORMAT0:
        min= DC1394_VIDEO_MODE_FORMAT0_MIN;
        break;
    case DC1394_FORMAT1:
        min= DC1394_VIDEO_MODE_FORMAT1_MIN;
        break;
    case DC1394_FORMAT2:
        min= DC1394_VIDEO_MODE_FORMAT2_MIN;
        break;
    case DC1394_FORMAT6:
        min= DC1394_VIDEO_MODE_FORMAT6_MIN;
        break;
    case DC1394_FORMAT7:
        min= DC1394_VIDEO_MODE_FORMAT7_MIN;
        break;
    default:
        return DC1394_INVALID_VIDEO_MODE;
        break;
    }

    //if (format>FORMAT2)
    //  format+=DC1394_FORMAT_GAP;

    err=dc1394_set_control_register(camera, REG_CAMERA_VIDEO_FORMAT, (uint32_t)(((format - DC1394_FORMAT_MIN) & 0x7UL) << 29));
    DC1394_ERR_RTN(err, "Could not set video format");

    err=dc1394_set_control_register(camera, REG_CAMERA_VIDEO_MODE, (uint32_t)(((mode - min) & 0x7UL) << 29));
    DC1394_ERR_RTN(err, "Could not set video mode");

    return err;

}

dc1394error_t
dc1394_video_get_iso_speed(dc1394camera_t *camera, dc1394speed_t *speed)
{
    dc1394error_t err;
    uint32_t value;

    err=dc1394_get_control_register(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_RTN(err, "Could not get ISO data");

    if (camera->bmode_capable) { // check if 1394b is available
        if (value & 0x00008000) { //check if we are now using 1394b
            *speed= (uint32_t)(value& 0x7UL);
            if ((*speed<DC1394_ISO_SPEED_MIN)||(*speed>DC1394_ISO_SPEED_MAX)) // abort if speed not within valid range
                return DC1394_INVALID_ISO_SPEED;
        }
        else { // fallback to legacy
            *speed= (uint32_t)((value >> 24) & 0x3UL);
            if ((*speed<DC1394_ISO_SPEED_MIN)||(*speed>DC1394_ISO_SPEED_400)) // abort if speed not within valid range
                return DC1394_INVALID_ISO_SPEED;
        }
    }
    else { // legacy
        *speed= (uint32_t)((value >> 24) & 0x3UL);
        if ((*speed<DC1394_ISO_SPEED_MIN)||(*speed>DC1394_ISO_SPEED_400)) // abort if speed not within valid range
            return DC1394_INVALID_ISO_SPEED;
    }

    return err;
}

dc1394error_t
dc1394_video_set_iso_speed(dc1394camera_t *camera, dc1394speed_t speed)
{
    dc1394error_t err;
    uint32_t value=0;
    int channel;

    if ((speed>DC1394_ISO_SPEED_MAX) || (speed<DC1394_ISO_SPEED_MIN))
        return DC1394_INVALID_ISO_SPEED;

    err=dc1394_get_control_register(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_RTN(err, "Could not get ISO data");

    // check if 1394b is available and if we are now using 1394b
    if ((camera->bmode_capable)&&(value & 0x00008000)) {
        err=dc1394_get_control_register(camera, REG_CAMERA_ISO_DATA, &value);
        DC1394_ERR_RTN(err, "oops");
        channel=(value >> 8) & 0x3FUL;
        err=dc1394_set_control_register(camera, REG_CAMERA_ISO_DATA,
                                        (uint32_t) ( ((channel & 0x3FUL) << 8) | (speed & 0x7UL) | (0x1 << 15) ));
        DC1394_ERR_RTN(err, "oops");
    }
    else { // fallback to legacy
        if (speed>DC1394_ISO_SPEED_400-DC1394_ISO_SPEED_MIN) {
            dc1394_log_error("An ISO speed >400Mbps was requested while the camera is in LEGACY mode. Please set the operation mode to OPERATION_MODE_1394B before asking for 1394b ISO speeds");
            return DC1394_INVALID_ISO_SPEED;
        }
        err=dc1394_get_control_register(camera, REG_CAMERA_ISO_DATA, &value);
        DC1394_ERR_RTN(err, "oops");
        channel=(value >> 28) & 0xFUL;
        err=dc1394_set_control_register(camera, REG_CAMERA_ISO_DATA,
                                        (uint32_t) (((channel & 0xFUL) << 28) |
                                                    ((speed & 0x3UL) << 24) ));
        DC1394_ERR_RTN(err, "Could not set ISO data register");
    }

    return err;;
}

dc1394error_t
dc1394_video_get_iso_channel(dc1394camera_t *camera, uint32_t * channel)
{
    dc1394error_t err;
    uint32_t value_inq, value;

    err=dc1394_get_control_register(camera, REG_CAMERA_BASIC_FUNC_INQ, &value_inq);
    DC1394_ERR_RTN(err, "Could not get basic function register");

    err=dc1394_get_control_register(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_RTN(err, "Could not get ISO data");

    // check if 1394b is available and if we are now using 1394b
    if ((value_inq & 0x00800000)&&(value & 0x00008000))
        *channel = (value >> 8) & 0x3FUL;
    else
        *channel = (value >> 28) & 0xFUL;

    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_video_set_iso_channel(dc1394camera_t *camera, uint32_t channel)
{
    dc1394error_t err;
    uint32_t value_inq, value=0;
    int speed;

    err=dc1394_get_control_register(camera, REG_CAMERA_BASIC_FUNC_INQ, &value_inq);
    DC1394_ERR_RTN(err, "Could not get basic function register");

    err=dc1394_get_control_register(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_RTN(err, "Could not get ISO data");

    // check if 1394b is available and if we are now using 1394b
    if ((value_inq & 0x00800000)&&(value & 0x00008000)) {
        err=dc1394_get_control_register(camera, REG_CAMERA_ISO_DATA, &value);
        DC1394_ERR_RTN(err, "oops");
        speed=value & 0x7UL;
        err=dc1394_set_control_register(camera, REG_CAMERA_ISO_DATA,
                                        (uint32_t) ( ((channel & 0x3FUL) << 8) | (speed & 0x7UL) | (0x1 << 15) ));
        DC1394_ERR_RTN(err, "oops");
    }
    else { // fallback to legacy
        err=dc1394_get_control_register(camera, REG_CAMERA_ISO_DATA, &value);
        DC1394_ERR_RTN(err, "oops");
        speed=(value >> 24) & 0x3UL;
        if (speed>DC1394_ISO_SPEED_400-DC1394_ISO_SPEED_MIN) {
            dc1394_log_error("an ISO speed >400Mbps was requested while the camera is in LEGACY mode              Please set the operation mode to OPERATION_MODE_1394B before asking for\n              1394b ISO speeds");
            return DC1394_FAILURE;
        }
        err=dc1394_set_control_register(camera, REG_CAMERA_ISO_DATA,
                                        (uint32_t) (((channel & 0xFUL) << 28) | ((speed & 0x3UL) << 24) ));
        DC1394_ERR_RTN(err, "Could not set ISO data register");
    }

    return err;
}

dc1394error_t
dc1394_video_get_operation_mode(dc1394camera_t *camera, dc1394operation_mode_t  *mode)
{
    dc1394error_t err;
    uint32_t value;

    err=dc1394_get_control_register(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_RTN(err, "Could not get ISO data");

    if (camera->bmode_capable==DC1394_TRUE) {
        if ((value & 0x00008000) >0)
            *mode=DC1394_OPERATION_MODE_1394B;
        else
            *mode=DC1394_OPERATION_MODE_LEGACY;
    }
    else {
        *mode=DC1394_OPERATION_MODE_LEGACY;
    }

    return err;
}


dc1394error_t
dc1394_video_set_operation_mode(dc1394camera_t *camera, dc1394operation_mode_t  mode)
{
    dc1394error_t err;
    uint32_t value;

    if ( (mode<DC1394_OPERATION_MODE_MIN) || (mode>DC1394_OPERATION_MODE_MAX) )
        return DC1394_INVALID_OPERATION_MODE;

    err=dc1394_get_control_register(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_RTN(err, "Could not get ISO data");

    if (mode==DC1394_OPERATION_MODE_LEGACY) {
        err=dc1394_set_control_register(camera, REG_CAMERA_ISO_DATA, (uint32_t) (value & 0xFFFF7FFF));
        DC1394_ERR_RTN(err, "Could not set ISO data");
    }
    else { // 1394b
        if (camera->bmode_capable) { // if 1394b available
            err=dc1394_set_control_register(camera, REG_CAMERA_ISO_DATA, (uint32_t) (value | 0x00008000));
            DC1394_ERR_RTN(err, "Could not set ISO data");
        }
        else { // 1394b asked, but it is not available
            return DC1394_FUNCTION_NOT_SUPPORTED;
        }
    }

    return DC1394_SUCCESS;

}

dc1394error_t
dc1394_camera_set_power(dc1394camera_t *camera, dc1394switch_t pwr)
{
    dc1394error_t err;
    switch (pwr) {
    case DC1394_ON:
        err=dc1394_set_control_register(camera, REG_CAMERA_POWER, DC1394_FEATURE_ON);
        DC1394_ERR_RTN(err, "Could not switch camera ON");
        break;
    case DC1394_OFF:
        err=dc1394_set_control_register(camera, REG_CAMERA_POWER, DC1394_FEATURE_OFF);
        DC1394_ERR_RTN(err, "Could not switch camera OFF");
        break;
    default:
        err=DC1394_INVALID_ARGUMENT_VALUE;
        DC1394_ERR_RTN(err, "Invalid switch value");
    }
    return err;
}

dc1394error_t
dc1394_video_set_transmission(dc1394camera_t *camera, dc1394switch_t pwr)
{
    dc1394error_t err;

    if (pwr==DC1394_ON) {
        err=dc1394_set_control_register(camera, REG_CAMERA_ISO_EN, DC1394_FEATURE_ON);
        DC1394_ERR_RTN(err, "Could not start ISO transmission");
    }
    else {
        // first we stop ISO
        err=dc1394_set_control_register(camera, REG_CAMERA_ISO_EN, DC1394_FEATURE_OFF);
        DC1394_ERR_RTN(err, "Could not stop ISO transmission");
    }

    return err;
}

dc1394error_t
dc1394_video_get_transmission(dc1394camera_t *camera, dc1394switch_t *is_on)
{
    dc1394error_t err;
    uint32_t value;
    err= dc1394_get_control_register(camera, REG_CAMERA_ISO_EN, &value);
    DC1394_ERR_RTN(err, "Could not get ISO status");

    *is_on= (value & DC1394_FEATURE_ON)>>31;
    return err;
}

dc1394error_t
dc1394_video_set_one_shot(dc1394camera_t *camera, dc1394switch_t pwr)
{
    dc1394error_t err;
    switch (pwr) {
    case DC1394_ON:
        err=dc1394_set_control_register(camera, REG_CAMERA_ONE_SHOT, DC1394_FEATURE_ON);
        DC1394_ERR_RTN(err, "Could not set one-shot");
        break;
    case DC1394_OFF:
        err=dc1394_set_control_register(camera, REG_CAMERA_ONE_SHOT, DC1394_FEATURE_OFF);
        DC1394_ERR_RTN(err, "Could not unset one-shot");
        break;
    default:
        err=DC1394_INVALID_ARGUMENT_VALUE;
        DC1394_ERR_RTN(err, "Invalid switch value");
    }
    return err;
}

dc1394error_t
dc1394_video_get_one_shot(dc1394camera_t *camera, dc1394bool_t *is_on)
{
    uint32_t value;
    dc1394error_t err = dc1394_get_control_register(camera, REG_CAMERA_ONE_SHOT, &value);
    DC1394_ERR_RTN(err, "Could not get one-shot status");
    *is_on = ( value & DC1394_FEATURE_ON) ? DC1394_TRUE : DC1394_FALSE;
    return err;
}

dc1394error_t
dc1394_video_get_multi_shot(dc1394camera_t *camera, dc1394bool_t *is_on, uint32_t *numFrames)
{
    uint32_t value;
    dc1394error_t err = dc1394_get_control_register(camera, REG_CAMERA_ONE_SHOT, &value);
    DC1394_ERR_RTN(err, "Could not get multishot status");
    *is_on = (value & (DC1394_FEATURE_ON>>1)) >> 30;
    *numFrames= value & 0xFFFFUL;

    return err;
}

dc1394error_t
dc1394_video_set_multi_shot(dc1394camera_t *camera, uint32_t numFrames, dc1394switch_t pwr)
{
    dc1394error_t err;
    switch (pwr) {
    case DC1394_ON:
        err=dc1394_set_control_register(camera, REG_CAMERA_ONE_SHOT, (0x40000000UL | (numFrames & 0xFFFFUL)));
        DC1394_ERR_RTN(err, "Could not set multishot");
        break;
    case DC1394_OFF:
        err=dc1394_video_set_one_shot(camera,pwr);
        DC1394_ERR_RTN(err, "Could not unset multishot");
        break;
    default:
        err=DC1394_INVALID_ARGUMENT_VALUE;
        DC1394_ERR_RTN(err, "Invalid switch value");
    }
    return err;
}

dc1394error_t
dc1394_feature_whitebalance_get_value(dc1394camera_t *camera, uint32_t *u_b_value, uint32_t *v_r_value)
{
    uint32_t value;
    dc1394error_t err= dc1394_get_control_register(camera, REG_CAMERA_WHITE_BALANCE, &value);
    DC1394_ERR_RTN(err, "Could not get white balance");

    *u_b_value= (uint32_t)((value & 0xFFF000UL) >> 12);
    *v_r_value= (uint32_t)(value & 0xFFFUL);
    return err;
}

dc1394error_t
dc1394_feature_whitebalance_set_value(dc1394camera_t *camera, uint32_t u_b_value, uint32_t v_r_value)
{
    uint32_t curval;
    dc1394error_t err;
    err=dc1394_get_control_register(camera, REG_CAMERA_WHITE_BALANCE, &curval);
    DC1394_ERR_RTN(err, "Could not get white balance");

    curval= (curval & 0xFF000000UL) | ( ((u_b_value & 0xFFFUL) << 12) | (v_r_value & 0xFFFUL) );
    err=dc1394_set_control_register(camera, REG_CAMERA_WHITE_BALANCE, curval);
    DC1394_ERR_RTN(err, "Could not set white balance");
    return err;
}

dc1394error_t
dc1394_feature_temperature_get_value(dc1394camera_t *camera, uint32_t *target_temperature, uint32_t *temperature)
{
    uint32_t value;
    dc1394error_t err= dc1394_get_control_register(camera, REG_CAMERA_TEMPERATURE, &value);
    DC1394_ERR_RTN(err, "Could not get temperature");
    *target_temperature= (uint32_t)((value >> 12) & 0xFFF);
    *temperature= (uint32_t)(value & 0xFFFUL);
    return err;
}

dc1394error_t
dc1394_feature_temperature_set_value(dc1394camera_t *camera, uint32_t target_temperature)
{
    dc1394error_t err;
    uint32_t curval;

    err=dc1394_get_control_register(camera, REG_CAMERA_TEMPERATURE, &curval);
    DC1394_ERR_RTN(err, "Could not get temperature");

    curval= (curval & 0xFF000FFFUL) | ((target_temperature & 0xFFFUL) << 12);
    err= dc1394_set_control_register(camera, REG_CAMERA_TEMPERATURE, curval);
    DC1394_ERR_RTN(err, "Could not set temperature");

    return err;
}

dc1394error_t
dc1394_feature_whiteshading_get_value(dc1394camera_t *camera, uint32_t *r_value, uint32_t *g_value, uint32_t *b_value)
{
    uint32_t value;
    dc1394error_t err= dc1394_get_control_register(camera, REG_CAMERA_WHITE_SHADING, &value);
    DC1394_ERR_RTN(err, "Could not get white shading");

    *r_value= (uint32_t)((value & 0xFF0000UL) >> 16);
    *g_value= (uint32_t)((value & 0xFF00UL) >> 8);
    *b_value= (uint32_t)(value & 0xFFUL);

    return err;
}

dc1394error_t
dc1394_feature_whiteshading_set_value(dc1394camera_t *camera, uint32_t r_value, uint32_t g_value, uint32_t b_value)
{
    uint32_t curval;

    dc1394error_t err=dc1394_get_control_register(camera, REG_CAMERA_WHITE_SHADING, &curval);
    DC1394_ERR_RTN(err, "Could not get white shading");

    curval= (curval & 0xFF000000UL) | ( ((r_value & 0xFFUL) << 16) |
                                        ((g_value & 0xFFUL) << 8) |
                                         (b_value & 0xFFUL) );
    err=dc1394_set_control_register(camera, REG_CAMERA_WHITE_SHADING, curval);
    DC1394_ERR_RTN(err, "Could not set white shading");

    return err;
}

dc1394error_t
dc1394_external_trigger_get_mode(dc1394camera_t *camera, dc1394trigger_mode_t *mode)
{
    uint32_t value;
    dc1394error_t err= dc1394_get_control_register(camera, REG_CAMERA_TRIGGER_MODE, &value);
    DC1394_ERR_RTN(err, "Could not get trigger mode");

    *mode= (uint32_t)( ((value >> 16) & 0xFUL) );
    if ((*mode)>5)
        (*mode)-=8;
    (*mode)+= DC1394_TRIGGER_MODE_MIN;

    return err;
}

dc1394error_t
dc1394_external_trigger_set_mode(dc1394camera_t *camera, dc1394trigger_mode_t mode)
{
    dc1394error_t err;
    uint32_t curval;

    if ( (mode < DC1394_TRIGGER_MODE_MIN) || (mode > DC1394_TRIGGER_MODE_MAX) ) {
        return DC1394_INVALID_TRIGGER_MODE;
    }

    err=dc1394_get_control_register(camera, REG_CAMERA_TRIGGER_MODE, &curval);
    DC1394_ERR_RTN(err, "Could not get trigger mode");

    mode-= DC1394_TRIGGER_MODE_MIN;
    if (mode>5)
        mode+=8;
    curval= (curval & 0xFFF0FFFFUL) | ((mode & 0xFUL) << 16);
    err=dc1394_set_control_register(camera, REG_CAMERA_TRIGGER_MODE, curval);
    DC1394_ERR_RTN(err, "Could not set trigger mode");
    return err;
}


dc1394error_t
dc1394_external_trigger_get_supported_sources(dc1394camera_t *camera, dc1394trigger_sources_t *sources)
{
    uint32_t value;
    dc1394error_t err;
    uint64_t offset;
    int i;

    FEATURE_TO_INQUIRY_OFFSET(DC1394_FEATURE_TRIGGER, offset);
    err=dc1394_get_control_register(camera, offset, &value);
    DC1394_ERR_RTN(err,"Could not query supported trigger sources");

    sources->num=0;
    for (i = 0; i < DC1394_TRIGGER_SOURCE_NUM; i++) {
        if (value & (0x1 << (23-i-(i>3)*3))){
            sources->sources[sources->num]=i+DC1394_TRIGGER_SOURCE_MIN;
            sources->num++;
        }
    }

    return err;
}


dc1394error_t
dc1394_external_trigger_get_source(dc1394camera_t *camera, dc1394trigger_source_t *source)
{
    uint32_t value;
    dc1394error_t err= dc1394_get_control_register(camera, REG_CAMERA_TRIGGER_MODE, &value);
    DC1394_ERR_RTN(err, "Could not get trigger source");

    *source= (uint32_t)( ((value >> 21) & 0x7UL) );
    if (*source > 3)
        *source -= 3;
    (*source)+= DC1394_TRIGGER_SOURCE_MIN;

    return err;
}

dc1394error_t
dc1394_external_trigger_set_source(dc1394camera_t *camera, dc1394trigger_source_t source)
{
    dc1394error_t err;
    uint32_t curval;

    if ( (source < DC1394_TRIGGER_SOURCE_MIN) || (source > DC1394_TRIGGER_SOURCE_MAX) ) {
        return DC1394_INVALID_TRIGGER_SOURCE;
    }

    err=dc1394_get_control_register(camera, REG_CAMERA_TRIGGER_MODE, &curval);
    DC1394_ERR_RTN(err, "Could not get trigger source");

    source-= DC1394_TRIGGER_SOURCE_MIN;
    if (source > 3)
        source += 3;
    curval= (curval & 0xFF1FFFFFUL) | ((source & 0x7UL) << 21);
    err=dc1394_set_control_register(camera, REG_CAMERA_TRIGGER_MODE, curval);
    DC1394_ERR_RTN(err, "Could not set trigger source");
    return err;
}

dc1394error_t
dc1394_feature_get_value(dc1394camera_t *camera, dc1394feature_t feature, uint32_t *value)
{
    uint32_t quadval;
    uint64_t offset;
    dc1394error_t err;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    if ((feature==DC1394_FEATURE_WHITE_BALANCE)||
        (feature==DC1394_FEATURE_WHITE_SHADING)||
        (feature==DC1394_FEATURE_TEMPERATURE)) {
        err=DC1394_INVALID_FEATURE;
        DC1394_ERR_RTN(err, "You should use the specific functions to read from multiple-value features");
    }

    FEATURE_TO_VALUE_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get feature value");
    *value= (uint32_t)(quadval & 0xFFFUL);

    return err;
}

dc1394error_t
dc1394_feature_set_value(dc1394camera_t *camera, dc1394feature_t feature, uint32_t value)
{
    uint32_t quadval;
    uint64_t offset;
    dc1394error_t err;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    if ((feature==DC1394_FEATURE_WHITE_BALANCE)||
        (feature==DC1394_FEATURE_WHITE_SHADING)||
        (feature==DC1394_FEATURE_TEMPERATURE)) {
        err=DC1394_INVALID_FEATURE;
        DC1394_ERR_RTN(err, "You should use the specific functions to write from multiple-value features");
    }

    FEATURE_TO_VALUE_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get feature value");

    err=dc1394_set_control_register(camera, offset, (quadval & 0xFFFFF000UL) | (value & 0xFFFUL));
    DC1394_ERR_RTN(err, "Could not set feature value");
    return err;
}

dc1394error_t
dc1394_feature_is_present(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value)
{
/*

  NOTE ON FEATURE PRESENCE DETECTION:

  The IIDC specs have 3 locations where the feature presence is notified, at offsets 0x40X,
  0x5XX and 0x8XX. The specs do not give any difference between the different locations,
  leading to different interpretations by different manufacturers, or even from model to
  model. Firmware revisions may also reflect a change in interpretation by a company. This
  problem is acknowledged by the IIDC working group and will be resolved in IIDC 1.32.

  In the meantime, the policy of libdc1394 is to make an AND of the three locations to
  determine if a feature is available or not. No other verifications is performed.

  Some manufacturer may choose to indicate feature presence by other means, such as setting
  a feature OFF and simultaneously disabling the capability to turn the feature ON. Another
  technique is to disable all control means (on/off, manual, auto, absolute, etc.),
  effectively resulting in a feature that can't be used.

  This kind of interpretation could be implemented in libdc1394. However, the feature may
  still be writable even if it is not possible to use it. To allow this off-state writing,
  the decision on whether a feature is available or not is not taking this into account.

  Damien

  */

    dc1394error_t err;
    uint64_t offset;
    uint32_t quadval;

    *value=DC1394_FALSE;

    if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) ) {
        return DC1394_INVALID_FEATURE;
    }

    if (feature < DC1394_FEATURE_ZOOM) {
        offset= REG_CAMERA_FEATURE_HI_INQ;
    }
    else {
        offset= REG_CAMERA_FEATURE_LO_INQ;
    }

    // check feature presence in 0x40x
    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get register for feature");

    if (is_feature_bit_set(quadval, feature)!=DC1394_TRUE) {
        *value=DC1394_FALSE;
        return DC1394_SUCCESS;
    }

    // if feature is present in 0x40x, check for availability in 0x5xx
    FEATURE_TO_INQUIRY_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get register for feature");

    if (quadval & 0x80000000UL) {
        *value= DC1394_TRUE;
    }
    else {
        *value= DC1394_FALSE;
        return DC1394_SUCCESS;
    }

    // if feature is present in 0x5xx, check for availability in 0x8xx
    FEATURE_TO_VALUE_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get register for feature");

    if (quadval & 0x80000000UL) {
        *value= DC1394_TRUE;
    }
    else {
        *value= DC1394_FALSE;
        return DC1394_SUCCESS;
    }

    return err;
}

dc1394error_t
dc1394_feature_is_readable(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t quadval;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    FEATURE_TO_INQUIRY_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get read-out capability for feature");

    *value = (quadval & 0x08000000UL) ? DC1394_TRUE: DC1394_FALSE;

    return err;
}

dc1394error_t
dc1394_feature_is_switchable(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t quadval;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    FEATURE_TO_INQUIRY_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get power capability for feature");

    *value = (quadval & 0x04000000UL) ? DC1394_TRUE: DC1394_FALSE;

    return err;
}

dc1394error_t
dc1394_feature_get_power(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t *value)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t quadval;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    FEATURE_TO_VALUE_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get feature status");

    *value = (quadval & 0x02000000UL) ? DC1394_TRUE: DC1394_FALSE;

    return err;
}

dc1394error_t
dc1394_feature_set_power(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t value)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t curval;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    FEATURE_TO_VALUE_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &curval);
    DC1394_ERR_RTN(err, "Could not get feature register");

    if (value && !(curval & 0x02000000UL)) {
        curval|= 0x02000000UL;
        err=dc1394_set_control_register(camera, offset, curval);
        DC1394_ERR_RTN(err, "Could not set feature power");
    }
    else if (!value && (curval & 0x02000000UL)) {
        curval&= 0xFDFFFFFFUL;
        err=dc1394_set_control_register(camera, offset, curval);
        DC1394_ERR_RTN(err, "Could not set feature power");
    }

    return err;
}


dc1394error_t
dc1394_feature_get_modes(dc1394camera_t *camera, dc1394feature_t feature, dc1394feature_modes_t *modes)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t quadval;

    modes->num=0;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    if (feature == DC1394_FEATURE_TRIGGER) {
        return DC1394_SUCCESS; // success, but no mode is available.
    }

    FEATURE_TO_INQUIRY_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get mode availability for feature");

    if (quadval & 0x01000000UL) {
        modes->modes[modes->num]=DC1394_FEATURE_MODE_MANUAL;
        modes->num++;
    }
    if (quadval & 0x02000000UL) {
        modes->modes[modes->num]=DC1394_FEATURE_MODE_AUTO;
        modes->num++;
    }
    if (quadval & 0x10000000UL) {
        modes->modes[modes->num]=DC1394_FEATURE_MODE_ONE_PUSH_AUTO;
        modes->num++;
    }

    return err;
}


dc1394error_t
dc1394_feature_get_mode(dc1394camera_t *camera, dc1394feature_t feature, dc1394feature_mode_t *mode)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t quadval;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    if ((feature == DC1394_FEATURE_TRIGGER)||
        (feature == DC1394_FEATURE_TRIGGER_DELAY)) {
        *mode=DC1394_FEATURE_MODE_MANUAL;
    }

    FEATURE_TO_VALUE_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get feature auto status");

    if (quadval & 0x04000000UL) {
        *mode= DC1394_FEATURE_MODE_ONE_PUSH_AUTO;
    }
    else if (quadval & 0x01000000UL) {
        *mode= DC1394_FEATURE_MODE_AUTO;
    }
    else {
        *mode= DC1394_FEATURE_MODE_MANUAL;
    }

    return err;
}

dc1394error_t
dc1394_feature_set_mode(dc1394camera_t *camera, dc1394feature_t feature, dc1394feature_mode_t mode)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t curval;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    if ( (mode<DC1394_FEATURE_MODE_MIN) || (mode>DC1394_FEATURE_MODE_MAX) )
        return DC1394_INVALID_FEATURE_MODE;

    if (feature == DC1394_FEATURE_TRIGGER) {
        return DC1394_INVALID_FEATURE;
    }

    FEATURE_TO_VALUE_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &curval);
    DC1394_ERR_RTN(err, "Could not get feature register");

    if ((mode==DC1394_FEATURE_MODE_AUTO) && !(curval & 0x01000000UL)) {
        curval|= 0x01000000UL;
        err=dc1394_set_control_register(camera, offset, curval);
        DC1394_ERR_RTN(err, "Could not set auto mode for feature");
    }
    else if ((mode==DC1394_FEATURE_MODE_MANUAL) && (curval & 0x01000000UL)) {
        curval&= 0xFEFFFFFFUL;
        err=dc1394_set_control_register(camera, offset, curval);
        DC1394_ERR_RTN(err, "Could not set auto mode for feature");
    }
    else if ((mode==DC1394_FEATURE_MODE_ONE_PUSH_AUTO)&& !(curval & 0x04000000UL)) {
        curval|= 0x04000000UL;
        err=dc1394_set_control_register(camera, offset, curval);
        DC1394_ERR_RTN(err, "Could not sart one-push capability for feature");
    }

    return err;
}

dc1394error_t
dc1394_feature_get_boundaries(dc1394camera_t *camera, dc1394feature_t feature, uint32_t *min, uint32_t *max)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t quadval;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    if (feature == DC1394_FEATURE_TRIGGER) {
        return DC1394_INVALID_FEATURE;
    }

    FEATURE_TO_INQUIRY_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get feature min value");

    *min= (uint32_t)((quadval & 0xFFF000UL) >> 12);
    *max= (uint32_t)(quadval & 0xFFFUL);
    return err;
}

/*
 * Memory load/save functions
 */

dc1394error_t
dc1394_memory_busy(dc1394camera_t *camera, dc1394bool_t *value)
{
    uint32_t quadlet;
    dc1394error_t err= dc1394_get_control_register(camera, REG_CAMERA_MEMORY_SAVE, &quadlet);
    DC1394_ERR_RTN(err, "Could not get memory busy status");
    *value = (quadlet & DC1394_FEATURE_ON) >> 31;
    return err;
}

dc1394error_t
dc1394_memory_save(dc1394camera_t *camera, uint32_t channel)
{
    dc1394error_t err=dc1394_set_control_register(camera, REG_CAMERA_MEM_SAVE_CH, (uint32_t)((channel & 0xFUL) << 28));
    DC1394_ERR_RTN(err, "Could not save memory channel");

    err=dc1394_set_control_register(camera, REG_CAMERA_MEMORY_SAVE, DC1394_FEATURE_ON);
    DC1394_ERR_RTN(err, "Could not save to memory");
    return err;
}

dc1394error_t
dc1394_memory_load(dc1394camera_t *camera, uint32_t channel)
{
    dc1394error_t err=dc1394_set_control_register(camera, REG_CAMERA_CUR_MEM_CH, (uint32_t)((channel & 0xFUL) << 28));
    DC1394_ERR_RTN(err, "Could not load from memory");
    return err;
}

/*
 * Trigger functions
 */

dc1394error_t
dc1394_external_trigger_set_polarity(dc1394camera_t *camera, dc1394trigger_polarity_t polarity)
{
    dc1394error_t err;
    uint32_t curval;

    if ( (polarity<DC1394_TRIGGER_ACTIVE_MIN) || (polarity>DC1394_TRIGGER_ACTIVE_MAX) )
        return DC1394_INVALID_TRIGGER_POLARITY;

    err=dc1394_get_control_register(camera, REG_CAMERA_TRIGGER_MODE, &curval);
    DC1394_ERR_RTN(err, "Could not get trigger register");

    curval= (curval & 0xFEFFFFFFUL) | ((polarity & 0x1UL) << 24);
    err=dc1394_set_control_register(camera, REG_CAMERA_TRIGGER_MODE, curval);
    DC1394_ERR_RTN(err, "Could not set set trigger polarity");
    return err;
}

dc1394error_t
dc1394_external_trigger_get_polarity(dc1394camera_t *camera, dc1394trigger_polarity_t *polarity)
{
    uint32_t value;
    dc1394error_t err= dc1394_get_control_register(camera, REG_CAMERA_TRIGGER_MODE, &value);
    DC1394_ERR_RTN(err, "Could not get trigger polarity");

    *polarity= (uint32_t)( ((value >> 24) & 0x1UL) )+DC1394_TRIGGER_ACTIVE_MIN;
    return err;
}

dc1394error_t
dc1394_external_trigger_has_polarity(dc1394camera_t *camera, dc1394bool_t *polarity)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t quadval;

    offset= REG_CAMERA_FEATURE_HI_BASE_INQ;

    err=dc1394_get_control_register(camera, offset + ((DC1394_FEATURE_TRIGGER - DC1394_FEATURE_MIN) * 0x04U), &quadval);
    DC1394_ERR_RTN(err, "Could not get trigger polarity capability");

    *polarity = (quadval & 0x02000000UL) ? DC1394_TRUE: DC1394_FALSE;

    return err;
}

dc1394error_t
dc1394_external_trigger_set_power(dc1394camera_t *camera, dc1394switch_t pwr)
{
    dc1394error_t err=dc1394_feature_set_power(camera, DC1394_FEATURE_TRIGGER, pwr);
    DC1394_ERR_RTN(err, "Could not set external trigger");
    return err;
}

dc1394error_t
dc1394_external_trigger_get_power(dc1394camera_t *camera, dc1394switch_t *pwr)
{
    dc1394error_t err=dc1394_feature_get_power(camera, DC1394_FEATURE_TRIGGER, pwr);
    DC1394_ERR_RTN(err, "Could not set external trigger");
    return err;
}

dc1394error_t
dc1394_software_trigger_set_power(dc1394camera_t *camera, dc1394switch_t pwr)
{
    dc1394error_t err;

    if (pwr==DC1394_ON) {
        err=dc1394_set_control_register(camera, REG_CAMERA_SOFT_TRIGGER, DC1394_FEATURE_ON);
    }
    else {
        err=dc1394_set_control_register(camera, REG_CAMERA_SOFT_TRIGGER, DC1394_FEATURE_OFF);
    }
    DC1394_ERR_RTN(err, "Could not set software trigger");
    return err;
}

dc1394error_t
dc1394_software_trigger_get_power(dc1394camera_t *camera, dc1394switch_t *pwr)
{
    uint32_t value;
    dc1394error_t err = dc1394_get_control_register(camera, REG_CAMERA_SOFT_TRIGGER, &value);
    DC1394_ERR_RTN(err, "Could not get software trigger status");

    *pwr = (value & DC1394_FEATURE_ON)? DC1394_ON : DC1394_OFF;

    return err;
}

dc1394error_t
dc1394_video_get_data_depth(dc1394camera_t *camera, uint32_t *depth)
{
    dc1394error_t err;
    uint32_t value;
    dc1394video_mode_t mode;
    dc1394color_coding_t coding;

    *depth = 0;
    if (camera->iidc_version >= DC1394_IIDC_VERSION_1_31) {
        err= dc1394_get_control_register(camera, REG_CAMERA_DATA_DEPTH, &value);
        if (err==DC1394_SUCCESS)
            *depth = value >> 24;
    }

    /* For cameras that do not have the DATA_DEPTH register, perform a
       sane default. */
    if (*depth == 0) {
        err = dc1394_video_get_mode(camera, &mode);
        DC1394_ERR_RTN(err, "Could not get video mode");

        if (dc1394_is_video_mode_scalable (mode))
            return dc1394_format7_get_data_depth (camera, mode, depth);

        err = dc1394_get_color_coding_from_video_mode (camera, mode, &coding);
        DC1394_ERR_RTN(err, "Could not get color coding");

        err = dc1394_get_color_coding_data_depth (coding, depth);
        DC1394_ERR_RTN(err, "Could not get data depth from color coding");

        return err;
    }

    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_feature_get_absolute_control(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t *pwr)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t quadval;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    FEATURE_TO_VALUE_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get get abs control for feature");

    *pwr = (quadval & 0x40000000UL) ? DC1394_TRUE: DC1394_FALSE;

    return err;
}

dc1394error_t
dc1394_feature_set_absolute_control(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t pwr)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t curval;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    FEATURE_TO_VALUE_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &curval);
    DC1394_ERR_RTN(err, "Could not get abs setting status for feature");

    if (pwr && !(curval & 0x40000000UL)) {
        curval|= 0x40000000UL;
        err=dc1394_set_control_register(camera, offset, curval);
        DC1394_ERR_RTN(err, "Could not set absolute control for feature");
    }
    else if (!pwr && (curval & 0x40000000UL)) {
        curval&= 0xBFFFFFFFUL;
        err=dc1394_set_control_register(camera, offset, curval);
        DC1394_ERR_RTN(err, "Could not set absolute control for feature");
    }

    return err;
}


dc1394error_t
dc1394_feature_has_absolute_control(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value)
{
    dc1394error_t err;
    uint64_t offset;
    uint32_t quadval;

    if ( (feature<DC1394_FEATURE_MIN) || (feature>DC1394_FEATURE_MAX) )
        return DC1394_INVALID_FEATURE;

    FEATURE_TO_INQUIRY_OFFSET(feature, offset);

    err=dc1394_get_control_register(camera, offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get absolute control register for feature");

    *value = (quadval & 0x40000000UL) ? DC1394_TRUE: DC1394_FALSE;

    return err;
}


dc1394error_t
dc1394_video_get_bandwidth_usage(dc1394camera_t *camera, uint32_t *bandwidth)
{
    uint32_t format, qpp;
    dc1394video_mode_t video_mode;
    dc1394speed_t speed;
    dc1394framerate_t framerate=0;
    dc1394error_t err;

    // get format and mode
    err=dc1394_video_get_mode(camera, &video_mode);
    DC1394_ERR_RTN(err, "Could not get video mode");

    err=get_format_from_mode(video_mode, &format);
    DC1394_ERR_RTN(err, "Invalid mode ID");

    if (format==DC1394_FORMAT7) {
        // use the bytes per packet value:
        err=dc1394_format7_get_packet_size(camera, video_mode, &qpp);
        DC1394_ERR_RTN(err, "Could not get BPP");
        qpp=qpp/4;
    }
    else {
        // get the framerate:
        err=dc1394_video_get_framerate(camera, &framerate);
        DC1394_ERR_RTN(err, "Could not get framerate");
        err=get_quadlets_per_packet(video_mode, framerate, &qpp);
    }
    // add the ISO header and footer:
    qpp+=3;

    // get camera ISO speed:
    err=dc1394_video_get_iso_speed(camera, &speed);
    DC1394_ERR_RTN(err, "Could not get ISO speed");

    // mutiply by 4 anyway because the best speed is SPEED_400 only
    if (speed>=DC1394_ISO_SPEED_1600)
        *bandwidth = qpp >> (speed-DC1394_ISO_SPEED_1600);
    else
        *bandwidth = qpp << (DC1394_ISO_SPEED_1600-speed);

    return err;
}

dc1394error_t
dc1394_feature_get_absolute_boundaries(dc1394camera_t *camera, dc1394feature_t feature, float *min, float *max)
{
    dc1394error_t err=DC1394_SUCCESS;

    if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) ) {
        return DC1394_INVALID_FEATURE;
    }

    err=dc1394_get_absolute_register(camera, feature, REG_CAMERA_ABS_MAX, (uint32_t*)max);
    DC1394_ERR_RTN(err,"Could not get maximal absolute value");
    err=dc1394_get_absolute_register(camera, feature, REG_CAMERA_ABS_MIN, (uint32_t*)min);
    DC1394_ERR_RTN(err,"Could not get minimal absolute value");

    return err;
}


dc1394error_t
dc1394_feature_get_absolute_value(dc1394camera_t *camera, dc1394feature_t feature, float *value)
{
    dc1394error_t err=DC1394_SUCCESS;

    if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) ) {
        return DC1394_INVALID_FEATURE;
    }
    err=dc1394_get_absolute_register(camera, feature, REG_CAMERA_ABS_VALUE, (uint32_t*)value);
    DC1394_ERR_RTN(err,"Could not get current absolute value");

    return err;
}


dc1394error_t
dc1394_feature_set_absolute_value(dc1394camera_t *camera, dc1394feature_t feature, float value)
{
    dc1394error_t err=DC1394_SUCCESS;

    uint32_t tempq;
    memcpy(&tempq,&value,4);

    if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) ) {
        return DC1394_INVALID_FEATURE;
    }

    dc1394_set_absolute_register(camera, feature, REG_CAMERA_ABS_VALUE, tempq);
    DC1394_ERR_RTN(err,"Could not get current absolute value");

    return err;
}


dc1394error_t
dc1394_pio_set(dc1394camera_t *camera, uint32_t value)
{
    dc1394error_t err=DC1394_SUCCESS;

    err=dc1394_set_PIO_register(camera, REG_CAMERA_PIO_OUT, value);
    DC1394_ERR_RTN(err,"Could not set PIO value");

    return err;
}


dc1394error_t
dc1394_pio_get(dc1394camera_t *camera, uint32_t *value)
{
    dc1394error_t err=DC1394_SUCCESS;

    err=dc1394_get_PIO_register(camera, REG_CAMERA_PIO_IN, value);
    DC1394_ERR_RTN(err,"Could not get PIO value");

    return err;
}

/*******************************************************************************
   New API for camera detection. For now we only have wrappers around existing
   functions, which is of course far from optimal.
 *******************************************************************************/

/*
  Create a new dc1394 struct, which also initialises the library
*/
dc1394_t *
dc1394_new (void)
{
    dc1394_t * d = calloc (1, sizeof (dc1394_t));
#ifdef HAVE_LINUX
#ifdef HAVE_LIBRAW1394
    linux_init (d);
#endif
    juju_init (d);
#endif
#ifdef HAVE_MACOSX
    macosx_init (d);
#endif
#ifdef HAVE_WINDOWS
    windows_init (d);
#endif
#ifdef HAVE_LIBUSB
    dc1394_usb_init (d);
#endif

    int i;
    int initializations = 0;
    for (i = 0; i < d->num_platforms; i++) {
        dc1394_log_debug ("Initializing platform %d: %s",
                i, d->platforms[i].name);
        d->platforms[i].p = d->platforms[i].dispatch->platform_new ();
        if (d->platforms[i].p) {
            initializations++;
            dc1394_log_debug ("Initialized platform %d", i);
        }
        else {
            dc1394_log_debug ("Failed to initialize platform %d", i);
        }
    }

    if (initializations == 0) {
        dc1394_free (d);
        dc1394_log_error ("Failed to initialize libdc1394");
        return NULL;
    }
    return d;
}

/*
  Free a dc1394 struct, which also terminates the use of the library
*/
void
dc1394_free (dc1394_t * d)
{
    free_enumeration (d);
    int i;
    for (i = 0; i < d->num_platforms; i++) {
        if (d->platforms[i].p)
            d->platforms[i].dispatch->platform_free (d->platforms[i].p);
        d->platforms[i].p = NULL;
    }
    free (d->platforms);
    d->platforms = NULL;
    free (d);
}

void register_platform (dc1394_t * d, const platform_dispatch_t * dispatch,
        const char * name)
{
    if (!dispatch->platform_new || !dispatch->platform_free ||
            !dispatch->get_device_list || !dispatch->free_device_list ||
            !dispatch->device_get_config_rom ||
            !dispatch->camera_new || !dispatch->camera_free ||
            !dispatch->camera_set_parent || !dispatch->camera_read ||
            !dispatch->camera_write) {
        dc1394_log_error ("Platform %s is missing required functions", name);
        return;
    }
    int n = d->num_platforms;
    d->platforms = realloc(d->platforms, (n+1)*sizeof(platform_info_t));
    d->platforms[n].dispatch = dispatch;
    d->platforms[n].name = name;
    d->platforms[n].device_list = NULL;
    d->platforms[n].p = NULL;
    d->num_platforms++;
}

char *
get_leaf_string (platform_camera_t * pcam, const platform_dispatch_t * disp,
        uint32_t offset)
{
    uint32_t quad;
    int len, i;
    char * str;

    if (disp->camera_read (pcam, offset, &quad, 1) < 0)
        return NULL;

    len = quad >> 16;
    str = malloc (4 * (len - 2) + 1);
    for (i = 0; i < len - 2; i++) {
        if (disp->camera_read (pcam, offset + 12 + 4 * i, &quad, 1) < 0) {
            free (str);
            return NULL;
        }
        str[4*i+0] = quad >> 24;
        str[4*i+1] = (quad >> 16) & 0xff;
        str[4*i+2] = (quad >> 8) & 0xff;
        str[4*i+3] = quad & 0xff;
    }
    str[4*i] = '\0';
    return str;
}

dc1394camera_t *
dc1394_camera_new_unit (dc1394_t * d, uint64_t guid, int unit)
{
    int i;
    camera_info_t * info = NULL;
    platform_camera_t * pcam;
    const platform_dispatch_t * disp;
    uint32_t command_regs_base = 0;
    uint32_t vendor_name_offset = 0;
    uint32_t model_name_offset = 0;
    uint32_t unit_sub_sw_version = 0;
    uint32_t ghigh, glow, quad;
    uint32_t offset, num_entries;
    dc1394camera_t * camera;
    dc1394camera_priv_t * cpriv;

    if (!d->num_cameras)
        refresh_enumeration (d);

    for (i = 0; i < d->num_cameras; i++) {
        if (d->cameras[i].guid == guid &&
            (unit < 0 || d->cameras[i].unit == unit)) {
            info = d->cameras + i;
            break;
        }
    }
    if (!info)
        return NULL;

    disp = info->platform->dispatch;
    pcam = disp->camera_new (info->platform->p, info->device,
            info->unit_dependent_directory);
    if (!pcam)
        return NULL;

    /* Check to make sure the GUID still matches. */
    if (disp->camera_read (pcam, 0x40C, &ghigh, 1) < 0 ||
        disp->camera_read (pcam, 0x410, &glow, 1) < 0)
        goto fail;

    if (ghigh != (info->guid >> 32) || glow != (info->guid & 0xffffffff))
        goto fail;

    if (disp->camera_read (pcam, info->unit_dependent_directory,
                &quad, 1) < 0)
        goto fail;

    num_entries = quad >> 16;
    offset = info->unit_dependent_directory + 4;
    for (i = 0; i < num_entries; i++) {
        if (disp->camera_read (pcam, offset + 4 * i, &quad, 1) < 0)
            goto fail;
        if ((quad >> 24) == 0x40)
            command_regs_base = quad & 0xffffff;
        else if ((quad >> 24) == 0x81) {
            /*
               The iSight version 1.0.3 has two 0x81 (vendor) leaves instead
               of a 0x81 and a 0x82 (model leaf). To go around this problem,
               we save the second vendor leaf as the model leaf. This is safe
               because if there is two 0x81 AND a 0x82, the real model leaf
               will overwrite the spurious second vendor string.
            */
            if (vendor_name_offset==0)
                vendor_name_offset = offset + 4 * ((quad & 0xffffff) + i);
            else
                model_name_offset = offset + 4 * ((quad & 0xffffff) + i);
        }
        else if ((quad >> 24) == 0x82)
            model_name_offset = offset + 4 * ((quad & 0xffffff) + i);
        else if ((quad >> 24) == 0x38)
            unit_sub_sw_version = quad & 0xffffff;
    }

    if (!command_regs_base)
        goto fail;

    camera = calloc (1, sizeof (dc1394camera_priv_t));
    cpriv = DC1394_CAMERA_PRIV (camera);

    cpriv->pcam = pcam;
    cpriv->platform = info->platform;
    camera->guid = info->guid;
    camera->unit = info->unit;
    camera->unit_spec_ID = info->unit_spec_ID;
    camera->unit_sw_version = info->unit_sw_version;
    camera->unit_sub_sw_version = unit_sub_sw_version;
    camera->unit_directory = info->unit_directory;
    camera->unit_dependent_directory = info->unit_dependent_directory;
    camera->command_registers_base = command_regs_base * 4;
    camera->vendor_id = info->vendor_id;
    camera->model_id = info->model_id;

    camera->vendor = get_leaf_string (pcam, disp, vendor_name_offset);
    camera->model = get_leaf_string (pcam, disp, model_name_offset);

    if (camera->unit_spec_ID == 0xA02D) {
        if (info->unit_sw_version == 0x100)
            camera->iidc_version = DC1394_IIDC_VERSION_1_04;
        else if (info->unit_sw_version == 0x101)
            camera->iidc_version = DC1394_IIDC_VERSION_1_20;
        else if (info->unit_sw_version == 0x102) {
            camera->iidc_version = DC1394_IIDC_VERSION_1_30;
            // only add sub_sw_version if it is valid. Otherwise
            // consider that it's IIDC 1.30 (hence add nothing)
            if ((unit_sub_sw_version >> 4)<=9)
                camera->iidc_version += unit_sub_sw_version >> 4;
        }
    }
    else
        camera->iidc_version = DC1394_IIDC_VERSION_PTGREY;

    disp->camera_set_parent (cpriv->pcam, camera);
    update_camera_info (camera);

    return camera;

 fail:
    disp->camera_free (pcam);
    return NULL;
}

dc1394camera_t *
dc1394_camera_new (dc1394_t * d, uint64_t guid)
{
    return dc1394_camera_new_unit (d, guid, -1);
}

/*
  Free a camera structure as well as resources used by that camera (bandwidth,
  ISO channels, etc...)
*/
void
dc1394_camera_free(dc1394camera_t *camera)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);

    if (cpriv->iso_persist!=1)
        dc1394_iso_release_all(camera);

    cpriv->platform->dispatch->camera_free (cpriv->pcam);
    free (camera->vendor);
    free (camera->model);
    free (camera);
}
