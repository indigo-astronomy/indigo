/*
 * 1394-Based Digital Camera Control Library
 *
 * Allied Vision Technologies (AVT) specific extensions
 * 
 * Written by Pierre MOOS <pierre.moos@gmail.com>
 *
 * Copyright (C) 2005 Inria Sophia-Antipolis
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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "vendor/avt.h"

/********************************************************/
/* Configuration Register Offsets for Advances features */
/********************************************************/

#define REG_CAMERA_AVT_VERSION_INFO1                        0x010U
#define REG_CAMERA_AVT_VERSION_INFO3                        0x018U
#define REG_CAMERA_AVT_ADV_INQ_1                        0x040U
#define REG_CAMERA_AVT_ADV_INQ_2                        0x044U
#define REG_CAMERA_AVT_ADV_INQ_3                        0x048U
#define REG_CAMERA_AVT_ADV_INQ_4                        0x04CU
#define REG_CAMERA_AVT_MAX_RESOLUTION                        0x200U
#define REG_CAMERA_AVT_TIMEBASE                                0x208U
#define REG_CAMERA_AVT_EXTD_SHUTTER                        0x20CU
#define REG_CAMERA_AVT_TEST_IMAGE                        0x210U
#define REG_CAMERA_AVT_SEQUENCE_CTRL                        0x220U
#define REG_CAMERA_AVT_SEQUENCE_PARAM                        0x224U
#define REG_CAMERA_AVT_LUT_CTRL                                0x240U
#define REG_CAMERA_AVT_LUT_MEM_CTRL                        0x244U
#define REG_CAMERA_AVT_LUT_INFO                                0x248U
#define REG_CAMERA_AVT_SHDG_CTRL                        0x250U
#define REG_CAMERA_AVT_SHDG_MEM_CTRL                        0x254U
#define REG_CAMERA_AVT_SHDG_INFO                        0x258U
#define REG_CAMERA_AVT_DEFERRED_TRANS                        0x260U
#define REG_CAMERA_AVT_FRAMEINFO                        0x270U
#define REG_CAMERA_AVT_FRAMECOUNTER                        0x274U
#define REG_CAMERA_AVT_HDR_CONTROL                        0x280U
#define REG_CAMERA_AVT_KNEEPOINT_1                        0x284U
#define REG_CAMERA_AVT_KNEEPOINT_2                        0x288U
#define REG_CAMERA_AVT_KNEEPOINT_3                        0x28CU
#define REG_CAMERA_AVT_DSNU_CONTROL                        0x290U
#define REG_CAMERA_AVT_BLEMISH_CONTROL                        0x294U
#define REG_CAMERA_AVT_IO_INP_CTRL1                        0x300U
#define REG_CAMERA_AVT_IO_INP_CTRL2                        0x304U
#define REG_CAMERA_AVT_IO_INP_CTRL3                        0x308U
#define REG_CAMERA_AVT_IO_INP_CTRL4                        0x30CU
#define REG_CAMERA_AVT_IO_OUTP_CTRL1                        0x320U
#define REG_CAMERA_AVT_IO_OUTP_CTRL2                        0x324U
#define REG_CAMERA_AVT_IO_OUTP_CTRL3                        0x328U
#define REG_CAMERA_AVT_IO_OUTP_CTRL4                        0x32CU
#define REG_CAMERA_AVT_IO_INTENA_DELAY                        0x340U
#define REG_CAMERA_AVT_AUTOSHUTTER_CTRL                        0x360U
#define REG_CAMERA_AVT_AUTOSHUTTER_LO                        0x364U
#define REG_CAMERA_AVT_AUTOSHUTTER_HI                        0x368U
#define REG_CAMERA_AVT_AUTOGAIN_CTRL                        0x370U
#define REG_CAMERA_AVT_AUTOFNC_AOI                        0x390U
#define REG_CAMERA_AVT_AF_AREA_POSITION                        0x394U
#define REG_CAMERA_AVT_AF_AREA_SIZE                        0x398U
#define REG_CAMERA_AVT_COLOR_CORR                        0x3A0U
#define REG_CAMERA_AVT_COLOR_CORR_CRR                        0x3A4U
#define REG_CAMERA_AVT_COLOR_CORR_CGR                        0x3A8U
#define REG_CAMERA_AVT_COLOR_CORR_CBR                        0x3ACU
#define REG_CAMERA_AVT_COLOR_CORR_CRG                        0x3B0U
#define REG_CAMERA_AVT_COLOR_CORR_CGG                        0x3B4U
#define REG_CAMERA_AVT_COLOR_CORR_CBG                        0x3B8U
#define REG_CAMERA_AVT_COLOR_CORR_CRB                        0x3BCU
#define REG_CAMERA_AVT_COLOR_CORR_CGB                        0x3C0U
#define REG_CAMERA_AVT_COLOR_CORR_CBB                        0x3C4U
#define REG_CAMERA_AVT_TRIGGER_DELAY                        0x400U
#define REG_CAMERA_AVT_MIRROR_IMAGE                        0x410U
#define REG_CAMERA_AVT_CHANNEL_ADJUST_CTRL              0x420U
#define REG_CAMERA_AVT_CHANNEL_ADJUST_VALUE                0x424U
#define REG_CAMERA_AVT_SOFT_RESET                        0x510U
#define REG_CAMERA_AVT_HSNRR                                 0x520U
#define REG_CAMERA_AVT_GPDATA_INFO                        0xFFCU
#define REG_CAMERA_AVT_GPDATA_BUFFER                        0x1000U

/************************************************************************/
/* Get Version          (Read Only)                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_get_version(dc1394camera_t *camera,
                       uint32_t *UCType, uint32_t *Version,
                       uint32_t *Camera_ID, uint32_t *FPGA_Version)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve uC */
    *UCType =dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_VERSION_INFO1,&value);

    /* uC Version : Bits 16..31 */
    *Version =(uint32_t)(value & 0xFFFFUL );

    /*  Retrieve Camera ID and FPGA_Version */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_VERSION_INFO3, &value);
    DC1394_ERR_RTN(err,"Could not get AVT version info 3");

    /* Camera_ID : bit 0-15 */
    *Camera_ID =(uint32_t)(value >>16 );

    /* FPGA_Version : bit 16-31 */
    *FPGA_Version=(uint32_t)(value & 0xFFFFUL );

    return DC1394_SUCCESS;

}

/************************************************************************/
/* Get Advanced feature inquiry                                                */
/************************************************************************/
dc1394error_t
dc1394_avt_get_advanced_feature_inquiry(dc1394camera_t *camera,
                                        dc1394_avt_adv_feature_info_t *adv_feature)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve first group of features presence */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_ADV_INQ_1, &value);
    DC1394_ERR_RTN(err,"Could not get AVT advanced features INQ 1");

    adv_feature->MaxResolution=                (value & 0x80000000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->TimeBase=                        (value & 0x40000000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->ExtdShutter=                        (value & 0x20000000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->TestImage=                        (value & 0x10000000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->FrameInfo=                        (value & 0x08000000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->Sequences=                        (value & 0x04000000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->VersionInfo=                        (value & 0x02000000UL) ? DC1394_TRUE : DC1394_FALSE;
    //ADV_INQ_1 7
    adv_feature->Lookup_Tables=                (value & 0x00800000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->Shading=                                (value & 0x00400000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->DeferredTrans=                (value & 0x00200000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->HDR_Mode=                        (value & 0x00100000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->DSNU=                                (value & 0x00080000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->BlemishCorrection=        (value & 0x00040000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->TriggerDelay=                (value & 0x00020000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->MirrorImage=                        (value & 0x00010000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->SoftReset=                        (value & 0x00008000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->HSNR=                                (value & 0x00004000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->ColorCorrection=                (value & 0x00002000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->UserProfiles=                (value & 0x00001000UL) ? DC1394_TRUE : DC1394_FALSE;
    //ADV_INQ_1 20
    adv_feature->UserSets=                        (value & 0x00000800UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->TimeStamp=                        (value & 0x00000400UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->FrmCntStamp=                        (value & 0x00000200UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->TrgCntStamp=                        (value & 0x00000100UL) ? DC1394_TRUE : DC1394_FALSE;
    //ADV_INQ_1 25-30
    adv_feature->GP_Buffer=                        (value & 0x00000001UL) ? DC1394_TRUE : DC1394_FALSE;

    /* Remember this request have been done */
    adv_feature->features_requested = DC1394_TRUE;

    /* Retrieve second group of features presence */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_ADV_INQ_2, &value);
    DC1394_ERR_RTN(err,"Could not get AVT advanced features INQ 2");

    adv_feature->Input_1 =                        (value & 0x80000000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->Input_2 =                        (value & 0x40000000UL) ? DC1394_TRUE : DC1394_FALSE;
    //ADV_INQ_2 2-7
    adv_feature->Output_1=                        (value & 0x00800000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->Output_2=                        (value & 0x00400000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->Output_3=                        (value & 0x00200000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->Output_4=                        (value & 0x00100000UL) ? DC1394_TRUE : DC1394_FALSE;
    //ADV_INQ_2 12-15
    adv_feature->IntEnaDelay=                        (value & 0x00008000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->IncDecoder=                        (value & 0x00004000UL) ? DC1394_TRUE : DC1394_FALSE;
    //ADV_INQ_2 18-31

    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_ADV_INQ_3, &value);
    DC1394_ERR_RTN(err,"Could not get AVT advanced features INQ 3");

    adv_feature->CameraStatus=                (value & 0x80000000UL) ? DC1394_TRUE : DC1394_FALSE;
    //ADV_INQ_3 1-3
    adv_feature->AutoShutter=                        (value & 0x08000000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->AutoGain=                        (value & 0x04000000UL) ? DC1394_TRUE : DC1394_FALSE;
    adv_feature->AutoFunctionAOI=                (value & 0x02000000UL) ? DC1394_TRUE : DC1394_FALSE;
    //ADV_INQ_3 7-31

    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_ADV_INQ_4, &value);
    DC1394_ERR_RTN(err,"Could not get AVT advanced features INQ 4");

    adv_feature->HDRPike=                (value & 0x80000000UL) ? DC1394_TRUE : DC1394_FALSE;
    //ADV_INQ_4 1-31

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Print Advanced features                                                 */
/************************************************************************/
dc1394error_t
dc1394_avt_print_advanced_feature(dc1394_avt_adv_feature_info_t *adv_feature)
{

    puts ("ADVANCED FEATURES SUPPORTED:");
    if(adv_feature->MaxResolution == DC1394_TRUE) puts (" MaxResolution ");
    if(adv_feature->TimeBase == DC1394_TRUE)         puts (" TimeBase ");
    if(adv_feature->ExtdShutter == DC1394_TRUE)         puts (" ExtdShutter ");
    if(adv_feature->TestImage == DC1394_TRUE)         puts (" TestImage ");
    if(adv_feature->FrameInfo == DC1394_TRUE)         puts (" FrameInfo ");
    if(adv_feature->Sequences == DC1394_TRUE)         puts (" Sequences ");
    if(adv_feature->VersionInfo == DC1394_TRUE)         puts (" VersionInfo ");
    //ADV_INQ_1 7
    if(adv_feature->Lookup_Tables == DC1394_TRUE)        puts (" Lookup_Tables ");
    if(adv_feature->Shading == DC1394_TRUE)         puts (" Shading ");
    if(adv_feature->DeferredTrans == DC1394_TRUE) puts (" DeferredTrans ");
    if(adv_feature->HDR_Mode == DC1394_TRUE)         puts (" HDR_Mode ");
    if(adv_feature->DSNU == DC1394_TRUE)                 puts (" DSNU ");
    if(adv_feature->BlemishCorrection == DC1394_TRUE)                 puts (" BlemishCorrection ");
    if(adv_feature->TriggerDelay == DC1394_TRUE)         puts (" TriggerDelay ");
    if(adv_feature->MirrorImage == DC1394_TRUE)         puts (" MirrorImage ");
    if(adv_feature->SoftReset == DC1394_TRUE)         puts (" SoftReset ");
    if(adv_feature->HSNR == DC1394_TRUE)         puts (" HSNR ");
    if(adv_feature->ColorCorrection == DC1394_TRUE)         puts (" ColorCorrection ");
    if(adv_feature->UserProfiles == DC1394_TRUE)         puts (" UserProfiles ");
    //ADV_INQ_1 20
    if(adv_feature->UserSets == DC1394_TRUE)         puts (" UserSets ");
    if(adv_feature->TimeStamp == DC1394_TRUE)         puts (" TimeStamp ");
    if(adv_feature->FrmCntStamp == DC1394_TRUE)         puts (" FrmCntStamp ");
    if(adv_feature->TrgCntStamp == DC1394_TRUE)         puts (" TrgCntStamp ");
    //ADV_INQ_1 25-30
    if(adv_feature->GP_Buffer == DC1394_TRUE)         puts (" GP_Buffer ");


    if(adv_feature->Input_1 == DC1394_TRUE)        puts (" Input_1 ");
    if(adv_feature->Input_2 == DC1394_TRUE)         puts (" Input_2 ");
    //ADV_INQ_2 2-7
    if(adv_feature->Output_1 == DC1394_TRUE)         puts (" Output_1 ");
    if(adv_feature->Output_2 == DC1394_TRUE)         puts (" Output_2 ");
    if(adv_feature->Output_3 == DC1394_TRUE)         puts (" Output_3 ");
    if(adv_feature->Output_4 == DC1394_TRUE)         puts (" Output_4 ");
    //ADV_INQ_2 12-15
    if(adv_feature->IntEnaDelay == DC1394_TRUE)         puts (" IntEnaDelay ");
    if(adv_feature->IncDecoder == DC1394_TRUE)         puts (" IncDecoder ");
    //ADV_INQ_2 18-31

    if(adv_feature->CameraStatus == DC1394_TRUE)         puts (" CameraStatus ");
    //ADV_INQ_3 1-3
    if(adv_feature->AutoShutter == DC1394_TRUE)         puts (" AutoShutter ");
    if(adv_feature->AutoGain == DC1394_TRUE)         puts (" AutoGain ");
    if(adv_feature->AutoFunctionAOI == DC1394_TRUE)         puts (" AutoFunctionAOI ");
    //ADV_INQ_3 7-31

    if(adv_feature->HDRPike == DC1394_TRUE)         puts (" HDRPike ");
    //ADV_INQ_4 1-31

    return DC1394_SUCCESS;

}


/************************************************************************/
/* Get shading mode                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_get_shading(dc1394camera_t *camera,
                       dc1394bool_t *on_off, dc1394bool_t *compute,
                       dc1394bool_t *show, uint32_t *frame_nb)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve shading properties */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_SHDG_CTRL, &value);
    DC1394_ERR_RTN(err,"Could not get AVT shading control reg");

    /* Shading ON / OFF : Bit 6 */
    if (on_off)
        *on_off = (uint32_t)((value & 0x2000000UL) >> 25);

    /* Compute : Bit 5 */
    if (compute)
        *compute = (uint32_t)((value & 0x4000000UL) >> 26);

    /* Show image : Bit 4 */
    if (show)
        *show = (uint32_t)((value & 0x8000000UL) >> 27);

    /* Number of images for auto computing of the shading reference: Bits 24..31 */
    if (frame_nb)
        *frame_nb =(uint32_t)((value & 0xFFUL));

    return DC1394_SUCCESS;

}


/************************************************************************/
/* Set shading mode                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_set_shading(dc1394camera_t *camera,
                       dc1394bool_t on_off, dc1394bool_t compute,
                       dc1394bool_t show, uint32_t frame_nb)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current shading properties */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_SHDG_CTRL, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT shading control reg");

    /* Shading ON / OFF : Bit 6 */
    curval = (curval & 0xFDFFFFFFUL) | ((on_off ) << 25);

    /* Compute : Bit 5 */
    curval = (curval & 0xFBFFFFFFUL) | ((compute ) << 26);

    /* Show Image : Bit 4 */
    curval = (curval & 0xF7FFFFFFUL) | ((show ) << 27);

    /* Number of images : Bits 24..31 */
    curval = (curval & 0xFFFFFF00UL) | ((frame_nb & 0xFFUL ));

    /* Set new parameters */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_SHDG_CTRL, curval);
    DC1394_ERR_RTN(err,"Could not set AVT shading control reg");

    return DC1394_SUCCESS;

}


/************************************************************************/
/* Get shading  mem ctrl                                                */
/************************************************************************/
dc1394error_t
dc1394_avt_get_shading_mem_ctrl(dc1394camera_t *camera, dc1394bool_t *en_write,
                                dc1394bool_t *en_read, uint32_t *addroffset)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve current memory shading properties */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_SHDG_MEM_CTRL, &value);
    DC1394_ERR_RTN(err,"Could not get AVT shading memory control");

    /* Enable write access : Bit 5 */
    if (en_write)
        *en_write = (uint32_t)((value & 0x4000000UL) >> 26);

    /* Enable read access : Bit 6 */
    if (en_read)
        *en_read = (uint32_t)((value & 0x2000000UL) >> 25);

    /* addroffset in byte : Bits 8..31 */
    if (addroffset)
        *addroffset =(uint32_t)((value & 0xFFFFFFUL));

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Set shading mem ctrl                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_set_shading_mem_ctrl(dc1394camera_t *camera,
                                dc1394bool_t en_write, dc1394bool_t en_read, uint32_t addroffset)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current shading properties */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_SHDG_MEM_CTRL, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT shading memory control");

    /* read access enable : Bit 6 */
    curval = (curval & 0xFDFFFFFFUL) | ((en_read ) << 25);

    /* write access enable : Bit 5 */
    curval = (curval & 0xFBFFFFFFUL) | ((en_write ) << 26);

    /* Number of images : Bits 8..31 */
    curval = (curval & 0xFF000000UL) | ((addroffset & 0xFFFFFFUL ));

    /* Set new parameters */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_SHDG_MEM_CTRL, curval);
    DC1394_ERR_RTN(err,"Could not get AVT LUT memory control");

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Get shading  info                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_get_shading_info(dc1394camera_t *camera, uint32_t *MaxImageSize)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve shading info */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_SHDG_INFO, &value);
    DC1394_ERR_RTN(err,"Could not get AVT shading info");

    /* Max Shading Image size(byte) : Bits 8..31 */
    *MaxImageSize =(uint32_t)((value & 0xFFFFFFUL));

    return DC1394_SUCCESS;

}


/************************************************************************/
/* Get Multiple slope parameters        (HDR)                                */
/************************************************************************/
dc1394error_t
dc1394_avt_get_multiple_slope(dc1394camera_t *camera,
                              dc1394bool_t *on_off, uint32_t *points_nb,uint32_t *kneepoint1,
                              uint32_t *kneepoint2, uint32_t *kneepoint3)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve current hdr parameters */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_HDR_CONTROL, &value);
    DC1394_ERR_RTN(err,"Could not get AVT HDR control register");

    /* Multiple slope ON / OFF : Bit 6 */
    *on_off = (uint32_t)((value & 0x2000000UL) >> 25);

    /* Number of actives points : Bits 28..31 */
    *points_nb =(uint32_t)((value & 0xFUL));

    /* kneepoints */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_KNEEPOINT_1, kneepoint1);
    DC1394_ERR_RTN(err,"Could not get AVT kneepoint 1");
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_KNEEPOINT_2, kneepoint2);
    DC1394_ERR_RTN(err,"Could not get AVT kneepoint 2");
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_KNEEPOINT_3, kneepoint3);
    DC1394_ERR_RTN(err,"Could not get AVT kneepoint 3");

    return DC1394_SUCCESS;

}


/************************************************************************/
/* Set Multiple slope parameters                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_set_multiple_slope(dc1394camera_t *camera,
                              dc1394bool_t on_off, uint32_t points_nb, uint32_t kneepoint1,
                              uint32_t kneepoint2, uint32_t kneepoint3)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current hdr parameters */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_HDR_CONTROL, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT HDR control reg");

    /* Shading ON / OFF : Bit 6 */
    curval = (curval & 0xFDFFFFFFUL) | ((on_off ) << 25);

    /* Number of points : Bits 28..31 */
    curval = (curval & 0xFFFFFFF0UL) | ((points_nb & 0xFUL ));

    /* Set new hdr parameters */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_HDR_CONTROL, curval);
    DC1394_ERR_RTN(err,"Could not set AVT HDR control reg");

    /* kneepoints */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_KNEEPOINT_1, kneepoint1);
    DC1394_ERR_RTN(err,"Could not set AVT kneepoint 1");
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_KNEEPOINT_2, kneepoint2);
    DC1394_ERR_RTN(err,"Could not set AVT kneepoint 2");
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_KNEEPOINT_3, kneepoint3);
    DC1394_ERR_RTN(err,"Could not set AVT kneepoint 3");

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Shutter Timebase                                                 */
/************************************************************************/
dc1394error_t
dc1394_avt_get_timebase(dc1394camera_t *camera, uint32_t *timebase_id)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve current timebase */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_TIMEBASE, &value);
    DC1394_ERR_RTN(err,"Could not get AVT timebase");

    /* Time base ID : Bits 29..31 */
    *timebase_id =(uint32_t)((value & 0xFUL));

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Shutter Timebase (acquisition must be stopped)                        */
/************************************************************************/
dc1394error_t
dc1394_avt_set_timebase(dc1394camera_t *camera, uint32_t timebase_id)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current timebase */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_TIMEBASE, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT timebase");

    curval = (curval & 0xFFFFFFF0UL) | ((timebase_id & 0xFUL ));

    /* Set new timebase */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_TIMEBASE, curval);
    DC1394_ERR_RTN(err,"Could not set AVT timebase");

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Extented Shutter                                                  */
/************************************************************************/
dc1394error_t
dc1394_avt_get_extented_shutter(dc1394camera_t *camera, uint32_t *timebase_id)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve current extented shutter value */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_EXTD_SHUTTER, &value);
    DC1394_ERR_RTN(err,"Could not get AVT extended shutter reg");

    /* Exposure Time in us: Bits 6..31 */
    *timebase_id =(uint32_t)((value & 0xFFFFFFFUL));

    return DC1394_SUCCESS;

}


/************************************************************************/
/* Set Extented shutter                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_set_extented_shutter(dc1394camera_t *camera, uint32_t timebase_id)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current extented shutter value */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_EXTD_SHUTTER, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT extended shutter reg");

    /* Time base ID : Bits 6..31 */
    curval = (curval & 0xF0000000UL) | ((timebase_id & 0x0FFFFFFFUL ));

    /* Set new extented shutter value */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_EXTD_SHUTTER, curval);
    DC1394_ERR_RTN(err,"Could not set AVT extended shutter reg");

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get MaxResolution          (Read Only)                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_get_MaxResolution(dc1394camera_t *camera, uint32_t *MaxHeight, uint32_t *MaxWidth)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve the maximum resolution */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_MAX_RESOLUTION, &value);
    DC1394_ERR_RTN(err,"Could not get AVT max resolution");

    /* MaxHeight : Bits 0..15 */
    *MaxHeight =(uint32_t)(value >> 16);
    /* MaxWidth : Bits 16..31 */
    *MaxWidth =(uint32_t)(value & 0xFFFFUL );

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Auto Shutter                                                          */
/************************************************************************/
dc1394error_t
dc1394_avt_get_auto_shutter(dc1394camera_t *camera, uint32_t *MinValue, uint32_t *MaxValue)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve current min auto shutter value */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_AUTOSHUTTER_LO, &value);
    DC1394_ERR_RTN(err,"Could not get AVT autoshutter LSB");

    *MinValue =(uint32_t)value;

    /* Retrieve current max auto shutter value */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_AUTOSHUTTER_HI, &value);
    DC1394_ERR_RTN(err,"Could not get AVT autoshutter MSB");

    *MaxValue =(uint32_t)value;

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Auto shutter                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_set_auto_shutter(dc1394camera_t *camera, uint32_t MinValue, uint32_t MaxValue)
{
    dc1394error_t err;
    /* Set min auto shutter value */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_AUTOSHUTTER_LO, MinValue);
    DC1394_ERR_RTN(err,"Could not set AVT autoshutter LSB");

    /* Set max auto shutter value */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_AUTOSHUTTER_HI, MaxValue);
    DC1394_ERR_RTN(err,"Could not set AVT autoshutter MSB");

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Auto Gain                                                          */
/************************************************************************/
dc1394error_t
dc1394_avt_get_auto_gain(dc1394camera_t *camera, uint32_t *MinValue, uint32_t *MaxValue)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve auto gain values */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_AUTOGAIN_CTRL, &value);
    DC1394_ERR_RTN(err,"Could not get AVT autogain");

    /* Min : bits 20..31 */
    *MinValue =(uint32_t)(value & 0xFFFUL);
    /* Max : bits 4..15 */
    *MaxValue =(uint32_t)((value >> 16) & 0xFFFUL);

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Auto gain                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_set_auto_gain(dc1394camera_t *camera, uint32_t MinValue, uint32_t MaxValue)
{
    dc1394error_t err;
    uint32_t value;

    /* Max : bits 4..15, Min : bits 20..31  */
    value = ( MaxValue <<16 ) | ( MinValue );

    /* Set new parameters */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_AUTOGAIN_CTRL,value);
    DC1394_ERR_RTN(err,"Could not set AVT autogain");

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Trigger delay                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_get_trigger_delay(dc1394camera_t *camera, dc1394bool_t *on_off, uint32_t *DelayTime)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve trigger delay */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_TRIGGER_DELAY, &value);
    DC1394_ERR_RTN(err,"Could not get AVT trigger delay");

    /* trigger_delay ON / OFF : Bit 6 */
    *on_off = (uint32_t)((value & 0x2000000UL) >> 25);

    /* Delai time in us : Bits 11..31 */
    *DelayTime =(uint32_t)((value & 0xFFFFFUL));

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Trigger delay                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_set_trigger_delay(dc1394camera_t *camera, dc1394bool_t on_off, uint32_t DelayTime)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve trigger delay */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_TRIGGER_DELAY, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT trigger delay");

    /* trigger_delay ON / OFF : Bit 6 */
    curval = (curval & 0xFDFFFFFFUL) | ((on_off ) << 25);

    /* Delay time in us : Bits 11..31 */
    curval = (curval & 0xFFF00000UL) | DelayTime;

    /* Set new parameters */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_TRIGGER_DELAY, curval);
    DC1394_ERR_RTN(err,"Could not set AVT trigger delay");

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Mirror                                                                 */
/************************************************************************/
dc1394error_t
dc1394_avt_get_mirror(dc1394camera_t *camera, dc1394bool_t *on_off)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve Mirror mode */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_MIRROR_IMAGE, &value);
    DC1394_ERR_RTN(err,"Could not get AVT mirror image");

    /* mirror ON / OFF : Bit 6 */
    *on_off = (uint32_t)((value & 0x2000000UL) >> 25);

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Set Mirror                                                                */
/************************************************************************/
dc1394error_t
dc1394_avt_set_mirror(dc1394camera_t *camera, dc1394bool_t on_off)
{
    dc1394error_t err;
    uint32_t curval;

    /* ON / OFF : Bit 6 */
    curval = on_off << 25;

    /* Set mirror mode */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_MIRROR_IMAGE, curval);
    DC1394_ERR_RTN(err,"Could not set AVT mirror image");

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Get DSNU                                                                 */
/************************************************************************/
dc1394error_t
dc1394_avt_get_dsnu(dc1394camera_t *camera, dc1394bool_t *on_off,uint32_t *frame_nb)
{
    dc1394error_t err;
    uint32_t value;
    /* Retrieve dsnu parameters */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_DSNU_CONTROL, &value);
    DC1394_ERR_RTN(err,"Could not get AVT DSNU control");

    /* ON / OFF : Bit 6 */
    *on_off = !(uint32_t)((value & 0x2000000UL) >> 25);

    /* Number of images : Bits 24..31 */
    *frame_nb =(uint32_t)((value & 0xFFUL));

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Set DSNU                                                                */
/************************************************************************/
dc1394error_t
dc1394_avt_set_dsnu(dc1394camera_t *camera,
                    dc1394bool_t on_off, dc1394bool_t compute, uint32_t frame_nb)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current dsnu parameters */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_DSNU_CONTROL, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT DSNU control");

    /* Compute : Bit 5 */
    curval = (curval & 0xFBFFFFFFUL) | ((compute ) << 26);

    /* ON / OFF : Bit 6 */
    curval = (curval & 0xFDFFFFFFUL) | ((!on_off ) << 25);

    /* Number of images : Bits 24..31 */
    curval = (curval & 0xFFFFFF00UL) | ((frame_nb & 0xFFUL ));

    /* Set new dsnu parameters */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_DSNU_CONTROL, curval);
    DC1394_ERR_RTN(err,"Could not set AVT DSNU control");

    int cont=1;
    while (cont) {
        usleep(50000);
        err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_DSNU_CONTROL, &curval);
        DC1394_ERR_RTN(err,"Could not get AVT DSNU control");
        if ((curval & 0x01000000UL)==0)
            cont=0;
    }
    return DC1394_SUCCESS;
}

/************************************************************************/
/* Get BLEMISH                                                                 */
/************************************************************************/
dc1394error_t
dc1394_avt_get_blemish(dc1394camera_t *camera, dc1394bool_t *on_off, uint32_t *frame_nb)
{
    dc1394error_t err;
    uint32_t value;

    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_BLEMISH_CONTROL, &value);
    DC1394_ERR_RTN(err,"Could not get AVT blemish control");

    /* ON / OFF : Bit 6 */
    *on_off = (uint32_t)((value & 0x2000000UL) >> 25);

    /* Number of images : Bits 24..31 */
    *frame_nb =(uint32_t)((value & 0xFFUL));

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Set BLEMISH                                                                */
/************************************************************************/
dc1394error_t
dc1394_avt_set_blemish(dc1394camera_t *camera,
                       dc1394bool_t on_off, dc1394bool_t compute, uint32_t frame_nb)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current blemish parameters */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_BLEMISH_CONTROL, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT blemish control");

    /* Compute : Bit 5 */
    curval = (curval & 0xFBFFFFFFUL) | ((compute ) << 26);

    /* ON / OFF : Bit 6 */
    curval = (curval & 0xFDFFFFFFUL) | ((on_off ) << 25);

    /* Number of images : Bits 24..31 */
    curval = (curval & 0xFFFFFF00UL) | ((frame_nb & 0xFFUL ));

    /* Set new blemish parameters */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_BLEMISH_CONTROL, curval);
    DC1394_ERR_RTN(err,"Could not set AVT blemish control");

    int cont=1;
    while (cont) {
        usleep(50000);
        err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_BLEMISH_CONTROL, &curval);
        DC1394_ERR_RTN(err,"Could not get AVT DSNU control");
        if ((curval & 0x01000000UL)==0)
            cont=0;
    }

    return DC1394_SUCCESS;
}



/************************************************************************/
/* Get IO   REG_CAMERA_AVT_IO_INP_CTRLx        or REG_CAMERA_AVT_IO_OUTP_CTRLx        */
/************************************************************************/
dc1394error_t
dc1394_avt_get_io(dc1394camera_t *camera, uint32_t IO,
                  dc1394bool_t *polarity, uint32_t *mode, dc1394bool_t *pinstate)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve IO parameters */
    err=dc1394_get_adv_control_register(camera,IO, &value);
    DC1394_ERR_RTN(err,"Could not get AVT IO register");

    /* polarity : Bit 7 */
    *polarity = (uint32_t)((value & 0x1000000UL) >> 24);

    /* pinstate : Bit 31 */
    *pinstate = (uint32_t)((value & 0x1UL));

    /* mode : Bits 11..15 */
    *mode =(uint32_t)((value >> 16 ) & 0x1FUL);

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Set IO   REG_CAMERA_AVT_IO_INP_CTRLx        or REG_CAMERA_AVT_IO_OUTP_CTRLx        */
/************************************************************************/
dc1394error_t
dc1394_avt_set_io(dc1394camera_t *camera,uint32_t IO,
                  dc1394bool_t polarity, uint32_t mode, dc1394bool_t pinstate)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current IO parameters */
    err=dc1394_get_adv_control_register(camera,IO, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT IO register");

    /* polarity : Bit 7 */
    curval = (curval & 0xFEFFFFFFUL) | ((polarity ) << 24);

    /* mode : Bits 11..15 */
    curval = (curval & 0xFFE0FFFFUL) | ((mode << 16) & 0x1F0000UL );

    /* Pin state: bit 31 */
    if (mode==1)
        curval = (curval & 0xFFFFFFFEUL) | pinstate;

    /* Set  new IO parameters */
    err=dc1394_set_adv_control_register(camera,IO, curval);
    DC1394_ERR_RTN(err,"Could not set AVT IO register");

    return DC1394_SUCCESS;
}

/************************************************************************/
/* BusReset IEEE1394                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_reset(dc1394camera_t *camera)
{
    dc1394error_t err;
    uint32_t value;
    /* ON / OFF : Bit 6 */
    value= (1<<25) + 200; /*2sec*/
    /* Reset */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_SOFT_RESET,value);
    DC1394_ERR_RTN(err,"Could not set AVT soft reset");

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Lookup Tables (LUT)                                                */
/************************************************************************/
dc1394error_t
dc1394_avt_get_lut(dc1394camera_t *camera, dc1394bool_t *on_off, uint32_t *lutnb)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve current luts parameters */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_LUT_CTRL, &value);
    DC1394_ERR_RTN(err,"Could not get AVT LUT control");

    /* Shading ON / OFF : Bit 6 */
    *on_off = (uint32_t)((value & 0x2000000UL) >> 25);

    /* Number of lut : Bits 26..31 */
    *lutnb =(uint32_t)((value & 0x3FUL));

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Lookup Tables (LUT)                                                */
/************************************************************************/
dc1394error_t
dc1394_avt_set_lut(dc1394camera_t *camera, dc1394bool_t on_off, uint32_t lutnb)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current luts parameters */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_LUT_CTRL, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT LUT control");

    /* Shading ON / OFF : Bit 6 */
    curval = (curval & 0xFDFFFFFFUL) | ((on_off ) << 25);

    /* Number of lut : Bits 26..31 */
    curval = (curval & 0xFFFFFFB0UL) | ((lutnb & 0x3FUL ));

    /* Set new luts parameters */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_LUT_CTRL, curval);
    DC1394_ERR_RTN(err,"Could not set AVT LUT control");

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get LUT ctrl                                                                */
/************************************************************************/
dc1394error_t
dc1394_avt_get_lut_mem_ctrl(dc1394camera_t *camera, dc1394bool_t *en_write,
                            uint32_t * AccessLutNo,uint32_t *addroffset)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve current memory luts parameters */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_LUT_MEM_CTRL, &value);
    DC1394_ERR_RTN(err,"Could not get AVT LUT memory control");

    /* Enable write access : Bit 5 */
    *en_write = (uint32_t)((value & 0x4000000UL) >> 26);

    /* AccessLutNo : Bits 8..15 */
    *AccessLutNo=(uint32_t)((value >> 16) & 0xFFUL);

    /* addroffset in byte : Bits 16..31 */
    *addroffset =(uint32_t)((value & 0xFFFFUL));

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Set LUT ctrl                                                                 */
/************************************************************************/
dc1394error_t
dc1394_avt_set_lut_mem_ctrl(dc1394camera_t *camera,
                            dc1394bool_t en_write, uint32_t AccessLutNo, uint32_t addroffset)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current memory luts parameters */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_LUT_MEM_CTRL, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT LUT memory control");

    /* write access enable : Bit 5 */
    curval = (curval & 0xFBFFFFFFUL) | ((en_write ) << 26);

    /* AccessLutNo : Bits 8..15 */
    curval = (curval & 0xFF00FFFFUL) | ((AccessLutNo << 16) & 0xFF0000UL );

    /* Number of images : Bits 16..31 */
    curval = (curval & 0xFFFF0000UL) | ((addroffset & 0xFFFFUL ));

    /* Set new parameters */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_LUT_MEM_CTRL, curval);
    DC1394_ERR_RTN(err,"Could not set AVT LUT memory control");

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get LUT  info                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_get_lut_info(dc1394camera_t *camera, uint32_t *NumOfLuts, uint32_t *MaxLutSize)
{
    dc1394error_t err;
    uint32_t value;
    /* Retrieve luts info */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_LUT_INFO, &value);
    DC1394_ERR_RTN(err,"Could not get AVT LUT info");

    /* NumOfLuts : Bits 8..15 */
    *NumOfLuts=(uint32_t)((value >> 16) & 0xFFUL);

    /* MaxLutSize : Bits 16..31 */
    *MaxLutSize =(uint32_t)((value & 0xFFFFUL));

    return DC1394_SUCCESS;
}



/************************************************************************/
/* Get Automatic white balance        with Area Of Interest AOI                */
/************************************************************************/
dc1394error_t
dc1394_avt_get_aoi(dc1394camera_t *camera,
                   dc1394bool_t *on_off, int *left, int *top, int *width, int *height)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve current mode*/
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_AUTOFNC_AOI, &value);
    DC1394_ERR_RTN(err,"Could not get AVT autofocus AOI");

    /*  ON / OFF : Bit 6 */
    *on_off = (uint32_t)((value & 0x2000000UL) >> 25);

    /* Retrieve current size of area*/
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_AF_AREA_SIZE, &value);
    DC1394_ERR_RTN(err,"Could not get AVT AF area size");

    /* width : Bits 0..15 */
    *width =(uint32_t)(value >> 16);
    /* height : Bits 16..31 */
    *height =(uint32_t)(value & 0xFFFFUL );

    /* Retrieve current position of area*/
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_AF_AREA_POSITION, &value);
    DC1394_ERR_RTN(err,"Could not get AVT AF area position");

    /* left : Bits 0..15 */
    *left =(uint32_t)(value >> 16);
    /* top : Bits 16..31 */
    *top =(uint32_t)(value & 0xFFFFUL );

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Set Automatic white balance with Area Of Interest AOI                */
/************************************************************************/
dc1394error_t
dc1394_avt_set_aoi(dc1394camera_t *camera,
                   dc1394bool_t on_off,int left, int top, int width, int height)
{
    dc1394error_t err;
    uint32_t curval;

    /* ON / OFF : Bit 6 */
    curval = on_off << 25;

    /* Set feature on off */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_AUTOFNC_AOI, curval);
    DC1394_ERR_RTN(err,"Could not set AVT autofocus AOI");

    /* Set size */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_AF_AREA_SIZE, (width << 16) | height);
    DC1394_ERR_RTN(err,"Could not set AVT AF area size");

    /* Set position */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_AF_AREA_POSITION,(left << 16) | top );
    DC1394_ERR_RTN(err,"Could not set AVT AF area position");

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Get test_images                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_get_test_images(dc1394camera_t *camera, uint32_t *image_no)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve test image number */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_TEST_IMAGE, &value);
    DC1394_ERR_RTN(err,"Could not get AVT test image");

    /* Numero Image : Bits 28..31 */
    *image_no =(uint32_t)((value & 0xFUL));

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Set test_images                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_set_test_images(dc1394camera_t *camera, uint32_t image_no)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current test image */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_TEST_IMAGE, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT test image");

    /* Numero Image : Bits 28..31 */
    curval = (curval & 0xFFFFFFF0UL) | ((image_no & 0xFUL ));

    /* Set new test image */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_TEST_IMAGE,curval);
    DC1394_ERR_RTN(err,"Could not set AVT test image");

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get frame info                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_get_frame_info(dc1394camera_t *camera, uint32_t *framecounter)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve frame info */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_FRAMECOUNTER, &value);
    DC1394_ERR_RTN(err,"Could not get AVT framecounter");

    /* framecounter : Bits 0..31 */
    *framecounter =(uint32_t)(value);

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Reset frame info                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_reset_frame_info(dc1394camera_t *camera)
{
    dc1394error_t err;
    /* Reset counter */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_FRAMEINFO,1 << 30);
    DC1394_ERR_RTN(err,"Could not get AVT frame info");

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Get Deferred image transport                                                */
/************************************************************************/
dc1394error_t
dc1394_avt_get_deferred_trans(dc1394camera_t *camera,
                              dc1394bool_t *HoldImage, dc1394bool_t * FastCapture,
                              uint32_t *FifoSize, uint32_t *NumOfImages )
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve Deferred image transport mode */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_DEFERRED_TRANS, &value);
    DC1394_ERR_RTN(err,"Could not get AVT deferred transfer info");

    /* enable/disable deferred transport mode : Bit 6 */
    *HoldImage = (uint32_t)((value & 0x2000000UL) >> 25);

    /* enable/disable fast capture mode (format 7 only) : Bit 7 */
    *FastCapture = (uint32_t)((value & 0x1000000UL) >> 24);

    /* Size of fifo in number of image : Bits 16..23 */
    *FifoSize =(uint32_t)((value >> 8 & 0xFFUL));

    /* Number of images in buffer: Bits 24..31 */
    *NumOfImages =(uint32_t)((value & 0xFFUL));

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Deferred image transport                                                */
/************************************************************************/
dc1394error_t
dc1394_avt_set_deferred_trans(dc1394camera_t *camera,
                              dc1394bool_t HoldImage, dc1394bool_t FastCapture,
                              uint32_t FifoSize, uint32_t NumOfImages,
                              dc1394bool_t SendImage)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve current image transport mode */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_DEFERRED_TRANS, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT deferred transfer info");

    /* Send NumOfImages now : Bit 5 */
    curval = (curval & 0xFBFFFFFFUL) | ((SendImage ) << 26);

    /* enable/disable deferred transport mode : Bit 6 */
    curval = (curval & 0xFDFFFFFFUL) | ((HoldImage ) << 25);

    /* enable/disable fast capture mode (format 7 only) : Bit 7 */
    curval = (curval & 0xFEFFFFFFUL) | ((FastCapture ) << 24);

    /* Size of fifo in number of image : Bits 16..23 */
    curval = (curval & 0xFFFF00FFUL) | (((FifoSize << 8) & 0xFF00UL ));

    /* Number of images : Bits 24..31 */
    curval = (curval & 0xFFFFFF00UL) | ((NumOfImages & 0xFFUL ));

    /* Set new parameters */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_DEFERRED_TRANS, curval);
    DC1394_ERR_RTN(err,"Could not set AVT deferred transfer info");

    return DC1394_SUCCESS;
}



/************************************************************************/
/* Get GPData info                                                        */
/************************************************************************/
dc1394error_t
dc1394_avt_get_gpdata_info(dc1394camera_t *camera, uint32_t *BufferSize)
{
    dc1394error_t err;
    uint32_t value;
    /* Retrieve info on the general purpose buffer */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_GPDATA_INFO, &value);
    DC1394_ERR_RTN(err,"Could not get AVT GP data info");

    /* BufferSize : Bits 16..31 */
    *BufferSize =(uint32_t)((value & 0xFFFFUL));

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Common code for GPData register access computation                        */
/************************************************************************/
static void gpdata_io_common(uint32_t *buf_local, uint32_t gpdata_numquads,
                             uint32_t *nextIndex, uint32_t index, uint32_t size,
                             uint32_t *newBufferSize, uint32_t *nQuadWriteSize,
                             dc1394bool_t *finish)
{
    /* clear buffer */
    memset(buf_local, 0, gpdata_numquads * sizeof(uint32_t));

    /* calculate the index after writing the next block */
    *nextIndex = index + (gpdata_numquads * 4);
    /* if the next index lies behind the allocated memory -> align */
    if (size < *nextIndex) {
        *newBufferSize = (gpdata_numquads * 4) - (*nextIndex - size);
        /* if the reduced write-buffer size (buffer-size - 'overhang') is dividable by 4 */
        *nQuadWriteSize = *newBufferSize / 4;
        if ((*newBufferSize % 4) != 0)
            *nQuadWriteSize = *nQuadWriteSize + 1;

        *finish = DC1394_TRUE; /* ...because it's the last block */
    }
    else
        *nQuadWriteSize = gpdata_numquads;

    if (*nextIndex == size)
        *finish = DC1394_TRUE;
}


/************************************************************************/
/* Read size number of bytes from GPData buffer                                */
/************************************************************************/
dc1394error_t
dc1394_avt_read_gpdata(dc1394camera_t *camera, unsigned char *buf, uint32_t size)
{
    uint32_t gpdata_numquads, gpdata_bufsize;
    uint32_t nQuadReadSize, newBufferSize;
    uint32_t i, index = 0, nextIndex;
    uint32_t *buf_local;
    dc1394bool_t finish = DC1394_FALSE;
    dc1394error_t err;

    /* determine gpdata_bufsize (as read-block-size) */
    err = dc1394_avt_get_gpdata_info(camera, &gpdata_bufsize);
    DC1394_ERR_RTN(err,"Could not get AVT GPData info");

    /* calculate the number of quadlets in the gpdata buffer */
    gpdata_numquads = gpdata_bufsize / 4;
    if ((gpdata_bufsize % 4) != 0)
        gpdata_numquads++;

    /* allocate memory for the 'read-buffer' */
    buf_local = malloc(gpdata_numquads * sizeof(uint32_t));
    if (buf_local == NULL)
        return DC1394_FAILURE;

    do {
        gpdata_io_common(buf_local, gpdata_numquads, &nextIndex, index, size, &newBufferSize, &nQuadReadSize, &finish);

        /* read block */
        err = dc1394_get_adv_control_registers(camera, REG_CAMERA_AVT_GPDATA_BUFFER,
                                               buf_local, nQuadReadSize);
        if (err != DC1394_SUCCESS) {
            free(buf_local);
            return DC1394_FAILURE;
        }

        /* copy block-contents to user buf */
        for (i = 0; i < nQuadReadSize; i++)
            memcpy(buf + index + (i * 4), &buf_local[i], sizeof(uint32_t));

        index += (nQuadReadSize * 4);

        /* loop until all bytes are read */
    } while (!finish);

    free(buf_local);
    return DC1394_SUCCESS;
}


/************************************************************************/
/* Write size number of bytes to GPData buffer                                */
/************************************************************************/
dc1394error_t
dc1394_avt_write_gpdata(dc1394camera_t *camera, unsigned char *buf, uint32_t size)
{
    uint32_t gpdata_bufsize, gpdata_numquads;
    uint32_t nQuadWriteSize, newBufferSize;
    uint32_t i, index = 0, nextIndex;
    uint32_t *buf_local;
    dc1394bool_t finish = DC1394_FALSE;
    dc1394error_t err;

    /* determine gpdata_bufsize */
    err = dc1394_avt_get_gpdata_info(camera, &gpdata_bufsize);
    DC1394_ERR_RTN(err,"Could not get AVT GPData info");

    /* calculate the number of quadlets in the gpdata buffer */
    gpdata_numquads = gpdata_bufsize / 4;
    if ((gpdata_bufsize % 4) != 0)
        gpdata_numquads++;

    /* allocate memory for the write buffer */
    buf_local = malloc(gpdata_numquads * sizeof(uint32_t));
    if (buf_local == NULL)
        return DC1394_FAILURE;

    do {
        gpdata_io_common(buf_local, gpdata_numquads, &nextIndex, index, size, &newBufferSize, &nQuadWriteSize, &finish);

        /* copy block-contents to buf_local */
        for (i = 0; i < nQuadWriteSize; i++)
            memcpy(&buf_local[i], buf + index + (i * 4), sizeof(uint32_t));

        /* write block */
        err = dc1394_set_adv_control_registers(camera, REG_CAMERA_AVT_GPDATA_BUFFER,
                                               buf_local, nQuadWriteSize);
        if (err != DC1394_SUCCESS) {
            free(buf_local);
            return DC1394_FAILURE;
        }

        index += (nQuadWriteSize * 4);

        /* loop until all bytes are read */
    } while (!finish);

    free(buf_local);
    return DC1394_SUCCESS;
}


/************************************************************************/
/* Read shading image from camera into buffer                               */
/************************************************************************/
dc1394error_t
dc1394_avt_read_shading_img(dc1394camera_t *camera, unsigned char *buf,
                            uint32_t size)
{
    dc1394error_t err;
    dc1394bool_t en_write;
    uint32_t addr;

    /* Enable read at address 0 */
    err = dc1394_avt_get_shading_mem_ctrl(camera, &en_write, NULL, NULL);
    DC1394_ERR_RTN(err,"Could not read AVT shading mem ctrl");
    err = dc1394_avt_set_shading_mem_ctrl(camera, en_write, DC1394_TRUE, 0);
    DC1394_ERR_RTN(err,"Could not write AVT shading mem ctrl");

    /* Read data */
    err = dc1394_avt_read_gpdata(camera, buf, size);
    DC1394_ERR_RTN(err,"Could not read AVT gpdata");

    /* Disable read */
    err = dc1394_avt_get_shading_mem_ctrl(camera, &en_write, NULL, &addr);
    DC1394_ERR_RTN(err,"Could not read AVT shading mem ctrl");
    err = dc1394_avt_set_shading_mem_ctrl(camera, en_write, DC1394_FALSE, addr);
    DC1394_ERR_RTN(err,"Could not write AVT shading mem ctrl");

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Write shading image from buffer to camera                                */
/************************************************************************/
dc1394error_t
dc1394_avt_write_shading_img(dc1394camera_t *camera, unsigned char *buf,
                             uint32_t size)
{
    dc1394error_t err;
    dc1394bool_t en_read;
    uint32_t addr;

    /* Enable write at address 0 */
    err = dc1394_avt_get_shading_mem_ctrl(camera, NULL, &en_read, NULL);
    DC1394_ERR_RTN(err,"Could not read AVT shading mem ctrl");
    err = dc1394_avt_set_shading_mem_ctrl(camera, DC1394_TRUE, en_read, 0);
    DC1394_ERR_RTN(err,"Could not write AVT shading mem ctrl");

    /* Write data */
    err = dc1394_avt_write_gpdata(camera, buf, size);
    DC1394_ERR_RTN(err,"Could not write AVT gpdata");

    /* Disable write */
    err = dc1394_avt_get_shading_mem_ctrl(camera, NULL, &en_read, &addr);
    DC1394_ERR_RTN(err,"Could not read AVT shading mem ctrl");
    err = dc1394_avt_set_shading_mem_ctrl(camera, DC1394_FALSE, en_read, addr);
    DC1394_ERR_RTN(err,"Could not write AVT shading mem ctrl");

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Read channel adjust (AVT Pike)                                           */
/************************************************************************/
dc1394error_t dc1394_avt_get_channel_adjust(dc1394camera_t *camera, int16_t *channel_adjust)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve current channel adjust */
    err=dc1394_get_adv_control_register(camera,REG_CAMERA_AVT_CHANNEL_ADJUST_VALUE, &value);
    DC1394_ERR_RTN(err,"Could not get AVT channel adjust");

    /* channel adjust: Bits 16..31 */
    *channel_adjust = (int16_t)value;

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Write channel adjust (AVT Pike)                                           */
/************************************************************************/
dc1394error_t dc1394_avt_set_channel_adjust(dc1394camera_t *camera, int16_t channel_adjust)
{
    dc1394error_t err;

    /* Set new channel adjust */
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_CHANNEL_ADJUST_VALUE, (uint32_t)channel_adjust);
    DC1394_ERR_RTN(err,"Could not set AVT channel adjust");

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Set Color Correction + Coefficients                                          */
/************************************************************************/

dc1394error_t dc1394_avt_set_color_corr(dc1394camera_t *camera, dc1394bool_t on_off, dc1394bool_t reset, int32_t Crr, int32_t Cgr, int32_t Cbr, int32_t Crg, int32_t Cgg, int32_t Cbg, int32_t Crb, int32_t Cgb, int32_t Cbb)
{
    dc1394error_t err;
    uint32_t curval;

    //retrieve color correction
    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT color correction");

    //ON / OFF : Bit 6
    curval = (curval & 0xFDFFFFFFUL) | ((on_off) << 25);

    //reset coefficients to defaults : Bit 7
    curval = (curval & 0xFEFFFFFFUL) | ((reset) << 24);

    //set new parameters
    err=dc1394_set_adv_control_register(camera,REG_CAMERA_AVT_COLOR_CORR, curval);
    DC1394_ERR_RTN(err,"Could not set AVT color correction");

    if (!reset) {
        //red channel coefficients
        err=dc1394_set_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CRR, Crr);
        DC1394_ERR_RTN(err,"Could not set AVT color correction coefficient Crr");

        err=dc1394_set_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CGR, Cgr);
        DC1394_ERR_RTN(err,"Could not set AVT color correction coefficient Cgr");

        err=dc1394_set_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CBR, Cbr);
        DC1394_ERR_RTN(err,"Could not set AVT color correction coefficient Cbr");

        //green channel coefficients
        err=dc1394_set_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CRG, Crg);
        DC1394_ERR_RTN(err,"Could not set AVT color correction coefficient Crg");

        err=dc1394_set_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CGG, Cgg);
        DC1394_ERR_RTN(err,"Could not set AVT color correction coefficient Cgg");

        err=dc1394_set_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CBG, Cbg);
        DC1394_ERR_RTN(err,"Could not set AVT color correction coefficient Cbg");

        //blue channel coefficients
        err=dc1394_set_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CRB, Crb);
        DC1394_ERR_RTN(err,"Could not set AVT color correction coefficient Crb");

        err=dc1394_set_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CGB, Cgb);
        DC1394_ERR_RTN(err,"Could not set AVT color correction coefficient Cgb");

        err=dc1394_set_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CBB, Cbb);
        DC1394_ERR_RTN(err,"Could not set AVT color correction coefficient Cbb");
    }

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Color Correction + Coefficients                                            */
/************************************************************************/

dc1394error_t dc1394_avt_get_color_corr(dc1394camera_t *camera, dc1394bool_t *on_off, int32_t *Crr, int32_t *Cgr, int32_t *Cbr, int32_t *Crg, int32_t *Cgg, int32_t *Cbg, int32_t *Crb, int32_t *Cgb, int32_t *Cbb)
{
    dc1394error_t err;
    uint32_t value;

    //retrieve color correction
    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR, &value);
    DC1394_ERR_RTN(err,"Could not get AVT color correction");

    //ON / OFF : Bit 6
    *on_off = (uint32_t)((value & 0x2000000UL) >> 25);

    //red channel coefficients
    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CRR, (uint32_t *)Crr);
    DC1394_ERR_RTN(err,"Could not get AVT color correction coefficient Crr");

    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CGR, (uint32_t *)Cgr);
    DC1394_ERR_RTN(err,"Could not get AVT color correction coefficient Cgr");

    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CBR, (uint32_t *)Cbr);
    DC1394_ERR_RTN(err,"Could not get AVT color correction coefficient Cbr");

    //green channel coefficients
    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CRG, (uint32_t *)Crg);
    DC1394_ERR_RTN(err,"Could not get AVT color correction coefficient Crg");

    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CGG, (uint32_t *)Cgg);
    DC1394_ERR_RTN(err,"Could not get AVT color correction coefficient Cgg");

    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CBG, (uint32_t *)Cbg);
    DC1394_ERR_RTN(err,"Could not get AVT color correction coefficient Cbg");

    //blue channel coefficients
    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CRB, (uint32_t *)Crb);
    DC1394_ERR_RTN(err,"Could not get AVT color correction coefficient Crb");

    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CGB, (uint32_t *)Cgb);
    DC1394_ERR_RTN(err,"Could not get AVT color correction coefficient Cgb");

    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_COLOR_CORR_CBB, (uint32_t *)Cbb);
    DC1394_ERR_RTN(err,"Could not get AVT color correction coefficient Cbb");

    return DC1394_SUCCESS;
}

/************************************************************************/
/* Get HSNR                                                                   */
/************************************************************************/

dc1394error_t dc1394_avt_get_hsnr(dc1394camera_t *camera, dc1394bool_t *on_off, uint32_t *grabCount)
{
    dc1394error_t err;
    uint32_t value;

    /* Retrieve HSNRR */
    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_HSNRR, &value);
    DC1394_ERR_RTN(err,"Could not get AVT HSNRR");

    /*ON / OFF : Bit 6 */
    *on_off = (uint32_t)((value & 0x2000000UL) >> 25);

    /* grabCount: Bits 23..31 */
    *grabCount =(uint32_t)((value & 0x1FFUL));

    return DC1394_SUCCESS;
}


/************************************************************************/
/* Set HSNR                                                                      */
/************************************************************************/

dc1394error_t dc1394_avt_set_hsnr(dc1394camera_t *camera, dc1394bool_t on_off, uint32_t grabCount)
{
    dc1394error_t err;
    uint32_t curval;

    /* Retrieve HSNR */
    err=dc1394_get_adv_control_register(camera, REG_CAMERA_AVT_HSNRR, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT HSNRR");

    /*ON / OFF : Bit 6 */
    curval = (curval & 0xFDFFFFFFUL) | ((on_off) << 25);

    /* grabCount: Bits 23..31 */
    curval = (curval & 0xFFFFFE00UL) | grabCount;

    /* Set new parameters */
    err=dc1394_set_adv_control_register(camera, REG_CAMERA_AVT_HSNRR, curval);
    DC1394_ERR_RTN(err,"Could not set AVT HSNRR");

    return DC1394_SUCCESS;
}
