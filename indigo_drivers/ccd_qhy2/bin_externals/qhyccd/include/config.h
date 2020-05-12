#ifndef __CONFIG_H__
#define __CONFIG_H__

/*
 20, 2, 3,1

0204
294
411
411eris
550
6060
QHYCCD  (Beijing) Technology Co., Ltd.
 
system_profiler SPUSBDataType
 
/lib/firmware/qhy
*/




#define CALLBACK_MODE_SUPPORT		1



#if defined (_WIN32)
#define QHYCCD_OPENCV_SUPPORT
#define WINDOWS_PTHREAD_SUPPORT		1
#define WINPCAP_MODE_SUPPORT		0
#define PCIE_MODE_SUPPORT			0
#define CYUSB_MODE_SUPPORT  		1
#define WINUSB_MODE_SUPPORT  		1
#define LIBUSB_MODE_SUPPORT  		0
#define PCIE_CAPTURE_TEST			0
#else
#undef  QHYCCD_OPENCV_SUPPORT
#define WINDOWS_PTHREAD_SUPPORT		0
#define WINPCAP_MODE_SUPPORT		0
#define PCIE_MODE_SUPPORT			0
#define CYUSB_MODE_SUPPORT  		0
#define WINUSB_MODE_SUPPORT  		0
#define LIBUSB_MODE_SUPPORT  		1
#define PCIE_CAPTURE_TEST			0
#endif



#endif

