#ifndef __QHYCCDINTERNAL_H__
#define __QHYCCDINTERNAL_H__

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>


#if defined (_WIN32)
#include <windows.h>
#include <process.h>
#include <time.h>
#include <mmsystem.h>
#else
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libusb-1.0/libusb.h>
#endif




#endif

