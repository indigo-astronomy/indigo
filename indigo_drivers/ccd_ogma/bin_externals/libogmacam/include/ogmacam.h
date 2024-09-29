#ifndef __ogmacam_h__
#define __ogmacam_h__

/* Version: 57.26598.20240928 */
/*
   Platform & Architecture:
       (1) Win32:
              (a) x64: Win7 or above
              (b) x86: XP SP3 or above; CPU supports SSE2 instruction set or above
              (c) arm64: Win10 or above
              (d) arm: Win10 or above
       (2) WinRT: x64, x86, arm64, arm; Win10 or above
       (3) macOS:
              (a) x64+x86: macOS 10.10 or above
              (b) x64+arm64: macOS 11.0 or above, support x64 and Apple silicon (such as M1, M2, etc)
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

#if defined(_WIN32)
#ifndef _INC_WINDOWS
#include <windows.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__cplusplus) && (__cplusplus >= 201402L)
#define OGMACAM_DEPRECATED  [[deprecated]]
#elif defined(_MSC_VER)
#define OGMACAM_DEPRECATED  __declspec(deprecated)
#elif defined(__GNUC__) || defined(__clang__)
#define OGMACAM_DEPRECATED  __attribute__((deprecated))
#else
#define OGMACAM_DEPRECATED
#endif

#if defined(_WIN32) /* Windows */
#pragma pack(push, 8)
#ifdef OGMACAM_EXPORTS
#define OGMACAM_API(x)    __declspec(dllexport)   x   __stdcall  /* in Windows, we use __stdcall calling convention, see https://docs.microsoft.com/en-us/cpp/cpp/stdcall */
#elif !defined(OGMACAM_NOIMPORTS)
#define OGMACAM_API(x)    __declspec(dllimport)   x   __stdcall
#else
#define OGMACAM_API(x)    x   __stdcall
#endif
#else   /* Linux or macOS */
#define OGMACAM_API(x)    x
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
#if defined(OGMACAM_HRESULT_ERRORCODE_NEEDED)
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
#endif

/* handle */
typedef struct Ogmacam_t { int unused; } *HOgmacam;

#define OGMACAM_MAX                      128
                                         
#define OGMACAM_FLAG_CMOS                0x00000001  /* cmos sensor */
#define OGMACAM_FLAG_CCD_PROGRESSIVE     0x00000002  /* progressive ccd sensor */
#define OGMACAM_FLAG_CCD_INTERLACED      0x00000004  /* interlaced ccd sensor */
#define OGMACAM_FLAG_ROI_HARDWARE        0x00000008  /* support hardware ROI */
#define OGMACAM_FLAG_MONO                0x00000010  /* monochromatic */
#define OGMACAM_FLAG_BINSKIP_SUPPORTED   0x00000020  /* support bin/skip mode, see Ogmacam_put_Mode and Ogmacam_get_Mode */
#define OGMACAM_FLAG_USB30               0x00000040  /* usb3.0 */
#define OGMACAM_FLAG_TEC                 0x00000080  /* Thermoelectric Cooler */
#define OGMACAM_FLAG_USB30_OVER_USB20    0x00000100  /* usb3.0 camera connected to usb2.0 port */
#define OGMACAM_FLAG_ST4                 0x00000200  /* ST4 port */
#define OGMACAM_FLAG_GETTEMPERATURE      0x00000400  /* support to get the temperature of the sensor */
#define OGMACAM_FLAG_HIGH_FULLWELL       0x00000800  /* high fullwell capacity */
#define OGMACAM_FLAG_RAW10               0x00001000  /* pixel format, RAW 10bits */
#define OGMACAM_FLAG_RAW12               0x00002000  /* pixel format, RAW 12bits */
#define OGMACAM_FLAG_RAW14               0x00004000  /* pixel format, RAW 14bits */
#define OGMACAM_FLAG_RAW16               0x00008000  /* pixel format, RAW 16bits */
#define OGMACAM_FLAG_FAN                 0x00010000  /* cooling fan */
#define OGMACAM_FLAG_TEC_ONOFF           0x00020000  /* Thermoelectric Cooler can be turn on or off, support to set the target temperature of TEC */
#define OGMACAM_FLAG_ISP                 0x00040000  /* ISP (Image Signal Processing) chip */
#define OGMACAM_FLAG_TRIGGER_SOFTWARE    0x00080000  /* support software trigger */
#define OGMACAM_FLAG_TRIGGER_EXTERNAL    0x00100000  /* support external trigger */
#define OGMACAM_FLAG_TRIGGER_SINGLE      0x00200000  /* only support trigger single: one trigger, one image */
#define OGMACAM_FLAG_BLACKLEVEL          0x00400000  /* support set and get the black level */
#define OGMACAM_FLAG_AUTO_FOCUS          0x00800000  /* support auto focus */
#define OGMACAM_FLAG_BUFFER              0x01000000  /* frame buffer */
#define OGMACAM_FLAG_DDR                 0x02000000  /* use very large capacity DDR (Double Data Rate SDRAM) for frame buffer. The capacity is not less than one full frame */
#define OGMACAM_FLAG_CG                  0x04000000  /* Conversion Gain: HCG, LCG */
#define OGMACAM_FLAG_YUV411              0x08000000  /* pixel format, yuv411 */
#define OGMACAM_FLAG_VUYY                0x10000000  /* pixel format, yuv422, VUYY */
#define OGMACAM_FLAG_YUV444              0x20000000  /* pixel format, yuv444 */
#define OGMACAM_FLAG_RGB888              0x40000000  /* pixel format, RGB888 */
#define OGMACAM_FLAG_RAW8                0x80000000  /* pixel format, RAW 8 bits */
#define OGMACAM_FLAG_GMCY8               0x0000000100000000  /* pixel format, GMCY, 8bits */
#define OGMACAM_FLAG_GMCY12              0x0000000200000000  /* pixel format, GMCY, 12bits */
#define OGMACAM_FLAG_UYVY                0x0000000400000000  /* pixel format, yuv422, UYVY */
#define OGMACAM_FLAG_CGHDR               0x0000000800000000  /* Conversion Gain: HCG, LCG, HDR */
#define OGMACAM_FLAG_GLOBALSHUTTER       0x0000001000000000  /* global shutter */
#define OGMACAM_FLAG_FOCUSMOTOR          0x0000002000000000  /* support focus motor */
#define OGMACAM_FLAG_PRECISE_FRAMERATE   0x0000004000000000  /* support precise framerate & bandwidth, see OGMACAM_OPTION_PRECISE_FRAMERATE & OGMACAM_OPTION_BANDWIDTH */
#define OGMACAM_FLAG_HEAT                0x0000008000000000  /* support heat to prevent fogging up */
#define OGMACAM_FLAG_LOW_NOISE           0x0000010000000000  /* support low noise mode (Higher signal noise ratio, lower frame rate) */
#define OGMACAM_FLAG_LEVELRANGE_HARDWARE 0x0000020000000000  /* hardware level range, put(get)_LevelRangeV2 */
#define OGMACAM_FLAG_EVENT_HARDWARE      0x0000040000000000  /* hardware event, such as exposure start & stop */
#define OGMACAM_FLAG_LIGHTSOURCE         0x0000080000000000  /* embedded light source */
#define OGMACAM_FLAG_FILTERWHEEL         0x0000100000000000  /* astro filter wheel */
#define OGMACAM_FLAG_GIGE                0x0000200000000000  /* 1 Gigabit GigE */
#define OGMACAM_FLAG_10GIGE              0x0000400000000000  /* 10 Gigabit GigE */
#define OGMACAM_FLAG_5GIGE               0x0000800000000000  /* 5 Gigabit GigE */
#define OGMACAM_FLAG_25GIGE              0x0001000000000000  /* 2.5 Gigabit GigE */
#define OGMACAM_FLAG_AUTOFOCUSER         0x0002000000000000  /* astro auto focuser */
#define OGMACAM_FLAG_LIGHT_SOURCE        0x0004000000000000  /* stand alone light source */
#define OGMACAM_FLAG_CAMERALINK          0x0008000000000000  /* camera link */
#define OGMACAM_FLAG_CXP                 0x0010000000000000  /* CXP: CoaXPress */
#define OGMACAM_FLAG_RAW12PACK           0x0020000000000000  /* pixel format, RAW 12bits packed */
#define OGMACAM_FLAG_SELFTRIGGER         0x0040000000000000  /* self trigger */
#define OGMACAM_FLAG_RAW11               0x0080000000000000  /* pixel format, RAW 11bits */
#define OGMACAM_FLAG_GHOPTO              0x0100000000000000  /* ghopto sensor */

#define OGMACAM_EXPOGAIN_DEF             100     /* exposure gain, default value */
#define OGMACAM_EXPOGAIN_MIN             100     /* exposure gain, minimum value */
#define OGMACAM_TEMP_DEF                 6503    /* color temperature, default value */
#define OGMACAM_TEMP_MIN                 2000    /* color temperature, minimum value */
#define OGMACAM_TEMP_MAX                 15000   /* color temperature, maximum value */
#define OGMACAM_TINT_DEF                 1000    /* tint */
#define OGMACAM_TINT_MIN                 200     /* tint */
#define OGMACAM_TINT_MAX                 2500    /* tint */
#define OGMACAM_HUE_DEF                  0       /* hue */
#define OGMACAM_HUE_MIN                  (-180)  /* hue */
#define OGMACAM_HUE_MAX                  180     /* hue */
#define OGMACAM_SATURATION_DEF           128     /* saturation */
#define OGMACAM_SATURATION_MIN           0       /* saturation */
#define OGMACAM_SATURATION_MAX           255     /* saturation */
#define OGMACAM_BRIGHTNESS_DEF           0       /* brightness */
#define OGMACAM_BRIGHTNESS_MIN           (-255)  /* brightness */
#define OGMACAM_BRIGHTNESS_MAX           255     /* brightness */
#define OGMACAM_CONTRAST_DEF             0       /* contrast */
#define OGMACAM_CONTRAST_MIN             (-255)  /* contrast */
#define OGMACAM_CONTRAST_MAX             255     /* contrast */
#define OGMACAM_GAMMA_DEF                100     /* gamma */
#define OGMACAM_GAMMA_MIN                20      /* gamma */
#define OGMACAM_GAMMA_MAX                180     /* gamma */
#define OGMACAM_AETARGET_DEF             120     /* target of auto exposure */
#define OGMACAM_AETARGET_MIN             16      /* target of auto exposure */
#define OGMACAM_AETARGET_MAX             220     /* target of auto exposure */
#define OGMACAM_WBGAIN_DEF               0       /* white balance gain */
#define OGMACAM_WBGAIN_MIN               (-127)  /* white balance gain */
#define OGMACAM_WBGAIN_MAX               127     /* white balance gain */
#define OGMACAM_BLACKLEVEL_MIN           0       /* minimum black level */
#define OGMACAM_BLACKLEVEL8_MAX          31              /* maximum black level for bitdepth = 8 */
#define OGMACAM_BLACKLEVEL10_MAX         (31 * 4)        /* maximum black level for bitdepth = 10 */
#define OGMACAM_BLACKLEVEL11_MAX         (31 * 8)        /* maximum black level for bitdepth = 11 */
#define OGMACAM_BLACKLEVEL12_MAX         (31 * 16)       /* maximum black level for bitdepth = 12 */
#define OGMACAM_BLACKLEVEL14_MAX         (31 * 64)       /* maximum black level for bitdepth = 14 */
#define OGMACAM_BLACKLEVEL16_MAX         (31 * 256)      /* maximum black level for bitdepth = 16 */
#define OGMACAM_SHARPENING_STRENGTH_DEF  0       /* sharpening strength */
#define OGMACAM_SHARPENING_STRENGTH_MIN  0       /* sharpening strength */
#define OGMACAM_SHARPENING_STRENGTH_MAX  500     /* sharpening strength */
#define OGMACAM_SHARPENING_RADIUS_DEF    2       /* sharpening radius */
#define OGMACAM_SHARPENING_RADIUS_MIN    1       /* sharpening radius */
#define OGMACAM_SHARPENING_RADIUS_MAX    10      /* sharpening radius */
#define OGMACAM_SHARPENING_THRESHOLD_DEF 0       /* sharpening threshold */
#define OGMACAM_SHARPENING_THRESHOLD_MIN 0       /* sharpening threshold */
#define OGMACAM_SHARPENING_THRESHOLD_MAX 255     /* sharpening threshold */
#define OGMACAM_AUTOEXPO_THRESHOLD_DEF   5       /* auto exposure threshold */
#define OGMACAM_AUTOEXPO_THRESHOLD_MIN   2       /* auto exposure threshold */
#define OGMACAM_AUTOEXPO_THRESHOLD_MAX   15      /* auto exposure threshold */
#define OGMACAM_AUTOEXPO_DAMP_DEF        0       /* auto exposure damping coefficient: thousandths */
#define OGMACAM_AUTOEXPO_DAMP_MIN        0       /* auto exposure damping coefficient: thousandths */
#define OGMACAM_AUTOEXPO_DAMP_MAX        1000    /* auto exposure damping coefficient: thousandths */
#define OGMACAM_BANDWIDTH_DEF            100     /* bandwidth */
#define OGMACAM_BANDWIDTH_MIN            1       /* bandwidth */
#define OGMACAM_BANDWIDTH_MAX            100     /* bandwidth */
#define OGMACAM_DENOISE_DEF              0       /* denoise */
#define OGMACAM_DENOISE_MIN              0       /* denoise */
#define OGMACAM_DENOISE_MAX              100     /* denoise */
#define OGMACAM_HEARTBEAT_MIN            100     /* millisecond */
#define OGMACAM_HEARTBEAT_MAX            10000   /* millisecond */
#define OGMACAM_AE_PERCENT_MIN           0       /* auto exposure percent; 0 or 100 => full roi average, means "disabled" */
#define OGMACAM_AE_PERCENT_MAX           100
#define OGMACAM_AE_PERCENT_DEF           10      /* auto exposure percent: enabled, percentage = 10% */
#define OGMACAM_NOPACKET_TIMEOUT_MIN     500     /* no packet timeout minimum: 500ms */
#define OGMACAM_NOFRAME_TIMEOUT_MIN      500     /* no frame timeout minimum: 500ms */
#define OGMACAM_DYNAMIC_DEFECT_T1_MIN    0       /* dynamic defect pixel correction, dead pixel ratio: the smaller the dead ratio is, the more stringent the conditions for processing dead pixels are, and fewer pixels will be processed */
#define OGMACAM_DYNAMIC_DEFECT_T1_MAX    100     /* means: 1.0 */
#define OGMACAM_DYNAMIC_DEFECT_T1_DEF    90      /* means: 0.9 */
#define OGMACAM_DYNAMIC_DEFECT_T2_MIN    0       /* dynamic defect pixel correction, hot pixel ratio: the smaller the hot ratio is, the more stringent the conditions for processing hot pixels are, and fewer pixels will be processed */
#define OGMACAM_DYNAMIC_DEFECT_T2_MAX    100
#define OGMACAM_DYNAMIC_DEFECT_T2_DEF    90
#define OGMACAM_HDR_K_MIN                1       /* HDR synthesize */
#define OGMACAM_HDR_K_MAX                25500
#define OGMACAM_HDR_B_MIN                0
#define OGMACAM_HDR_B_MAX                65535
#define OGMACAM_HDR_THRESHOLD_MIN        0
#define OGMACAM_HDR_THRESHOLD_MAX        4094
#define OGMACAM_CDS_MIN                  0       /* Correlated Double Sampling */
#define OGMACAM_CDS_MAX                  100

typedef struct {
    unsigned    width;
    unsigned    height;
} OgmacamResolution;

/* In Windows platform, we always use UNICODE wchar_t */
/* In Linux or macOS, we use char */

typedef struct {
#if defined(_WIN32)
    const wchar_t*      name;        /* model name, in Windows, we use unicode */
#else
    const char*         name;        /* model name */
#endif
    unsigned long long  flag;        /* OGMACAM_FLAG_xxx, 64 bits */
    unsigned            maxspeed;    /* number of speed level, same as Ogmacam_get_MaxSpeed(), speed range = [0, maxspeed], closed interval */
    unsigned            preview;     /* number of preview resolution, same as Ogmacam_get_ResolutionNumber() */
    unsigned            still;       /* number of still resolution, same as Ogmacam_get_StillResolutionNumber() */
    unsigned            maxfanspeed; /* maximum fan speed, fan speed range = [0, max], closed interval */
    unsigned            ioctrol;     /* number of input/output control */
    float               xpixsz;      /* physical pixel size in micrometer */
    float               ypixsz;      /* physical pixel size in micrometer */
    OgmacamResolution   res[16];
} OgmacamModelV2; /* device model v2 */

typedef struct {
#if defined(_WIN32)
    wchar_t               displayname[64];    /* display name: model name or user-defined name (if any and Ogmacam_EnumWithName) */
    wchar_t               id[64];             /* unique and opaque id of a connected camera, for Ogmacam_Open */
#else
    char                  displayname[64];    /* display name: model name or user-defined name (if any and Ogmacam_EnumWithName) */
    char                  id[64];             /* unique and opaque id of a connected camera, for Ogmacam_Open */
#endif
    const OgmacamModelV2* model;
} OgmacamDeviceV2; /* device instance for enumerating */

/*
    get the version of this dll/so/dylib, which is: 57.26598.20240928
*/
#if defined(_WIN32)
OGMACAM_API(const wchar_t*)   Ogmacam_Version();
#else
OGMACAM_API(const char*)      Ogmacam_Version();
#endif

/*
    enumerate the cameras connected to the computer, return the number of enumerated.

    OgmacamDeviceV2 arr[OGMACAM_MAX];
    unsigned cnt = Ogmacam_EnumV2(arr);
    for (unsigned i = 0; i < cnt; ++i)
        ...

    if arr == NULL, then, only the number is returned.
    Ogmacam_Enum is obsolete.
*/
OGMACAM_API(unsigned) Ogmacam_EnumV2(OgmacamDeviceV2 arr[OGMACAM_MAX]);

/* use the camId of OgmacamDeviceV2, which is enumerated by Ogmacam_EnumV2.
    if camId is NULL, Ogmacam_Open will open the first enumerated camera.
    For the issue of opening the camera on Android, please refer to the documentation
*/
#if defined(_WIN32)
OGMACAM_API(HOgmacam) Ogmacam_Open(const wchar_t* camId);
#else
OGMACAM_API(HOgmacam) Ogmacam_Open(const char* camId);
#endif

/*
    the same with Ogmacam_Open, but use the index as the parameter. such as:
    index == 0, open the first camera,
    index == 1, open the second camera,
    etc
*/
OGMACAM_API(HOgmacam) Ogmacam_OpenByIndex(unsigned index);

/* close the handle. After it is closed, never use the handle any more. */
OGMACAM_API(void)     Ogmacam_Close(HOgmacam h);

#define OGMACAM_EVENT_EXPOSURE          0x0001    /* exposure time or gain changed */
#define OGMACAM_EVENT_TEMPTINT          0x0002    /* white balance changed, Temp/Tint mode */
#define OGMACAM_EVENT_IMAGE             0x0004    /* live image arrived, use Ogmacam_PullImageXXXX to get this image */
#define OGMACAM_EVENT_STILLIMAGE        0x0005    /* snap (still) frame arrived, use Ogmacam_PullStillImageXXXX to get this frame */
#define OGMACAM_EVENT_WBGAIN            0x0006    /* white balance changed, RGB Gain mode */
#define OGMACAM_EVENT_TRIGGERFAIL       0x0007    /* trigger failed */
#define OGMACAM_EVENT_BLACK             0x0008    /* black balance changed */
#define OGMACAM_EVENT_FFC               0x0009    /* flat field correction status changed */
#define OGMACAM_EVENT_DFC               0x000a    /* dark field correction status changed */
#define OGMACAM_EVENT_ROI               0x000b    /* roi changed */
#define OGMACAM_EVENT_LEVELRANGE        0x000c    /* level range changed */
#define OGMACAM_EVENT_AUTOEXPO_CONV     0x000d    /* auto exposure convergence */
#define OGMACAM_EVENT_AUTOEXPO_CONVFAIL 0x000e    /* auto exposure once mode convergence failed */
#define OGMACAM_EVENT_FPNC              0x000f    /* fix pattern noise correction status changed */
#define OGMACAM_EVENT_ERROR             0x0080    /* generic error */
#define OGMACAM_EVENT_DISCONNECTED      0x0081    /* camera disconnected */
#define OGMACAM_EVENT_NOFRAMETIMEOUT    0x0082    /* no frame timeout error */
#define OGMACAM_EVENT_FOCUSPOS          0x0084    /* focus positon */
#define OGMACAM_EVENT_NOPACKETTIMEOUT   0x0085    /* no packet timeout */
#define OGMACAM_EVENT_EXPO_START        0x4000    /* hardware event: exposure start */
#define OGMACAM_EVENT_EXPO_STOP         0x4001    /* hardware event: exposure stop */
#define OGMACAM_EVENT_TRIGGER_ALLOW     0x4002    /* hardware event: next trigger allow */
#define OGMACAM_EVENT_HEARTBEAT         0x4003    /* hardware event: heartbeat, can be used to monitor whether the camera is alive */
#define OGMACAM_EVENT_TRIGGER_IN        0x4004    /* hardware event: trigger in */
#define OGMACAM_EVENT_FACTORY           0x8001    /* restore factory settings */

#if defined(_WIN32)
OGMACAM_API(HRESULT)  Ogmacam_StartPullModeWithWndMsg(HOgmacam h, HWND hWnd, UINT nMsg);
#endif

/* Do NOT call Ogmacam_Close, Ogmacam_Stop in this callback context, it deadlocks. */
/* Do NOT call Ogmacam_put_Option with OGMACAM_OPTION_TRIGGER, OGMACAM_OPTION_BITDEPTH, OGMACAM_OPTION_PIXEL_FORMAT, OGMACAM_OPTION_BINNING, OGMACAM_OPTION_ROTATE, it will fail with error code E_WRONG_THREAD */
typedef void (__stdcall* POGMACAM_EVENT_CALLBACK)(unsigned nEvent, void* ctxEvent);
OGMACAM_API(HRESULT)  Ogmacam_StartPullModeWithCallback(HOgmacam h, POGMACAM_EVENT_CALLBACK funEvent, void* ctxEvent);

#define OGMACAM_FRAMEINFO_FLAG_SEQ          0x00000001 /* frame sequence number */
#define OGMACAM_FRAMEINFO_FLAG_TIMESTAMP    0x00000002 /* timestamp */
#define OGMACAM_FRAMEINFO_FLAG_EXPOTIME     0x00000004 /* exposure time */
#define OGMACAM_FRAMEINFO_FLAG_EXPOGAIN     0x00000008 /* exposure gain */
#define OGMACAM_FRAMEINFO_FLAG_BLACKLEVEL   0x00000010 /* black level */
#define OGMACAM_FRAMEINFO_FLAG_SHUTTERSEQ   0x00000020 /* sequence shutter counter */
#define OGMACAM_FRAMEINFO_FLAG_GPS          0x00000040 /* GPS */
#define OGMACAM_FRAMEINFO_FLAG_AUTOFOCUS    0x00000080 /* auto focus: uLum & uFV */
#define OGMACAM_FRAMEINFO_FLAG_COUNT        0x00000100 /* timecount, framecount, tricount */
#define OGMACAM_FRAMEINFO_FLAG_STILL        0x00008000 /* still image */

typedef struct {
    unsigned            width;
    unsigned            height;
    unsigned            flag;       /* OGMACAM_FRAMEINFO_FLAG_xxxx */
    unsigned            seq;        /* frame sequence number */
    unsigned long long  timestamp;  /* microsecond */
    unsigned            shutterseq; /* sequence shutter counter */
    unsigned            expotime;   /* exposure time */
    unsigned short      expogain;   /* exposure gain */
    unsigned short      blacklevel; /* black level */
} OgmacamFrameInfoV3;

typedef struct {
    unsigned long long utcstart;    /* exposure start time: nanosecond since epoch (00:00:00 UTC on Thursday, 1 January 1970, see https://en.wikipedia.org/wiki/Unix_time) */
    unsigned long long utcend;      /* exposure end time */
    int                longitude;   /* millionth of a degree, 0.000001 degree */
    int                latitude;
    int                altitude;    /* millimeter */
    unsigned short     satellite;   /* number of satellite */
    unsigned short     reserved;    /* not used */
} OgmacamGps;

typedef struct {
    OgmacamFrameInfoV3 v3;
    unsigned reserved; /* not used */
    unsigned uLum;
    unsigned long long uFV;
    unsigned long long timecount;
    unsigned framecount, tricount;
    OgmacamGps gps;
} OgmacamFrameInfoV4;

/*
    nWaitMS: The timeout interval, in milliseconds. If a nonzero value is specified, the function waits until the image is ok or the interval elapses.
             If nWaitMS is zero, the function does not enter a wait state if the image is not available; it always returns immediately; this is equal to Ogmacam_PullImageV4.
    bStill: to pull still image, set to 1, otherwise 0
    bits: 24 (RGB24), 32 (RGB32), 48 (RGB48), 8 (Grey), 16 (Grey), 64 (RGB64).
          In RAW mode, this parameter is ignored.
          bits = 0 means using default bits base on OGMACAM_OPTION_RGB.
          When bits and OGMACAM_OPTION_RGB are inconsistent, format conversion will have to be performed, resulting in loss of efficiency.
          See the following bits and OGMACAM_OPTION_RGB correspondence table:
            ----------------------------------------------------------------------------------------------------------------------
            | OGMACAM_OPTION_RGB |   0 (RGB24)   |   1 (RGB48)   |   2 (RGB32)   |   3 (Grey8)   |  4 (Grey16)   |   5 (RGB64)   |
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
OGMACAM_API(HRESULT)  Ogmacam_PullImageV4(HOgmacam h, void* pImageData, int bStill, int bits, int rowPitch, OgmacamFrameInfoV4* pInfo);
OGMACAM_API(HRESULT)  Ogmacam_WaitImageV4(HOgmacam h, unsigned nWaitMS, void* pImageData, int bStill, int bits, int rowPitch, OgmacamFrameInfoV4* pInfo);
OGMACAM_API(HRESULT)  Ogmacam_PullImageV3(HOgmacam h, void* pImageData, int bStill, int bits, int rowPitch, OgmacamFrameInfoV3* pInfo);
OGMACAM_API(HRESULT)  Ogmacam_WaitImageV3(HOgmacam h, unsigned nWaitMS, void* pImageData, int bStill, int bits, int rowPitch, OgmacamFrameInfoV3* pInfo);

typedef struct {
    unsigned            width;
    unsigned            height;
    unsigned            flag;       /* OGMACAM_FRAMEINFO_FLAG_xxxx */
    unsigned            seq;        /* frame sequence number */
    unsigned long long  timestamp;  /* microsecond */
} OgmacamFrameInfoV2;

OGMACAM_API(HRESULT)  Ogmacam_PullImageV2(HOgmacam h, void* pImageData, int bits, OgmacamFrameInfoV2* pInfo);
OGMACAM_API(HRESULT)  Ogmacam_PullStillImageV2(HOgmacam h, void* pImageData, int bits, OgmacamFrameInfoV2* pInfo);
OGMACAM_API(HRESULT)  Ogmacam_PullImageWithRowPitchV2(HOgmacam h, void* pImageData, int bits, int rowPitch, OgmacamFrameInfoV2* pInfo);
OGMACAM_API(HRESULT)  Ogmacam_PullStillImageWithRowPitchV2(HOgmacam h, void* pImageData, int bits, int rowPitch, OgmacamFrameInfoV2* pInfo);

OGMACAM_API(HRESULT)  Ogmacam_PullImage(HOgmacam h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);
OGMACAM_API(HRESULT)  Ogmacam_PullStillImage(HOgmacam h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);
OGMACAM_API(HRESULT)  Ogmacam_PullImageWithRowPitch(HOgmacam h, void* pImageData, int bits, int rowPitch, unsigned* pnWidth, unsigned* pnHeight);
OGMACAM_API(HRESULT)  Ogmacam_PullStillImageWithRowPitch(HOgmacam h, void* pImageData, int bits, int rowPitch, unsigned* pnWidth, unsigned* pnHeight);

/*
    (NULL == pData) means something error
    ctxData is the callback context which is passed by Ogmacam_StartPushModeV3
    bSnap: TRUE if Ogmacam_Snap

    funData is callbacked by an internal thread of ogmacam.dll, so please pay attention to multithread problem.
    Do NOT call Ogmacam_Close, Ogmacam_Stop in this callback context, it deadlocks.
*/
typedef void (__stdcall* POGMACAM_DATA_CALLBACK_V4)(const void* pData, const OgmacamFrameInfoV3* pInfo, int bSnap, void* ctxData);
OGMACAM_API(HRESULT)  Ogmacam_StartPushModeV4(HOgmacam h, POGMACAM_DATA_CALLBACK_V4 funData, void* ctxData, POGMACAM_EVENT_CALLBACK funEvent, void* ctxEvent);

typedef void (__stdcall* POGMACAM_DATA_CALLBACK_V3)(const void* pData, const OgmacamFrameInfoV2* pInfo, int bSnap, void* ctxData);
OGMACAM_API(HRESULT)  Ogmacam_StartPushModeV3(HOgmacam h, POGMACAM_DATA_CALLBACK_V3 funData, void* ctxData, POGMACAM_EVENT_CALLBACK funEvent, void* ctxEvent);

OGMACAM_API(HRESULT)  Ogmacam_Stop(HOgmacam h);
OGMACAM_API(HRESULT)  Ogmacam_Pause(HOgmacam h, int bPause); /* 1 => pause, 0 => continue */

/*  for pull mode: OGMACAM_EVENT_STILLIMAGE, and then Ogmacam_PullStillImageXXXX/Ogmacam_PullImageV4
    for push mode: the snapped image will be return by POGMACAM_DATA_CALLBACK(V2/V3), with the parameter 'bSnap' set to 'TRUE'
    nResolutionIndex = 0xffffffff means use the cureent preview resolution
*/
OGMACAM_API(HRESULT)  Ogmacam_Snap(HOgmacam h, unsigned nResolutionIndex);  /* still image snap */
OGMACAM_API(HRESULT)  Ogmacam_SnapN(HOgmacam h, unsigned nResolutionIndex, unsigned nNumber);  /* multiple still image snap */
OGMACAM_API(HRESULT)  Ogmacam_SnapR(HOgmacam h, unsigned nResolutionIndex, unsigned nNumber);  /* multiple RAW still image snap */
/*
    soft trigger:
    nNumber:    0xffff:     trigger continuously
                0:          cancel trigger
                others:     number of images to be triggered
*/
OGMACAM_API(HRESULT)  Ogmacam_Trigger(HOgmacam h, unsigned short nNumber);

/*
    trigger synchronously
    nWaitMS:    0:              by default, exposure * 102% + 4000 milliseconds
                0xffffffff:     wait infinite
                other:          milliseconds to wait
*/
OGMACAM_API(HRESULT)  Ogmacam_TriggerSyncV4(HOgmacam h, unsigned nWaitMS, void* pImageData, int bits, int rowPitch, OgmacamFrameInfoV4* pInfo);
OGMACAM_API(HRESULT)  Ogmacam_TriggerSync(HOgmacam h, unsigned nWaitMS, void* pImageData, int bits, int rowPitch, OgmacamFrameInfoV3* pInfo);

/*
    put_Size, put_eSize, can be used to set the video output resolution BEFORE Ogmacam_StartXXXX.
    put_Size use width and height parameters, put_eSize use the index parameter.
    for example, UCMOS03100KPA support the following resolutions:
            index 0:    2048,   1536
            index 1:    1024,   768
            index 2:    680,    510
    so, we can use put_Size(h, 1024, 768) or put_eSize(h, 1). Both have the same effect.
*/
OGMACAM_API(HRESULT)  Ogmacam_put_Size(HOgmacam h, int nWidth, int nHeight);
OGMACAM_API(HRESULT)  Ogmacam_get_Size(HOgmacam h, int* pWidth, int* pHeight);
OGMACAM_API(HRESULT)  Ogmacam_put_eSize(HOgmacam h, unsigned nResolutionIndex);
OGMACAM_API(HRESULT)  Ogmacam_get_eSize(HOgmacam h, unsigned* pnResolutionIndex);

/*
    final image size after ROI, rotate, binning
*/
OGMACAM_API(HRESULT)  Ogmacam_get_FinalSize(HOgmacam h, int* pWidth, int* pHeight);

OGMACAM_API(HRESULT)  Ogmacam_get_ResolutionNumber(HOgmacam h);
OGMACAM_API(HRESULT)  Ogmacam_get_Resolution(HOgmacam h, unsigned nResolutionIndex, int* pWidth, int* pHeight);
/*
    numerator/denominator, such as: 1/1, 1/2, 1/3
*/
OGMACAM_API(HRESULT)  Ogmacam_get_ResolutionRatio(HOgmacam h, unsigned nResolutionIndex, int* pNumerator, int* pDenominator);
OGMACAM_API(HRESULT)  Ogmacam_get_Field(HOgmacam h);

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
OGMACAM_API(HRESULT)  Ogmacam_get_RawFormat(HOgmacam h, unsigned* pFourCC, unsigned* pBitsPerPixel);

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

#ifndef __OGMACAM_CALLBACK_DEFINED__
#define __OGMACAM_CALLBACK_DEFINED__
typedef void (__stdcall* PIOGMACAM_EXPOSURE_CALLBACK)(void* ctxExpo);                                 /* auto exposure */
typedef void (__stdcall* PIOGMACAM_WHITEBALANCE_CALLBACK)(const int aGain[3], void* ctxWB);           /* once white balance, RGB Gain mode */
typedef void (__stdcall* PIOGMACAM_BLACKBALANCE_CALLBACK)(const unsigned short aSub[3], void* ctxBB); /* once black balance */
typedef void (__stdcall* PIOGMACAM_TEMPTINT_CALLBACK)(const int nTemp, const int nTint, void* ctxTT); /* once white balance, Temp/Tint Mode */
typedef void (__stdcall* PIOGMACAM_HISTOGRAM_CALLBACK)(const float aHistY[256], const float aHistR[256], const float aHistG[256], const float aHistB[256], void* ctxHistogram);
typedef void (__stdcall* PIOGMACAM_CHROME_CALLBACK)(void* ctxChrome);
typedef void (__stdcall* PIOGMACAM_PROGRESS)(int percent, void* ctxProgress);
#endif

/*
* nFlag & 0x00008000: mono or color
* nFlag & 0x0f: bitdepth
* so the size of aHist is:
    int arraySize = 1 << (nFlag & 0x0f);
    if ((nFlag & 0x00008000) == 0)
        arraySize *= 3;
*/
typedef void (__stdcall* PIOGMACAM_HISTOGRAM_CALLBACKV2)(const unsigned* aHist, unsigned nFlag, void* ctxHistogramV2);

/*
* bAutoExposure:
*   0: disable auto exposure
*   1: auto exposure continue mode
*   2: auto exposure once mode
*/
OGMACAM_API(HRESULT)  Ogmacam_get_AutoExpoEnable(HOgmacam h, int* bAutoExposure);
OGMACAM_API(HRESULT)  Ogmacam_put_AutoExpoEnable(HOgmacam h, int bAutoExposure);

OGMACAM_API(HRESULT)  Ogmacam_get_AutoExpoTarget(HOgmacam h, unsigned short* Target);
OGMACAM_API(HRESULT)  Ogmacam_put_AutoExpoTarget(HOgmacam h, unsigned short Target);

/* set the maximum/minimal auto exposure time and agin. The default maximum auto exposure time is 350ms */
OGMACAM_API(HRESULT)  Ogmacam_put_AutoExpoRange(HOgmacam h, unsigned maxTime, unsigned minTime, unsigned short maxGain, unsigned short minGain);
OGMACAM_API(HRESULT)  Ogmacam_get_AutoExpoRange(HOgmacam h, unsigned* maxTime, unsigned* minTime, unsigned short* maxGain, unsigned short* minGain);
OGMACAM_API(HRESULT)  Ogmacam_put_MaxAutoExpoTimeAGain(HOgmacam h, unsigned maxTime, unsigned short maxGain);
OGMACAM_API(HRESULT)  Ogmacam_get_MaxAutoExpoTimeAGain(HOgmacam h, unsigned* maxTime, unsigned short* maxGain);
OGMACAM_API(HRESULT)  Ogmacam_put_MinAutoExpoTimeAGain(HOgmacam h, unsigned minTime, unsigned short minGain);
OGMACAM_API(HRESULT)  Ogmacam_get_MinAutoExpoTimeAGain(HOgmacam h, unsigned* minTime, unsigned short* minGain);

OGMACAM_API(HRESULT)  Ogmacam_get_ExpoTime(HOgmacam h, unsigned* Time); /* in microseconds */
OGMACAM_API(HRESULT)  Ogmacam_put_ExpoTime(HOgmacam h, unsigned Time); /* in microseconds */
OGMACAM_API(HRESULT)  Ogmacam_get_RealExpoTime(HOgmacam h, unsigned* Time); /* in microseconds, based on 50HZ/60HZ/DC */
OGMACAM_API(HRESULT)  Ogmacam_get_ExpTimeRange(HOgmacam h, unsigned* nMin, unsigned* nMax, unsigned* nDef);

OGMACAM_API(HRESULT)  Ogmacam_get_ExpoAGain(HOgmacam h, unsigned short* Gain); /* percent, such as 300 */
OGMACAM_API(HRESULT)  Ogmacam_put_ExpoAGain(HOgmacam h, unsigned short Gain); /* percent */
OGMACAM_API(HRESULT)  Ogmacam_get_ExpoAGainRange(HOgmacam h, unsigned short* nMin, unsigned short* nMax, unsigned short* nDef);

/* Auto White Balance "Once", Temp/Tint Mode */
OGMACAM_API(HRESULT)  Ogmacam_AwbOnce(HOgmacam h, PIOGMACAM_TEMPTINT_CALLBACK funTT, void* ctxTT); /* auto white balance "once". This function must be called AFTER Ogmacam_StartXXXX */

/* Auto White Balance "Once", RGB Gain Mode */
OGMACAM_API(HRESULT)  Ogmacam_AwbInit(HOgmacam h, PIOGMACAM_WHITEBALANCE_CALLBACK funWB, void* ctxWB);

/* White Balance, Temp/Tint mode */
OGMACAM_API(HRESULT)  Ogmacam_put_TempTint(HOgmacam h, int nTemp, int nTint);
OGMACAM_API(HRESULT)  Ogmacam_get_TempTint(HOgmacam h, int* nTemp, int* nTint);

/* White Balance, RGB Gain mode */
OGMACAM_API(HRESULT)  Ogmacam_put_WhiteBalanceGain(HOgmacam h, int aGain[3]);
OGMACAM_API(HRESULT)  Ogmacam_get_WhiteBalanceGain(HOgmacam h, int aGain[3]);

/* Black Balance */
OGMACAM_API(HRESULT)  Ogmacam_AbbOnce(HOgmacam h, PIOGMACAM_BLACKBALANCE_CALLBACK funBB, void* ctxBB); /* auto black balance "once". This function must be called AFTER Ogmacam_StartXXXX */
OGMACAM_API(HRESULT)  Ogmacam_put_BlackBalance(HOgmacam h, unsigned short aSub[3]);
OGMACAM_API(HRESULT)  Ogmacam_get_BlackBalance(HOgmacam h, unsigned short aSub[3]);

/* Flat Field Correction */
OGMACAM_API(HRESULT)  Ogmacam_FfcOnce(HOgmacam h);
#if defined(_WIN32)
OGMACAM_API(HRESULT)  Ogmacam_FfcExport(HOgmacam h, const wchar_t* filePath);
OGMACAM_API(HRESULT)  Ogmacam_FfcImport(HOgmacam h, const wchar_t* filePath);
#else
OGMACAM_API(HRESULT)  Ogmacam_FfcExport(HOgmacam h, const char* filePath);
OGMACAM_API(HRESULT)  Ogmacam_FfcImport(HOgmacam h, const char* filePath);
#endif

/* Dark Field Correction */
OGMACAM_API(HRESULT)  Ogmacam_DfcOnce(HOgmacam h);

#if defined(_WIN32)
OGMACAM_API(HRESULT)  Ogmacam_DfcExport(HOgmacam h, const wchar_t* filePath);
OGMACAM_API(HRESULT)  Ogmacam_DfcImport(HOgmacam h, const wchar_t* filePath);
#else
OGMACAM_API(HRESULT)  Ogmacam_DfcExport(HOgmacam h, const char* filePath);
OGMACAM_API(HRESULT)  Ogmacam_DfcImport(HOgmacam h, const char* filePath);
#endif

/* Fix Pattern Noise Correction */
OGMACAM_API(HRESULT)  Ogmacam_FpncOnce(HOgmacam h);

#if defined(_WIN32)
OGMACAM_API(HRESULT)  Ogmacam_FpncExport(HOgmacam h, const wchar_t* filePath);
OGMACAM_API(HRESULT)  Ogmacam_FpncImport(HOgmacam h, const wchar_t* filePath);
#else
OGMACAM_API(HRESULT)  Ogmacam_FpncExport(HOgmacam h, const char* filePath);
OGMACAM_API(HRESULT)  Ogmacam_FpncImport(HOgmacam h, const char* filePath);
#endif

OGMACAM_API(HRESULT)  Ogmacam_put_Hue(HOgmacam h, int Hue);
OGMACAM_API(HRESULT)  Ogmacam_get_Hue(HOgmacam h, int* Hue);
OGMACAM_API(HRESULT)  Ogmacam_put_Saturation(HOgmacam h, int Saturation);
OGMACAM_API(HRESULT)  Ogmacam_get_Saturation(HOgmacam h, int* Saturation);
OGMACAM_API(HRESULT)  Ogmacam_put_Brightness(HOgmacam h, int Brightness);
OGMACAM_API(HRESULT)  Ogmacam_get_Brightness(HOgmacam h, int* Brightness);
OGMACAM_API(HRESULT)  Ogmacam_get_Contrast(HOgmacam h, int* Contrast);
OGMACAM_API(HRESULT)  Ogmacam_put_Contrast(HOgmacam h, int Contrast);
OGMACAM_API(HRESULT)  Ogmacam_get_Gamma(HOgmacam h, int* Gamma); /* percent */
OGMACAM_API(HRESULT)  Ogmacam_put_Gamma(HOgmacam h, int Gamma);  /* percent */

OGMACAM_API(HRESULT)  Ogmacam_get_Chrome(HOgmacam h, int* bChrome);  /* 1 => monochromatic mode, 0 => color mode */
OGMACAM_API(HRESULT)  Ogmacam_put_Chrome(HOgmacam h, int bChrome);

OGMACAM_API(HRESULT)  Ogmacam_get_VFlip(HOgmacam h, int* bVFlip);  /* vertical flip */
OGMACAM_API(HRESULT)  Ogmacam_put_VFlip(HOgmacam h, int bVFlip);
OGMACAM_API(HRESULT)  Ogmacam_get_HFlip(HOgmacam h, int* bHFlip);
OGMACAM_API(HRESULT)  Ogmacam_put_HFlip(HOgmacam h, int bHFlip); /* horizontal flip */

OGMACAM_API(HRESULT)  Ogmacam_get_Negative(HOgmacam h, int* bNegative);  /* negative film */
OGMACAM_API(HRESULT)  Ogmacam_put_Negative(HOgmacam h, int bNegative);

OGMACAM_API(HRESULT)  Ogmacam_put_Speed(HOgmacam h, unsigned short nSpeed);
OGMACAM_API(HRESULT)  Ogmacam_get_Speed(HOgmacam h, unsigned short* pSpeed);
OGMACAM_API(HRESULT)  Ogmacam_get_MaxSpeed(HOgmacam h); /* get the maximum speed, see "Frame Speed Level", the speed range = [0, max], closed interval */

OGMACAM_API(HRESULT)  Ogmacam_get_FanMaxSpeed(HOgmacam h); /* get the maximum fan speed, the fan speed range = [0, max], closed interval */

OGMACAM_API(HRESULT)  Ogmacam_get_MaxBitDepth(HOgmacam h); /* get the max bitdepth of this camera, such as 8, 10, 12, 14, 16 */

/* power supply of lighting:
        0 => 60HZ AC
        1 => 50Hz AC
        2 => DC
*/
OGMACAM_API(HRESULT)  Ogmacam_put_HZ(HOgmacam h, int nHZ);
OGMACAM_API(HRESULT)  Ogmacam_get_HZ(HOgmacam h, int* nHZ);

OGMACAM_API(HRESULT)  Ogmacam_put_Mode(HOgmacam h, int bSkip); /* skip or bin */
OGMACAM_API(HRESULT)  Ogmacam_get_Mode(HOgmacam h, int* bSkip); /* If the model don't support bin/skip mode, return E_NOTIMPL */

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

OGMACAM_API(HRESULT)  Ogmacam_put_AWBAuxRect(HOgmacam h, const RECT* pAuxRect); /* auto white balance ROI */
OGMACAM_API(HRESULT)  Ogmacam_get_AWBAuxRect(HOgmacam h, RECT* pAuxRect);
OGMACAM_API(HRESULT)  Ogmacam_put_AEAuxRect(HOgmacam h, const RECT* pAuxRect);  /* auto exposure ROI */
OGMACAM_API(HRESULT)  Ogmacam_get_AEAuxRect(HOgmacam h, RECT* pAuxRect);

OGMACAM_API(HRESULT)  Ogmacam_put_ABBAuxRect(HOgmacam h, const RECT* pAuxRect); /* auto black balance ROI */
OGMACAM_API(HRESULT)  Ogmacam_get_ABBAuxRect(HOgmacam h, RECT* pAuxRect);

/*
    S_FALSE:    color mode
    S_OK:       mono mode, such as EXCCD00300KMA and UHCCD01400KMA
*/
OGMACAM_API(HRESULT)  Ogmacam_get_MonoMode(HOgmacam h);

OGMACAM_API(HRESULT)  Ogmacam_get_StillResolutionNumber(HOgmacam h);
OGMACAM_API(HRESULT)  Ogmacam_get_StillResolution(HOgmacam h, unsigned nResolutionIndex, int* pWidth, int* pHeight);

/*  0: no realtime
          stop grab frame when frame buffer deque is full, until the frames in the queue are pulled away and the queue is not full
    1: realtime
          use minimum frame buffer. When new frame arrive, drop all the pending frame regardless of whether the frame buffer is full.
          If DDR present, also limit the DDR frame buffer to only one frame.
    2: soft realtime
          Drop the oldest frame when the queue is full and then enqueue the new frame
    default: 0
*/
OGMACAM_API(HRESULT)  Ogmacam_put_RealTime(HOgmacam h, int val);
OGMACAM_API(HRESULT)  Ogmacam_get_RealTime(HOgmacam h, int* val);

/* discard the current internal frame cache.
    If DDR present, also discard the frames in the DDR.
    Ogmacam_Flush is obsolete, recommend using Ogmacam_put_Option(h, OGMACAM_OPTION_FLUSH, 3)
*/
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_Flush(HOgmacam h);

/* get the temperature of the sensor, in 0.1 degrees Celsius (32 means 3.2 degrees Celsius, -35 means -3.5 degree Celsius)
    return E_NOTIMPL if not supported
*/
OGMACAM_API(HRESULT)  Ogmacam_get_Temperature(HOgmacam h, short* pTemperature);

/* set the target temperature of the sensor or TEC, in 0.1 degrees Celsius (32 means 3.2 degrees Celsius, -35 means -3.5 degree Celsius)
    set "-2730" or below means using the default value of this model
    return E_NOTIMPL if not supported
*/
OGMACAM_API(HRESULT)  Ogmacam_put_Temperature(HOgmacam h, short nTemperature);

/*
    get the revision
*/
OGMACAM_API(HRESULT)  Ogmacam_get_Revision(HOgmacam h, unsigned short* pRevision);

/*
    get the serial number which is always 32 chars which is zero-terminated such as "TP110826145730ABCD1234FEDC56787"
*/
OGMACAM_API(HRESULT)  Ogmacam_get_SerialNumber(HOgmacam h, char sn[32]);

/*
    get the camera firmware version, such as: 3.2.1.20140922
*/
OGMACAM_API(HRESULT)  Ogmacam_get_FwVersion(HOgmacam h, char fwver[16]);

/*
    get the camera hardware version, such as: 3.12
*/
OGMACAM_API(HRESULT)  Ogmacam_get_HwVersion(HOgmacam h, char hwver[16]);

/*
    get the production date, such as: 20150327, YYYYMMDD, (YYYY: year, MM: month, DD: day)
*/
OGMACAM_API(HRESULT)  Ogmacam_get_ProductionDate(HOgmacam h, char pdate[10]);

/*
    get the FPGA version, such as: 1.13
*/
OGMACAM_API(HRESULT)  Ogmacam_get_FpgaVersion(HOgmacam h, char fpgaver[16]);

/*
    get the sensor pixel size, such as: 2.4um x 2.4um
*/
OGMACAM_API(HRESULT)  Ogmacam_get_PixelSize(HOgmacam h, unsigned nResolutionIndex, float* x, float* y);

/* software level range */
OGMACAM_API(HRESULT)  Ogmacam_put_LevelRange(HOgmacam h, unsigned short aLow[4], unsigned short aHigh[4]);
OGMACAM_API(HRESULT)  Ogmacam_get_LevelRange(HOgmacam h, unsigned short aLow[4], unsigned short aHigh[4]);

/* hardware level range mode */
#define OGMACAM_LEVELRANGE_MANUAL       0x0000  /* manual */
#define OGMACAM_LEVELRANGE_ONCE         0x0001  /* once */
#define OGMACAM_LEVELRANGE_CONTINUE     0x0002  /* continue */
#define OGMACAM_LEVELRANGE_ROI          0xffff  /* update roi rect only */
OGMACAM_API(HRESULT)  Ogmacam_put_LevelRangeV2(HOgmacam h, unsigned short mode, const RECT* pRoiRect, unsigned short aLow[4], unsigned short aHigh[4]);
OGMACAM_API(HRESULT)  Ogmacam_get_LevelRangeV2(HOgmacam h, unsigned short* pMode, RECT* pRoiRect, unsigned short aLow[4], unsigned short aHigh[4]);

/*
    The following functions must be called AFTER Ogmacam_StartPushMode or Ogmacam_StartPullModeWithWndMsg or Ogmacam_StartPullModeWithCallback
*/
OGMACAM_API(HRESULT)  Ogmacam_LevelRangeAuto(HOgmacam h);  /* software level range */
OGMACAM_API(HRESULT)  Ogmacam_GetHistogram(HOgmacam h, PIOGMACAM_HISTOGRAM_CALLBACK funHistogram, void* ctxHistogram);
OGMACAM_API(HRESULT)  Ogmacam_GetHistogramV2(HOgmacam h, PIOGMACAM_HISTOGRAM_CALLBACKV2 funHistogramV2, void* ctxHistogramV2);

/* led state:
    iLed: Led index, (0, 1, 2, ...)
    iState: 1 => Ever bright; 2 => Flashing; other => Off
    iPeriod: Flashing Period (>= 500ms)
*/
OGMACAM_API(HRESULT)  Ogmacam_put_LEDState(HOgmacam h, unsigned short iLed, unsigned short iState, unsigned short iPeriod);

OGMACAM_API(HRESULT)  Ogmacam_write_EEPROM(HOgmacam h, unsigned addr, const unsigned char* pBuffer, unsigned nBufferLen);
OGMACAM_API(HRESULT)  Ogmacam_read_EEPROM(HOgmacam h, unsigned addr, unsigned char* pBuffer, unsigned nBufferLen);

OGMACAM_API(HRESULT)  Ogmacam_read_Pipe(HOgmacam h, unsigned pipeId, void* pBuffer, unsigned nBufferLen);
OGMACAM_API(HRESULT)  Ogmacam_write_Pipe(HOgmacam h, unsigned pipeId, const void* pBuffer, unsigned nBufferLen);
OGMACAM_API(HRESULT)  Ogmacam_feed_Pipe(HOgmacam h, unsigned pipeId);

#define OGMACAM_OPTION_NOFRAME_TIMEOUT        0x01       /* no frame timeout: 0 => disable, positive value (>= OGMACAM_NOFRAME_TIMEOUT_MIN) => timeout milliseconds. default: disable */
#define OGMACAM_OPTION_THREAD_PRIORITY        0x02       /* set the priority of the internal thread which grab data from the usb device.
                                                             Win: iValue: 0 => THREAD_PRIORITY_NORMAL; 1 => THREAD_PRIORITY_ABOVE_NORMAL; 2 => THREAD_PRIORITY_HIGHEST; 3 => THREAD_PRIORITY_TIME_CRITICAL; default: 1; see: https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadpriority
                                                             Linux & macOS: The high 16 bits for the scheduling policy, and the low 16 bits for the priority; see: https://linux.die.net/man/3/pthread_setschedparam
                                                         */
#define OGMACAM_OPTION_PROCESSMODE            0x03       /* obsolete & useless, noop. 0 = better image quality, more cpu usage. this is the default value; 1 = lower image quality, less cpu usage */
#define OGMACAM_OPTION_RAW                    0x04       /* raw data mode, read the sensor "raw" data. This can be set only while camea is NOT running. 0 = rgb, 1 = raw, default value: 0 */
#define OGMACAM_OPTION_HISTOGRAM              0x05       /* 0 = only one, 1 = continue mode */
#define OGMACAM_OPTION_BITDEPTH               0x06       /* 0 = 8 bits mode, 1 = 16 bits mode, subset of OGMACAM_OPTION_PIXEL_FORMAT */
#define OGMACAM_OPTION_FAN                    0x07       /* 0 = turn off the cooling fan, [1, max] = fan speed, , set to "-1" means to use default fan speed */
#define OGMACAM_OPTION_TEC                    0x08       /* 0 = turn off the thermoelectric cooler, 1 = turn on the thermoelectric cooler */
#define OGMACAM_OPTION_LINEAR                 0x09       /* 0 = turn off the builtin linear tone mapping, 1 = turn on the builtin linear tone mapping, default value: 1 */
#define OGMACAM_OPTION_CURVE                  0x0a       /* 0 = turn off the builtin curve tone mapping, 1 = turn on the builtin polynomial curve tone mapping, 2 = logarithmic curve tone mapping, default value: 2 */
#define OGMACAM_OPTION_TRIGGER                0x0b       /* 0 = video mode, 1 = software or simulated trigger mode, 2 = external trigger mode, 3 = external + software trigger, 4 = self trigger, default value = 0 */
#define OGMACAM_OPTION_RGB                    0x0c       /* 0 => RGB24; 1 => enable RGB48 format when bitdepth > 8; 2 => RGB32; 3 => 8 Bits Grey (only for mono camera); 4 => 16 Bits Grey (only for mono camera when bitdepth > 8); 5 => 64(RGB64) */
#define OGMACAM_OPTION_COLORMATIX             0x0d       /* enable or disable the builtin color matrix, default value: 1 */
#define OGMACAM_OPTION_WBGAIN                 0x0e       /* enable or disable the builtin white balance gain, default value: 1 */
#define OGMACAM_OPTION_TECTARGET              0x0f       /* get or set the target temperature of the thermoelectric cooler, in 0.1 degree Celsius. For example, 125 means 12.5 degree Celsius, -35 means -3.5 degree Celsius. Set "-2730" or below means using the default for that model */
#define OGMACAM_OPTION_AUTOEXP_POLICY         0x10       /* auto exposure policy:
                                                             0: Exposure Only
                                                             1: Exposure Preferred
                                                             2: Gain Only
                                                             3: Gain Preferred
                                                             default value: 1
                                                         */
#define OGMACAM_OPTION_FRAMERATE              0x11       /* limit the frame rate, the default value 0 means no limit */
#define OGMACAM_OPTION_DEMOSAIC               0x12       /* demosaic method for both video and still image: BILINEAR = 0, VNG(Variable Number of Gradients) = 1, PPG(Patterned Pixel Grouping) = 2, AHD(Adaptive Homogeneity Directed) = 3, EA(Edge Aware) = 4, see https://en.wikipedia.org/wiki/Demosaicing
                                                              In terms of CPU usage, EA is the lowest, followed by BILINEAR, and the others are higher.
                                                              default value: 0
                                                         */
#define OGMACAM_OPTION_DEMOSAIC_VIDEO         0x13       /* demosaic method for video */
#define OGMACAM_OPTION_DEMOSAIC_STILL         0x14       /* demosaic method for still image */
#define OGMACAM_OPTION_BLACKLEVEL             0x15       /* black level */
#define OGMACAM_OPTION_MULTITHREAD            0x16       /* multithread image processing */
#define OGMACAM_OPTION_BINNING                0x17       /* binning
                                                                0x01: (no binning)
                                                                n: (saturating add, n*n), 0x02(2*2), 0x03(3*3), 0x04(4*4), 0x05(5*5), 0x06(6*6), 0x07(7*7), 0x08(8*8). The Bitdepth of the data remains unchanged.
                                                                0x40 | n: (unsaturated add, n*n, works only in RAW mode), 0x42(2*2), 0x43(3*3), 0x44(4*4), 0x45(5*5), 0x46(6*6), 0x47(7*7), 0x48(8*8). The Bitdepth of the data is increased. For example, the original data with bitdepth of 12 will increase the bitdepth by 2 bits and become 14 after 2*2 binning.
                                                                0x80 | n: (average, n*n), 0x82(2*2), 0x83(3*3), 0x84(4*4), 0x85(5*5), 0x86(6*6), 0x87(7*7), 0x88(8*8). The Bitdepth of the data remains unchanged.
                                                            The final image size is rounded down to an even number, such as 640/3 to get 212
                                                         */
#define OGMACAM_OPTION_ROTATE                 0x18       /* rotate clockwise: 0, 90, 180, 270 */
#define OGMACAM_OPTION_CG                     0x19       /* Conversion Gain:
                                                                0 = LCG
                                                                1 = HCG
                                                                2 = HDR (for camera with flag OGMACAM_FLAG_CGHDR)
                                                                2 = MCG (for camera with flag OGMACAM_FLAG_GHOPTO)
                                                         */
#define OGMACAM_OPTION_PIXEL_FORMAT           0x1a       /* pixel format, OGMACAM_PIXELFORMAT_xxxx */
#define OGMACAM_OPTION_FFC                    0x1b       /* flat field correction
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
#define OGMACAM_OPTION_DDR_DEPTH              0x1c       /* the number of the frames that DDR can cache
                                                                 1: DDR cache only one frame
                                                                 0: Auto:
                                                                         => one for video mode when auto exposure is enabled
                                                                         => full capacity for others
                                                                -1: DDR can cache frames to full capacity
                                                         */
#define OGMACAM_OPTION_DFC                    0x1d       /* dark field correction
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
#define OGMACAM_OPTION_SHARPENING             0x1e       /* Sharpening: (threshold << 24) | (radius << 16) | strength)
                                                             strength: [0, 500], default: 0 (disable)
                                                             radius: [1, 10]
                                                             threshold: [0, 255]
                                                         */
#define OGMACAM_OPTION_FACTORY                0x1f       /* restore the factory settings */
#define OGMACAM_OPTION_TEC_VOLTAGE            0x20       /* get the current TEC voltage in 0.1V, 59 mean 5.9V; readonly */
#define OGMACAM_OPTION_TEC_VOLTAGE_MAX        0x21       /* TEC maximum voltage in 0.1V */
#define OGMACAM_OPTION_DEVICE_RESET           0x22       /* reset usb device, simulate a replug */
#define OGMACAM_OPTION_UPSIDE_DOWN            0x23       /* upsize down:
                                                             1: yes
                                                             0: no
                                                             default: 1 (win), 0 (linux/macos)
                                                         */
#define OGMACAM_OPTION_FOCUSPOS               0x24       /* focus positon */
#define OGMACAM_OPTION_AFMODE                 0x25       /* auto focus mode, see OgmacamAFMode */
#define OGMACAM_OPTION_AFSTATUS               0x27       /* auto focus status, see OgmacamAFStaus */
#define OGMACAM_OPTION_TESTPATTERN            0x28       /* test pattern:
                                                            0: off
                                                            3: monochrome diagonal stripes
                                                            5: monochrome vertical stripes
                                                            7: monochrome horizontal stripes
                                                            9: chromatic diagonal stripes
                                                         */
#define OGMACAM_OPTION_AUTOEXP_THRESHOLD      0x29       /* threshold of auto exposure, default value: 5, range = [2, 15] */
#define OGMACAM_OPTION_BYTEORDER              0x2a       /* Byte order, BGR or RGB: 0 => RGB, 1 => BGR, default value: 1(Win), 0(macOS, Linux, Android) */
#define OGMACAM_OPTION_NOPACKET_TIMEOUT       0x2b       /* no packet timeout: 0 => disable, positive value (>= OGMACAM_NOPACKET_TIMEOUT_MIN) => timeout milliseconds. default: disable */
#define OGMACAM_OPTION_MAX_PRECISE_FRAMERATE  0x2c       /* get the precise frame rate maximum value in 0.1 fps, such as 115 means 11.5 fps */
#define OGMACAM_OPTION_PRECISE_FRAMERATE      0x2d       /* precise frame rate current value in 0.1 fps. use OGMACAM_OPTION_MAX_PRECISE_FRAMERATE, OGMACAM_OPTION_MIN_PRECISE_FRAMERATE to get the range. if the set value is out of range, E_INVALIDARG will be returned */
#define OGMACAM_OPTION_BANDWIDTH              0x2e       /* bandwidth, [1-100]% */
#define OGMACAM_OPTION_RELOAD                 0x2f       /* reload the last frame in trigger mode */
#define OGMACAM_OPTION_CALLBACK_THREAD        0x30       /* dedicated thread for callback: 0 => disable, 1 => enable, default: 0 */
#define OGMACAM_OPTION_FRONTEND_DEQUE_LENGTH  0x31       /* frontend (raw) frame buffer deque length, range: [2, 1024], default: 4
                                                            All the memory will be pre-allocated when the camera starts, so, please attention to memory usage
                                                         */
#define OGMACAM_OPTION_FRAME_DEQUE_LENGTH     0x31       /* alias of OGMACAM_OPTION_FRONTEND_DEQUE_LENGTH */
#define OGMACAM_OPTION_MIN_PRECISE_FRAMERATE  0x32       /* get the precise frame rate minimum value in 0.1 fps, such as 15 means 1.5 fps */
#define OGMACAM_OPTION_SEQUENCER_ONOFF        0x33       /* sequencer trigger: on/off */
#define OGMACAM_OPTION_SEQUENCER_NUMBER       0x34       /* sequencer trigger: number, range = [1, 255] */
#define OGMACAM_OPTION_SEQUENCER_EXPOTIME     0x01000000 /* sequencer trigger: exposure time, iOption = OGMACAM_OPTION_SEQUENCER_EXPOTIME | index, iValue = exposure time
                                                             For example, to set the exposure time of the third group to 50ms, call:
                                                                Ogmacam_put_Option(OGMACAM_OPTION_SEQUENCER_EXPOTIME | 3, 50000)
                                                         */
#define OGMACAM_OPTION_SEQUENCER_EXPOGAIN     0x02000000 /* sequencer trigger: exposure gain, iOption = OGMACAM_OPTION_SEQUENCER_EXPOGAIN | index, iValue = gain */
#define OGMACAM_OPTION_DENOISE                0x35       /* denoise, strength range: [0, 100], 0 means disable */
#define OGMACAM_OPTION_HEAT_MAX               0x36       /* get maximum level: heat to prevent fogging up */
#define OGMACAM_OPTION_HEAT                   0x37       /* heat to prevent fogging up */
#define OGMACAM_OPTION_LOW_NOISE              0x38       /* low noise mode (Higher signal noise ratio, lower frame rate): 1 => enable */
#define OGMACAM_OPTION_POWER                  0x39       /* get power consumption, unit: milliwatt */
#define OGMACAM_OPTION_GLOBAL_RESET_MODE      0x3a       /* global reset mode */
#define OGMACAM_OPTION_OPEN_ERRORCODE         0x3b       /* get the open camera error code */
#define OGMACAM_OPTION_FLUSH                  0x3d       /* 1 = hard flush, discard frames cached by camera DDR (if any)
                                                            2 = soft flush, discard frames cached by ogmacam.dll (if any)
                                                            3 = both flush
                                                            Ogmacam_Flush means 'both flush'
                                                            return the number of soft flushed frames if successful, HRESULT if failed
                                                         */
#define OGMACAM_OPTION_NUMBER_DROP_FRAME      0x3e       /* get the number of frames that have been grabbed from the USB but dropped by the software */
#define OGMACAM_OPTION_DUMP_CFG               0x3f       /* 0 = when camera is stopped, do not dump configuration automatically
                                                            1 = when camera is stopped, dump configuration automatically
                                                            -1 = explicitly dump configuration once
                                                            default: 1
                                                         */
#define OGMACAM_OPTION_DEFECT_PIXEL           0x40       /* Defect Pixel Correction: 0 => disable, 1 => enable; default: 1 */
#define OGMACAM_OPTION_BACKEND_DEQUE_LENGTH   0x41       /* backend (pipelined) frame buffer deque length (Only available in pull mode), range: [2, 1024], default: 3
                                                            All the memory will be pre-allocated when the camera starts, so, please attention to memory usage
                                                         */
#define OGMACAM_OPTION_LIGHTSOURCE_MAX        0x42       /* get the light source range, [0 ~ max] */
#define OGMACAM_OPTION_LIGHTSOURCE            0x43       /* light source */
#define OGMACAM_OPTION_HEARTBEAT              0x44       /* Heartbeat interval in millisecond, range = [OGMACAM_HEARTBEAT_MIN, OGMACAM_HEARTBEAT_MAX], 0 = disable, default: disable */
#define OGMACAM_OPTION_FRONTEND_DEQUE_CURRENT 0x45       /* get the current number in frontend deque */
#define OGMACAM_OPTION_BACKEND_DEQUE_CURRENT  0x46       /* get the current number in backend deque */
#define OGMACAM_OPTION_EVENT_HARDWARE         0x04000000 /* enable or disable hardware event: 0 => disable, 1 => enable; default: disable
                                                                (1) iOption = OGMACAM_OPTION_EVENT_HARDWARE, master switch for notification of all hardware events
                                                                (2) iOption = OGMACAM_OPTION_EVENT_HARDWARE | (event type), a specific type of sub-switch
                                                            Only if both the master switch and the sub-switch of a particular type remain on are actually enabled for that type of event notification.
                                                         */
#define OGMACAM_OPTION_PACKET_NUMBER          0x47       /* get the received packet number */
#define OGMACAM_OPTION_FILTERWHEEL_SLOT       0x48       /* filter wheel slot number */
#define OGMACAM_OPTION_FILTERWHEEL_POSITION   0x49       /* filter wheel position:
                                                                set:
                                                                    -1: reset
                                                                    val & 0xff: position between 0 and N-1, where N is the number of filter slots
                                                                    (val >> 8) & 0x1: direction, 0 => clockwise spinning, 1 => auto direction spinning
                                                                get:
                                                                     -1: in motion
                                                                     val: position arrived
                                                         */
#define OGMACAM_OPTION_AUTOEXPOSURE_PERCENT   0x4a       /* auto exposure percent to average:
                                                                1~99: peak percent average
                                                                0 or 100: full roi average, means "disabled"
                                                         */
#define OGMACAM_OPTION_ANTI_SHUTTER_EFFECT    0x4b       /* anti shutter effect: 1 => disable, 0 => disable; default: 0 */
#define OGMACAM_OPTION_CHAMBER_HT             0x4c       /* get chamber humidity & temperature:
                                                                high 16 bits: humidity, in 0.1%, such as: 325 means humidity is 32.5%
                                                                low 16 bits: temperature, in 0.1 degrees Celsius, such as: 32 means 3.2 degrees Celsius
                                                         */
#define OGMACAM_OPTION_ENV_HT                 0x4d       /* get environment humidity & temperature */
#define OGMACAM_OPTION_EXPOSURE_PRE_DELAY     0x4e       /* exposure signal pre-delay, microsecond */
#define OGMACAM_OPTION_EXPOSURE_POST_DELAY    0x4f       /* exposure signal post-delay, microsecond */
#define OGMACAM_OPTION_AUTOEXPO_CONV          0x50       /* get auto exposure convergence status: 1(YES) or 0(NO), -1(NA) */
#define OGMACAM_OPTION_AUTOEXPO_TRIGGER       0x51       /* auto exposure on trigger mode: 0 => disable, 1 => enable; default: 0 */
#define OGMACAM_OPTION_LINE_PRE_DELAY         0x52       /* specified line signal pre-delay, microsecond */
#define OGMACAM_OPTION_LINE_POST_DELAY        0x53       /* specified line signal post-delay, microsecond */
#define OGMACAM_OPTION_TEC_VOLTAGE_MAX_RANGE  0x54       /* get the tec maximum voltage range:
                                                                high 16 bits: max
                                                                low 16 bits: min
                                                         */
#define OGMACAM_OPTION_HIGH_FULLWELL          0x55       /* high fullwell capacity: 0 => disable, 1 => enable */
#define OGMACAM_OPTION_DYNAMIC_DEFECT         0x56       /* dynamic defect pixel correction:
                                                                dead pixel ratio, t1: (high 16 bits): [0, 100], means: [0.0, 1.0]
                                                                hot pixel ratio, t2: (low 16 bits): [0, 100], means: [0.0, 1.0]
                                                         */
#define OGMACAM_OPTION_HDR_KB                 0x57       /* HDR synthesize
                                                                K (high 16 bits): [1, 25500]
                                                                B (low 16 bits): [0, 65535]
                                                                0xffffffff => set to default
                                                         */
#define OGMACAM_OPTION_HDR_THRESHOLD          0x58       /* HDR synthesize
                                                                threshold: [1, 4094]
                                                                0xffffffff => set to default
                                                         */
#define OGMACAM_OPTION_GIGETIMEOUT            0x5a       /* For GigE cameras, the application periodically sends heartbeat signals to the camera to keep the connection to the camera alive.
                                                            If the camera doesn't receive heartbeat signals within the time period specified by the heartbeat timeout counter, the camera resets the connection.
                                                            When the application is stopped by the debugger, the application cannot send the heartbeat signals
                                                                0 => auto: when the camera is opened, enable if no debugger is present or disable if debugger is present
                                                                1 => enable
                                                                2 => disable
                                                                default: auto
                                                         */
#define OGMACAM_OPTION_EEPROM_SIZE            0x5b       /* get EEPROM size */
#define OGMACAM_OPTION_OVERCLOCK_MAX          0x5c       /* get overclock range: [0, max] */
#define OGMACAM_OPTION_OVERCLOCK              0x5d       /* overclock, default: 0 */
#define OGMACAM_OPTION_RESET_SENSOR           0x5e       /* reset sensor */
#define OGMACAM_OPTION_ISP                    0x5f       /* Enable hardware ISP: 0 => auto (disable in RAW mode, otherwise enable), 1 => enable, -1 => disable; default: 0 */
#define OGMACAM_OPTION_AUTOEXP_EXPOTIME_DAMP  0x60       /* Auto exposure damping coefficient: time (thousandths). The larger the damping coefficient, the smoother and slower the exposure time changes */
#define OGMACAM_OPTION_AUTOEXP_GAIN_DAMP      0x61       /* Auto exposure damping coefficient: gain (thousandths). The larger the damping coefficient, the smoother and slower the gain changes */
#define OGMACAM_OPTION_MOTOR_NUMBER           0x62       /* range: [1, 20] */
#define OGMACAM_OPTION_MOTOR_POS              0x10000000 /* range: [1, 702] */
#define OGMACAM_OPTION_PSEUDO_COLOR_START     0x63       /* Pseudo: start color, BGR format */
#define OGMACAM_OPTION_PSEUDO_COLOR_END       0x64       /* Pseudo: end color, BGR format */
#define OGMACAM_OPTION_PSEUDO_COLOR_ENABLE    0x65       /* Pseudo: -1 => custom: use startcolor & endcolor to generate the colormap
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
#define OGMACAM_OPTION_LOW_POWERCONSUMPTION   0x66       /* Low Power Consumption: 0 => disable, 1 => enable */
#define OGMACAM_OPTION_FPNC                   0x67       /* Fix Pattern Noise Correction
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
#define OGMACAM_OPTION_OVEREXP_POLICY         0x68       /* Auto exposure over exposure policy: when overexposed,
                                                                0 => directly reduce the exposure time/gain to the minimum value; or
                                                                1 => reduce exposure time/gain in proportion to current and target brightness.
                                                                n(n>1) => first adjust the exposure time to (maximum automatic exposure time * maximum automatic exposure gain) * n / 1000, and then adjust according to the strategy of 1
                                                            The advantage of policy 0 is that the convergence speed is faster, but there is black screen.
                                                            Policy 1 avoids the black screen, but the convergence speed is slower.
                                                            Default: 0
                                                         */
#define OGMACAM_OPTION_READOUT_MODE           0x69       /* Readout mode: 0 = IWR (Integrate While Read), 1 = ITR (Integrate Then Read) */
#define OGMACAM_OPTION_TAILLIGHT              0x6a       /* Turn on/off tail Led light: 0 => off, 1 => on; default: on */
#define OGMACAM_OPTION_LENSSTATE              0x6b       /* Load/Save lens state to EEPROM: 0 => load, 1 => save */
#define OGMACAM_OPTION_AWB_CONTINUOUS         0x6c       /* Auto White Balance: continuous mode
                                                                0:  disable (default)
                                                                n>0: every n millisecond(s)
                                                                n<0: every -n frame
                                                         */
#define OGMACAM_OPTION_TECTARGET_RANGE        0x6d       /* TEC target range: min(low 16 bits) = (short)(val & 0xffff), max(high 16 bits) = (short)((val >> 16) & 0xffff) */
#define OGMACAM_OPTION_CDS                    0x6e       /* Correlated Double Sampling */
#define OGMACAM_OPTION_LOW_POWER_EXPOTIME     0x6f       /* Low Power Consumption: Enable if exposure time is greater than the set value */
#define OGMACAM_OPTION_ZERO_OFFSET            0x70       /* Sensor output offset to zero: 0 => disable, 1 => eanble; default: 0 */
#define OGMACAM_OPTION_GVCP_TIMEOUT           0x71       /* GVCP Timeout: millisecond, range = [3, 75], default: 15
                                                              Unless in very special circumstances, generally no modification is required, just use the default value
                                                         */
#define OGMACAM_OPTION_GVCP_RETRY             0x72       /* GVCP Retry: range = [2, 8], default: 4
                                                              Unless in very special circumstances, generally no modification is required, just use the default value
                                                         */

/* pixel format */
#define OGMACAM_PIXELFORMAT_RAW8              0x00
#define OGMACAM_PIXELFORMAT_RAW10             0x01
#define OGMACAM_PIXELFORMAT_RAW12             0x02
#define OGMACAM_PIXELFORMAT_RAW14             0x03
#define OGMACAM_PIXELFORMAT_RAW16             0x04
#define OGMACAM_PIXELFORMAT_YUV411            0x05
#define OGMACAM_PIXELFORMAT_VUYY              0x06
#define OGMACAM_PIXELFORMAT_YUV444            0x07
#define OGMACAM_PIXELFORMAT_RGB888            0x08
#define OGMACAM_PIXELFORMAT_GMCY8             0x09   /* map to RGGB 8 bits */
#define OGMACAM_PIXELFORMAT_GMCY12            0x0a   /* map to RGGB 12 bits */
#define OGMACAM_PIXELFORMAT_UYVY              0x0b
#define OGMACAM_PIXELFORMAT_RAW12PACK         0x0c
#define OGMACAM_PIXELFORMAT_RAW11             0x0d
#define OGMACAM_PIXELFORMAT_HDR8HL            0x0e   /* HDR, Bitdepth: 8, Conversion Gain: High + Low */
#define OGMACAM_PIXELFORMAT_HDR10HL           0x0f   /* HDR, Bitdepth: 10, Conversion Gain: High + Low */
#define OGMACAM_PIXELFORMAT_HDR11HL           0x10   /* HDR, Bitdepth: 11, Conversion Gain: High + Low */
#define OGMACAM_PIXELFORMAT_HDR12HL           0x11   /* HDR, Bitdepth: 12, Conversion Gain: High + Low */
#define OGMACAM_PIXELFORMAT_HDR14HL           0x12   /* HDR, Bitdepth: 14, Conversion Gain: High + Low */

/*
* cmd: input
*    -1:       query the number
*    0~number: query the nth pixel format
* pixelFormat: output, OGMACAM_PIXELFORMAT_xxxx
*/
OGMACAM_API(HRESULT)     Ogmacam_get_PixelFormatSupport(HOgmacam h, char cmd, int* pixelFormat);

/*
* pixelFormat: OGMACAM_PIXELFORMAT_XXXX
*/
OGMACAM_API(const char*) Ogmacam_get_PixelFormatName(int pixelFormat);

OGMACAM_API(HRESULT)  Ogmacam_put_Option(HOgmacam h, unsigned iOption, int iValue);
OGMACAM_API(HRESULT)  Ogmacam_get_Option(HOgmacam h, unsigned iOption, int* piValue);

/*
    xOffset, yOffset, xWidth, yHeight: must be even numbers
*/
OGMACAM_API(HRESULT)  Ogmacam_put_Roi(HOgmacam h, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight);
OGMACAM_API(HRESULT)  Ogmacam_get_Roi(HOgmacam h, unsigned* pxOffset, unsigned* pyOffset, unsigned* pxWidth, unsigned* pyHeight);

/* multiple Roi */
OGMACAM_API(HRESULT)  Ogmacam_put_RoiN(HOgmacam h, unsigned xOffset[], unsigned yOffset[], unsigned xWidth[], unsigned yHeight[], unsigned Num);

OGMACAM_API(HRESULT)  Ogmacam_put_XY(HOgmacam h, int x, int y);

#define OGMACAM_IOCONTROLTYPE_GET_SUPPORTEDMODE            0x01 /* 0x01 => Input, 0x02 => Output, (0x01 | 0x02) => support both Input and Output */
#define OGMACAM_IOCONTROLTYPE_GET_GPIODIR                  0x03 /* 0x00 => Input, 0x01 => Output */
#define OGMACAM_IOCONTROLTYPE_SET_GPIODIR                  0x04
#define OGMACAM_IOCONTROLTYPE_GET_FORMAT                   0x05 /*
                                                                    0x00 => not connected
                                                                    0x01 => Tri-state: Tri-state mode (Not driven)
                                                                    0x02 => TTL: TTL level signals
                                                                    0x03 => LVDS: LVDS level signals
                                                                    0x04 => RS422: RS422 level signals
                                                                    0x05 => Opto-coupled
                                                                */
#define OGMACAM_IOCONTROLTYPE_SET_FORMAT                   0x06
#define OGMACAM_IOCONTROLTYPE_GET_OUTPUTINVERTER           0x07 /* boolean, only support output signal */
#define OGMACAM_IOCONTROLTYPE_SET_OUTPUTINVERTER           0x08
#define OGMACAM_IOCONTROLTYPE_GET_INPUTACTIVATION          0x09 /* 0x00 => Rising edge, 0x01 => Falling edge, 0x02 => Level high, 0x03 => Level low */
#define OGMACAM_IOCONTROLTYPE_SET_INPUTACTIVATION          0x0a
#define OGMACAM_IOCONTROLTYPE_GET_DEBOUNCERTIME            0x0b /* debouncer time in microseconds, range: [0, 20000] */
#define OGMACAM_IOCONTROLTYPE_SET_DEBOUNCERTIME            0x0c
#define OGMACAM_IOCONTROLTYPE_GET_TRIGGERSOURCE            0x0d /*
                                                                   0x00 => Opto-isolated input
                                                                   0x01 => GPIO0
                                                                   0x02 => GPIO1
                                                                   0x03 => Counter
                                                                   0x04 => PWM
                                                                   0x05 => Software
                                                                */
#define OGMACAM_IOCONTROLTYPE_SET_TRIGGERSOURCE            0x0e
#define OGMACAM_IOCONTROLTYPE_GET_TRIGGERDELAY             0x0f /* Trigger delay time in microseconds, range: [0, 5000000] */
#define OGMACAM_IOCONTROLTYPE_SET_TRIGGERDELAY             0x10
#define OGMACAM_IOCONTROLTYPE_GET_BURSTCOUNTER             0x11 /* Burst Counter, range: [1 ~ 65535] */
#define OGMACAM_IOCONTROLTYPE_SET_BURSTCOUNTER             0x12
#define OGMACAM_IOCONTROLTYPE_GET_COUNTERSOURCE            0x13 /* 0x00 => Opto-isolated input, 0x01 => GPIO0, 0x02 => GPIO1 */
#define OGMACAM_IOCONTROLTYPE_SET_COUNTERSOURCE            0x14
#define OGMACAM_IOCONTROLTYPE_GET_COUNTERVALUE             0x15 /* Counter Value, range: [1 ~ 65535] */
#define OGMACAM_IOCONTROLTYPE_SET_COUNTERVALUE             0x16
#define OGMACAM_IOCONTROLTYPE_SET_RESETCOUNTER             0x18
#define OGMACAM_IOCONTROLTYPE_GET_PWM_FREQ                 0x19
#define OGMACAM_IOCONTROLTYPE_SET_PWM_FREQ                 0x1a
#define OGMACAM_IOCONTROLTYPE_GET_PWM_DUTYRATIO            0x1b
#define OGMACAM_IOCONTROLTYPE_SET_PWM_DUTYRATIO            0x1c
#define OGMACAM_IOCONTROLTYPE_GET_PWMSOURCE                0x1d /* 0x00 => Opto-isolated input, 0x01 => GPIO0, 0x02 => GPIO1 */
#define OGMACAM_IOCONTROLTYPE_SET_PWMSOURCE                0x1e
#define OGMACAM_IOCONTROLTYPE_GET_OUTPUTMODE               0x1f /*
                                                                   0x00 => Frame Trigger Wait
                                                                   0x01 => Exposure Active
                                                                   0x02 => Strobe
                                                                   0x03 => User output
                                                                   0x04 => Counter Output
                                                                   0x05 => Timer Output
                                                                */
#define OGMACAM_IOCONTROLTYPE_SET_OUTPUTMODE               0x20
#define OGMACAM_IOCONTROLTYPE_GET_STROBEDELAYMODE          0x21 /* boolean, 0 => pre-delay, 1 => delay; compared to exposure active signal */
#define OGMACAM_IOCONTROLTYPE_SET_STROBEDELAYMODE          0x22
#define OGMACAM_IOCONTROLTYPE_GET_STROBEDELAYTIME          0x23 /* Strobe delay or pre-delay time in microseconds, range: [0, 5000000] */
#define OGMACAM_IOCONTROLTYPE_SET_STROBEDELAYTIME          0x24
#define OGMACAM_IOCONTROLTYPE_GET_STROBEDURATION           0x25 /* Strobe duration time in microseconds, range: [0, 5000000] */
#define OGMACAM_IOCONTROLTYPE_SET_STROBEDURATION           0x26
#define OGMACAM_IOCONTROLTYPE_GET_USERVALUE                0x27 /*
                                                                   bit0 => Opto-isolated output
                                                                   bit1 => GPIO0 output
                                                                   bit2 => GPIO1 output
                                                                */
#define OGMACAM_IOCONTROLTYPE_SET_USERVALUE                0x28
#define OGMACAM_IOCONTROLTYPE_GET_UART_ENABLE              0x29 /* enable: 1 => on; 0 => off */
#define OGMACAM_IOCONTROLTYPE_SET_UART_ENABLE              0x2a
#define OGMACAM_IOCONTROLTYPE_GET_UART_BAUDRATE            0x2b /* baud rate: 0 => 9600; 1 => 19200; 2 => 38400; 3 => 57600; 4 => 115200 */
#define OGMACAM_IOCONTROLTYPE_SET_UART_BAUDRATE            0x2c
#define OGMACAM_IOCONTROLTYPE_GET_UART_LINEMODE            0x2d /* line mode: 0 => TX(GPIO_0)/RX(GPIO_1); 1 => TX(GPIO_1)/RX(GPIO_0) */
#define OGMACAM_IOCONTROLTYPE_SET_UART_LINEMODE            0x2e
#define OGMACAM_IOCONTROLTYPE_GET_EXPO_ACTIVE_MODE         0x2f /* exposure time signal: 0 => specified line, 1 => common exposure time */
#define OGMACAM_IOCONTROLTYPE_SET_EXPO_ACTIVE_MODE         0x30
#define OGMACAM_IOCONTROLTYPE_GET_EXPO_START_LINE          0x31 /* exposure start line, default: 0 */
#define OGMACAM_IOCONTROLTYPE_SET_EXPO_START_LINE          0x32
#define OGMACAM_IOCONTROLTYPE_GET_EXPO_END_LINE            0x33 /* exposure end line, default: 0
                                                                   end line must be no less than start line
                                                                */
#define OGMACAM_IOCONTROLTYPE_SET_EXPO_END_LINE            0x34
#define OGMACAM_IOCONTROLTYPE_GET_EXEVT_ACTIVE_MODE        0x35 /* exposure event: 0 => specified line, 1 => common exposure time */
#define OGMACAM_IOCONTROLTYPE_SET_EXEVT_ACTIVE_MODE        0x36
#define OGMACAM_IOCONTROLTYPE_GET_OUTPUTCOUNTERVALUE       0x37 /* Output Counter Value, range: [0 ~ 65535] */
#define OGMACAM_IOCONTROLTYPE_SET_OUTPUTCOUNTERVALUE       0x38
#define OGMACAM_IOCONTROLTYPE_SET_OUTPUT_PAUSE             0x3a /* Output pause: 1 => puase, 0 => unpause */
#define OGMACAM_IOCONTROLTYPE_GET_INPUT_STATE              0x3b /* Input state: 0 (low level) or 1 (high level) */
#define OGMACAM_IOCONTROLTYPE_GET_USER_PULSE_HIGH          0x3d /* User pulse high level time: us */
#define OGMACAM_IOCONTROLTYPE_SET_USER_PULSE_HIGH          0x3e
#define OGMACAM_IOCONTROLTYPE_GET_USER_PULSE_LOW           0x3f /* User pulse low level time: us */
#define OGMACAM_IOCONTROLTYPE_SET_USER_PULSE_LOW           0x40
#define OGMACAM_IOCONTROLTYPE_GET_USER_PULSE_NUMBER        0x41 /* User pulse number: default 0 */
#define OGMACAM_IOCONTROLTYPE_SET_USER_PULSE_NUMBER        0x42
#define OGMACAM_IOCONTROLTYPE_GET_EXTERNAL_TRIGGER_NUMBER  0x43 /* External trigger number */
#define OGMACAM_IOCONTROLTYPE_GET_DEBOUNCER_TRIGGER_NUMBER 0x45 /* Trigger signal number after debounce */
#define OGMACAM_IOCONTROLTYPE_GET_EFFECTIVE_TRIGGER_NUMBER 0x47 /* Effective trigger signal number */

#define OGMACAM_IOCONTROL_DELAYTIME_MAX                    (5 * 1000 * 1000)

/*
  ioLineNumber:
    0 => Opto-isolated input
    1 => Opto-isolated output
    2 => GPIO0
    3 => GPIO1
*/
OGMACAM_API(HRESULT)  Ogmacam_IoControl(HOgmacam h, unsigned ioLineNumber, unsigned nType, int outVal, int* inVal);

#ifndef __OGMACAMSELFTRIGGER_DEFINED__
#define __OGMACAMSELFTRIGGER_DEFINED__
typedef struct {
    unsigned sensingLeft, sensingTop, sensingWidth, sensingHeight; /* Sensing Area */
    unsigned hThreshold, lThreshold; /* threshold High side, threshold Low side */
    unsigned expoTime; /* Exposure Time */
    unsigned short expoGain; /* Exposure Gain */
    unsigned short hCount, lCount; /* Count threshold High side, Count threshold Low side, thousandths of Sensing Area */
    unsigned short reserved;
} OgmacamSelfTrigger;
#endif
OGMACAM_API(HRESULT)  Ogmacam_put_SelfTrigger(HOgmacam h, const OgmacamSelfTrigger* pSt);
OGMACAM_API(HRESULT)  Ogmacam_get_SelfTrigger(HOgmacam h, OgmacamSelfTrigger* pSt);

#define OGMACAM_FLASH_SIZE      0x00    /* query total size */
#define OGMACAM_FLASH_EBLOCK    0x01    /* query erase block size */
#define OGMACAM_FLASH_RWBLOCK   0x02    /* query read/write block size */
#define OGMACAM_FLASH_STATUS    0x03    /* query status */
#define OGMACAM_FLASH_READ      0x04    /* read */
#define OGMACAM_FLASH_WRITE     0x05    /* write */
#define OGMACAM_FLASH_ERASE     0x06    /* erase */
/* Flash:
 action = OGMACAM_FLASH_XXXX: read, write, erase, query total size, query read/write block size, query erase block size
 addr = address
 see democpp
*/
OGMACAM_API(HRESULT)  Ogmacam_rwc_Flash(HOgmacam h, unsigned action, unsigned addr, unsigned len, void* pData);

OGMACAM_API(HRESULT)  Ogmacam_write_UART(HOgmacam h, const unsigned char* pData, unsigned nDataLen);
OGMACAM_API(HRESULT)  Ogmacam_read_UART(HOgmacam h, unsigned char* pBuffer, unsigned nBufferLen);

/* Initialize support for GigE cameras. If online/offline notifications are not required, the callback function can be set to NULL */
typedef void (__stdcall* POGMACAM_HOTPLUG)(void* ctxHotPlug);
OGMACAM_API(HRESULT)  Ogmacam_GigeEnable(POGMACAM_HOTPLUG funHotPlug, void* ctxHotPlug);

/*
 filePath:
    "*": export to EEPROM
    "0x????" or "0X????": export to EEPROM specified address
    file path: export to file in ini format
*/
OGMACAM_API(HRESULT)  Ogmacam_export_Cfg(HOgmacam h, const char* filePath);

/*
This function is only available on macOS and Linux, it's unnecessary on Windows & Android. To process the device plug in / pull out:
  (1) On Windows, please refer to the MSDN
       (a) Device Management, https://docs.microsoft.com/en-us/windows/win32/devio/device-management
       (b) Detecting Media Insertion or Removal, https://docs.microsoft.com/en-us/windows/win32/devio/detecting-media-insertion-or-removal
  (2) On Android, please refer to https://developer.android.com/guide/topics/connectivity/usb/host
  (3) On Linux / macOS, please call this function to register the callback function.
      When the device is inserted or pulled out, you will be notified by the callback funcion, and then call Ogmacam_EnumV2(...) again to enum the cameras.
  (4) On macOS, IONotificationPortCreate series APIs can also be used as an alternative.
Recommendation: for better rubustness, when notify of device insertion arrives, don't open handle of this device immediately, but open it after delaying a short time (e.g., 200 milliseconds).
*/
#if !defined(_WIN32) && !defined(__ANDROID__)
OGMACAM_API(void)   Ogmacam_HotPlug(POGMACAM_HOTPLUG funHotPlug, void* ctxHotPlug);
#endif

OGMACAM_API(unsigned) Ogmacam_EnumWithName(OgmacamDeviceV2 pti[OGMACAM_MAX]);
OGMACAM_API(HRESULT)  Ogmacam_set_Name(HOgmacam h, const char* name);
OGMACAM_API(HRESULT)  Ogmacam_query_Name(HOgmacam h, char name[64]);
#if defined(_WIN32)
OGMACAM_API(HRESULT)  Ogmacam_put_Name(const wchar_t* camId, const char* name);
OGMACAM_API(HRESULT)  Ogmacam_get_Name(const wchar_t* camId, char name[64]);
#else
OGMACAM_API(HRESULT)  Ogmacam_put_Name(const char* camId, const char* name);
OGMACAM_API(HRESULT)  Ogmacam_get_Name(const char* camId, char name[64]);
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
} OgmacamLensInfo;

OGMACAM_API(HRESULT)  Ogmacam_get_LensInfo(HOgmacam h, OgmacamLensInfo* pInfo);

typedef enum
{
    OgmacamAFMode_CALIBRATE = 0x0,/* lens calibration mode */
    OgmacamAFMode_MANUAL    = 0x1,/* manual focus mode */
    OgmacamAFMode_ONCE      = 0x2,/* onepush focus mode */
    OgmacamAFMode_AUTO      = 0x3,/* autofocus mode */
    OgmacamAFMode_NONE      = 0x4,/* no active selection of focus mode */
    OgmacamAFMode_IDLE      = 0x5,
    OgmacamAFMode_UNUSED    = 0xffffffff
} OgmacamAFMode;

typedef enum
{
    OgmacamAFStatus_NA           = 0x0,/* Not available */
    OgmacamAFStatus_PEAKPOINT    = 0x1,/* Focus completed, find the focus position */
    OgmacamAFStatus_DEFOCUS      = 0x2,/* End of focus, defocus */
    OgmacamAFStatus_NEAR         = 0x3,/* Focusing ended, object too close */
    OgmacamAFStatus_FAR          = 0x4,/* Focusing ended, object too far */
    OgmacamAFStatus_ROICHANGED   = 0x5,/* Focusing ends, roi changes */
    OgmacamAFStatus_SCENECHANGED = 0x6,/* Focusing ends, scene changes */
    OgmacamAFStatus_MODECHANGED  = 0x7,/* The end of focusing and the change in focusing mode is usually determined by the user moderator */
    OgmacamAFStatus_UNFINISH     = 0x8,/* The focus is not complete. At the beginning of focusing, it will be set as incomplete */
    OgmacamAfStatus_UNUSED       = 0xffffffff
} OgmacamAFStatus;/* Focus Status */

typedef struct {
    OgmacamAFMode    AF_Mode;
    OgmacamAFStatus  AF_Status;
    unsigned char    AF_LensAP_Update_Flag;  /* mark for whether the lens aperture is calibrated */
    unsigned char    Reserved[3];
} OgmacamAFState;

OGMACAM_API(HRESULT)  Ogmacam_get_AFState(HOgmacam h, OgmacamAFState* pState);

OGMACAM_API(HRESULT)  Ogmacam_put_AFMode(HOgmacam h, OgmacamAFMode mode, int bFixedWD, unsigned uiNear, unsigned uiFar);
OGMACAM_API(HRESULT)  Ogmacam_put_AFRoi(HOgmacam h, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight);
OGMACAM_API(HRESULT)  Ogmacam_get_AFRoi(HOgmacam h, unsigned* pxOffset, unsigned* pyOffset, unsigned* pxWidth, unsigned* pyHeight);
OGMACAM_API(HRESULT)  Ogmacam_put_AFAperture(HOgmacam h, int iAperture);
OGMACAM_API(HRESULT)  Ogmacam_put_AFFMPos(HOgmacam h, int iFMPos);

/*  simulate replug:
    return > 0, the number of device has been replug
    return = 0, no device found
    return E_ACCESSDENIED if without UAC Administrator privileges
    for each device found, it will take about 3 seconds
*/
#if defined(_WIN32)
OGMACAM_API(HRESULT) Ogmacam_Replug(const wchar_t* camId);
#else
OGMACAM_API(HRESULT) Ogmacam_Replug(const char* camId);
#endif

OGMACAM_API(const OgmacamModelV2**) Ogmacam_all_Model(); /* return all supported USB model array */
OGMACAM_API(const OgmacamModelV2*) Ogmacam_query_Model(HOgmacam h);
OGMACAM_API(const OgmacamModelV2*) Ogmacam_get_Model(unsigned short idVendor, unsigned short idProduct);

/* firmware update:
    camId: camera ID
    filePath: ufw file full path
    funProgress, ctx: progress percent callback
Please do not unplug the camera or lost power during the upgrade process, this is very very important.
Once an unplugging or power outage occurs during the upgrade process, the camera will no longer be available and can only be returned to the factory for repair.
*/
#if defined(_WIN32)
OGMACAM_API(HRESULT)  Ogmacam_Update(const wchar_t* camId, const wchar_t* filePath, PIOGMACAM_PROGRESS funProgress, void* ctxProgress);
#else
OGMACAM_API(HRESULT)  Ogmacam_Update(const char* camId, const char* filePath, PIOGMACAM_PROGRESS funProgress, void* ctxProgress);
#endif

OGMACAM_API(HRESULT)  Ogmacam_put_Linear(HOgmacam h, const unsigned char* v8, const unsigned short* v16); /* v8, v16 pointer must remains valid while camera running */
OGMACAM_API(HRESULT)  Ogmacam_put_Curve(HOgmacam h, const unsigned char* v8, const unsigned short* v16); /* v8, v16 pointer must remains valid while camera running */
OGMACAM_API(HRESULT)  Ogmacam_put_ColorMatrix(HOgmacam h, const double v[9]); /* null => revert to model default */
OGMACAM_API(HRESULT)  Ogmacam_put_InitWBGain(HOgmacam h, const unsigned short v[3]); /* null => revert to model default */

/*
    get the frame rate: framerate (fps) = Frame * 1000.0 / nTime
*/
OGMACAM_API(HRESULT)  Ogmacam_get_FrameRate(HOgmacam h, unsigned* nFrame, unsigned* nTime, unsigned* nTotalFrame);

/* AAF: Astro Auto Focuser */
#define OGMACAM_AAF_SETPOSITION     0x01
#define OGMACAM_AAF_GETPOSITION     0x02
#define OGMACAM_AAF_SETZERO         0x03
#define OGMACAM_AAF_SETDIRECTION    0x05
#define OGMACAM_AAF_GETDIRECTION    0x06
#define OGMACAM_AAF_SETMAXINCREMENT 0x07
#define OGMACAM_AAF_GETMAXINCREMENT 0x08
#define OGMACAM_AAF_SETFINE         0x09
#define OGMACAM_AAF_GETFINE         0x0a
#define OGMACAM_AAF_SETCOARSE       0x0b
#define OGMACAM_AAF_GETCOARSE       0x0c
#define OGMACAM_AAF_SETBUZZER       0x0d
#define OGMACAM_AAF_GETBUZZER       0x0e
#define OGMACAM_AAF_SETBACKLASH     0x0f
#define OGMACAM_AAF_GETBACKLASH     0x10
#define OGMACAM_AAF_GETAMBIENTTEMP  0x12
#define OGMACAM_AAF_GETTEMP         0x14  /* in 0.1 degrees Celsius, such as: 32 means 3.2 degrees Celsius */
#define OGMACAM_AAF_ISMOVING        0x16
#define OGMACAM_AAF_HALT            0x17
#define OGMACAM_AAF_SETMAXSTEP      0x1b
#define OGMACAM_AAF_GETMAXSTEP      0x1c
#define OGMACAM_AAF_GETSTEPSIZE     0x1e
#define OGMACAM_AAF_RANGEMIN        0xfd  /* Range: min value */
#define OGMACAM_AAF_RANGEMAX        0xfe  /* Range: max value */
#define OGMACAM_AAF_RANGEDEF        0xff  /* Range: default value */
OGMACAM_API(HRESULT) Ogmacam_AAF(HOgmacam h, int action, int outVal, int* inVal);

/* astronomy: for ST4 guide, please see: ASCOM Platform Help ICameraV2.
    nDirect: 0 = North, 1 = South, 2 = East, 3 = West, 4 = Stop
    nDuration: in milliseconds
*/
OGMACAM_API(HRESULT)  Ogmacam_ST4PlusGuide(HOgmacam h, unsigned nDirect, unsigned nDuration);

/* S_OK: ST4 pulse guiding
   S_FALSE: ST4 not pulse guiding
*/
OGMACAM_API(HRESULT)  Ogmacam_ST4PlusGuideState(HOgmacam h);

OGMACAM_API(HRESULT)  Ogmacam_Gain2TempTint(const int gain[3], int* temp, int* tint);
OGMACAM_API(void)     Ogmacam_TempTint2Gain(const int temp, const int tint, int gain[3]);
/*
    calculate the clarity factor:
    pImageData: pointer to the image data
    bits: 8(Grey), 16(Grey), 24(RGB24), 32(RGB32), 48(RGB48), 64(RGB64)
    nImgWidth, nImgHeight: the image width and height
    xOffset, yOffset, xWidth, yHeight: the Roi used to calculate. If not specified, use 1/5 * 1/5 rectangle in the center
    return < 0.0 when error
*/
OGMACAM_API(double)   Ogmacam_calc_ClarityFactor(const void* pImageData, int bits, unsigned nImgWidth, unsigned nImgHeight);
OGMACAM_API(double)   Ogmacam_calc_ClarityFactorV2(const void* pImageData, int bits, unsigned nImgWidth, unsigned nImgHeight, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight);

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
OGMACAM_API(void)     Ogmacam_deBayerV2(unsigned nFourCC, int nW, int nH, const void* pRaw, void* pRGB, unsigned char nBitDepth, unsigned char nBitCount);


#ifndef __OGMACAMFOCUSMOTOR_DEFINED__
#define __OGMACAMFOCUSMOTOR_DEFINED__
typedef struct {
    int imax;    /* maximum auto focus sensor board positon */
    int imin;    /* minimum auto focus sensor board positon */
    int idef;    /* conjugate calibration positon */
    int imaxabs; /* maximum absolute auto focus sensor board positon, micrometer */
    int iminabs; /* maximum absolute auto focus sensor board positon, micrometer */
    int zoneh;   /* zone horizontal */
    int zonev;   /* zone vertical */
} OgmacamFocusMotor;
#endif

OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_get_FocusMotor(HOgmacam h, OgmacamFocusMotor* pFocusMotor);

/*
* raw image process
* step:
*  'F': very beginning
*  'B': just before black balance
*  'D': just before demosaic
 */
typedef void (__stdcall* POGMACAM_PROCESS_CALLBACK)(char step, char bStill, unsigned nFourCC, int nW, int nH, void* pRaw, unsigned char pixelFormat, void* ctxProcess);
OGMACAM_API(HRESULT)  Ogmacam_put_Process(HOgmacam h, POGMACAM_PROCESS_CALLBACK funProcess, void* ctxProcess);

/* debayer: raw to RGB */
typedef void (__stdcall* POGMACAM_DEMOSAIC_CALLBACK)(unsigned nFourCC, int nW, int nH, const void* pRaw, void* pRGB, unsigned char nBitDepth, void* ctxDemosaic);
OGMACAM_API(HRESULT)  Ogmacam_put_Demosaic(HOgmacam h, POGMACAM_DEMOSAIC_CALLBACK funDemosaic, void* ctxDemosaic);

/*
    obsolete, please use OgmacamModelV2
*/
typedef struct {
#if defined(_WIN32)
    const wchar_t*      name;       /* model name, in Windows, we use unicode */
#else
    const char*         name;       /* model name */
#endif
    unsigned            flag;       /* OGMACAM_FLAG_xxx */
    unsigned            maxspeed;   /* number of speed level, same as Ogmacam_get_MaxSpeed(), the speed range = [0, maxspeed], closed interval */
    unsigned            preview;    /* number of preview resolution, same as Ogmacam_get_ResolutionNumber() */
    unsigned            still;      /* number of still resolution, same as Ogmacam_get_StillResolutionNumber() */
    OgmacamResolution   res[16];
} OgmacamModel; /* camera model */

/*
    obsolete, please use Ogmacam_deBayerV2
*/
OGMACAM_DEPRECATED
OGMACAM_API(void)     Ogmacam_deBayer(unsigned nFourCC, int nW, int nH, const void* pRaw, void* pRGB, unsigned char nBitDepth);

/*
    obsolete, please use OgmacamDeviceV2
*/
typedef struct {
#if defined(_WIN32)
    wchar_t             displayname[64];    /* display name */
    wchar_t             id[64];             /* unique and opaque id of a connected camera, for Ogmacam_Open */
#else
    char                displayname[64];    /* display name */
    char                id[64];             /* unique and opaque id of a connected camera, for Ogmacam_Open */
#endif
    const OgmacamModel* model;
} OgmacamDevice; /* camera instance for enumerating */

/*
    obsolete, please use Ogmacam_EnumV2
*/
OGMACAM_DEPRECATED
OGMACAM_API(unsigned) Ogmacam_Enum(OgmacamDevice arr[OGMACAM_MAX]);

typedef POGMACAM_DATA_CALLBACK_V3 POGMACAM_DATA_CALLBACK_V2;
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_StartPushModeV2(HOgmacam h, POGMACAM_DATA_CALLBACK_V2 funData, void* ctxData);

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

typedef void (__stdcall* POGMACAM_DATA_CALLBACK)(const void* pData, const BITMAPINFOHEADER* pHeader, int bSnap, void* ctxData);
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_StartPushMode(HOgmacam h, POGMACAM_DATA_CALLBACK funData, void* ctxData);

OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_put_ExpoCallback(HOgmacam h, PIOGMACAM_EXPOSURE_CALLBACK funExpo, void* ctxExpo);
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_put_ChromeCallback(HOgmacam h, PIOGMACAM_CHROME_CALLBACK funChrome, void* ctxChrome);

/* Ogmacam_FfcOnePush is obsolete, recommend using Ogmacam_FfcOnce. */
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_FfcOnePush(HOgmacam h);

/* Ogmacam_DfcOnePush is obsolete, recommend using Ogmacam_DfcOnce. */
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_DfcOnePush(HOgmacam h);

/* Ogmacam_AwbOnePush is obsolete, recommend using Ogmacam_AwbOnce. */
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_AwbOnePush(HOgmacam h, PIOGMACAM_TEMPTINT_CALLBACK funTT, void* ctxTT);

/* Ogmacam_AbbOnePush is obsolete, recommend using Ogmacam_AbbOnce. */
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_AbbOnePush(HOgmacam h, PIOGMACAM_BLACKBALANCE_CALLBACK funBB, void* ctxBB);

#if defined(_WIN32)
/* Ogmacam_put_TempTintInit is obsolete, recommend using Ogmacam_AwbOnce */
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_put_TempTintInit(HOgmacam h, PIOGMACAM_TEMPTINT_CALLBACK funTT, void* ctxTT);

/* ProcessMode: obsolete & useless, noop */
#ifndef __OGMACAM_PROCESSMODE_DEFINED__
#define __OGMACAM_PROCESSMODE_DEFINED__
#define OGMACAM_PROCESSMODE_FULL        0x00    /* better image quality, more cpu usage. this is the default value */
#define OGMACAM_PROCESSMODE_FAST        0x01    /* lower image quality, less cpu usage */
#endif
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_put_ProcessMode(HOgmacam h, unsigned nProcessMode);
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_get_ProcessMode(HOgmacam h, unsigned* pnProcessMode);
#endif

/* obsolete, recommend using Ogmacam_put_Roi and Ogmacam_get_Roi */
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_put_RoiMode(HOgmacam h, int bRoiMode, int xOffset, int yOffset);
OGMACAM_DEPRECATED
OGMACAM_API(HRESULT)  Ogmacam_get_RoiMode(HOgmacam h, int* pbRoiMode, int* pxOffset, int* pyOffset);

/* obsolete:
     ------------------------------------------------------------|
     | Parameter         |   Range       |   Default             |
     |-----------------------------------------------------------|
     | VidgetAmount      |   -100~100    |   0                   |
     | VignetMidPoint    |   0~100       |   50                  |
     -------------------------------------------------------------
*/
OGMACAM_API(HRESULT)  Ogmacam_put_VignetEnable(HOgmacam h, int bEnable);
OGMACAM_API(HRESULT)  Ogmacam_get_VignetEnable(HOgmacam h, int* bEnable);
OGMACAM_API(HRESULT)  Ogmacam_put_VignetAmountInt(HOgmacam h, int nAmount);
OGMACAM_API(HRESULT)  Ogmacam_get_VignetAmountInt(HOgmacam h, int* nAmount);
OGMACAM_API(HRESULT)  Ogmacam_put_VignetMidPointInt(HOgmacam h, int nMidPoint);
OGMACAM_API(HRESULT)  Ogmacam_get_VignetMidPointInt(HOgmacam h, int* nMidPoint);

/* obsolete flags */
#define OGMACAM_FLAG_BITDEPTH10    OGMACAM_FLAG_RAW10  /* pixel format, RAW 10bits */
#define OGMACAM_FLAG_BITDEPTH12    OGMACAM_FLAG_RAW12  /* pixel format, RAW 12bits */
#define OGMACAM_FLAG_BITDEPTH14    OGMACAM_FLAG_RAW14  /* pixel format, RAW 14bits */
#define OGMACAM_FLAG_BITDEPTH16    OGMACAM_FLAG_RAW16  /* pixel format, RAW 16bits */

OGMACAM_API(HRESULT)  Ogmacam_log_File(const
#if defined(_WIN32)
                                       wchar_t*
#else
                                       char*
#endif
                                       filePath);
OGMACAM_API(HRESULT)  Ogmacam_log_Level(unsigned level); /* 0 => none; 1 => error; 2 => debug; 3 => verbose */

#if defined(_WIN32)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif
