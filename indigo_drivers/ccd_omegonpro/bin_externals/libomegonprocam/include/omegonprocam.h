﻿#ifndef __omegonprocam_h__
#define __omegonprocam_h__

/* Version: 59.28926.20250709 */
/*
   Platform & Architecture:
       (1) Win32:
              (a) x64: Win7 or above
              (b) x86: XP SP3 or above; CPU supports SSE2 instruction set or above
              (c) arm64: Win10 or above
              (d) arm: Win10 or above
       (2) WinRT: x64, x86, arm64, arm; Win10 or above
       (3) macOS: x64+arm64: macOS 11.0 or above, support x64 and Apple silicon (such as M1, M2, etc)
       (4) Linux: kernel 2.6.27 or above
              (a) x64: GLIBC 2.14 or above
              (b) x86: CPU supports SSE3 instruction set or above; GLIBC 2.8 or above
              (c) arm64: GLIBC 2.17 or above; built by toolchain aarch64-linux-gnu (version 5.4.0)
              (d) armhf: GLIBC 2.8 or above; built by toolchain arm-linux-gnueabihf (version 5.4.0)
              (e) armel: GLIBC 2.8 or above; built by toolchain arm-linux-gnueabi (version 5.4.0)
       (5) Android: __ANDROID_API__ >= 24 (Android 7.0); built by android-ndk-r18b; see https://developer.android.com/ndk/guides/abis
              (a) arm64: arm64-v8a
              (b) arm: armeabi-v7a
              (c) x64: x86_64
              (d) x86
*/
/*
    doc:
       (1) en.html, English
       (2) hans.html, Simplified Chinese
*/

/*
    Please distinguish between camera ID (camId) and camera SN:
        (a) SN is unique and persistent, fixed inside the camera and remains unchanged, and does not change with connection or system restart.
        (b) Camera ID (camId) may change due to connection or system restart. Enumerate the cameras to get the camera ID, and then call the Open function to pass in the camId parameter to open the camera.
*/

#if defined(_WIN32)
#ifndef _INC_WINDOWS
#include <windows.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__cplusplus) && (__cplusplus >= 201402L)
#define OMEGONPROCAM_DEPRECATED  [[deprecated]]
#elif defined(_MSC_VER)
#define OMEGONPROCAM_DEPRECATED  __declspec(deprecated)
#elif defined(__GNUC__) || defined(__clang__)
#define OMEGONPROCAM_DEPRECATED  __attribute__((deprecated))
#else
#define OMEGONPROCAM_DEPRECATED
#endif

#if defined(_WIN32) /* Windows */
#pragma pack(push, 8)
#ifdef OMEGONPROCAM_EXPORTS
#define OMEGONPROCAM_API(x)    __declspec(dllexport)   x   __stdcall  /* in Windows, we use __stdcall calling convention, see https://docs.microsoft.com/en-us/cpp/cpp/stdcall */
#define OMEGONPROCAM_APIV(x)   __declspec(dllexport)   x   __cdecl
#elif !defined(OMEGONPROCAM_NOIMPORTS)
#define OMEGONPROCAM_API(x)    __declspec(dllimport)   x   __stdcall
#define OMEGONPROCAM_APIV(x)   __declspec(dllimport)   x   __cdecl
#else
#define OMEGONPROCAM_API(x)    x   __stdcall
#define OMEGONPROCAM_APIV(x)   x   __cdecl
#endif
#else   /* Linux or macOS */
#define OMEGONPROCAM_API(x)    x
#define OMEGONPROCAM_APIV(x)   x
#if (!defined(HRESULT)) && (!defined(__COREFOUNDATION_CFPLUGINCOM__)) /* CFPlugInCOM.h */
#define HRESULT int
#endif
#ifndef SUCCEEDED
#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#endif
#ifndef FAILED
#define FAILED(hr)      (((HRESULT)(hr)) < 0)
#endif
#ifndef __stdcall
#define __stdcall
#endif
#endif

#ifndef TDIBWIDTHBYTES
#define TDIBWIDTHBYTES(bits)  ((unsigned)(((bits) + 31) & (~31)) / 8)
#endif

/********************************************************************************/
/* HRESULT: error code                                                          */
/* Please note that the return value >= 0 means success                         */
/* (especially S_FALSE is also successful, indicating that the internal value and the value set by the user is equivalent, which means "no operation"). */
/* Therefore, the SUCCEEDED and FAILED macros should generally be used to determine whether the return value is successful or failed. */
/* (Unless there are special needs, do not use "==S_OK" or "==0" to judge the return value) */
/*                                                                              */
/* #define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)                               */
/* #define FAILED(hr)      (((HRESULT)(hr)) < 0)                                */
/*                                                                              */
/********************************************************************************/
#if defined(OMEGONPROCAM_HRESULT_ERRORCODE_NEEDED)
#define S_OK                (HRESULT)(0x00000000) /* Success */
#define S_FALSE             (HRESULT)(0x00000001) /* Yet another success */ /* Remark: Different from S_OK, such as internal values and user-set values have coincided, equivalent to noop */
#define E_UNEXPECTED        (HRESULT)(0x8000ffff) /* Catastrophic failure */ /* Remark: Generally indicates that the conditions are not met, such as calling put_Option setting some options that do not support modification when the camera is running, and so on */
#define E_NOTIMPL           (HRESULT)(0x80004001) /* Not supported or not implemented */ /* Remark: This feature is not supported on this model of camera */
#define E_NOINTERFACE       (HRESULT)(0x80004002)
#define E_ACCESSDENIED      (HRESULT)(0x80070005) /* Permission denied */ /* Remark: The program on Linux does not have permission to open the USB device, please enable udev rules file or run as root */
#define E_OUTOFMEMORY       (HRESULT)(0x8007000e) /* Out of memory */
#define E_INVALIDARG        (HRESULT)(0x80070057) /* One or more arguments are not valid */
#define E_POINTER           (HRESULT)(0x80004003) /* Pointer that is not valid */ /* Remark: Pointer is NULL */
#define E_FAIL              (HRESULT)(0x80004005) /* Generic failure */
#define E_WRONG_THREAD      (HRESULT)(0x8001010e) /* Call function in the wrong thread */
#define E_GEN_FAILURE       (HRESULT)(0x8007001f) /* Device not functioning */ /* Remark: It is generally caused by hardware errors, such as cable problems, USB port problems, poor contact, camera hardware damage, etc */
#define E_BUSY              (HRESULT)(0x800700aa) /* The requested resource is in use */ /* Remark: The camera is already in use, such as duplicated opening/starting the camera, or being used by other application, etc */
#define E_PENDING           (HRESULT)(0x8000000a) /* The data necessary to complete this operation is not yet available */ /* Remark: No data is available at this time */
#define E_TIMEOUT           (HRESULT)(0x8001011f) /* This operation returned because the timeout period expired */
#define E_UNREACH           (HRESULT)(0x80072743) /* Network is unreachable */
#endif

/* handle */
typedef struct Omegonprocam_t { int unused; } *HOmegonprocam;

#define OMEGONPROCAM_MAX                       128
                                         
#define OMEGONPROCAM_FLAG_CMOS                 0x00000001  /* cmos sensor */
#define OMEGONPROCAM_FLAG_CCD_PROGRESSIVE      0x00000002  /* progressive ccd sensor */
#define OMEGONPROCAM_FLAG_CCD_INTERLACED       0x00000004  /* interlaced ccd sensor */
#define OMEGONPROCAM_FLAG_ROI_HARDWARE         0x00000008  /* support hardware ROI */
#define OMEGONPROCAM_FLAG_MONO                 0x00000010  /* monochromatic */
#define OMEGONPROCAM_FLAG_BINSKIP_SUPPORTED    0x00000020  /* support bin/skip mode, see Omegonprocam_put_Mode and Omegonprocam_get_Mode */
#define OMEGONPROCAM_FLAG_USB30                0x00000040  /* usb3.0 */
#define OMEGONPROCAM_FLAG_TEC                  0x00000080  /* Thermoelectric Cooler */
#define OMEGONPROCAM_FLAG_USB30_OVER_USB20     0x00000100  /* usb3.0 camera connected to usb2.0 port */
#define OMEGONPROCAM_FLAG_ST4                  0x00000200  /* ST4 port */
#define OMEGONPROCAM_FLAG_GETTEMPERATURE       0x00000400  /* support to get the temperature of the sensor */
#define OMEGONPROCAM_FLAG_HIGH_FULLWELL        0x00000800  /* high fullwell capacity */
#define OMEGONPROCAM_FLAG_RAW10                0x00001000  /* pixel format, RAW 10bits */
#define OMEGONPROCAM_FLAG_RAW12                0x00002000  /* pixel format, RAW 12bits */
#define OMEGONPROCAM_FLAG_RAW14                0x00004000  /* pixel format, RAW 14bits */
#define OMEGONPROCAM_FLAG_RAW16                0x00008000  /* pixel format, RAW 16bits */
#define OMEGONPROCAM_FLAG_FAN                  0x00010000  /* cooling fan */
#define OMEGONPROCAM_FLAG_TEC_ONOFF            0x00020000  /* Thermoelectric Cooler can be turn on or off, support to set the target temperature of TEC */
#define OMEGONPROCAM_FLAG_ISP                  0x00040000  /* ISP (Image Signal Processing) chip */
#define OMEGONPROCAM_FLAG_TRIGGER_SOFTWARE     0x00080000  /* support software trigger */
#define OMEGONPROCAM_FLAG_TRIGGER_EXTERNAL     0x00100000  /* support external trigger */
#define OMEGONPROCAM_FLAG_TRIGGER_SINGLE       0x00200000  /* only support trigger single: one trigger, one image */
#define OMEGONPROCAM_FLAG_BLACKLEVEL           0x00400000  /* support set and get the black level */
#define OMEGONPROCAM_FLAG_AUTO_FOCUS           0x00800000  /* support auto focus */
#define OMEGONPROCAM_FLAG_BUFFER               0x01000000  /* frame buffer */
#define OMEGONPROCAM_FLAG_DDR                  0x02000000  /* use very large capacity DDR (Double Data Rate SDRAM) for frame buffer. The capacity is not less than one full frame */
#define OMEGONPROCAM_FLAG_CG                   0x04000000  /* Conversion Gain: HCG, LCG */
#define OMEGONPROCAM_FLAG_YUV411               0x08000000  /* pixel format, yuv411 */
#define OMEGONPROCAM_FLAG_VUYY                 0x10000000  /* pixel format, yuv422, VUYY */
#define OMEGONPROCAM_FLAG_YUV444               0x20000000  /* pixel format, yuv444 */
#define OMEGONPROCAM_FLAG_RGB888               0x40000000  /* pixel format, RGB888 */
#define OMEGONPROCAM_FLAG_RAW8                 0x80000000  /* pixel format, RAW 8 bits */
#define OMEGONPROCAM_FLAG_GMCY8                0x0000000100000000  /* pixel format, GMCY, 8bits */
#define OMEGONPROCAM_FLAG_GMCY12               0x0000000200000000  /* pixel format, GMCY, 12bits */
#define OMEGONPROCAM_FLAG_UYVY                 0x0000000400000000  /* pixel format, yuv422, UYVY */
#define OMEGONPROCAM_FLAG_CGHDR                0x0000000800000000  /* Conversion Gain: HCG, LCG, HDR */
#define OMEGONPROCAM_FLAG_GLOBALSHUTTER        0x0000001000000000  /* global shutter */
#define OMEGONPROCAM_FLAG_FOCUSMOTOR           0x0000002000000000  /* support focus motor */
#define OMEGONPROCAM_FLAG_PRECISE_FRAMERATE    0x0000004000000000  /* support precise framerate & bandwidth, see OMEGONPROCAM_OPTION_PRECISE_FRAMERATE & OMEGONPROCAM_OPTION_BANDWIDTH */
#define OMEGONPROCAM_FLAG_HEAT                 0x0000008000000000  /* support heat to prevent fogging up */
#define OMEGONPROCAM_FLAG_LOW_NOISE            0x0000010000000000  /* support low noise mode (Higher signal noise ratio, lower frame rate) */
#define OMEGONPROCAM_FLAG_LEVELRANGE_HARDWARE  0x0000020000000000  /* hardware level range, put(get)_LevelRangeV2 */
#define OMEGONPROCAM_FLAG_EVENT_HARDWARE       0x0000040000000000  /* hardware event, such as exposure start & stop */
#define OMEGONPROCAM_FLAG_LIGHTSOURCE          0x0000080000000000  /* embedded light source */
#define OMEGONPROCAM_FLAG_FILTERWHEEL          0x0000100000000000  /* astro filter wheel */
#define OMEGONPROCAM_FLAG_GIGE                 0x0000200000000000  /* 1 Gigabit GigE */
#define OMEGONPROCAM_FLAG_10GIGE               0x0000400000000000  /* 10 Gigabit GigE */
#define OMEGONPROCAM_FLAG_5GIGE                0x0000800000000000  /* 5 Gigabit GigE */
#define OMEGONPROCAM_FLAG_25GIGE               0x0001000000000000  /* 2.5 Gigabit GigE */
#define OMEGONPROCAM_FLAG_AUTOFOCUSER          0x0002000000000000  /* astro auto focuser */
#define OMEGONPROCAM_FLAG_LIGHT_SOURCE         0x0004000000000000  /* stand alone light source */
#define OMEGONPROCAM_FLAG_CAMERALINK           0x0008000000000000  /* camera link */
#define OMEGONPROCAM_FLAG_CXP                  0x0010000000000000  /* CXP: CoaXPress */
#define OMEGONPROCAM_FLAG_RAW12PACK            0x0020000000000000  /* pixel format, RAW 12bits packed */
#define OMEGONPROCAM_FLAG_SELFTRIGGER          0x0040000000000000  /* self trigger */
#define OMEGONPROCAM_FLAG_RAW11                0x0080000000000000  /* pixel format, RAW 11bits */
#define OMEGONPROCAM_FLAG_GHOPTO               0x0100000000000000  /* ghopto sensor */
#define OMEGONPROCAM_FLAG_RAW10PACK            0x0200000000000000  /* pixel format, RAW 10bits packed */
#define OMEGONPROCAM_FLAG_USB32                0x0400000000000000  /* usb3.2 */
#define OMEGONPROCAM_FLAG_USB32_OVER_USB30     0x0800000000000000  /* usb3.2 camera connected to usb3.0 port */

#define OMEGONPROCAM_EXPOGAIN_DEF              100     /* exposure gain, default value */
#define OMEGONPROCAM_EXPOGAIN_MIN              100     /* exposure gain, minimum value */
#define OMEGONPROCAM_TEMP_DEF                  6503    /* color temperature, default value */
#define OMEGONPROCAM_TEMP_MIN                  2000    /* color temperature, minimum value */
#define OMEGONPROCAM_TEMP_MAX                  15000   /* color temperature, maximum value */
#define OMEGONPROCAM_TINT_DEF                  1000    /* tint */
#define OMEGONPROCAM_TINT_MIN                  200     /* tint */
#define OMEGONPROCAM_TINT_MAX                  2500    /* tint */
#define OMEGONPROCAM_HUE_DEF                   0       /* hue */
#define OMEGONPROCAM_HUE_MIN                   (-180)  /* hue */
#define OMEGONPROCAM_HUE_MAX                   180     /* hue */
#define OMEGONPROCAM_SATURATION_DEF            128     /* saturation */
#define OMEGONPROCAM_SATURATION_MIN            0       /* saturation */
#define OMEGONPROCAM_SATURATION_MAX            255     /* saturation */
#define OMEGONPROCAM_BRIGHTNESS_DEF            0       /* brightness */
#define OMEGONPROCAM_BRIGHTNESS_MIN            (-255)  /* brightness */
#define OMEGONPROCAM_BRIGHTNESS_MAX            255     /* brightness */
#define OMEGONPROCAM_CONTRAST_DEF              0       /* contrast */
#define OMEGONPROCAM_CONTRAST_MIN              (-255)  /* contrast */
#define OMEGONPROCAM_CONTRAST_MAX              255     /* contrast */
#define OMEGONPROCAM_GAMMA_DEF                 100     /* gamma */
#define OMEGONPROCAM_GAMMA_MIN                 20      /* gamma */
#define OMEGONPROCAM_GAMMA_MAX                 180     /* gamma */
#define OMEGONPROCAM_AETARGET_DEF              120     /* target of auto exposure */
#define OMEGONPROCAM_AETARGET_MIN              16      /* target of auto exposure */
#define OMEGONPROCAM_AETARGET_MAX              220     /* target of auto exposure */
#define OMEGONPROCAM_WBGAIN_DEF                0       /* white balance gain */
#define OMEGONPROCAM_WBGAIN_MIN                (-127)  /* white balance gain */
#define OMEGONPROCAM_WBGAIN_MAX                127     /* white balance gain */
#define OMEGONPROCAM_BLACKLEVEL_MIN            0       /* minimum black level */
#define OMEGONPROCAM_BLACKLEVEL8_MAX           31              /* maximum black level for bitdepth = 8 */
#define OMEGONPROCAM_BLACKLEVEL10_MAX          (31 * 4)        /* maximum black level for bitdepth = 10 */
#define OMEGONPROCAM_BLACKLEVEL11_MAX          (31 * 8)        /* maximum black level for bitdepth = 11 */
#define OMEGONPROCAM_BLACKLEVEL12_MAX          (31 * 16)       /* maximum black level for bitdepth = 12 */
#define OMEGONPROCAM_BLACKLEVEL14_MAX          (31 * 64)       /* maximum black level for bitdepth = 14 */
#define OMEGONPROCAM_BLACKLEVEL16_MAX          (31 * 256)      /* maximum black level for bitdepth = 16 */
#define OMEGONPROCAM_SHARPENING_STRENGTH_DEF   0       /* sharpening strength */
#define OMEGONPROCAM_SHARPENING_STRENGTH_MIN   0       /* sharpening strength */
#define OMEGONPROCAM_SHARPENING_STRENGTH_MAX   500     /* sharpening strength */
#define OMEGONPROCAM_SHARPENING_RADIUS_DEF     2       /* sharpening radius */
#define OMEGONPROCAM_SHARPENING_RADIUS_MIN     1       /* sharpening radius */
#define OMEGONPROCAM_SHARPENING_RADIUS_MAX     10      /* sharpening radius */
#define OMEGONPROCAM_SHARPENING_THRESHOLD_DEF  0       /* sharpening threshold */
#define OMEGONPROCAM_SHARPENING_THRESHOLD_MIN  0       /* sharpening threshold */
#define OMEGONPROCAM_SHARPENING_THRESHOLD_MAX  255     /* sharpening threshold */
#define OMEGONPROCAM_AUTOEXPO_THRESHOLD_DEF    5       /* auto exposure threshold */
#define OMEGONPROCAM_AUTOEXPO_THRESHOLD_MIN    2       /* auto exposure threshold */
#define OMEGONPROCAM_AUTOEXPO_THRESHOLD_MAX    15      /* auto exposure threshold */
#define OMEGONPROCAM_AUTOEXPO_THLD_TRIGGER_DEF 5       /* auto exposure trigger threshold */
#define OMEGONPROCAM_AUTOEXPO_THLD_TRIGGER_MIN 2       /* auto exposure trigger threshold */
#define OMEGONPROCAM_AUTOEXPO_THLD_TRIGGER_MAX 64      /* auto exposure trigger threshold */
#define OMEGONPROCAM_AUTOEXPO_DAMP_DEF         0       /* auto exposure damping coefficient: thousandths */
#define OMEGONPROCAM_AUTOEXPO_DAMP_MIN         0       /* auto exposure damping coefficient: thousandths */
#define OMEGONPROCAM_AUTOEXPO_DAMP_MAX         1000    /* auto exposure damping coefficient: thousandths */
#define OMEGONPROCAM_BANDWIDTH_DEF             100     /* bandwidth */
#define OMEGONPROCAM_BANDWIDTH_MIN             1       /* bandwidth */
#define OMEGONPROCAM_BANDWIDTH_MAX             100     /* bandwidth */
#define OMEGONPROCAM_DENOISE_DEF               0       /* denoise */
#define OMEGONPROCAM_DENOISE_MIN               0       /* denoise */
#define OMEGONPROCAM_DENOISE_MAX               100     /* denoise */
#define OMEGONPROCAM_HEARTBEAT_MIN             100     /* millisecond */
#define OMEGONPROCAM_HEARTBEAT_MAX             10000   /* millisecond */
#define OMEGONPROCAM_AE_PERCENT_MIN            0       /* auto exposure percent; 0 or 100 => full roi average, means "disabled" */
#define OMEGONPROCAM_AE_PERCENT_MAX            100
#define OMEGONPROCAM_AE_PERCENT_DEF            1       /* auto exposure percent: enabled, percentage = 1% */
#define OMEGONPROCAM_NOPACKET_TIMEOUT_MIN      500     /* no packet timeout minimum: 500ms */
#define OMEGONPROCAM_NOFRAME_TIMEOUT_MIN       500     /* no frame timeout minimum: 500ms */
#define OMEGONPROCAM_DYNAMIC_DEFECT_T1_MIN     0       /* dynamic defect pixel correction, dead pixel ratio: the smaller the dead ratio is, the more stringent the conditions for processing dead pixels are, and fewer pixels will be processed */
#define OMEGONPROCAM_DYNAMIC_DEFECT_T1_MAX     100     /* means: 1.0 */
#define OMEGONPROCAM_DYNAMIC_DEFECT_T1_DEF     90      /* means: 0.9 */
#define OMEGONPROCAM_DYNAMIC_DEFECT_T2_MIN     0       /* dynamic defect pixel correction, hot pixel ratio: the smaller the hot ratio is, the more stringent the conditions for processing hot pixels are, and fewer pixels will be processed */
#define OMEGONPROCAM_DYNAMIC_DEFECT_T2_MAX     100
#define OMEGONPROCAM_DYNAMIC_DEFECT_T2_DEF     90
#define OMEGONPROCAM_HDR_K_MIN                 1       /* HDR synthesize */
#define OMEGONPROCAM_HDR_K_MAX                 25500
#define OMEGONPROCAM_HDR_B_MIN                 0
#define OMEGONPROCAM_HDR_B_MAX                 65535
#define OMEGONPROCAM_HDR_THRESHOLD_MIN         0
#define OMEGONPROCAM_HDR_THRESHOLD_MAX         4094
#define OMEGONPROCAM_CDS_MIN                   0       /* Correlated Double Sampling */
#define OMEGONPROCAM_CDS_MAX                   100

typedef struct {
    unsigned    width;
    unsigned    height;
} OmegonprocamResolution;

/* In Windows platform, we always use UNICODE wchar_t */
/* In Linux or macOS, we use char */

typedef struct {
#if defined(_WIN32)
    const wchar_t*      name;        /* model name, in Windows, we use unicode */
#else
    const char*         name;        /* model name */
#endif
    unsigned long long  flag;        /* OMEGONPROCAM_FLAG_xxx, 64 bits */
    unsigned            maxspeed;    /* number of speed level, same as Omegonprocam_get_MaxSpeed(), speed range = [0, maxspeed], closed interval */
    unsigned            preview;     /* number of preview resolution, same as Omegonprocam_get_ResolutionNumber() */
    unsigned            still;       /* number of still resolution, same as Omegonprocam_get_StillResolutionNumber() */
    unsigned            maxfanspeed; /* maximum fan speed, fan speed range = [0, max], closed interval */
    unsigned            ioctrol;     /* number of input/output control */
    float               xpixsz;      /* physical pixel size in micrometer */
    float               ypixsz;      /* physical pixel size in micrometer */
    OmegonprocamResolution   res[16];
} OmegonprocamModelV2; /* device model v2 */

typedef struct {
#if defined(_WIN32)
    wchar_t               displayname[64];    /* display name: model name or user-defined name (if any and Omegonprocam_EnumWithName) */
    wchar_t               id[64];             /* unique and opaque id of a connected camera, for Omegonprocam_Open */
#else
    char                  displayname[64];    /* display name: model name or user-defined name (if any and Omegonprocam_EnumWithName) */
    char                  id[64];             /* unique and opaque id of a connected camera, for Omegonprocam_Open */
#endif
    const OmegonprocamModelV2* model;
} OmegonprocamDeviceV2; /* device instance for enumerating */

/*
    get the version of this dll/so/dylib, which is: 59.28926.20250709
*/
#if defined(_WIN32)
OMEGONPROCAM_API(const wchar_t*)   Omegonprocam_Version();
#else
OMEGONPROCAM_API(const char*)      Omegonprocam_Version();
#endif

/*
    enumerate the cameras connected to the computer, return the number of enumerated.

    OmegonprocamDeviceV2 arr[OMEGONPROCAM_MAX];
    unsigned cnt = Omegonprocam_EnumV2(arr);
    for (unsigned i = 0; i < cnt; ++i)
        ...

    if arr == NULL, then, only the number is returned.
    Omegonprocam_Enum is obsolete.
*/
OMEGONPROCAM_API(unsigned) Omegonprocam_EnumV2(OmegonprocamDeviceV2 arr[OMEGONPROCAM_MAX]);

/* use the camId of OmegonprocamDeviceV2, which is enumerated by Omegonprocam_EnumV2.
    if camId is NULL, Omegonprocam_Open will open the first enumerated camera.
    For USB, GigE or PCIe camera, the camId can the camId can also be specified as (case sensitive):
        (a) "sn:xxxxxxxxxxxx" (such as sn:ZP250212241204105)
        (b) "name:xxxxxx" (such as name: Camera1)
    Moreover, for GigE camera, the camId can also be specified as (case sensitive):
        (a) "ip:xxx.xxx.xxx.xxx" (such as ip:192.168.1.100) or
        (b) "mac:xxxxxxxxxxxx" (such as mac:d05f64ffff23)
    For the issue of opening the camera on Android, please refer to the documentation
*/
#if defined(_WIN32)
OMEGONPROCAM_API(HOmegonprocam) Omegonprocam_Open(const wchar_t* camId);
#else
OMEGONPROCAM_API(HOmegonprocam) Omegonprocam_Open(const char* camId);
#endif

/*
    the same with Omegonprocam_Open, but use the index as the parameter. such as:
    index == 0, open the first camera,
    index == 1, open the second camera,
    etc
*/
OMEGONPROCAM_API(HOmegonprocam) Omegonprocam_OpenByIndex(unsigned index);

/* close the handle. After it is closed, never use the handle any more. */
OMEGONPROCAM_API(void)     Omegonprocam_Close(HOmegonprocam h);

#define OMEGONPROCAM_EVENT_EXPOSURE          0x0001    /* exposure time or gain changed */
#define OMEGONPROCAM_EVENT_TEMPTINT          0x0002    /* white balance changed, Temp/Tint mode */
#define OMEGONPROCAM_EVENT_IMAGE             0x0004    /* live image arrived, use Omegonprocam_PullImageXXXX to get this image */
#define OMEGONPROCAM_EVENT_STILLIMAGE        0x0005    /* snap (still) frame arrived, use Omegonprocam_PullStillImageXXXX to get this frame */
#define OMEGONPROCAM_EVENT_WBGAIN            0x0006    /* white balance changed, RGB Gain mode */
#define OMEGONPROCAM_EVENT_TRIGGERFAIL       0x0007    /* trigger failed */
#define OMEGONPROCAM_EVENT_BLACK             0x0008    /* black balance changed */
#define OMEGONPROCAM_EVENT_FFC               0x0009    /* flat field correction status changed */
#define OMEGONPROCAM_EVENT_DFC               0x000a    /* dark field correction status changed */
#define OMEGONPROCAM_EVENT_ROI               0x000b    /* roi changed */
#define OMEGONPROCAM_EVENT_LEVELRANGE        0x000c    /* level range changed */
#define OMEGONPROCAM_EVENT_AUTOEXPO_CONV     0x000d    /* auto exposure convergence */
#define OMEGONPROCAM_EVENT_AUTOEXPO_CONVFAIL 0x000e    /* auto exposure once mode convergence failed */
#define OMEGONPROCAM_EVENT_FPNC              0x000f    /* fix pattern noise correction status changed */
#define OMEGONPROCAM_EVENT_ERROR             0x0080    /* generic error */
#define OMEGONPROCAM_EVENT_DISCONNECTED      0x0081    /* camera disconnected */
#define OMEGONPROCAM_EVENT_NOFRAMETIMEOUT    0x0082    /* no frame timeout error */
#define OMEGONPROCAM_EVENT_FOCUSPOS          0x0084    /* focus positon */
#define OMEGONPROCAM_EVENT_NOPACKETTIMEOUT   0x0085    /* no packet timeout */
#define OMEGONPROCAM_EVENT_EXPO_START        0x4000    /* hardware event: exposure start */
#define OMEGONPROCAM_EVENT_EXPO_STOP         0x4001    /* hardware event: exposure stop */
#define OMEGONPROCAM_EVENT_TRIGGER_ALLOW     0x4002    /* hardware event: next trigger allow */
#define OMEGONPROCAM_EVENT_HEARTBEAT         0x4003    /* hardware event: heartbeat, can be used to monitor whether the camera is alive */
#define OMEGONPROCAM_EVENT_TRIGGER_IN        0x4004    /* hardware event: trigger in */
#define OMEGONPROCAM_EVENT_FACTORY           0x8001    /* restore factory settings */

#if defined(_WIN32)
OMEGONPROCAM_API(HRESULT)  Omegonprocam_StartPullModeWithWndMsg(HOmegonprocam h, HWND hWnd, UINT nMsg);
#endif

/* Do NOT call Omegonprocam_Close, Omegonprocam_Stop in this callback context, it deadlocks. */
/* Do NOT call Omegonprocam_put_Option with OMEGONPROCAM_OPTION_TRIGGER, OMEGONPROCAM_OPTION_BITDEPTH, OMEGONPROCAM_OPTION_PIXEL_FORMAT, OMEGONPROCAM_OPTION_BINNING, OMEGONPROCAM_OPTION_ROTATE, it will fail with error code E_WRONG_THREAD */
typedef void (__stdcall* POMEGONPROCAM_EVENT_CALLBACK)(unsigned nEvent, void* ctxEvent);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_StartPullModeWithCallback(HOmegonprocam h, POMEGONPROCAM_EVENT_CALLBACK funEvent, void* ctxEvent);

#define OMEGONPROCAM_FRAMEINFO_FLAG_SEQ                0x00000001 /* frame sequence number */
#define OMEGONPROCAM_FRAMEINFO_FLAG_TIMESTAMP          0x00000002 /* timestamp */
#define OMEGONPROCAM_FRAMEINFO_FLAG_EXPOTIME           0x00000004 /* exposure time */
#define OMEGONPROCAM_FRAMEINFO_FLAG_EXPOGAIN           0x00000008 /* exposure gain */
#define OMEGONPROCAM_FRAMEINFO_FLAG_BLACKLEVEL         0x00000010 /* black level */
#define OMEGONPROCAM_FRAMEINFO_FLAG_SHUTTERSEQ         0x00000020 /* sequence shutter counter */
#define OMEGONPROCAM_FRAMEINFO_FLAG_GPS                0x00000040 /* GPS */
#define OMEGONPROCAM_FRAMEINFO_FLAG_AUTOFOCUS          0x00000080 /* auto focus: uLum & uFV */
#define OMEGONPROCAM_FRAMEINFO_FLAG_COUNT              0x00000100 /* timecount, framecount, tricount */
#define OMEGONPROCAM_FRAMEINFO_FLAG_MECHANICALSHUTTER  0x00000200 /* Mechanical shutter: closed */
#define OMEGONPROCAM_FRAMEINFO_FLAG_STILL              0x00008000 /* still image */
#define OMEGONPROCAM_FRAMEINFO_FLAG_CG                 0x00010000 /* Conversion Gain: High */

typedef struct {
    unsigned            width;
    unsigned            height;
    unsigned            flag;       /* OMEGONPROCAM_FRAMEINFO_FLAG_xxxx */
    unsigned            seq;        /* frame sequence number */
    unsigned long long  timestamp;  /* microsecond */
    unsigned            shutterseq; /* sequence shutter counter */
    unsigned            expotime;   /* exposure time */
    unsigned short      expogain;   /* exposure gain */
    unsigned short      blacklevel; /* black level */
} OmegonprocamFrameInfoV3;

typedef struct {
    unsigned long long utcstart;    /* exposure start time: nanosecond since epoch (00:00:00 UTC on Thursday, 1 January 1970, see https://en.wikipedia.org/wiki/Unix_time) */
    unsigned long long utcend;      /* exposure end time */
    int                longitude;   /* millionth of a degree, 0.000001 degree */
    int                latitude;
    int                altitude;    /* millimeter */
    unsigned short     satellite;   /* number of satellite */
    unsigned short     reserved;    /* not used */
} OmegonprocamGps;

typedef struct {
    OmegonprocamFrameInfoV3 v3;
    unsigned reserved; /* not used */
    unsigned uLum;
    unsigned long long uFV;
    unsigned long long timecount;
    unsigned framecount, tricount;
    OmegonprocamGps gps;
} OmegonprocamFrameInfoV4;

/*
    nWaitMS: The timeout interval, in milliseconds. If a nonzero value is specified, the function waits until the image is ok or the interval elapses.
             If nWaitMS is zero, the function does not enter a wait state if the image is not available; it always returns immediately; this is equal to Omegonprocam_PullImageV4.
    bStill: to pull still image, set to 1, otherwise 0
    bits: 24 (RGB24), 32 (RGB32), 48 (RGB48), 8 (Grey), 16 (Grey), 64 (RGB64).
          In RAW mode, this parameter is ignored.
          bits = 0 means using default bits base on OMEGONPROCAM_OPTION_RGB.
          When bits and OMEGONPROCAM_OPTION_RGB are inconsistent, format conversion will have to be performed, resulting in loss of efficiency.
          See the following bits and OMEGONPROCAM_OPTION_RGB correspondence table:
            ----------------------------------------------------------------------------------------------------------------------
            | OMEGONPROCAM_OPTION_RGB |   0 (RGB24)   |   1 (RGB48)   |   2 (RGB32)   |   3 (Grey8)   |  4 (Grey16)   |   5 (RGB64)   |
            |--------------------|---------------|---------------|---------------|---------------|---------------|---------------|
            | bits = 0           |      24       |       48      |      32       |       8       |       16      |       64      |
            |--------------------|---------------|---------------|---------------|---------------|---------------|---------------|
            | bits = 24          |      24       |       NA      | Convert to 24 | Convert to 24 |       NA      |       NA      |
            |--------------------|---------------|---------------|---------------|---------------|---------------|---------------|
            | bits = 32          | Convert to 32 |       NA      |       32      | Convert to 32 |       NA      |       NA      |
            |--------------------|---------------|---------------|---------------|---------------|---------------|---------------|
            | bits = 48          |      NA       |       48      |       NA      |       NA      | Convert to 48 | Convert to 48 |
            |--------------------|---------------|---------------|---------------|---------------|---------------|---------------|
            | bits = 8           | Convert to 8  |       NA      | Convert to 8  |       8       |       NA      |       NA      |
            |--------------------|---------------|---------------|---------------|---------------|---------------|---------------|
            | bits = 16          |      NA       | Convert to 16 |       NA      |       NA      |       16      | Convert to 16 |
            |--------------------|---------------|---------------|---------------|---------------|---------------|---------------|
            | bits = 64          |      NA       | Convert to 64 |       NA      |       NA      | Convert to 64 |       64      |
            |--------------------|---------------|---------------|---------------|---------------|---------------|---------------|

    rowPitch: The distance from one row to the next row. rowPitch = 0 means using the default row pitch. rowPitch = -1 means zero padding, see below:
            ----------------------------------------------------------------------------------------------
            | format                             | 0 means default row pitch     | -1 means zero padding |
            |------------------------------------|-------------------------------|-----------------------|
            | RGB       | RGB24                  | TDIBWIDTHBYTES(24 * Width)    | Width * 3             |
            |           | RGB32                  | Width * 4                     | Width * 4             |
            |           | RGB48                  | TDIBWIDTHBYTES(48 * Width)    | Width * 6             |
            |           | GREY8                  | TDIBWIDTHBYTES(8 * Width)     | Width                 |
            |           | GREY16                 | TDIBWIDTHBYTES(16 * Width)    | Width * 2             |
            |           | RGB64                  | Width * 8                     | Width * 8             |
            |-----------|------------------------|-------------------------------|-----------------------|
            | RAW       | 8bits Mode             | Width                         | Width                 |
            |           | 10/12/14/16bits Mode   | Width * 2                     | Width * 2             |
            |-----------|------------------------|-------------------------------|-----------------------|
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_PullImageV4(HOmegonprocam h, void* pImageData, int bStill, int bits, int rowPitch, OmegonprocamFrameInfoV4* pInfo);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_WaitImageV4(HOmegonprocam h, unsigned nWaitMS, void* pImageData, int bStill, int bits, int rowPitch, OmegonprocamFrameInfoV4* pInfo);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_PullImageV3(HOmegonprocam h, void* pImageData, int bStill, int bits, int rowPitch, OmegonprocamFrameInfoV3* pInfo);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_WaitImageV3(HOmegonprocam h, unsigned nWaitMS, void* pImageData, int bStill, int bits, int rowPitch, OmegonprocamFrameInfoV3* pInfo);

typedef struct {
    unsigned            width;
    unsigned            height;
    unsigned            flag;       /* OMEGONPROCAM_FRAMEINFO_FLAG_xxxx */
    unsigned            seq;        /* frame sequence number */
    unsigned long long  timestamp;  /* microsecond */
} OmegonprocamFrameInfoV2;

OMEGONPROCAM_API(HRESULT)  Omegonprocam_PullImageV2(HOmegonprocam h, void* pImageData, int bits, OmegonprocamFrameInfoV2* pInfo);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_PullStillImageV2(HOmegonprocam h, void* pImageData, int bits, OmegonprocamFrameInfoV2* pInfo);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_PullImageWithRowPitchV2(HOmegonprocam h, void* pImageData, int bits, int rowPitch, OmegonprocamFrameInfoV2* pInfo);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_PullStillImageWithRowPitchV2(HOmegonprocam h, void* pImageData, int bits, int rowPitch, OmegonprocamFrameInfoV2* pInfo);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_PullImage(HOmegonprocam h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_PullStillImage(HOmegonprocam h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_PullImageWithRowPitch(HOmegonprocam h, void* pImageData, int bits, int rowPitch, unsigned* pnWidth, unsigned* pnHeight);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_PullStillImageWithRowPitch(HOmegonprocam h, void* pImageData, int bits, int rowPitch, unsigned* pnWidth, unsigned* pnHeight);

/*
    (NULL == pData) means something error
    ctxData is the callback context which is passed by Omegonprocam_StartPushModeV3
    bSnap: TRUE if Omegonprocam_Snap

    funData is callbacked by an internal thread of omegonprocam.dll, so please pay attention to multithread problem.
    Do NOT call Omegonprocam_Close, Omegonprocam_Stop in this callback context, it deadlocks.
*/
typedef void (__stdcall* POMEGONPROCAM_DATA_CALLBACK_V4)(const void* pData, const OmegonprocamFrameInfoV3* pInfo, int bSnap, void* ctxData);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_StartPushModeV4(HOmegonprocam h, POMEGONPROCAM_DATA_CALLBACK_V4 funData, void* ctxData, POMEGONPROCAM_EVENT_CALLBACK funEvent, void* ctxEvent);

typedef void (__stdcall* POMEGONPROCAM_DATA_CALLBACK_V3)(const void* pData, const OmegonprocamFrameInfoV2* pInfo, int bSnap, void* ctxData);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_StartPushModeV3(HOmegonprocam h, POMEGONPROCAM_DATA_CALLBACK_V3 funData, void* ctxData, POMEGONPROCAM_EVENT_CALLBACK funEvent, void* ctxEvent);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_Stop(HOmegonprocam h);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_Pause(HOmegonprocam h, int bPause); /* 1 => pause, 0 => continue */

/*  for pull mode: OMEGONPROCAM_EVENT_STILLIMAGE, and then Omegonprocam_PullStillImageXXXX/Omegonprocam_PullImageV4
    for push mode: the snapped image will be return by POMEGONPROCAM_DATA_CALLBACK(V2/V3), with the parameter 'bSnap' set to 'TRUE'
    nResolutionIndex = 0xffffffff means use the cureent preview resolution
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_Snap(HOmegonprocam h, unsigned nResolutionIndex);  /* still image snap */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_SnapN(HOmegonprocam h, unsigned nResolutionIndex, unsigned nNumber);  /* multiple still image snap */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_SnapR(HOmegonprocam h, unsigned nResolutionIndex, unsigned nNumber);  /* multiple RAW still image snap */
/*
    soft trigger:
    nNumber:    0xffff:     trigger continuously
                0:          cancel trigger, see OMEGONPROCAM_OPTION_TRIGGER_CANCEL_MODE
                others:     number of images to be triggered
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_Trigger(HOmegonprocam h, unsigned short nNumber);

/*
    trigger synchronously
    nWaitMS:    0:              by default, exposure * 102% + 4000 milliseconds
                0xffffffff:     wait infinite
                other:          milliseconds to wait
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_TriggerSyncV4(HOmegonprocam h, unsigned nWaitMS, void* pImageData, int bits, int rowPitch, OmegonprocamFrameInfoV4* pInfo);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_TriggerSync(HOmegonprocam h, unsigned nWaitMS, void* pImageData, int bits, int rowPitch, OmegonprocamFrameInfoV3* pInfo);

/*
    put_Size, put_eSize, can be used to set the video output resolution BEFORE Omegonprocam_StartXXXX.
    put_Size use width and height parameters, put_eSize use the index parameter.
    for example, UCMOS03100KPA support the following resolutions:
            index 0:    2048,   1536
            index 1:    1024,   768
            index 2:    680,    510
    so, we can use put_Size(h, 1024, 768) or put_eSize(h, 1). Both have the same effect.
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Size(HOmegonprocam h, int nWidth, int nHeight);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Size(HOmegonprocam h, int* pWidth, int* pHeight);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_eSize(HOmegonprocam h, unsigned nResolutionIndex);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_eSize(HOmegonprocam h, unsigned* pnResolutionIndex);

/*
    final image size after ROI, rotate, binning
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_FinalSize(HOmegonprocam h, int* pWidth, int* pHeight);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_ResolutionNumber(HOmegonprocam h);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Resolution(HOmegonprocam h, unsigned nResolutionIndex, int* pWidth, int* pHeight);
/*
    numerator/denominator, such as: 1/1, 1/2, 1/3
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_ResolutionRatio(HOmegonprocam h, unsigned nResolutionIndex, int* pNumerator, int* pDenominator);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Field(HOmegonprocam h);

/*
see: http://www.siliconimaging.com/RGB%20Bayer.htm
FourCC:
    MAKEFOURCC('G', 'B', 'R', 'G')
    MAKEFOURCC('R', 'G', 'G', 'B')
    MAKEFOURCC('B', 'G', 'G', 'R')
    MAKEFOURCC('G', 'R', 'B', 'G')
    MAKEFOURCC('Y', 'Y', 'Y', 'Y'), monochromatic sensor
    MAKEFOURCC('Y', '4', '1', '1'), yuv411
    MAKEFOURCC('V', 'U', 'Y', 'Y'), yuv422
    MAKEFOURCC('U', 'Y', 'V', 'Y'), yuv422
    MAKEFOURCC('Y', '4', '4', '4'), yuv444
    MAKEFOURCC('R', 'G', 'B', '8'), RGB888

#ifndef MAKEFOURCC
#define MAKEFOURCC(a, b, c, d) ((unsigned)(unsigned char)(a) | ((unsigned)(unsigned char)(b) << 8) | ((unsigned)(unsigned char)(c) << 16) | ((unsigned)(unsigned char)(d) << 24))
#endif
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_RawFormat(HOmegonprocam h, unsigned* pFourCC, unsigned* pBitsPerPixel);

/*
    ------------------------------------------------------------------|
    | Parameter               |   Range       |   Default             |
    |-----------------------------------------------------------------|
    | Auto Exposure Target    |   10~220      |   120                 |
    | Exposure Gain           |   100~        |   100                 |
    | Temp                    |   1000~25000  |   6503                |
    | Tint                    |   100~2500    |   1000                |
    | LevelRange              |   0~255       |   Low = 0, High = 255 |
    | Contrast                |   -255~255    |   0                   |
    | Hue                     |   -180~180    |   0                   |
    | Saturation              |   0~255       |   128                 |
    | Brightness              |   -255~255    |   0                   |
    | Gamma                   |   20~180      |   100                 |
    | WBGain                  |   -127~127    |   0                   |
    ------------------------------------------------------------------|
*/

#ifndef __OMEGONPROCAM_CALLBACK_DEFINED__
#define __OMEGONPROCAM_CALLBACK_DEFINED__
typedef void (__stdcall* PIOMEGONPROCAM_EXPOSURE_CALLBACK)(void* ctxExpo);                                 /* auto exposure */
typedef void (__stdcall* PIOMEGONPROCAM_WHITEBALANCE_CALLBACK)(const int aGain[3], void* ctxWB);           /* once white balance, RGB Gain mode */
typedef void (__stdcall* PIOMEGONPROCAM_BLACKBALANCE_CALLBACK)(const unsigned short aSub[3], void* ctxBB); /* once black balance */
typedef void (__stdcall* PIOMEGONPROCAM_TEMPTINT_CALLBACK)(const int nTemp, const int nTint, void* ctxTT); /* once white balance, Temp/Tint Mode */
typedef void (__stdcall* PIOMEGONPROCAM_HISTOGRAM_CALLBACK)(const float aHistY[256], const float aHistR[256], const float aHistG[256], const float aHistB[256], void* ctxHistogram);
typedef void (__stdcall* PIOMEGONPROCAM_CHROME_CALLBACK)(void* ctxChrome);
typedef void (__stdcall* PIOMEGONPROCAM_PROGRESS)(int percent, void* ctxProgress);
#endif

/*
* nFlag & 0x00008000: mono or color
* nFlag & 0x0f: bitdepth
* so the size of aHist is:
    int arraySize = 1 << (nFlag & 0x0f);
    if ((nFlag & 0x00008000) == 0)
        arraySize *= 3;
*/
typedef void (__stdcall* PIOMEGONPROCAM_HISTOGRAM_CALLBACKV2)(const unsigned* aHist, unsigned nFlag, void* ctxHistogramV2);

/*
* bAutoExposure:
*   0: disable auto exposure
*   1: auto exposure continue mode
*   2: auto exposure once mode
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_AutoExpoEnable(HOmegonprocam h, int* bAutoExposure);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_AutoExpoEnable(HOmegonprocam h, int bAutoExposure);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_AutoExpoTarget(HOmegonprocam h, unsigned short* Target);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_AutoExpoTarget(HOmegonprocam h, unsigned short Target);

/* set the maximum/minimal auto exposure time and agin. The default maximum auto exposure time is 350ms */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_AutoExpoRange(HOmegonprocam h, unsigned maxTime, unsigned minTime, unsigned short maxGain, unsigned short minGain);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_AutoExpoRange(HOmegonprocam h, unsigned* maxTime, unsigned* minTime, unsigned short* maxGain, unsigned short* minGain);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_MaxAutoExpoTimeAGain(HOmegonprocam h, unsigned maxTime, unsigned short maxGain);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_MaxAutoExpoTimeAGain(HOmegonprocam h, unsigned* maxTime, unsigned short* maxGain);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_MinAutoExpoTimeAGain(HOmegonprocam h, unsigned minTime, unsigned short minGain);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_MinAutoExpoTimeAGain(HOmegonprocam h, unsigned* minTime, unsigned short* minGain);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_ExpoTime(HOmegonprocam h, unsigned* Time); /* in microseconds */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_ExpoTime(HOmegonprocam h, unsigned Time); /* in microseconds */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_RealExpoTime(HOmegonprocam h, unsigned* Time); /* actual exposure time */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_ExpTimeRange(HOmegonprocam h, unsigned* nMin, unsigned* nMax, unsigned* nDef);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_ExpoAGain(HOmegonprocam h, unsigned short* Gain); /* percent, such as 300 */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_ExpoAGain(HOmegonprocam h, unsigned short Gain); /* percent */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_ExpoAGainRange(HOmegonprocam h, unsigned short* nMin, unsigned short* nMax, unsigned short* nDef);

/* Auto White Balance "Once", Temp/Tint Mode */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_AwbOnce(HOmegonprocam h, PIOMEGONPROCAM_TEMPTINT_CALLBACK funTT, void* ctxTT); /* auto white balance "once". This function must be called AFTER Omegonprocam_StartXXXX */

/* Auto White Balance "Once", RGB Gain Mode */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_AwbInit(HOmegonprocam h, PIOMEGONPROCAM_WHITEBALANCE_CALLBACK funWB, void* ctxWB);

/* White Balance, Temp/Tint mode */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_TempTint(HOmegonprocam h, int nTemp, int nTint);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_TempTint(HOmegonprocam h, int* nTemp, int* nTint);

/* White Balance, RGB Gain mode */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_WhiteBalanceGain(HOmegonprocam h, int aGain[3]);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_WhiteBalanceGain(HOmegonprocam h, int aGain[3]);

/* Black Balance */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_AbbOnce(HOmegonprocam h, PIOMEGONPROCAM_BLACKBALANCE_CALLBACK funBB, void* ctxBB); /* auto black balance "once". This function must be called AFTER Omegonprocam_StartXXXX */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_BlackBalance(HOmegonprocam h, unsigned short aSub[3]);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_BlackBalance(HOmegonprocam h, unsigned short aSub[3]);

/* Flat Field Correction */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FfcOnce(HOmegonprocam h);
#if defined(_WIN32)
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FfcExport(HOmegonprocam h, const wchar_t* filePath);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FfcImport(HOmegonprocam h, const wchar_t* filePath);
#else
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FfcExport(HOmegonprocam h, const char* filePath);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FfcImport(HOmegonprocam h, const char* filePath);
#endif
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FfcFile(const void* data[], unsigned num, unsigned width, unsigned height, unsigned fourcc, unsigned bits, const char* outfile);

/* Dark Field Correction */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_DfcOnce(HOmegonprocam h);

#if defined(_WIN32)
OMEGONPROCAM_API(HRESULT)  Omegonprocam_DfcExport(HOmegonprocam h, const wchar_t* filePath);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_DfcImport(HOmegonprocam h, const wchar_t* filePath);
#else
OMEGONPROCAM_API(HRESULT)  Omegonprocam_DfcExport(HOmegonprocam h, const char* filePath);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_DfcImport(HOmegonprocam h, const char* filePath);
#endif

/* Fix Pattern Noise Correction */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FpncOnce(HOmegonprocam h);

#if defined(_WIN32)
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FpncExport(HOmegonprocam h, const wchar_t* filePath);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FpncImport(HOmegonprocam h, const wchar_t* filePath);
#else
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FpncExport(HOmegonprocam h, const char* filePath);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FpncImport(HOmegonprocam h, const char* filePath);
#endif

OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Hue(HOmegonprocam h, int Hue);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Hue(HOmegonprocam h, int* Hue);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Saturation(HOmegonprocam h, int Saturation);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Saturation(HOmegonprocam h, int* Saturation);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Brightness(HOmegonprocam h, int Brightness);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Brightness(HOmegonprocam h, int* Brightness);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Contrast(HOmegonprocam h, int* Contrast);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Contrast(HOmegonprocam h, int Contrast);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Gamma(HOmegonprocam h, int* Gamma); /* percent */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Gamma(HOmegonprocam h, int Gamma);  /* percent */

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Chrome(HOmegonprocam h, int* bChrome);  /* 1 => monochromatic mode, 0 => color mode */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Chrome(HOmegonprocam h, int bChrome);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_VFlip(HOmegonprocam h, int* bVFlip);  /* vertical flip */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_VFlip(HOmegonprocam h, int bVFlip);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_HFlip(HOmegonprocam h, int* bHFlip);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_HFlip(HOmegonprocam h, int bHFlip); /* horizontal flip */

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Negative(HOmegonprocam h, int* bNegative);  /* negative film */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Negative(HOmegonprocam h, int bNegative);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Speed(HOmegonprocam h, unsigned short nSpeed);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Speed(HOmegonprocam h, unsigned short* pSpeed);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_MaxSpeed(HOmegonprocam h); /* get the maximum speed, see "Frame Speed Level", the speed range = [0, max], closed interval */

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_FanMaxSpeed(HOmegonprocam h); /* get the maximum fan speed, the fan speed range = [0, max], closed interval */

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_MaxBitDepth(HOmegonprocam h); /* get the max bitdepth of this camera, such as 8, 10, 12, 14, 16 */

/* power supply of lighting:
        0 => 60HZ AC
        1 => 50Hz AC
        2 => DC
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_HZ(HOmegonprocam h, int nHZ);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_HZ(HOmegonprocam h, int* nHZ);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Mode(HOmegonprocam h, int bSkip); /* skip or bin */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Mode(HOmegonprocam h, int* bSkip); /* If the model don't support bin/skip mode, return E_NOTIMPL */

#if !defined(_WIN32)
#ifndef __RECT_DEFINED__
#define __RECT_DEFINED__
typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} RECT, *PRECT;
#endif
#endif

OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_AWBAuxRect(HOmegonprocam h, const RECT* pAuxRect); /* auto white balance ROI */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_AWBAuxRect(HOmegonprocam h, RECT* pAuxRect);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_AEAuxRect(HOmegonprocam h, const RECT* pAuxRect);  /* auto exposure ROI */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_AEAuxRect(HOmegonprocam h, RECT* pAuxRect);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_ABBAuxRect(HOmegonprocam h, const RECT* pAuxRect); /* auto black balance ROI */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_ABBAuxRect(HOmegonprocam h, RECT* pAuxRect);

/*
    S_FALSE:    color mode
    S_OK:       mono mode, such as EXCCD00300KMA and UHCCD01400KMA
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_MonoMode(HOmegonprocam h);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_StillResolutionNumber(HOmegonprocam h);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_StillResolution(HOmegonprocam h, unsigned nResolutionIndex, int* pWidth, int* pHeight);

/*  0: no realtime
          stop grab frame when frame buffer deque is full, until the frames in the queue are pulled away and the queue is not full
    1: realtime
          use minimum frame buffer. When new frame arrive, drop all the pending frame regardless of whether the frame buffer is full.
          If DDR present, also limit the DDR frame buffer to only one frame.
    2: soft realtime
          Drop the oldest frame when the queue is full and then enqueue the new frame
    default: 0
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_RealTime(HOmegonprocam h, int val);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_RealTime(HOmegonprocam h, int* val);

/* discard the current internal frame cache.
    If DDR present, also discard the frames in the DDR.
    Omegonprocam_Flush is obsolete, recommend using Omegonprocam_put_Option(h, OMEGONPROCAM_OPTION_FLUSH, 3)
*/
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_Flush(HOmegonprocam h);

/* get the temperature of the sensor, in 0.1 degrees Celsius (32 means 3.2 degrees Celsius, -35 means -3.5 degree Celsius)
    return E_NOTIMPL if not supported
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Temperature(HOmegonprocam h, short* pTemperature);

/* set the target temperature of the sensor or TEC, in 0.1 degrees Celsius (32 means 3.2 degrees Celsius, -35 means -3.5 degree Celsius)
    set "-2730" or below means using the default value of this model
    return E_NOTIMPL if not supported
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Temperature(HOmegonprocam h, short nTemperature);

/*
    get the revision
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Revision(HOmegonprocam h, unsigned short* pRevision);

/*
    get the serial number which is always 32 chars which is zero-terminated such as "TP110826145730ABCD1234FEDC56787"
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_SerialNumber(HOmegonprocam h, char sn[32]);

/*
    get the camera firmware version, such as: 3.2.1.20140922
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_FwVersion(HOmegonprocam h, char fwver[16]);

/*
    get the camera hardware version, such as: 3.12
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_HwVersion(HOmegonprocam h, char hwver[16]);

/*
    get the production date, such as: 20150327, YYYYMMDD, (YYYY: year, MM: month, DD: day)
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_ProductionDate(HOmegonprocam h, char pdate[10]);

/*
    get the FPGA version, such as: 1.13
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_FpgaVersion(HOmegonprocam h, char fpgaver[16]);

/*
    get the sensor pixel size, such as: 2.4um x 2.4um
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_PixelSize(HOmegonprocam h, unsigned nResolutionIndex, float* x, float* y);

/* software level range */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_LevelRange(HOmegonprocam h, unsigned short aLow[4], unsigned short aHigh[4]);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_LevelRange(HOmegonprocam h, unsigned short aLow[4], unsigned short aHigh[4]);

/* hardware level range mode */
#define OMEGONPROCAM_LEVELRANGE_MANUAL       0x0000  /* manual */
#define OMEGONPROCAM_LEVELRANGE_ONCE         0x0001  /* once */
#define OMEGONPROCAM_LEVELRANGE_CONTINUE     0x0002  /* continue */
#define OMEGONPROCAM_LEVELRANGE_ROI          0xffff  /* update roi rect only */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_LevelRangeV2(HOmegonprocam h, unsigned short mode, const RECT* pRoiRect, unsigned short aLow[4], unsigned short aHigh[4]);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_LevelRangeV2(HOmegonprocam h, unsigned short* pMode, RECT* pRoiRect, unsigned short aLow[4], unsigned short aHigh[4]);

/*
    The following functions must be called AFTER Omegonprocam_StartPushMode or Omegonprocam_StartPullModeWithWndMsg or Omegonprocam_StartPullModeWithCallback
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_LevelRangeAuto(HOmegonprocam h);  /* software level range */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_GetHistogram(HOmegonprocam h, PIOMEGONPROCAM_HISTOGRAM_CALLBACK funHistogram, void* ctxHistogram);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_GetHistogramV2(HOmegonprocam h, PIOMEGONPROCAM_HISTOGRAM_CALLBACKV2 funHistogramV2, void* ctxHistogramV2);

/* led state:
    iLed: Led index, (0, 1, 2, ...)
    iState: 1 => Ever bright; 2 => Flashing; other => Off
    iPeriod: Flashing Period (>= 500ms)
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_LEDState(HOmegonprocam h, unsigned short iLed, unsigned short iState, unsigned short iPeriod);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_write_EEPROM(HOmegonprocam h, unsigned addr, const unsigned char* pBuffer, unsigned nBufferLen);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_read_EEPROM(HOmegonprocam h, unsigned addr, unsigned char* pBuffer, unsigned nBufferLen);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_read_Pipe(HOmegonprocam h, unsigned pipeId, void* pBuffer, unsigned nBufferLen);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_write_Pipe(HOmegonprocam h, unsigned pipeId, const void* pBuffer, unsigned nBufferLen);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_feed_Pipe(HOmegonprocam h, unsigned pipeId);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Option(HOmegonprocam h, unsigned iOption, int iValue);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Option(HOmegonprocam h, unsigned iOption, int* piValue);

/* [RW] = Read/Write; [RO] = Read Only; [WO] = Write Only */
#define OMEGONPROCAM_OPTION_NOFRAME_TIMEOUT        0x01       /* [RW] no frame timeout: 0 => disable, positive value (>= OMEGONPROCAM_NOFRAME_TIMEOUT_MIN) => timeout milliseconds. default: disable */
#define OMEGONPROCAM_OPTION_THREAD_PRIORITY        0x02       /* [RW] set the priority of the internal thread which grab data from the usb device.
                                                             Win: iValue: 0 => THREAD_PRIORITY_NORMAL; 1 => THREAD_PRIORITY_ABOVE_NORMAL; 2 => THREAD_PRIORITY_HIGHEST; 3 => THREAD_PRIORITY_TIME_CRITICAL; default: 1; see: https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadpriority
                                                             Linux & macOS: The high 16 bits for the scheduling policy, and the low 16 bits for the priority; see: https://linux.die.net/man/3/pthread_setschedparam
                                                         */
#define OMEGONPROCAM_OPTION_PROCESSMODE            0x03       /* [RW] obsolete & useless, noop. 0 = better image quality, more cpu usage. this is the default value; 1 = lower image quality, less cpu usage */
#define OMEGONPROCAM_OPTION_RAW                    0x04       /* [RW]
                                                            0: RGB mode
                                                            1: RAW mode, read the CMOS or CCD raw data
                                                            -1: RAW mode, the difference from 1 is the execution of FFC, DFC, FPNC, black balance, and white balance
                                                            default value: 0
                                                         */
#define OMEGONPROCAM_OPTION_HISTOGRAM              0x05       /* [RW] 0 = only one, 1 = continue mode */
#define OMEGONPROCAM_OPTION_BITDEPTH               0x06       /* [RW] 0 = 8 bits mode, 1 = 16 bits mode, subset of OMEGONPROCAM_OPTION_PIXEL_FORMAT */
#define OMEGONPROCAM_OPTION_FAN                    0x07       /* [RW] 0 = turn off the cooling fan, [1, max] = fan speed, , set to "-1" means to use default fan speed */
#define OMEGONPROCAM_OPTION_TEC                    0x08       /* [RW] 0 = turn off the thermoelectric cooler, 1 = turn on the thermoelectric cooler */
#define OMEGONPROCAM_OPTION_LINEAR                 0x09       /* [RW] 0 = turn off the builtin linear tone mapping, 1 = turn on the builtin linear tone mapping, default value: 1 */
#define OMEGONPROCAM_OPTION_CURVE                  0x0a       /* [RW] 0 = turn off the builtin curve tone mapping, 1 = turn on the builtin polynomial curve tone mapping, 2 = logarithmic curve tone mapping, default value: 2 */
#define OMEGONPROCAM_OPTION_TRIGGER                0x0b       /* [RW] 0 = video mode, 1 = software or simulated trigger mode, 2 = external trigger mode, 3 = external + software trigger, 4 = self trigger, default value = 0 */
#define OMEGONPROCAM_OPTION_RGB                    0x0c       /* [RW] 0 => RGB24; 1 => enable RGB48 format when bitdepth > 8; 2 => RGB32; 3 => 8 Bits Grey (only for mono camera); 4 => 16 Bits Grey (only for mono camera when bitdepth > 8); 5 => 64(RGB64) */
#define OMEGONPROCAM_OPTION_COLORMATIX             0x0d       /* [RW] enable or disable the builtin color matrix, default value: 1 */
#define OMEGONPROCAM_OPTION_WBGAIN                 0x0e       /* [RW] enable or disable the builtin white balance gain, default value: 1 */
#define OMEGONPROCAM_OPTION_TECTARGET              0x0f       /* [RW] get or set the target temperature of the thermoelectric cooler, in 0.1 degree Celsius. For example, 125 means 12.5 degree Celsius, -35 means -3.5 degree Celsius. Set "-2730" or below means using the default for that model */
#define OMEGONPROCAM_OPTION_AUTOEXP_POLICY         0x10       /* [RW] auto exposure policy:
                                                             0: Exposure Only
                                                             1: Exposure Preferred
                                                             2: Gain Only
                                                             3: Gain Preferred
                                                             default value: 1
                                                         */
#define OMEGONPROCAM_OPTION_FRAMERATE              0x11       /* [RW] limit the frame rate, the default value 0 means no limit */
#define OMEGONPROCAM_OPTION_DEMOSAIC               0x12       /* [RW] demosaic method for both video and still image: BILINEAR = 0, VNG(Variable Number of Gradients) = 1, PPG(Patterned Pixel Grouping) = 2, AHD(Adaptive Homogeneity Directed) = 3, EA(Edge Aware) = 4, see https://en.wikipedia.org/wiki/Demosaicing
                                                              In terms of CPU usage, EA is the lowest, followed by BILINEAR, and the others are higher.
                                                              default value: 0
                                                         */
#define OMEGONPROCAM_OPTION_DEMOSAIC_VIDEO         0x13       /* [RW] demosaic method for video */
#define OMEGONPROCAM_OPTION_DEMOSAIC_STILL         0x14       /* [RW] demosaic method for still image */
#define OMEGONPROCAM_OPTION_BLACKLEVEL             0x15       /* [RW] black level */
#define OMEGONPROCAM_OPTION_MULTITHREAD            0x16       /* [RW] multithread image processing */
#define OMEGONPROCAM_OPTION_BINNING                0x17       /* [RW] binning
                                                                0x01: (no binning)
                                                                n: (saturating add, n*n), 0x02(2*2), 0x03(3*3), 0x04(4*4), 0x05(5*5), 0x06(6*6), 0x07(7*7), 0x08(8*8). The Bitdepth of the data remains unchanged.
                                                                0x40 | n: (unsaturated add, n*n, works only in RAW mode), 0x42(2*2), 0x43(3*3), 0x44(4*4), 0x45(5*5), 0x46(6*6), 0x47(7*7), 0x48(8*8). The Bitdepth of the data is increased. For example, the original data with bitdepth of 12 will increase the bitdepth by 2 bits and become 14 after 2*2 binning.
                                                                0x80 | n: (average, n*n), 0x82(2*2), 0x83(3*3), 0x84(4*4), 0x85(5*5), 0x86(6*6), 0x87(7*7), 0x88(8*8). The Bitdepth of the data remains unchanged.
                                                            The final image size is rounded down to an even number, such as 640/3 to get 212
                                                         */
#define OMEGONPROCAM_OPTION_ROTATE                 0x18       /* [RW] rotate clockwise: 0, 90, 180, 270 */
#define OMEGONPROCAM_OPTION_CG                     0x19       /* [RW] Conversion Gain:
                                                                0 = LCG
                                                                1 = HCG
                                                                2 = HDR (for camera with flag OMEGONPROCAM_FLAG_CGHDR)
                                                                2 = MCG (for camera with flag OMEGONPROCAM_FLAG_GHOPTO)
                                                         */
#define OMEGONPROCAM_OPTION_PIXEL_FORMAT           0x1a       /* [RW] pixel format, OMEGONPROCAM_PIXELFORMAT_xxxx */
#define OMEGONPROCAM_OPTION_FFC                    0x1b       /* [RW] flat field correction
                                                             set:
                                                                  0: disable
                                                                  1: enable
                                                                 -1: reset
                                                                 (0xff000000 | n): set the average number to n, [1~255]
                                                             get:
                                                                  (val & 0xff): 0 => disable, 1 => enable, 2 => inited
                                                                  ((val & 0xff00) >> 8): sequence
                                                                  ((val & 0xff0000) >> 16): average number
                                                         */
#define OMEGONPROCAM_OPTION_DDR_DEPTH              0x1c       /* [RW] the number of the frames that DDR can cache
                                                                 1: DDR cache only one frame
                                                                 0: Auto:
                                                                         => one for video mode when auto exposure is enabled
                                                                         => full capacity for others
                                                                -1: DDR can cache frames to full capacity
                                                         */
#define OMEGONPROCAM_OPTION_DFC                    0x1d       /* [RW] dark field correction
                                                             set:
                                                                 0: disable
                                                                 1: enable
                                                                -1: reset
                                                                 (0xff000000 | n): set the average number to n, [1~255]
                                                             get:
                                                                 (val & 0xff): 0 => disable, 1 => enable, 2 => inited
                                                                 ((val & 0xff00) >> 8): sequence
                                                                 ((val & 0xff0000) >> 16): average number
                                                         */
#define OMEGONPROCAM_OPTION_SHARPENING             0x1e       /* [RW] Sharpening: (threshold << 24) | (radius << 16) | strength)
                                                             strength: [0, 500], default: 0 (disable)
                                                             radius: [1, 10]
                                                             threshold: [0, 255]
                                                         */
#define OMEGONPROCAM_OPTION_FACTORY                0x1f       /* [WO] restore the factory settings */
#define OMEGONPROCAM_OPTION_TEC_VOLTAGE            0x20       /* [RO] get the current TEC voltage in 0.1V, 59 mean 5.9V; readonly */
#define OMEGONPROCAM_OPTION_TEC_VOLTAGE_MAX        0x21       /* [RW] TEC maximum voltage in 0.1V */
#define OMEGONPROCAM_OPTION_DEVICE_RESET           0x22       /* [WO] reset usb device, simulate a replug */
#define OMEGONPROCAM_OPTION_UPSIDE_DOWN            0x23       /* [RW] upsize down:
                                                             1: yes
                                                             0: no
                                                             default: 1 (win), 0 (linux/macos)
                                                         */
#define OMEGONPROCAM_OPTION_FOCUSPOS               0x24       /* [RW] focus positon */
#define OMEGONPROCAM_OPTION_AFMODE                 0x25       /* [RW] auto focus mode, see OmegonprocamAFMode */
#define OMEGONPROCAM_OPTION_AFSTATUS               0x27       /* [RO] auto focus status, see OmegonprocamAFStaus */
#define OMEGONPROCAM_OPTION_TESTPATTERN            0x28       /* [RW] test pattern:
                                                            0: off
                                                            3: monochrome diagonal stripes
                                                            5: monochrome vertical stripes
                                                            7: monochrome horizontal stripes
                                                            9: chromatic diagonal stripes
                                                         */
#define OMEGONPROCAM_OPTION_AUTOEXP_THRESHOLD      0x29       /* [RW] threshold of auto exposure, default value: 5, range = [2, 15] */
#define OMEGONPROCAM_OPTION_BYTEORDER              0x2a       /* [RW] Byte order, BGR or RGB: 0 => RGB, 1 => BGR, default value: 1(Win), 0(macOS, Linux, Android) */
#define OMEGONPROCAM_OPTION_NOPACKET_TIMEOUT       0x2b       /* [RW] no packet timeout: 0 => disable, positive value (>= OMEGONPROCAM_NOPACKET_TIMEOUT_MIN) => timeout milliseconds. default: disable */
#define OMEGONPROCAM_OPTION_MAX_PRECISE_FRAMERATE  0x2c       /* [RO] get the precise frame rate maximum value in 0.1 fps, such as 115 means 11.5 fps */
#define OMEGONPROCAM_OPTION_PRECISE_FRAMERATE      0x2d       /* [RW] precise frame rate current value in 0.1 fps. use OMEGONPROCAM_OPTION_MAX_PRECISE_FRAMERATE, OMEGONPROCAM_OPTION_MIN_PRECISE_FRAMERATE to get the range. if the set value is out of range, E_INVALIDARG will be returned */
#define OMEGONPROCAM_OPTION_BANDWIDTH              0x2e       /* [RW] bandwidth, [1-100]% */
#define OMEGONPROCAM_OPTION_RELOAD                 0x2f       /* [RW] reload the last frame in trigger mode */
#define OMEGONPROCAM_OPTION_CALLBACK_THREAD        0x30       /* [RW] dedicated thread for callback: 0 => disable, 1 => enable, default: 0 */
#define OMEGONPROCAM_OPTION_FRONTEND_DEQUE_LENGTH  0x31       /* [RW] frontend (raw) frame buffer deque length, range: [2, 1024], default: 4
                                                            All the memory will be pre-allocated when the camera starts, so, please attention to memory usage
                                                         */
#define OMEGONPROCAM_OPTION_FRAME_DEQUE_LENGTH     0x31       /* [RW] alias of OMEGONPROCAM_OPTION_FRONTEND_DEQUE_LENGTH */
#define OMEGONPROCAM_OPTION_MIN_PRECISE_FRAMERATE  0x32       /* [RO] get the precise frame rate minimum value in 0.1 fps, such as 15 means 1.5 fps */
#define OMEGONPROCAM_OPTION_SEQUENCER_ONOFF        0x33       /* [RW] sequencer trigger: on/off */
#define OMEGONPROCAM_OPTION_SEQUENCER_NUMBER       0x34       /* [RW] sequencer trigger: number, range = [1, 255] */
#define OMEGONPROCAM_OPTION_SEQUENCER_EXPOTIME     0x01000000 /* [RW] sequencer trigger: exposure time, iOption = OMEGONPROCAM_OPTION_SEQUENCER_EXPOTIME | index, iValue = exposure time
                                                             For example, to set the exposure time of the third group to 50ms, call:
                                                                Omegonprocam_put_Option(OMEGONPROCAM_OPTION_SEQUENCER_EXPOTIME | 3, 50000)
                                                         */
#define OMEGONPROCAM_OPTION_SEQUENCER_EXPOGAIN     0x02000000 /* [RW] sequencer trigger: exposure gain, iOption = OMEGONPROCAM_OPTION_SEQUENCER_EXPOGAIN | index, iValue = gain */
#define OMEGONPROCAM_OPTION_DENOISE                0x35       /* [RW] denoise, strength range: [0, 100], 0 means disable */
#define OMEGONPROCAM_OPTION_HEAT_MAX               0x36       /* [RO] get maximum level: heat to prevent fogging up */
#define OMEGONPROCAM_OPTION_HEAT                   0x37       /* [RW] heat to prevent fogging up */
#define OMEGONPROCAM_OPTION_LOW_NOISE              0x38       /* [RW] low noise mode (Higher signal noise ratio, lower frame rate): 1 => enable */
#define OMEGONPROCAM_OPTION_POWER                  0x39       /* [RO] get power consumption, unit: milliwatt */
#define OMEGONPROCAM_OPTION_GLOBAL_RESET_MODE      0x3a       /* [RW] global reset mode */
#define OMEGONPROCAM_OPTION_OPEN_ERRORCODE         0x3b       /* [RO] get the open camera error code */
#define OMEGONPROCAM_OPTION_FLUSH                  0x3d       /* [WO]
                                                            1 = hard flush, discard frames cached by camera DDR (if any)
                                                            2 = soft flush, discard frames cached by omegonprocam.dll (if any)
                                                            3 = both flush
                                                            Omegonprocam_Flush means 'both flush'
                                                            return the number of soft flushed frames if successful, HRESULT if failed
                                                         */
#define OMEGONPROCAM_OPTION_NUMBER_DROP_FRAME      0x3e       /* [RO] get the number of frames that have been grabbed from the USB but dropped by the software */
#define OMEGONPROCAM_OPTION_DUMP_CFG               0x3f       /* [RW] 0 = when camera is stopped, do not dump configuration automatically
                                                            1 = when camera is stopped, dump configuration automatically
                                                            -1 = explicitly dump configuration once
                                                            default: 1
                                                         */
#define OMEGONPROCAM_OPTION_DEFECT_PIXEL           0x40       /* [RW] Defect Pixel Correction: 0 => disable, 1 => enable; default: 1 */
#define OMEGONPROCAM_OPTION_BACKEND_DEQUE_LENGTH   0x41       /* [RW] backend (pipelined) frame buffer deque length (Only available in pull mode), range: [2, 1024], default: 3
                                                            All the memory will be pre-allocated when the camera starts, so, please attention to memory usage
                                                         */
#define OMEGONPROCAM_OPTION_LIGHTSOURCE_MAX        0x42       /* [RO] get the light source range, [0 ~ max] */
#define OMEGONPROCAM_OPTION_LIGHTSOURCE            0x43       /* [RW] light source */
#define OMEGONPROCAM_OPTION_HEARTBEAT              0x44       /* [RW] Heartbeat interval in millisecond, range = [OMEGONPROCAM_HEARTBEAT_MIN, OMEGONPROCAM_HEARTBEAT_MAX], 0 = disable, default: disable */
#define OMEGONPROCAM_OPTION_FRONTEND_DEQUE_CURRENT 0x45       /* [RO] get the current number in frontend deque */
#define OMEGONPROCAM_OPTION_BACKEND_DEQUE_CURRENT  0x46       /* [RO] get the current number in backend deque */
#define OMEGONPROCAM_OPTION_EVENT_HARDWARE         0x04000000 /* [RW] enable or disable hardware event: 0 => disable, 1 => enable; default: disable
                                                                (1) iOption = OMEGONPROCAM_OPTION_EVENT_HARDWARE, master switch for notification of all hardware events
                                                                (2) iOption = OMEGONPROCAM_OPTION_EVENT_HARDWARE | (event type), a specific type of sub-switch
                                                            Only if both the master switch and the sub-switch of a particular type remain on are actually enabled for that type of event notification.
                                                         */
#define OMEGONPROCAM_OPTION_PACKET_NUMBER          0x47       /* [RO] get the received packet number */
#define OMEGONPROCAM_OPTION_FILTERWHEEL_SLOT       0x48       /* [RW] filter wheel slot number */
#define OMEGONPROCAM_OPTION_FILTERWHEEL_POSITION   0x49       /* [RW] filter wheel position:
                                                                set:
                                                                    -1: reset
                                                                    val & 0xff: position between 0 and N-1, where N is the number of filter slots
                                                                    (val >> 8) & 0x1: direction, 0 => clockwise spinning, 1 => auto direction spinning
                                                                get:
                                                                     -1: in motion
                                                                     val: position arrived
                                                         */
#define OMEGONPROCAM_OPTION_AUTOEXPOSURE_PERCENT   0x4a       /* [RW] auto exposure percent to average:
                                                                1~99: peak percent average
                                                                0 or 100: full roi average, means "disabled"
                                                         */
#define OMEGONPROCAM_OPTION_ANTI_SHUTTER_EFFECT    0x4b       /* [RW] anti shutter effect: 1 => enable, 0 => disable; default: 0 */
#define OMEGONPROCAM_OPTION_CHAMBER_HT             0x4c       /* [RO] get chamber humidity & temperature:
                                                                high 16 bits: humidity, in 0.1%, such as: 325 means humidity is 32.5%
                                                                low 16 bits: temperature, in 0.1 degrees Celsius, such as: 32 means 3.2 degrees Celsius
                                                         */
#define OMEGONPROCAM_OPTION_ENV_HT                 0x4d       /* [RO] get environment humidity & temperature */
#define OMEGONPROCAM_OPTION_EXPOSURE_PRE_DELAY     0x4e       /* [RW] exposure signal pre-delay, microsecond */
#define OMEGONPROCAM_OPTION_EXPOSURE_POST_DELAY    0x4f       /* [RW] exposure signal post-delay, microsecond */
#define OMEGONPROCAM_OPTION_AUTOEXPO_CONV          0x50       /* [RO] get auto exposure convergence status: 1(YES) or 0(NO), -1(PENDING) */
#define OMEGONPROCAM_OPTION_AUTOEXPO_TRIGGER       0x51       /* [RW] auto exposure on trigger mode: 0 => disable, 1 => enable; default: 0 */
#define OMEGONPROCAM_OPTION_LINE_PRE_DELAY         0x52       /* [RW] specified line signal pre-delay, microsecond */
#define OMEGONPROCAM_OPTION_LINE_POST_DELAY        0x53       /* [RW] specified line signal post-delay, microsecond */
#define OMEGONPROCAM_OPTION_TEC_VOLTAGE_MAX_RANGE  0x54       /* [RO] get the tec maximum voltage range:
                                                                high 16 bits: max
                                                                low 16 bits: min
                                                         */
#define OMEGONPROCAM_OPTION_HIGH_FULLWELL          0x55       /* [RW] high fullwell capacity: 0 => disable, 1 => enable */
#define OMEGONPROCAM_OPTION_DYNAMIC_DEFECT         0x56       /* [RW] dynamic defect pixel correction:
                                                                dead pixel ratio, t1: (high 16 bits): [0, 100], means: [0.0, 1.0]
                                                                hot pixel ratio, t2: (low 16 bits): [0, 100], means: [0.0, 1.0]
                                                         */
#define OMEGONPROCAM_OPTION_HDR_KB                 0x57       /* [RW] HDR synthesize
                                                                K (high 16 bits): [1, 25500]
                                                                B (low 16 bits): [0, 65535]
                                                                0xffffffff => set to default
                                                         */
#define OMEGONPROCAM_OPTION_HDR_THRESHOLD          0x58       /* [RW] HDR synthesize
                                                                threshold: [1, 4094]
                                                                0xffffffff => set to default
                                                         */
#define OMEGONPROCAM_OPTION_GIGETIMEOUT            0x5a       /* [RW] For GigE cameras, the application periodically sends heartbeat signals to the camera to keep the connection to the camera alive.
                                                            If the camera doesn't receive heartbeat signals within the time period specified by the heartbeat timeout counter, the camera resets the connection.
                                                            When the application is stopped by the debugger, the application cannot send the heartbeat signals
                                                                0 => auto: when the camera is opened, enable if no debugger is present or disable if debugger is present
                                                                1 => enable
                                                                2 => disable
                                                                default: auto
                                                         */
#define OMEGONPROCAM_OPTION_EEPROM_SIZE            0x5b       /* [RO] get EEPROM size */
#define OMEGONPROCAM_OPTION_OVERCLOCK_MAX          0x5c       /* [RO] get overclock range: [0, max] */
#define OMEGONPROCAM_OPTION_OVERCLOCK              0x5d       /* [RW] overclock, default: 0 */
#define OMEGONPROCAM_OPTION_RESET_SENSOR           0x5e       /* [WO] reset sensor */
#define OMEGONPROCAM_OPTION_ISP                    0x5f       /* [RW] Enable hardware ISP: 0 => auto (disable in RAW mode, otherwise enable), 1 => enable, -1 => disable; default: 0 */
#define OMEGONPROCAM_OPTION_AUTOEXP_EXPOTIME_DAMP  0x60       /* [RW] Auto exposure damping coefficient: time (thousandths). The larger the damping coefficient, the smoother and slower the exposure time changes */
#define OMEGONPROCAM_OPTION_AUTOEXP_GAIN_DAMP      0x61       /* [RW] Auto exposure damping coefficient: gain (thousandths). The larger the damping coefficient, the smoother and slower the gain changes */
#define OMEGONPROCAM_OPTION_MOTOR_NUMBER           0x62       /* [RW] range: [1, 20] */
#define OMEGONPROCAM_OPTION_MOTOR_POS              0x10000000 /* [RW] range: [1, 702] */
#define OMEGONPROCAM_OPTION_PSEUDO_COLOR_START     0x63       /* [RW] Pseudo: start color, BGR format */
#define OMEGONPROCAM_OPTION_PSEUDO_COLOR_END       0x64       /* [RW] Pseudo: end color, BGR format */
#define OMEGONPROCAM_OPTION_PSEUDO_COLOR_ENABLE    0x65       /* [RW] Pseudo: -1 => custom: use startcolor & endcolor to generate the colormap
                                                                    0 => disable
                                                                    1 => spot
                                                                    2 => spring
                                                                    3 => summer
                                                                    4 => autumn
                                                                    5 => winter
                                                                    6 => bone
                                                                    7 => jet
                                                                    8 => rainbow
                                                                    9 => deepgreen
                                                                    10 => ocean
                                                                    11 => cool
                                                                    12 => hsv
                                                                    13 => pink
                                                                    14 => hot
                                                                    15 => parula
                                                                    16 => magma
                                                                    17 => inferno
                                                                    18 => plasma
                                                                    19 => viridis
                                                                    20 => cividis
                                                                    21 => twilight
                                                                    22 => twilight_shifted
                                                                    23 => turbo
                                                                    24 => red
                                                                    25 => green
                                                                    26 => blue
                                                         */
#define OMEGONPROCAM_OPTION_LOW_POWERCONSUMPTION   0x66       /* [RW] Low Power Consumption: 0 => disable, 1 => enable */
#define OMEGONPROCAM_OPTION_FPNC                   0x67       /* [RW] Fix Pattern Noise Correction
                                                             set:
                                                                 0: disable
                                                                 1: enable
                                                                -1: reset
                                                                 (0xff000000 | n): set the average number to n, [1~255]
                                                             get:
                                                                 (val & 0xff): 0 => disable, 1 => enable, 2 => inited
                                                                 ((val & 0xff00) >> 8): sequence
                                                                 ((val & 0xff0000) >> 16): average number
                                                         */
#define OMEGONPROCAM_OPTION_OVEREXP_POLICY         0x68       /* [RW] Auto exposure over exposure policy: when overexposed,
                                                                0 => directly reduce the exposure time/gain to the minimum value; or
                                                                1 => reduce exposure time/gain in proportion to current and target brightness.
                                                                n(n>1) => first adjust the exposure time to (maximum automatic exposure time * maximum automatic exposure gain) * n / 1000, and then adjust according to the strategy of 1
                                                            The advantage of policy 0 is that the convergence speed is faster, but there is black screen.
                                                            Policy 1 avoids the black screen, but the convergence speed is slower.
                                                            Default: 0
                                                         */
#define OMEGONPROCAM_OPTION_READOUT_MODE           0x69       /* [RW] Readout mode: 0 = IWR (Integrate While Read), 1 = ITR (Integrate Then Read)
                                                              The working modes of the detector readout circuit can be divided into two types: ITR and IWR. Using the IWR readout mode can greatly increase the frame rate. In the ITR mode, the integration of the (n+1)th frame starts after all the data of the nth frame are read out, while in the IWR mode, the data of the nth frame is read out at the same time when the (n+1)th frame is integrated
                                                         */
#define OMEGONPROCAM_OPTION_TAILLIGHT              0x6a       /* [RW] Turn on/off tail Led light: 0 => off, 1 => on; default: on */
#define OMEGONPROCAM_OPTION_LENSSTATE              0x6b       /* [RW] Load/Save lens state to EEPROM: 0 => load, 1 => save */
#define OMEGONPROCAM_OPTION_AWB_CONTINUOUS         0x6c       /* [RW] Auto White Balance: continuous mode
                                                                0:  disable (default)
                                                                n>0: every n millisecond(s)
                                                                n<0: every -n frame
                                                         */
#define OMEGONPROCAM_OPTION_TECTARGET_RANGE        0x6d       /* [RO] TEC target range: min(low 16 bits) = (short)(val & 0xffff), max(high 16 bits) = (short)((val >> 16) & 0xffff) */
#define OMEGONPROCAM_OPTION_CDS                    0x6e       /* [RW] Correlated Double Sampling */
#define OMEGONPROCAM_OPTION_LOW_POWER_EXPOTIME     0x6f       /* [RW] Low Power Consumption: Enable if exposure time is greater than the set value */
#define OMEGONPROCAM_OPTION_ZERO_OFFSET            0x70       /* [RW] Sensor output offset to zero: 0 => disable, 1 => eanble; default: 0 */
#define OMEGONPROCAM_OPTION_GVCP_TIMEOUT           0x71       /* [RW] GVCP Timeout: millisecond, range = [3, 75], default: 15
                                                              Unless in very special circumstances, generally no modification is required, just use the default value
                                                         */
#define OMEGONPROCAM_OPTION_GVCP_RETRY             0x72       /* [RW] GVCP Retry: range = [2, 8], default: 4
                                                              Unless in very special circumstances, generally no modification is required, just use the default value
                                                         */
#define OMEGONPROCAM_OPTION_GVSP_WAIT_PERCENT      0x73       /* [RW] GVSP wait percent: range = [0, 100], default = (trigger mode: 100, realtime: 0, other: 1) */
#define OMEGONPROCAM_OPTION_RESET_SEQ_TIMESTAMP    0x74       /* [WO] Reset to 0: 1 => seq; 2 => timestamp; 3 => both */
#define OMEGONPROCAM_OPTION_TRIGGER_CANCEL_MODE    0x75       /* [RW] Trigger cancel mode: 0 => no frame, 1 => output frame; default: 0 */
#define OMEGONPROCAM_OPTION_MECHANICALSHUTTER      0x76       /* [RW] Mechanical shutter: 0 => open, 1 => close; default: 0 */
#define OMEGONPROCAM_OPTION_LINE_TIME              0x77       /* [RO] Line-time of sensor in nanosecond */
#define OMEGONPROCAM_OPTION_ZERO_PADDING           0x78       /* [RW] Zero padding: 0 => high, 1 => low; default: 0 */
#define OMEGONPROCAM_OPTION_UPTIME                 0x79       /* [RO] device uptime in millisecond */
#define OMEGONPROCAM_OPTION_BITRANGE               0x7a       /* [RW] Bit range: [0, 8] */
#define OMEGONPROCAM_OPTION_MODE_SEQ_TIMESTAMP     0x7b       /* [RW] Mode of seq & timestamp: 0 => reset to 0 automatically; 1 => never reset automatically; default: 0 */
#define TOUPCAP_OPTION_TIMED_TRIGGER_NUM      0x7c       /* [RW] Timed trigger number */
#define OMEGONPROCAM_OPTION_TIMED_TRIGGER_LOW      0x20000000 /* [RW] Timed trigger: lower 32 bits of 64-bit integer, nanosecond since epoch (00:00:00 UTC on Thursday, 1 January 1970, see https://en.wikipedia.org/wiki/Unix_time) */
#define OMEGONPROCAM_OPTION_TIMED_TRIGGER_HIGH     0x40000000 /* [RW] Timed trigger: high 32 bits. The lower 32 bits must be set first, followed by the higher 32 bits */
#define OMEGONPROCAM_OPTION_AUTOEXP_THLD_TRIGGER   0x7d       /* [RW] trigger threshold of auto exposure */
#define OMEGONPROCAM_OPTION_LANE                   0x7e       /* [RW] */
#define OMEGONPROCAM_OPTION_VOLTAGEBIAS            0x7f       /* [RW] Voltage bias */
#define OMEGONPROCAM_OPTION_VOLTAGEBIAS_RANGE      0x80       /* [RO] Voltage bias range: min (low 16bits), max (high 16bits) */
#define OMEGONPROCAM_OPTION_READ_TIME              0x81       /* [RO] */
#define OMEGONPROCAM_OPTION_ALL_EXPO_TIME          0x82       /* [RO] */
#define OMEGONPROCAM_OPTION_CAMERA_PERIOD          0x83       /* [RO] */
#define OMEGONPROCAM_OPTION_FRONTEND_FULL          0x84       /* [RO] get the number of frontend deque full */
#define OMEGONPROCAM_OPTION_BACKEND_FULL           0x85       /* [RO] get the number of backend deque full */
#define OMEGONPROCAM_OPTION_GPS                    0x86       /* [RO] gps status: 0 => not supported; -1 => gps device offline; 1 => gps device online */
#define OMEGONPROCAM_OPTION_LINE_LENGTH            0x87       /* [RW] Line length in pixel clock */
#define OMEGONPROCAM_OPTION_SCAN_DIRECTION         0x88       /* [RW] Scan direction: 0 (forward), 1(reverse), 2(alternate) */

/* pixel format */
#define OMEGONPROCAM_PIXELFORMAT_RAW8              0x00
#define OMEGONPROCAM_PIXELFORMAT_RAW10             0x01
#define OMEGONPROCAM_PIXELFORMAT_RAW12             0x02
#define OMEGONPROCAM_PIXELFORMAT_RAW14             0x03
#define OMEGONPROCAM_PIXELFORMAT_RAW16             0x04
#define OMEGONPROCAM_PIXELFORMAT_YUV411            0x05
#define OMEGONPROCAM_PIXELFORMAT_VUYY              0x06
#define OMEGONPROCAM_PIXELFORMAT_YUV444            0x07
#define OMEGONPROCAM_PIXELFORMAT_RGB888            0x08
#define OMEGONPROCAM_PIXELFORMAT_GMCY8             0x09   /* map to RGGB 8 bits */
#define OMEGONPROCAM_PIXELFORMAT_GMCY12            0x0a   /* map to RGGB 12 bits */
#define OMEGONPROCAM_PIXELFORMAT_UYVY              0x0b
#define OMEGONPROCAM_PIXELFORMAT_RAW12PACK         0x0c
#define OMEGONPROCAM_PIXELFORMAT_RAW11             0x0d
#define OMEGONPROCAM_PIXELFORMAT_HDR8HL            0x0e   /* HDR, Bitdepth: 8, Conversion Gain: High + Low */
#define OMEGONPROCAM_PIXELFORMAT_HDR10HL           0x0f   /* HDR, Bitdepth: 10, Conversion Gain: High + Low */
#define OMEGONPROCAM_PIXELFORMAT_HDR11HL           0x10   /* HDR, Bitdepth: 11, Conversion Gain: High + Low */
#define OMEGONPROCAM_PIXELFORMAT_HDR12HL           0x11   /* HDR, Bitdepth: 12, Conversion Gain: High + Low */
#define OMEGONPROCAM_PIXELFORMAT_HDR14HL           0x12   /* HDR, Bitdepth: 14, Conversion Gain: High + Low */
#define OMEGONPROCAM_PIXELFORMAT_RAW10PACK         0x13

/*
* cmd: input
*    -1:       query the number
*    0~number: query the nth pixel format
* pixelFormat: output, OMEGONPROCAM_PIXELFORMAT_xxxx
*/
OMEGONPROCAM_API(HRESULT)     Omegonprocam_get_PixelFormatSupport(HOmegonprocam h, char cmd, int* pixelFormat);

/*
* pixelFormat: OMEGONPROCAM_PIXELFORMAT_XXXX
*/
OMEGONPROCAM_API(const char*) Omegonprocam_get_PixelFormatName(int pixelFormat);

/*
    xOffset, yOffset, xWidth, yHeight: must be even numbers
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Roi(HOmegonprocam h, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Roi(HOmegonprocam h, unsigned* pxOffset, unsigned* pyOffset, unsigned* pxWidth, unsigned* pyHeight);

/* multiple Roi */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_RoiN(HOmegonprocam h, unsigned xOffset[], unsigned yOffset[], unsigned xWidth[], unsigned yHeight[], unsigned Num);

/* Hardware Binning
* Value: 1x1, 2x2, etc
* Method: Average, Add, Skip
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Binning(HOmegonprocam h, const char* pValue, const char* pMethod);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Binning(HOmegonprocam h, const char** ppValue, const char** ppMethod);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_BinningNumber(HOmegonprocam h);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_BinningValue(HOmegonprocam h, unsigned index, const char** ppValue);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_BinningMethod(HOmegonprocam h, unsigned index, const char** ppMethod);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_XY(HOmegonprocam h, int x, int y);

#define OMEGONPROCAM_IOCONTROLTYPE_GET_SUPPORTEDMODE            0x01 /* 0x01 => Input, 0x02 => Output, (0x01 | 0x02) => support both Input and Output */
#define OMEGONPROCAM_IOCONTROLTYPE_GET_GPIODIR                  0x03 /* 0x00 => Input, 0x01 => Output */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_GPIODIR                  0x04
#define OMEGONPROCAM_IOCONTROLTYPE_GET_FORMAT                   0x05 /*
                                                                    0x00 => not connected
                                                                    0x01 => Tri-state: Tri-state mode (Not driven)
                                                                    0x02 => TTL: TTL level signals
                                                                    0x03 => LVDS: LVDS level signals
                                                                    0x04 => RS422: RS422 level signals
                                                                    0x05 => Opto-coupled
                                                                */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_FORMAT                   0x06
#define OMEGONPROCAM_IOCONTROLTYPE_GET_OUTPUTINVERTER           0x07 /* boolean, only support output signal */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_OUTPUTINVERTER           0x08
#define OMEGONPROCAM_IOCONTROLTYPE_GET_INPUTACTIVATION          0x09 /* 0x00 => Rising edge, 0x01 => Falling edge, 0x02 => Level high, 0x03 => Level low */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_INPUTACTIVATION          0x0a
#define OMEGONPROCAM_IOCONTROLTYPE_GET_DEBOUNCERTIME            0x0b /* debouncer time in microseconds, range: [0, 20000] */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_DEBOUNCERTIME            0x0c
#define OMEGONPROCAM_IOCONTROLTYPE_GET_TRIGGERSOURCE            0x0d /*
                                                                   0x00 => Opto-isolated input
                                                                   0x01 => GPIO0
                                                                   0x02 => GPIO1
                                                                   0x03 => Counter
                                                                   0x04 => PWM
                                                                   0x05 => Software
                                                                */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_TRIGGERSOURCE            0x0e
#define OMEGONPROCAM_IOCONTROLTYPE_GET_TRIGGERDELAY             0x0f /* Trigger delay time in microseconds, range: [0, 5000000] */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_TRIGGERDELAY             0x10
#define OMEGONPROCAM_IOCONTROLTYPE_GET_BURSTCOUNTER             0x11 /* Burst Counter, range: [1 ~ 65535] */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_BURSTCOUNTER             0x12
#define OMEGONPROCAM_IOCONTROLTYPE_GET_COUNTERSOURCE            0x13 /* 0x00 => Opto-isolated input, 0x01 => GPIO0, 0x02 => GPIO1 */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_COUNTERSOURCE            0x14
#define OMEGONPROCAM_IOCONTROLTYPE_GET_COUNTERVALUE             0x15 /* Counter Value, range: [1 ~ 65535] */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_COUNTERVALUE             0x16
#define OMEGONPROCAM_IOCONTROLTYPE_SET_RESETCOUNTER             0x18
#define OMEGONPROCAM_IOCONTROLTYPE_GET_PWM_FREQ                 0x19 /* PWM Frequency */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_PWM_FREQ                 0x1a
#define OMEGONPROCAM_IOCONTROLTYPE_GET_PWM_DUTYRATIO            0x1b /* PWM Duty Ratio */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_PWM_DUTYRATIO            0x1c
#define OMEGONPROCAM_IOCONTROLTYPE_GET_PWMSOURCE                0x1d /* PWM Source: 0x00 => Opto-isolated input, 0x01 => GPIO0, 0x02 => GPIO1 */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_PWMSOURCE                0x1e
#define OMEGONPROCAM_IOCONTROLTYPE_GET_OUTPUTMODE               0x1f /*
                                                                   0x00 => Frame Trigger Wait
                                                                   0x01 => Exposure Active
                                                                   0x02 => Strobe
                                                                   0x03 => User output
                                                                   0x04 => Counter Output
                                                                   0x05 => Timer Output
                                                                */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_OUTPUTMODE               0x20
#define OMEGONPROCAM_IOCONTROLTYPE_GET_STROBEDELAYMODE          0x21 /* boolean, 0 => pre-delay, 1 => delay; compared to exposure active signal */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_STROBEDELAYMODE          0x22
#define OMEGONPROCAM_IOCONTROLTYPE_GET_STROBEDELAYTIME          0x23 /* Strobe delay or pre-delay time in microseconds, range: [0, 5000000] */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_STROBEDELAYTIME          0x24
#define OMEGONPROCAM_IOCONTROLTYPE_GET_STROBEDURATION           0x25 /* Strobe duration time in microseconds, range: [0, 5000000] */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_STROBEDURATION           0x26
#define OMEGONPROCAM_IOCONTROLTYPE_GET_USERVALUE                0x27 /*
                                                                   bit0 => Opto-isolated output
                                                                   bit1 => GPIO0 output
                                                                   bit2 => GPIO1 output
                                                                */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_USERVALUE                0x28
#define OMEGONPROCAM_IOCONTROLTYPE_GET_UART_ENABLE              0x29 /* enable: 1 => on; 0 => off */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_UART_ENABLE              0x2a
#define OMEGONPROCAM_IOCONTROLTYPE_GET_UART_BAUDRATE            0x2b /* baud rate: 0 => 9600; 1 => 19200; 2 => 38400; 3 => 57600; 4 => 115200 */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_UART_BAUDRATE            0x2c
#define OMEGONPROCAM_IOCONTROLTYPE_GET_UART_LINEMODE            0x2d /* line mode: 0 => TX(GPIO_0)/RX(GPIO_1); 1 => TX(GPIO_1)/RX(GPIO_0) */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_UART_LINEMODE            0x2e
#define OMEGONPROCAM_IOCONTROLTYPE_GET_EXPO_ACTIVE_MODE         0x2f /* exposure time signal: 0 => specified line, 1 => common exposure time */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_EXPO_ACTIVE_MODE         0x30
#define OMEGONPROCAM_IOCONTROLTYPE_GET_EXPO_START_LINE          0x31 /* exposure start line, default: 0 */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_EXPO_START_LINE          0x32
#define OMEGONPROCAM_IOCONTROLTYPE_GET_EXPO_END_LINE            0x33 /* exposure end line, default: 0
                                                                   end line must be no less than start line
                                                                */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_EXPO_END_LINE            0x34
#define OMEGONPROCAM_IOCONTROLTYPE_GET_EXEVT_ACTIVE_MODE        0x35 /* exposure event: 0 => specified line, 1 => common exposure time */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_EXEVT_ACTIVE_MODE        0x36
#define OMEGONPROCAM_IOCONTROLTYPE_GET_OUTPUTCOUNTERVALUE       0x37 /* Output Counter Value, range: [0 ~ 65535] */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_OUTPUTCOUNTERVALUE       0x38
#define OMEGONPROCAM_IOCONTROLTYPE_SET_OUTPUT_PAUSE             0x3a /* Output pause: 1 => puase, 0 => unpause */
#define OMEGONPROCAM_IOCONTROLTYPE_GET_INPUT_STATE              0x3b /* Input state: 0 (low level) or 1 (high level) */
#define OMEGONPROCAM_IOCONTROLTYPE_GET_USER_PULSE_HIGH          0x3d /* User pulse high level time: us */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_USER_PULSE_HIGH          0x3e
#define OMEGONPROCAM_IOCONTROLTYPE_GET_USER_PULSE_LOW           0x3f /* User pulse low level time: us */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_USER_PULSE_LOW           0x40
#define OMEGONPROCAM_IOCONTROLTYPE_GET_USER_PULSE_NUMBER        0x41 /* User pulse number: default 0 */
#define OMEGONPROCAM_IOCONTROLTYPE_SET_USER_PULSE_NUMBER        0x42
#define OMEGONPROCAM_IOCONTROLTYPE_GET_EXTERNAL_TRIGGER_NUMBER  0x43 /* External trigger number */
#define OMEGONPROCAM_IOCONTROLTYPE_GET_DEBOUNCER_TRIGGER_NUMBER 0x45 /* Trigger signal number after debounce */
#define OMEGONPROCAM_IOCONTROLTYPE_GET_EFFECTIVE_TRIGGER_NUMBER 0x47 /* Effective trigger signal number */

#define OMEGONPROCAM_IOCONTROL_DELAYTIME_MAX                    (5 * 1000 * 1000)

/*
  ioLine:
    0 => Opto-isolated input
    1 => Opto-isolated output
    2 => GPIO0
    3 => GPIO1
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_IoControl(HOmegonprocam h, unsigned ioLine, unsigned nType, int outVal, int* inVal);

#ifndef __OMEGONPROCAMSELFTRIGGER_DEFINED__
#define __OMEGONPROCAMSELFTRIGGER_DEFINED__
typedef struct {
    unsigned sensingLeft, sensingTop, sensingWidth, sensingHeight; /* Sensing Area */
    unsigned hThreshold, lThreshold; /* threshold High side, threshold Low side */
    unsigned expoTime; /* Exposure Time */
    unsigned short expoGain; /* Exposure Gain */
    unsigned short hCount, lCount; /* Count threshold High side, Count threshold Low side, thousandths of Sensing Area */
    unsigned short reserved;
} OmegonprocamSelfTrigger;
#endif
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_SelfTrigger(HOmegonprocam h, const OmegonprocamSelfTrigger* pSt);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_SelfTrigger(HOmegonprocam h, OmegonprocamSelfTrigger* pSt);

#define OMEGONPROCAM_FLASH_SIZE      0x00    /* query total size */
#define OMEGONPROCAM_FLASH_EBLOCK    0x01    /* query erase block size */
#define OMEGONPROCAM_FLASH_RWBLOCK   0x02    /* query read/write block size */
#define OMEGONPROCAM_FLASH_STATUS    0x03    /* query status */
#define OMEGONPROCAM_FLASH_READ      0x04    /* read */
#define OMEGONPROCAM_FLASH_WRITE     0x05    /* write */
#define OMEGONPROCAM_FLASH_ERASE     0x06    /* erase */
/* Flash:
 action = OMEGONPROCAM_FLASH_XXXX: read, write, erase, query total size, query read/write block size, query erase block size
 addr = address
 see democpp
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_rwc_Flash(HOmegonprocam h, unsigned action, unsigned addr, unsigned len, void* pData);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_write_UART(HOmegonprocam h, const unsigned char* pData, unsigned nDataLen);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_read_UART(HOmegonprocam h, unsigned char* pBuffer, unsigned nBufferLen);

/* Initialize support for GigE cameras. If online/offline notifications are not required, the callback function can be set to NULL */
typedef void (__stdcall* POMEGONPROCAM_HOTPLUG)(void* ctxHotPlug);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_GigeEnable(POMEGONPROCAM_HOTPLUG funHotPlug, void* ctxHotPlug);

/* Initialize support for PCIe cameras. If online/offline notifications are not required, the callback function can be set to NULL */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_PciEnable(POMEGONPROCAM_HOTPLUG funHotPlug, void* ctxHotPlug);

/* Initialize support for *.cti cameras. If online/offline notifications are not required, the callback function can be set to NULL
* (1) ctiPath = NULL means all *.cti in GENICAM_GENTL64_PATH/GENICAM_GENTL32_PATH
* or
* (2) ctiPath[] = {
            "/full/path/to/file.cti"
            ...
            NULLL
        }
*/
#if defined(_WIN32)
OMEGONPROCAM_API(HRESULT)  Omegonprocam_CtiEnable(POMEGONPROCAM_HOTPLUG funHotPlug, void* ctxHotPlug, const wchar_t* ctiPath[]);
#else
OMEGONPROCAM_API(HRESULT)  Omegonprocam_CtiEnable(POMEGONPROCAM_HOTPLUG funHotPlug, void* ctxHotPlug, const char* ctiPath[]);
#endif

/*
 filePath:
    "*": export to EEPROM
    "0x????" or "0X????": export to EEPROM specified address
    file path: export to file in ini format
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_export_Cfg(HOmegonprocam h, const char* filePath);

/*
This function is only available on macOS and Linux, it's unnecessary on Windows & Android. To process the device plug in / pull out:
  (1) On Windows, please refer to the MSDN
       (a) Device Management, https://docs.microsoft.com/en-us/windows/win32/devio/device-management
       (b) Detecting Media Insertion or Removal, https://docs.microsoft.com/en-us/windows/win32/devio/detecting-media-insertion-or-removal
  (2) On Android, please refer to https://developer.android.com/guide/topics/connectivity/usb/host
  (3) On Linux / macOS, please call this function to register the callback function.
      When the device is inserted or pulled out, you will be notified by the callback funcion, and then call Omegonprocam_EnumV2(...) again to enum the cameras.
  (4) On macOS, IONotificationPortCreate series APIs can also be used as an alternative.
Recommendation: for better rubustness, when notify of device insertion arrives, don't open handle of this device immediately, but open it after delaying a short time (e.g., 200 milliseconds).
*/
#if !defined(_WIN32) && !defined(__ANDROID__)
OMEGONPROCAM_API(void)   Omegonprocam_HotPlug(POMEGONPROCAM_HOTPLUG funHotPlug, void* ctxHotPlug);
#endif

OMEGONPROCAM_API(unsigned) Omegonprocam_EnumWithName(OmegonprocamDeviceV2 pti[OMEGONPROCAM_MAX]);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_set_Name(HOmegonprocam h, const char* name);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_query_Name(HOmegonprocam h, char name[64]);
#if defined(_WIN32)
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Name(const wchar_t* camId, const char* name);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Name(const wchar_t* camId, char name[64]);
#else
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Name(const char* camId, const char* name);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_Name(const char* camId, char name[64]);
#endif

typedef struct {
    unsigned short lensID;
    unsigned char  lensType;
    unsigned char  statusAfmf;      /* LENS_AF = 0x00,  LENS_MF = 0x80 */

    unsigned short maxFocalLength;
    unsigned short curFocalLength;
    unsigned short minFocalLength;

    short          farFM;           /* focus motor, absolute value */
    short          curFM;           /* current focus motor */
    short          nearFM;

    unsigned short maxFocusDistance;
    unsigned short minFocusDistance;

    char           curAM;
    unsigned char  maxAM;           /* maximum Aperture, mimimum F# */
    unsigned char  minAM;           /* mimimum Aperture, maximum F# */
    unsigned char  posAM;           /* used for set aperture motor to posAM, it is an index */
    int            posFM;           /* used for set focus motor to posFM */

    unsigned       sizeFN;
    const char**   arrayFN;
    const char*    lensName;        /* lens Name */
} OmegonprocamLensInfo;

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_LensInfo(HOmegonprocam h, OmegonprocamLensInfo* pInfo);

typedef enum
{
    OmegonprocamAFMode_CALIBRATE = 0x0,/* lens calibration mode */
    OmegonprocamAFMode_MANUAL    = 0x1,/* manual focus mode */
    OmegonprocamAFMode_ONCE      = 0x2,/* onepush focus mode */
    OmegonprocamAFMode_AUTO      = 0x3,/* autofocus mode */
    OmegonprocamAFMode_NONE      = 0x4,/* no active selection of focus mode */
    OmegonprocamAFMode_IDLE      = 0x5,
    OmegonprocamAFMode_UNUSED    = 0xffffffff
} OmegonprocamAFMode;

typedef enum
{
    OmegonprocamAFStatus_NA           = 0x0,/* Not available */
    OmegonprocamAFStatus_PEAKPOINT    = 0x1,/* Focus completed, find the focus position */
    OmegonprocamAFStatus_DEFOCUS      = 0x2,/* End of focus, defocus */
    OmegonprocamAFStatus_NEAR         = 0x3,/* Focusing ended, object too close */
    OmegonprocamAFStatus_FAR          = 0x4,/* Focusing ended, object too far */
    OmegonprocamAFStatus_ROICHANGED   = 0x5,/* Focusing ends, roi changes */
    OmegonprocamAFStatus_SCENECHANGED = 0x6,/* Focusing ends, scene changes */
    OmegonprocamAFStatus_MODECHANGED  = 0x7,/* The end of focusing and the change in focusing mode is usually determined by the user moderator */
    OmegonprocamAFStatus_UNFINISH     = 0x8,/* The focus is not complete. At the beginning of focusing, it will be set as incomplete */
    OmegonprocamAfStatus_UNUSED       = 0xffffffff
} OmegonprocamAFStatus;/* Focus Status */

typedef struct {
    OmegonprocamAFMode    AF_Mode;
    OmegonprocamAFStatus  AF_Status;
    unsigned char    AF_LensAP_Update_Flag;  /* mark for whether the lens aperture is calibrated */
    unsigned char    Reserved[3];
} OmegonprocamAFState;

OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_AFState(HOmegonprocam h, OmegonprocamAFState* pState);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_AFMode(HOmegonprocam h, OmegonprocamAFMode mode, int bFixedWD, unsigned uiNear, unsigned uiFar);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_AFRoi(HOmegonprocam h, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_AFRoi(HOmegonprocam h, unsigned* pxOffset, unsigned* pyOffset, unsigned* pxWidth, unsigned* pyHeight);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_AFAperture(HOmegonprocam h, int iAperture);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_AFFMPos(HOmegonprocam h, int iFMPos);

/*  simulate replug:
    return > 0, the number of device has been replug
    return = 0, no device found
    return E_ACCESSDENIED if without UAC Administrator privileges
    for each device found, it will take about 3 seconds
*/
#if defined(_WIN32)
OMEGONPROCAM_API(HRESULT) Omegonprocam_Replug(const wchar_t* camId);
OMEGONPROCAM_API(HRESULT) Omegonprocam_Enable(const wchar_t* camId, int enable); /* 1 => enable, 0 => disable */
#else
OMEGONPROCAM_API(HRESULT) Omegonprocam_Replug(const char* camId);
OMEGONPROCAM_API(HRESULT) Omegonprocam_Enable(const char* camId, int enable); /* 1 => enable, 0 => disable */
#endif

OMEGONPROCAM_API(const OmegonprocamModelV2**) Omegonprocam_all_Model(); /* return all supported USB model array */
OMEGONPROCAM_API(const OmegonprocamModelV2*) Omegonprocam_query_Model(HOmegonprocam h);
OMEGONPROCAM_API(const OmegonprocamModelV2*) Omegonprocam_get_Model(unsigned short idVendor, unsigned short idProduct);

/* firmware update:
    camId: camera ID
    filePath: ufw file full path
    funProgress, ctx: progress percent callback
Please do not unplug the camera or lost power during the upgrade process, this is very very important.
Once an unplugging or power outage occurs during the upgrade process, the camera will no longer be available and can only be returned to the factory for repair.
*/
#if defined(_WIN32)
OMEGONPROCAM_API(HRESULT)  Omegonprocam_Update(const wchar_t* camId, const wchar_t* filePath, PIOMEGONPROCAM_PROGRESS funProgress, void* ctxProgress);
#else
OMEGONPROCAM_API(HRESULT)  Omegonprocam_Update(const char* camId, const char* filePath, PIOMEGONPROCAM_PROGRESS funProgress, void* ctxProgress);
#endif

OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Linear(HOmegonprocam h, const unsigned char* v8, const unsigned short* v16); /* v8, v16 pointer must remains valid while camera running */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Curve(HOmegonprocam h, const unsigned char* v8, const unsigned short* v16); /* v8, v16 pointer must remains valid while camera running */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_ColorMatrix(HOmegonprocam h, const double v[9]); /* null => revert to model default */
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_InitWBGain(HOmegonprocam h, const unsigned short v[3]); /* null => revert to model default */

/*
    get the actual frame rate of the camera at the most recent time (about a few seconds):
    framerate (fps) = nFrame * 1000.0 / nTime
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_FrameRate(HOmegonprocam h, unsigned* nFrame, unsigned* nTime, unsigned* nTotalFrame);

/* AAF: Astro Auto Focuser */
#define OMEGONPROCAM_AAF_SETPOSITION     0x01
#define OMEGONPROCAM_AAF_GETPOSITION     0x02
#define OMEGONPROCAM_AAF_SETZERO         0x03
#define OMEGONPROCAM_AAF_SETDIRECTION    0x05
#define OMEGONPROCAM_AAF_GETDIRECTION    0x06
#define OMEGONPROCAM_AAF_SETMAXINCREMENT 0x07
#define OMEGONPROCAM_AAF_GETMAXINCREMENT 0x08
#define OMEGONPROCAM_AAF_SETFINE         0x09
#define OMEGONPROCAM_AAF_GETFINE         0x0a
#define OMEGONPROCAM_AAF_SETCOARSE       0x0b
#define OMEGONPROCAM_AAF_GETCOARSE       0x0c
#define OMEGONPROCAM_AAF_SETBUZZER       0x0d
#define OMEGONPROCAM_AAF_GETBUZZER       0x0e
#define OMEGONPROCAM_AAF_SETBACKLASH     0x0f
#define OMEGONPROCAM_AAF_GETBACKLASH     0x10
#define OMEGONPROCAM_AAF_GETAMBIENTTEMP  0x12
#define OMEGONPROCAM_AAF_GETTEMP         0x14  /* in 0.1 degrees Celsius, such as: 32 means 3.2 degrees Celsius */
#define OMEGONPROCAM_AAF_ISMOVING        0x16
#define OMEGONPROCAM_AAF_HALT            0x17
#define OMEGONPROCAM_AAF_SETMAXSTEP      0x1b
#define OMEGONPROCAM_AAF_GETMAXSTEP      0x1c
#define OMEGONPROCAM_AAF_GETSTEPSIZE     0x1e
#define OMEGONPROCAM_AAF_RANGEMIN        0xfd  /* Range: min value */
#define OMEGONPROCAM_AAF_RANGEMAX        0xfe  /* Range: max value */
#define OMEGONPROCAM_AAF_RANGEDEF        0xff  /* Range: default value */
OMEGONPROCAM_API(HRESULT) Omegonprocam_AAF(HOmegonprocam h, int action, int outVal, int* inVal);

/* astronomy: for ST4 guide, please see: ASCOM Platform Help ICameraV2.
    nDirect: 0 = North, 1 = South, 2 = East, 3 = West, 4 = Stop
    nDuration: in milliseconds
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_ST4PlusGuide(HOmegonprocam h, unsigned nDirect, unsigned nDuration);

/* S_OK: ST4 pulse guiding
   S_FALSE: ST4 not pulse guiding
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_ST4PlusGuideState(HOmegonprocam h);

OMEGONPROCAM_API(HRESULT)  Omegonprocam_Gain2TempTint(const int gain[3], int* temp, int* tint);
OMEGONPROCAM_API(void)     Omegonprocam_TempTint2Gain(const int temp, const int tint, int gain[3]);
/*
    calculate the clarity factor:
    pImageData: pointer to the image data
    bits: 8(Grey), 16(Grey), 24(RGB24), 32(RGB32), 48(RGB48), 64(RGB64)
    nImgWidth, nImgHeight: the image width and height
    xOffset, yOffset, xWidth, yHeight: the Roi used to calculate. If not specified, use 1/5 * 1/5 rectangle in the center
    return < 0.0 when error
*/
OMEGONPROCAM_API(double)   Omegonprocam_calc_ClarityFactor(const void* pImageData, int bits, unsigned nImgWidth, unsigned nImgHeight);
OMEGONPROCAM_API(double)   Omegonprocam_calc_ClarityFactorV2(const void* pImageData, int bits, unsigned nImgWidth, unsigned nImgHeight, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight);

/*
    nBitCount: output bitmap bit count
    when nBitDepth == 8:
        nBitCount must be 24 or 32
    when nBitDepth > 8
        nBitCount:  24 => RGB24
                    32 => RGB32
                    48 => RGB48
                    64 => RGB64
*/
OMEGONPROCAM_API(void)     Omegonprocam_deBayerV2(unsigned nFourCC, int nW, int nH, const void* pRaw, void* pRGB, unsigned char nBitDepth, unsigned char nBitCount);


#ifndef __OMEGONPROCAMFOCUSMOTOR_DEFINED__
#define __OMEGONPROCAMFOCUSMOTOR_DEFINED__
typedef struct {
    int imax;    /* maximum auto focus sensor board positon */
    int imin;    /* minimum auto focus sensor board positon */
    int idef;    /* conjugate calibration positon */
    int imaxabs; /* maximum absolute auto focus sensor board positon, micrometer */
    int iminabs; /* maximum absolute auto focus sensor board positon, micrometer */
    int zoneh;   /* zone horizontal */
    int zonev;   /* zone vertical */
} OmegonprocamFocusMotor;
#endif

OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_FocusMotor(HOmegonprocam h, OmegonprocamFocusMotor* pFocusMotor);

/*
* raw image process
* step:
*  'F': very beginning
*  'B': just before black balance
*  'D': just before demosaic
 */
typedef void (__stdcall* POMEGONPROCAM_PROCESS_CALLBACK)(char step, char bStill, unsigned nFourCC, int nW, int nH, void* pRaw, unsigned char pixelFormat, void* ctxProcess);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Process(HOmegonprocam h, POMEGONPROCAM_PROCESS_CALLBACK funProcess, void* ctxProcess);

/* debayer: raw to RGB */
typedef void (__stdcall* POMEGONPROCAM_DEMOSAIC_CALLBACK)(unsigned nFourCC, int nW, int nH, const void* pRaw, void* pRGB, unsigned char nBitDepth, void* ctxDemosaic);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_Demosaic(HOmegonprocam h, POMEGONPROCAM_DEMOSAIC_CALLBACK funDemosaic, void* ctxDemosaic);

/*
    obsolete, please use OmegonprocamModelV2
*/
typedef struct {
#if defined(_WIN32)
    const wchar_t*      name;       /* model name, in Windows, we use unicode */
#else
    const char*         name;       /* model name */
#endif
    unsigned            flag;       /* OMEGONPROCAM_FLAG_xxx */
    unsigned            maxspeed;   /* number of speed level, same as Omegonprocam_get_MaxSpeed(), the speed range = [0, maxspeed], closed interval */
    unsigned            preview;    /* number of preview resolution, same as Omegonprocam_get_ResolutionNumber() */
    unsigned            still;      /* number of still resolution, same as Omegonprocam_get_StillResolutionNumber() */
    OmegonprocamResolution   res[16];
} OmegonprocamModel; /* camera model */

/*
    obsolete, please use Omegonprocam_deBayerV2
*/
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(void)     Omegonprocam_deBayer(unsigned nFourCC, int nW, int nH, const void* pRaw, void* pRGB, unsigned char nBitDepth);

/*
    obsolete, please use OmegonprocamDeviceV2
*/
typedef struct {
#if defined(_WIN32)
    wchar_t             displayname[64];    /* display name */
    wchar_t             id[64];             /* unique and opaque id of a connected camera, for Omegonprocam_Open */
#else
    char                displayname[64];    /* display name */
    char                id[64];             /* unique and opaque id of a connected camera, for Omegonprocam_Open */
#endif
    const OmegonprocamModel* model;
} OmegonprocamDevice; /* camera instance for enumerating */

/*
    obsolete, please use Omegonprocam_EnumV2
*/
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(unsigned) Omegonprocam_Enum(OmegonprocamDevice arr[OMEGONPROCAM_MAX]);

typedef POMEGONPROCAM_DATA_CALLBACK_V3 POMEGONPROCAM_DATA_CALLBACK_V2;
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_StartPushModeV2(HOmegonprocam h, POMEGONPROCAM_DATA_CALLBACK_V2 funData, void* ctxData);

#if !defined(_WIN32)
#ifndef __BITMAPINFOHEADER_DEFINED__
#define __BITMAPINFOHEADER_DEFINED__
typedef struct {
    unsigned        biSize;
    int             biWidth;
    int             biHeight;
    unsigned short  biPlanes;
    unsigned short  biBitCount;
    unsigned        biCompression;
    unsigned        biSizeImage;
    int             biXPelsPerMeter;
    int             biYPelsPerMeter;
    unsigned        biClrUsed;
    unsigned        biClrImportant;
} BITMAPINFOHEADER;
#endif
#endif

typedef void (__stdcall* POMEGONPROCAM_DATA_CALLBACK)(const void* pData, const BITMAPINFOHEADER* pHeader, int bSnap, void* ctxData);
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_StartPushMode(HOmegonprocam h, POMEGONPROCAM_DATA_CALLBACK funData, void* ctxData);

OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_ExpoCallback(HOmegonprocam h, PIOMEGONPROCAM_EXPOSURE_CALLBACK funExpo, void* ctxExpo);
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_ChromeCallback(HOmegonprocam h, PIOMEGONPROCAM_CHROME_CALLBACK funChrome, void* ctxChrome);

/* Omegonprocam_FfcOnePush is obsolete, recommend using Omegonprocam_FfcOnce. */
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_FfcOnePush(HOmegonprocam h);

/* Omegonprocam_DfcOnePush is obsolete, recommend using Omegonprocam_DfcOnce. */
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_DfcOnePush(HOmegonprocam h);

/* Omegonprocam_AwbOnePush is obsolete, recommend using Omegonprocam_AwbOnce. */
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_AwbOnePush(HOmegonprocam h, PIOMEGONPROCAM_TEMPTINT_CALLBACK funTT, void* ctxTT);

/* Omegonprocam_AbbOnePush is obsolete, recommend using Omegonprocam_AbbOnce. */
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_AbbOnePush(HOmegonprocam h, PIOMEGONPROCAM_BLACKBALANCE_CALLBACK funBB, void* ctxBB);

#if defined(_WIN32)
/* Omegonprocam_put_TempTintInit is obsolete, recommend using Omegonprocam_AwbOnce */
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_TempTintInit(HOmegonprocam h, PIOMEGONPROCAM_TEMPTINT_CALLBACK funTT, void* ctxTT);

/* ProcessMode: obsolete & useless, noop */
#ifndef __OMEGONPROCAM_PROCESSMODE_DEFINED__
#define __OMEGONPROCAM_PROCESSMODE_DEFINED__
#define OMEGONPROCAM_PROCESSMODE_FULL        0x00    /* better image quality, more cpu usage. this is the default value */
#define OMEGONPROCAM_PROCESSMODE_FAST        0x01    /* lower image quality, less cpu usage */
#endif
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_ProcessMode(HOmegonprocam h, unsigned nProcessMode);
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_ProcessMode(HOmegonprocam h, unsigned* pnProcessMode);
#endif

/* obsolete, recommend using Omegonprocam_put_Roi and Omegonprocam_get_Roi */
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_RoiMode(HOmegonprocam h, int bRoiMode, int xOffset, int yOffset);
OMEGONPROCAM_DEPRECATED
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_RoiMode(HOmegonprocam h, int* pbRoiMode, int* pxOffset, int* pyOffset);

/* obsolete:
     ------------------------------------------------------------|
     | Parameter         |   Range       |   Default             |
     |-----------------------------------------------------------|
     | VidgetAmount      |   -100~100    |   0                   |
     | VignetMidPoint    |   0~100       |   50                  |
     -------------------------------------------------------------
*/
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_VignetEnable(HOmegonprocam h, int bEnable);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_VignetEnable(HOmegonprocam h, int* bEnable);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_VignetAmountInt(HOmegonprocam h, int nAmount);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_VignetAmountInt(HOmegonprocam h, int* nAmount);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_put_VignetMidPointInt(HOmegonprocam h, int nMidPoint);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_get_VignetMidPointInt(HOmegonprocam h, int* nMidPoint);

/* obsolete flags */
#define OMEGONPROCAM_FLAG_BITDEPTH10    OMEGONPROCAM_FLAG_RAW10  /* pixel format, RAW 10bits */
#define OMEGONPROCAM_FLAG_BITDEPTH12    OMEGONPROCAM_FLAG_RAW12  /* pixel format, RAW 12bits */
#define OMEGONPROCAM_FLAG_BITDEPTH14    OMEGONPROCAM_FLAG_RAW14  /* pixel format, RAW 14bits */
#define OMEGONPROCAM_FLAG_BITDEPTH16    OMEGONPROCAM_FLAG_RAW16  /* pixel format, RAW 16bits */

OMEGONPROCAM_API(HRESULT)  Omegonprocam_log_File(const
#if defined(_WIN32)
                                       wchar_t*
#else
                                       char*
#endif
                                       filePath);
OMEGONPROCAM_API(HRESULT)  Omegonprocam_log_Level(unsigned level); /* 0 => none; 1 => error; 2 => debug; 3 => verbose */
OMEGONPROCAM_API(void)     Omegonprocam_log_str(unsigned level, const char* str);
OMEGONPROCAM_APIV(void)    Omegonprocam_log(unsigned level, const char* format, ...);

#if defined(OMEGONPROCAM_LOG)
#define OMEGONPROCAM_LOG_NONE(format, ...)	  Omegonprocam_log(0, format, ##__VA_ARGS__)
#define OMEGONPROCAM_LOG_ERROR(format, ...)	  Omegonprocam_log(1, format, ##__VA_ARGS__)
#define OMEGONPROCAM_LOG_DEBUG(format, ...)	  Omegonprocam_log(2, format, ##__VA_ARGS__)
#define OMEGONPROCAM_LOG_VERBOSE(format, ...)  Omegonprocam_log(3, format, ##__VA_ARGS__)
/* for example: OMEGONPROCAM_LOG_DEBUG("%s: blahblah, x = %d, y = %f", __func__ x, y); */
#endif

#if defined(_WIN32)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif
