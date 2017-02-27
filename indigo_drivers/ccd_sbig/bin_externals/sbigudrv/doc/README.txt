--------------------------------------------------------------------------

LinuxDevKit.

--------------------------------------------------------------------------
Supported processors & operating systems:
--------------------------------------------------------------------------

Currently, SBIG development kit supports Intel x86 32 & 64 bit 
processors under Linux OS. The driver was tested on the following 
Linux distributions: 

Debian 6.0.6
Ubuntu 12.04

The driver should work also on other Linux distributions, there is nothing
special to prevent it to work. User usually has to adapt an udev rules file
only.

--------------------------------------------------------------------------
Linux x86 notes:
--------------------------------------------------------------------------

This version of SBIG development kit is based on a new libusb v. 1.0.x 
USB library. For details please visit the following web page: 
http://www.libusb.org/

User has to install the libusb library first. If you use Debian/Ubuntu 
distributions it can be done using: sudo apt-get install libusb-1.0-0-dev

If you want to use our simple, command line test application (testapp),
you should also install the cfitsio FITS library because this application 
stores CCD images in this file format. To use the SBIG file format only, 
the following line of code needs to be commented out to block the FITS stuff.

csbigimg.h:

#define	INCLUDE_FITSIO	1


--------------------------------------------------------------------------
Linux Installation:
--------------------------------------------------------------------------

If you use SBIG ST-i, STF, STT, STX, STXL CCD cameras, then only  
51-sbig-debian.rules file has to be copied into the /etc/udev/rules.d/
directory. 

If you use SBIG ST-7,8,9,..STL, ST-8300, etc. CCD cameras which uploads their
firmware during booting phase, then it is also necessary to copy all the
*.hex and *.bin files into /lib/firmware directory.

The SBIG's driver library libsbigudrv.so should go to: /usr/local/lib/
and the LD_LIBRARY_PATH should point to the /usr/local/lib directory.
                                                                             
--------------------------------------------------------------------------
SBIG testapp:
--------------------------------------------------------------------------

The Linux development kit contains simple, command line test application.
 
