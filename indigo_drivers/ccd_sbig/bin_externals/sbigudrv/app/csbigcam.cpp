/*
  
	csbigcam.cpp - Contains the code for the sbigcam class

	1. This software (c)2004-2011 Santa Barbara Instrument Group.
	2. This free software is provided as an example of how
	to communicate with SBIG cameras.  It is provided AS-IS
	without any guarantees by SBIG of suitability for a
	particular purpose and without any guarantee to be
	bug-free.  If you use it you agree to these terms and
	agree to do so at your own risk.
	3. Any distribution of this source code to include these
	terms.

	Revision History
	Date  		Modification
	=========================================================
	1/9/05		Fixed a bug in the ActivateRelay() method
				 where only the XPlus relay would ever
				 fire
				Renamed the development ST-F by it's
				 production name ST-402

	1/17/05		Added the Get/SetSubFrame() methods for
				 specifying a partial frame readout in
				 the GrabImage() method
				Added the GetFullFrame() method that returns
				 the size of the active CCD in the current
				 readout mode.

	1/18/05		Added the IsRelayActive() method.
	
	4/9/06		Added GetReadoutInfo() method
				Added GetFormattedCameraInfo() method
				Added the VERSION_STR constant and bumped
				 to Version 1.2
				Improved CFW Support with the following methods
					SetCFWModel()
					SetCFWPosition()
					GetCFWPositionAndStatus()
					GetCFWMaxPosition()
					GetCFWErrorString()
					GetCFWErrorString()
				Modified the GrabImage() method and added
				 the GetGrabState() method which allows
				 calling GrabImage() from a thread
					
	9/20/06		Fixed a bug with the on-chip binning modes
				(XX03, XX04 and XX05) where the image height
				was being set to 0.

	11/26/08	Brought up to date with Linux and added support for the STX
				
	12/5/07		Added support for ST-X and ST-4K cameras
				Bumped to version 1.21
 
	4/19/10		Merged PC and Mac code.
				Brought STX support up to date and added support
				for the ST-8300 cameras.
				Bumped to version 1.30
 
	2/13/11		Added support for the ST-i and STT model cameras.
				Bumped to version 1.31
				 
	9/1/11		Added preliminary suppport for the ST-8300B Camera
				Bumped to version 1.32
	 
	10/3/11		Renamed the ST-8300B as the STF-8300 Camera
				Bumped to version 1.33
	 
*/
#include "lpardrv.h"
#include "csbigcam.h"
#include "csbigimg.h"
#include <string>
#include <math.h>
#include <stdio.h>

#include <iostream>

using namespace std;

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif

#define VERSION_STR		"1.33"	/* version of this class */

/*
 Temperature Conversion Constants
 Defined in the SBIG Universal Driver Documentation
*/
#define T0      25.0
#define R0       3.0
#define DT_CCD  25.0
#define DT_AMB  45.0
#define RR_CCD   2.57
#define RR_AMB   7.791
#define RB_CCD  10.0
#define RB_AMB   3.0
#define MAX_AD  4096

/*
  
 hex2double:
	 
 Convert the passed hex value to double.
 The hex value is assumed to be in the
 format: XXXXXX.XX
	 
*/
static double hex2double(unsigned long ul)
{
	double res, mult;
	int i;
	
	res = 0.0;
	mult = 1.0;
	for (i=0; i<8; i++)
	{
		res += mult * (double)(ul & 0x0F);
		ul >>= 4;
		mult *= 10.0;
	}
	return res / 100.0;
	
}

/*
  
 Init:
	 
 Initialize the base variables.
 
*/
void CSBIGCam::Init()
{
	m_eLastError 			= CE_NO_ERROR;
	m_eLastCommand 		= (PAR_COMMAND)0;
	m_nDrvHandle 			= INVALID_HANDLE_VALUE;
	m_eCameraType 		= NO_CAMERA;
	m_eActiveCCD 			= CCD_IMAGING;
	m_dExposureTime 	= 0.1;
	m_uReadoutMode 		= 0;
	m_eABGState 			= ABG_CLK_MED7;
	m_nSubFrameLeft 	= m_nSubFrameTop = m_nSubFrameWidth = m_nSubFrameHeight = 0;
	m_eGrabState 			= GS_IDLE;
	m_dGrabPercent 		= 0.0;
	m_eCFWModel 			= CFWSEL_UNKNOWN;
	m_eCFWError 			= CFWE_NONE;
	m_FastReadout 		= false;
	m_DualChannelMode	= false;
}

/*
  
 CSBIGCam:
	 
 Stamdard constructor.  Initialize appropriate member variables.
 
*/
CSBIGCam::CSBIGCam()
{
	Init();
}

/*
  
 CSBIGCam:
	 
 Alternate constructor.  Init the vars, Open the driver and then
 try to open the passed device.
 
 If you want to use the Ethernet connection this is the best
 constructor.  If you're using the LPT or USB connections
 the alternate constructor below may make more sense.
 
*/
CSBIGCam::CSBIGCam(OpenDeviceParams odp)
{
	Init();
	if (OpenDriver() == CE_NO_ERROR)
	{
		m_eLastError = OpenDevice(odp);
	}
}

/*
  
 CSBIGCam:
	 
 Alternate constructor.  Init the vars, Open the driver and then
 try to open the passed device.
 
 This won't work the Ethernet port because no IP address
 is specified.  Use the constructor above where you can
 pass the OpenDeviceParams struct.
 
*/
CSBIGCam::CSBIGCam(SBIG_DEVICE_TYPE dev)
{
	OpenDeviceParams odp;
	
	odp.ipAddress 		= 0x00;
	odp.lptBaseAddress 	= 0x00;
	Init();

	if (dev == DEV_ETH)
	{
		m_eLastError = CE_BAD_PARAMETER;
	}
	else
	{
		odp.deviceType = dev;
		if (OpenDriver() == CE_NO_ERROR)
		{
			m_eLastError = OpenDevice(odp);
		}
	}
}


/*
  
 ~CSBIGCam:
 
 Standard destructor.  Close the device then the driver.
 
*/
CSBIGCam::~CSBIGCam()
{
	CloseDevice();
	CloseDriver();
}

/*
  
 GetError:
	 
 Return the error generated in the previous driver call.
 
*/
PAR_ERROR CSBIGCam::GetError()
{
	return m_eLastError;
}

/*
  
 GetCommand:
	 
 Return the command last passed to the driver.
 
*/
PAR_COMMAND CSBIGCam::GetCommand()
{
	return m_eLastCommand;
}

/*

  GetSubFrame:
  SetSubFrame:

  Set or return the coordinates of the subframe for
  readout.  Setting the Height or Width to zero uses
  the whole frame.

*/
void CSBIGCam::SetSubFrame(int nLeft, int nTop, int nWidth, int nHeight)
{
	m_nSubFrameLeft		= nLeft;
	m_nSubFrameTop 		= nTop;
	m_nSubFrameWidth 	= nWidth;
	m_nSubFrameHeight	= nHeight;
}

void CSBIGCam::GetSubFrame(int &nLeft, int &nTop, int &nWidth, int &nHeight)
{
	nLeft	= m_nSubFrameLeft;
	nTop 	= m_nSubFrameTop;
	nWidth 	= m_nSubFrameWidth;
	nHeight = m_nSubFrameHeight;
}

/*

  GetFullFrame:

  Return the size of the full frame image based upon the active
  CCD and current readout mode.

*/
PAR_ERROR CSBIGCam::GetFullFrame(int &nWidth, int &nHeight)
{
	GetCCDInfoResults0 	gcir;
	GetCCDInfoParams 	gcip;
	unsigned short 		vertNBinning;
	unsigned short 		rm;

	// Get the image dimensions
	vertNBinning = m_uReadoutMode >> 8;

	if (vertNBinning == 0)
	{
		vertNBinning = 1;
	}

	rm = m_uReadoutMode & 0xFF;
	gcip.request = (m_eActiveCCD == CCD_IMAGING ? CCD_INFO_IMAGING : CCD_INFO_TRACKING);

	if (SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir) != CE_NO_ERROR)
	{
		return m_eLastError;
	}

	if (rm >= gcir.readoutModes)
	{
		return CE_BAD_PARAMETER;
	}

	nWidth = gcir.readoutInfo[rm].width;

	if (m_eCameraType == STI_CAMERA && rm >= 2 && rm <= 3)
	{
		nHeight = gcir.readoutInfo[rm-2].height / vertNBinning;
	}
	else if (rm >= 3 && rm <= 5)
	{
		nHeight = gcir.readoutInfo[rm-3].height / vertNBinning;
	}
	else
	{
		nHeight = gcir.readoutInfo[rm].height / vertNBinning;
	}
	return CE_NO_ERROR;
}

/*
  
 GetCameraTypeString:
	 
 Return a string describing the model camera
 that has been linked to.
 
*/
/* typedef enum { ST7_CAMERA=4, ST8_CAMERA, ST5C_CAMERA, TCE_CONTROLLER,
	ST237_CAMERA, STK_CAMERA, ST9_CAMERA, STV_CAMERA, ST10_CAMERA,
	ST1K_CAMERA, ST2K_CAMERA, STL_CAMERA, ST402_CAMERA, STX_CAMERA,
  ST4K_CAMERA, STT_CAMERA, STI_CAMERA, STF8300_CAMERA, NEXT_CAMERA, NO_CAMERA=0xFFFF } CAMERA_TYPE; */
static const char *CAM_NAMES[] = {
	"Type 0", "Type 1", "Type 2", "Type 3",
	"ST-7", "ST-8", "ST-5C", "TCE",
	"ST-237", "ST-K", "ST-9", "STV", "ST-10",
	"ST-1K", "ST-2K", "STL", "ST-402", "STX",
	"ST-4K", "STT", "ST-i",	"STF-8300" };
string CSBIGCam::GetCameraTypeString(void)
{
	string s;
	GetCCDInfoParams gcip;
	GetCCDInfoResults0 gcir;
	char *p1, *p2;
	int isColor = FALSE;
	
	if ( m_eCameraType < (CAMERA_TYPE)(sizeof(CAM_NAMES)/sizeof(const char *)) ) {
		// default name
		s = CAM_NAMES[m_eCameraType];
		
		// Get name info
		gcip.request = CCD_INFO_IMAGING;
		if ( SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir) != CE_NO_ERROR )
			return s;
			
		// Color cameras report as SBIG ST-XXX Color...
		if ( strstr(gcir.name, "Color") != 0 )
			isColor = TRUE;
		
		// see if ST-237A and if so indicate it in the name
		if ( m_eCameraType == ST237_CAMERA && gcir.readoutInfo[0].gain >= 0x100 )
			s += "A";
		
		// include any sub-models
		if ( m_eCameraType == STL_CAMERA ||
			 m_eCameraType == ST402_CAMERA ||
			 m_eCameraType == STX_CAMERA ||
			 m_eCameraType == STT_CAMERA ) {
			// driver reports STL names as "SBIG ST-L-XXX..."
			// driver reports ST-402 names as "SBIG ST-XXX..."
			// driver reports ST-X names as "SBIG STX-XXX..."
			// driver reports STT names as "SBIG STT-XXX..."
			p1 = gcir.name + 5;
			if ( (p2 = strchr(p1,' ')) != NULL ) {
				*p2 = 0;
				s = p1;
			}
		}
		if ( isColor )
			s += "C";
	}
	else if ( m_eCameraType == NO_CAMERA )
		s = "No Camera";
	else
		s = "Unknown";
	return s;
}

/*
  
 SBIGUnivDrvCommand:
	 
 Bottleneck function for all calls to the driver that logs
 the command and error. First it activates our handle and
 then it calls the driver.  Activating the handle first
 allows having multiple instances of this class dealing
 with multiple cameras on different communications port.
 
 Also allows direct access to the SBIG Universal Driver after
 the driver has been opened.
 
*/
PAR_ERROR CSBIGCam::SBIGUnivDrvCommand(short command, void *Params, void *Results)
{
	SetDriverHandleParams sdhp;
	
	// make sure we have a valid handle to the driver
	m_eLastCommand = (PAR_COMMAND)command;

	if (m_nDrvHandle == INVALID_HANDLE_VALUE)
	{
		m_eLastError = CE_DRIVER_NOT_OPEN;
	}
	else
	{
		// handle is valid so install it in the driver
		sdhp.handle = m_nDrvHandle;
		if ((m_eLastError = (PAR_ERROR)::SBIGUnivDrvCommand(CC_SET_DRIVER_HANDLE, &sdhp, NULL)) == CE_NO_ERROR)
		{
			// call the desired command
			m_eLastError = (PAR_ERROR)::SBIGUnivDrvCommand(command, Params, Results);
		}
	}

	return m_eLastError;
}

/*
  
 OpenDriver:
	 
 Open the driver.  Must be made before any other calls and
 should be called only once per instance of the camera class.
 Based on the results of the open call to the driver this can
 open a new handle to the driver.
 
 The alternate constructors do this for you when you specify
 the communications port to use.
 
*/
PAR_ERROR CSBIGCam::OpenDriver()
{
	short res;
	GetDriverHandleResults gdhr;
	SetDriverHandleParams sdhp;
	
	// call the driver directly so doesn't install our handle
	res = ::SBIGUnivDrvCommand(m_eLastCommand = CC_OPEN_DRIVER, NULL, NULL);
	if ( res == CE_DRIVER_NOT_CLOSED )
	{
		/*
		   the driver is open already which we interpret
		   as having been opened by another instance of
		   the class so get the driver to allocate a new
		   handle and then record it
		*/
		sdhp.handle = INVALID_HANDLE_VALUE;
		res = ::SBIGUnivDrvCommand(CC_SET_DRIVER_HANDLE, &sdhp, NULL);
		if ( res == CE_NO_ERROR ) {
			res = ::SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
			if ( res == CE_NO_ERROR ) {
				res = ::SBIGUnivDrvCommand(CC_GET_DRIVER_HANDLE, NULL, &gdhr);
				if ( res == CE_NO_ERROR )
					m_nDrvHandle = gdhr.handle;
			}
		}
	}
	else if ( res == CE_NO_ERROR )
	{
		/*
		   the driver was not open so record the driver handle
		   so we can support multiple instances of this class
		   talking to multiple cameras
		 */
		res = ::SBIGUnivDrvCommand(CC_GET_DRIVER_HANDLE, NULL, &gdhr);
		if ( res == CE_NO_ERROR )
			m_nDrvHandle = gdhr.handle;
	}
	return m_eLastError = (PAR_ERROR)res;
}

/*
  
 CloseDriver:
	 
 Should be called for every call to OpenDriver.
 Standard destructor does this for you as well.
 Closing the Drriver multiple times won't hurt
 but will return an error.
 
 The destructor will do this for you if you
 don't do it explicitly.
 
*/
PAR_ERROR CSBIGCam::CloseDriver()
{
	PAR_ERROR res;
	
	res = SBIGUnivDrvCommand(CC_CLOSE_DRIVER, NULL, NULL);
	if ( res == CE_NO_ERROR )
		m_nDrvHandle = INVALID_HANDLE_VALUE;
	return res;
}

/*
  
 OpenDevice:
	 
 Call once to open a particular port (USB, LPT,
 Ethernet, etc).  Must be balanced with a call
 to CloseDevice.
 
 Note that the alternate constructors will make
 this call for you so you don't have to do it
 explicitly.
 
*/
PAR_ERROR CSBIGCam::OpenDevice(OpenDeviceParams odp)
{
	return SBIGUnivDrvCommand(CC_OPEN_DEVICE, &odp, NULL);
}

/*
  
 CloseDevice:
	 
 Closes which ever device was opened by OpenDriver.
 
 The destructor does this for you so you don't have
 to call it explicitly.
 
*/
PAR_ERROR CSBIGCam::CloseDevice()
{
	return SBIGUnivDrvCommand(CC_CLOSE_DEVICE, NULL, NULL);
}

/*
  
 GetErrorString:
	 
 Return a string object describing the passed error code.
 
*/
string CSBIGCam::GetErrorString(PAR_ERROR err)
{
	GetErrorStringParams gesp;
	GetErrorStringResults gesr;
	string s;
	
	gesp.errorNo = err;
	SBIGUnivDrvCommand(CC_GET_ERROR_STRING, &gesp, &gesr);
	s = gesr.errorString;
	return s;
}

/*
  
 GetDriverInfo:
	 
 Get the requested driver info for the passed request.
 This call only works with the DRIVER_STANDARD and
 DRIVER_EXTENDED requests as you pass it a result
 reference that only works with those 2 requests.
 For other requests simply call the
 SBIGUnivDrvCommand class function.
 
*/
PAR_ERROR CSBIGCam::GetDriverInfo(DRIVER_REQUEST request, GetDriverInfoResults0 &gdir)
{
	GetDriverInfoParams gdip;
	
	gdip.request = request;
	m_eLastCommand = CC_GET_DRIVER_INFO;
	if ( request > DRIVER_EXTENDED )
		return m_eLastError = CE_BAD_PARAMETER;
	else
		return SBIGUnivDrvCommand(CC_GET_DRIVER_INFO, &gdip, &gdir);
}

/*

	GetReadoutInfo:
	
	Return the pixel size (in mm) and Redout Gain for the
	selected readout mode.
	
*/
PAR_ERROR CSBIGCam::GetReadoutInfo(double &pixelWidth, double &pixelHeight, double &eGain)
{
	GetCCDInfoResults0 gcir;
	GetCCDInfoParams gcip;
	unsigned short vertNBinning;
	unsigned short rm;

	if ( SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir) != CE_NO_ERROR )
		return m_eLastError;
		
	vertNBinning = m_uReadoutMode >> 8;
	if ( vertNBinning == 0 )
		vertNBinning = 1;
	rm = m_uReadoutMode & 0xFF;
	if ( rm >= gcir.readoutModes )
		return CE_BAD_PARAMETER;

	eGain = hex2double(gcir.readoutInfo[rm].gain);
	pixelHeight = hex2double(gcir.readoutInfo[rm].pixelHeight) * vertNBinning / 1000.0;
	pixelWidth = hex2double(gcir.readoutInfo[rm].pixelWidth) / 1000.0;
	return CE_NO_ERROR;
}

/*

 GetGrabState:
 
 Return the state and percent completion of the GrabImage
 method.  This allows you to monitor the state of the
 GrabImage method in a multi-threading application.
 
*/
void CSBIGCam::GetGrabState(GRAB_STATE &grabState, double &percentComplete)
{
	grabState = m_eGrabState;
	percentComplete = m_dGrabPercent;
}

/*

  GrabSetup:
  
  Do the once per session processing for the grab command.  This allows
  you to grab multiple images in sequence to the same Image
  by first calling GrabSetup() then GrabMain() like the
  GrabImage() method.
  
*/
PAR_ERROR CSBIGCam::GrabSetup(CSBIGImg *pImg, SBIG_DARK_FRAME dark)
{
	GetCCDInfoParams 	gcip;
	GetCCDInfoResults0 	gcir;
	unsigned short 		es;
	string 				s;
	
	// Get the image dimensions
	m_eGrabState   = GS_DAWN;
	m_dGrabPercent = 0.0;
	m_sGrabInfo.vertNBinning = m_uReadoutMode >> 8;

	if (m_sGrabInfo.vertNBinning == 0)
	{
		m_sGrabInfo.vertNBinning = 1;
	}
	m_sGrabInfo.rm   = m_uReadoutMode & 0xFF;
	m_sGrabInfo.hBin = m_sGrabInfo.vBin = 1;

	if (m_eCameraType == STI_CAMERA)
	{
		if (m_sGrabInfo.rm < 2)
		{
			m_sGrabInfo.hBin = m_sGrabInfo.vBin = (m_sGrabInfo.rm + 1);
		}
		else if (m_sGrabInfo.rm < 4)
		{
			m_sGrabInfo.hBin = (m_sGrabInfo.rm - 1);
			m_sGrabInfo.vBin = m_sGrabInfo.vertNBinning;
		}
	}
	else
	{
		if (m_sGrabInfo.rm < 3)
		{
			m_sGrabInfo.hBin = m_sGrabInfo.vBin = (m_sGrabInfo.rm + 1);
		}
		else if (m_sGrabInfo.rm < 6)
		{
			m_sGrabInfo.hBin = (m_sGrabInfo.rm - 5);
			m_sGrabInfo.vBin = m_sGrabInfo.vertNBinning;
		}
		else if (m_sGrabInfo.rm < 9)
		{
			m_sGrabInfo.hBin = m_sGrabInfo.vBin = (m_sGrabInfo.rm - 8);
		}
		else if (m_sGrabInfo.rm == 9)
		{
			m_sGrabInfo.hBin = m_sGrabInfo.vBin = 9;
		}
	}
	gcip.request = (m_eActiveCCD == CCD_IMAGING ? CCD_INFO_IMAGING : CCD_INFO_TRACKING);

	if (SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir) != CE_NO_ERROR)
	{
		return m_eLastError;
	}

	if (m_sGrabInfo.rm >= gcir.readoutModes)
	{
		return CE_BAD_PARAMETER;
	}

	if (m_nSubFrameWidth == 0 || m_nSubFrameHeight == 0)
	{
		m_sGrabInfo.left  = m_sGrabInfo.top = 0;
		m_sGrabInfo.width = gcir.readoutInfo[m_sGrabInfo.rm].width;

		if (m_eCameraType == STI_CAMERA)
		{
			if (m_sGrabInfo.rm >= 2 && m_sGrabInfo.rm <= 3)
			{
				m_sGrabInfo.height = gcir.readoutInfo[m_sGrabInfo.rm-2].height / m_sGrabInfo.vertNBinning;
			}
			else
			{
				m_sGrabInfo.height = gcir.readoutInfo[m_sGrabInfo.rm].height / m_sGrabInfo.vertNBinning;
			}
		}
		else
		{
			if (m_sGrabInfo.rm >= 3 && m_sGrabInfo.rm <= 5)
			{
				m_sGrabInfo.height = gcir.readoutInfo[m_sGrabInfo.rm-3].height / m_sGrabInfo.vertNBinning;
			}
			else
			{
				m_sGrabInfo.height = gcir.readoutInfo[m_sGrabInfo.rm].height / m_sGrabInfo.vertNBinning;
			}
		}
	}
	else
	{
		m_sGrabInfo.left 	= m_nSubFrameLeft;
		m_sGrabInfo.top 	= m_nSubFrameTop;
		m_sGrabInfo.width 	= m_nSubFrameWidth;
		m_sGrabInfo.height 	= m_nSubFrameHeight;
	}

	// try to allocate the image buffer
	if (!pImg->AllocateImageBuffer(m_sGrabInfo.height, m_sGrabInfo.width))
	{
		m_eGrabState = GS_IDLE;
		return CE_MEMORY_ERROR;
	}
	pImg->SetImageModified(TRUE);

	// initialize some image header params
	pImg->SetEachExposure(m_dExposureTime);
	pImg->SetEGain(hex2double(gcir.readoutInfo[m_sGrabInfo.rm].gain));
	pImg->SetPixelHeight(hex2double(gcir.readoutInfo[m_sGrabInfo.rm].pixelHeight) * m_sGrabInfo.vertNBinning / 1000.0);
	pImg->SetPixelWidth(hex2double(gcir.readoutInfo[m_sGrabInfo.rm].pixelWidth) / 1000.0);
	es = ES_DCS_ENABLED | ES_DCR_DISABLED | ES_AUTOBIAS_ENABLED;

	if (m_eCameraType == ST5C_CAMERA)
	{
		es |= (ES_ABG_CLOCKED | ES_ABG_RATE_MED);
	}
	else if (m_eCameraType == ST237_CAMERA)
	{
		es |= (ES_ABG_CLOCKED | ES_ABG_RATE_FIXED);
	}
	else if (m_eActiveCCD == CCD_TRACKING)
	{
		es |= (ES_ABG_CLOCKED | ES_ABG_RATE_MED);
	}
	else
	{
		es |= ES_ABG_LOW;
	}
	pImg->SetExposureState(es);
	pImg->SetExposureTime(m_dExposureTime);
	pImg->SetNumberExposures(1);
	pImg->SetReadoutMode(m_uReadoutMode);

	s = GetCameraTypeString();
	if (m_eCameraType == ST5C_CAMERA || ( m_eCameraType == ST237_CAMERA && s.find("ST-237A", 0) == string::npos))
	{
		pImg->SetSaturationLevel(4095);
	}
	else
	{
		pImg->SetSaturationLevel(65535);
	}
	s = gcir.name;
	pImg->SetCameraModel(s);
	pImg->SetBinning(m_sGrabInfo.hBin, m_sGrabInfo.vBin);
	pImg->SetSubFrame(m_sGrabInfo.left, m_sGrabInfo.top);
	return CE_NO_ERROR;
}

/*

  GrabMain:
  
  Do the once per image processing for the Grab.  This assumes
  you have previously called the GrabSetup() method.
  
*/
PAR_ERROR CSBIGCam::GrabMain(CSBIGImg *pImg, SBIG_DARK_FRAME dark)
{
	int 								i;
	double 							ccdTemp = 0.0;
	time_t 							curTime;
	PAR_ERROR 					err;
	StartReadoutParams 	srp;
	ReadoutLineParams 	rlp;
	struct tm *					pLT;
	char 								cs[80];
	MY_LOGICAL 					expComp;
	
	// initialize some image header params
	if (GetCCDTemperature(ccdTemp) != CE_NO_ERROR)
	{
		return m_eLastError;
	}

	pImg->SetCCDTemperature(ccdTemp);
	
	// end any exposure in case one in progress
	EndExposure();

	if (m_eLastError != CE_NO_ERROR && m_eLastError != CE_NO_EXPOSURE_IN_PROGRESS)
	{
		return m_eLastError;
	}
	
	// Record the image size incase this is an STX and its needs
	// the info to start the exposure
	SetSubFrame(m_sGrabInfo.left, m_sGrabInfo.top, m_sGrabInfo.width, m_sGrabInfo.height);

	// start the exposure
	m_eGrabState = (dark == SBDF_LIGHT_ONLY ? GS_EXPOSING_LIGHT : GS_EXPOSING_DARK);
	
	if (StartExposure(dark == SBDF_LIGHT_ONLY ? SC_OPEN_SHUTTER : SC_CLOSE_SHUTTER) != CE_NO_ERROR)
	{
		return m_eLastError;
	}
	
	curTime = time(NULL);
	pImg->SetImageStartTime(curTime);

	// wait for exposure to complete
	do 
	{
		m_dGrabPercent = (double)(time(NULL) - curTime)/m_dExposureTime;
	} 
	while ((err = IsExposureComplete(expComp)) == CE_NO_ERROR && !expComp );
	
	EndExposure();

	if (err != CE_NO_ERROR)
	{
		return err;
	}
	
	if (m_eLastError != CE_NO_ERROR)
	{
		return m_eLastError;
	}
	
	// readout the CCD
	srp.ccd    = m_eActiveCCD;
	srp.left   = m_sGrabInfo.left;
	srp.top    = m_sGrabInfo.top;
	srp.height = m_sGrabInfo.height;
	srp.width  = m_sGrabInfo.width;
	srp.readoutMode = m_uReadoutMode;
	m_eGrabState = (dark == SBDF_LIGHT_ONLY ? GS_DIGITIZING_LIGHT : GS_DIGITIZING_DARK);
	
	if ( (err = StartReadout(srp)) == CE_NO_ERROR ) 
	{
		rlp.ccd = m_eActiveCCD;
		rlp.pixelStart = m_sGrabInfo.left;
		rlp.pixelLength = m_sGrabInfo.width;
		rlp.readoutMode = m_uReadoutMode;
	
		for (i = 0; i < m_sGrabInfo.height && err == CE_NO_ERROR; i++)
		{
			m_dGrabPercent = (double)(i+1) / m_sGrabInfo.height;
			err = ReadoutLine(rlp, FALSE, pImg->GetImagePointer() + (long)i * m_sGrabInfo.width);
		}
	}
	
	EndReadout();
	
	if (err != CE_NO_ERROR)
	{
		return err;
	}
	
	if (m_eLastError != CE_NO_ERROR)
	{
		return err;
	}
	
	// we're done unless we wanted a dark also image
	if (dark == SBDF_DARK_ALSO)
	{
		// start the light exposure
		m_eGrabState = GS_EXPOSING_LIGHT;

		if (StartExposure(SC_OPEN_SHUTTER) != CE_NO_ERROR)
		{
			return m_eLastError;
		}

		curTime = time(NULL);
		pImg->SetImageStartTime(curTime);

		// wait for exposure to complete
		do
		{
			m_dGrabPercent = (double)(time(NULL) - curTime)/m_dExposureTime;
		}
		while ((err = IsExposureComplete(expComp)) == CE_NO_ERROR && !expComp );

		EndExposure();

		if ( err != CE_NO_ERROR )
			return err;
		if ( m_eLastError != CE_NO_ERROR )
			return m_eLastError;

		// readout the CCD
		srp.ccd = m_eActiveCCD;
		srp.left = m_sGrabInfo.left;
		srp.top = m_sGrabInfo.top;
		srp.height = m_sGrabInfo.height;
		srp.width = m_sGrabInfo.width;
		srp.readoutMode = m_uReadoutMode;
		m_eGrabState = GS_DIGITIZING_LIGHT;
		if ( (err = StartReadout(srp)) == CE_NO_ERROR ) {
			rlp.ccd = m_eActiveCCD;
			rlp.pixelStart = m_sGrabInfo.left;
			rlp.pixelLength = m_sGrabInfo.width;
			rlp.readoutMode = m_uReadoutMode;
			for (i=0; i<m_sGrabInfo.height && err==CE_NO_ERROR; i++ ) {
				m_dGrabPercent = (double)(i+1)/m_sGrabInfo.height;
				err = ReadoutLine(rlp, TRUE, pImg->GetImagePointer() + (long)i * m_sGrabInfo.width);
			}
		}
		EndReadout();
		if (err != CE_NO_ERROR)
		{
			return err;
		}

		if (m_eLastError != CE_NO_ERROR)
		{
			return err;
		}

		// record dark subtraction in history
		m_eGrabState = GS_DUSK;
		if (m_eCameraType == ST5C_CAMERA || m_eCameraType == ST237_CAMERA)
		{
			pImg->SetHistory("f");
		}
		else
		{
			pImg->SetHistory("R");
		}
	}
	
	// set the note to the local time
	m_eGrabState = GS_DUSK;

	if (pImg->GetImageNote().length() == 0)
	{
		pLT = localtime(&curTime);
		sprintf(cs, "Local time:%d/%d/%4d at %d:%02d:%02d",
				pLT->tm_mon+1, pLT->tm_mday, pLT->tm_year+1900,
				pLT->tm_hour, pLT->tm_min, pLT->tm_sec);
		pImg->SetImageNote(cs);
	}
	
	return CE_NO_ERROR;	
}

/*
  
 GrabImage:
	 
 Grab an image into the passed image of the passed type.
 This does the whole processing for you: Starts
 and Ends the Exposure then Readsout the data.
 
*/
PAR_ERROR CSBIGCam::GrabImage(CSBIGImg *pImg, SBIG_DARK_FRAME dark)
{
	PAR_ERROR err;
	
	pImg->SetImageCanClose(FALSE);

	/* do the Once per image setup */
	if ((err = GrabSetup(pImg, dark)) == CE_NO_ERROR )
	{
		/* Grab the image */
		err = GrabMain(pImg, dark);
	}
	m_eGrabState = GS_IDLE;
	pImg->SetImageCanClose(TRUE);
	return err;	
}


/*
  
	StartExposure:
		
	Start an exposure in the camera.  Should be matched
	with an EndExposure call.
	
*/
PAR_ERROR CSBIGCam::StartExposure(SHUTTER_COMMAND shutterState)
{
	StartExposureParams 	sep;
	StartExposureParams2 	sep2;
	unsigned long 				ulExposure;
	
	if (!CheckLink())
	{
		return m_eLastError;
	}
	
	// For exposures less than 0.01 seconds use millisecond exposures
	// Note: This assumes the caller has used the GetCCDInfo command
	//       to insure the camera supports millisecond exposures
	if (m_dExposureTime < 0.01)
	{
		ulExposure = (unsigned long)(m_dExposureTime * 1000.0 + 0.5);

		if (ulExposure < 1)
		{
				ulExposure = 1;
		}
		ulExposure |= EXP_MS_EXPOSURE;
	}
	else
	{
		ulExposure = (unsigned long)(m_dExposureTime * 100.0 + 0.5);
		if (ulExposure < 1)
		{
			ulExposure = 1;
		}
	}
	
	if (GetFastReadout())
	{
			ulExposure |= EXP_FAST_READOUT;
	}
	else
	{
			ulExposure &= ~EXP_FAST_READOUT;
	}

	if (GetDualChannelMode())
	{
			ulExposure |= EXP_DUAL_CHANNEL_MODE;
	}
	else
	{
			ulExposure &= ~EXP_DUAL_CHANNEL_MODE;
	}

	if (m_eCameraType == STX_CAMERA || m_eCameraType == STT_CAMERA ||
		  m_eCameraType == STI_CAMERA || m_eCameraType == STF_CAMERA)
	{

		sep2.ccd 						= m_eActiveCCD;
		sep2.exposureTime 	= ulExposure;
		sep2.abgState 			= m_eABGState;
		sep2.openShutter 		= shutterState;
		sep2.top 						= m_nSubFrameTop;
		sep2.left 					= m_nSubFrameLeft;
		sep2.height 				= m_nSubFrameHeight;
		sep2.width 					= m_nSubFrameWidth;
		sep2.readoutMode 		= m_uReadoutMode;

		/*
		cout << "CSBIGCam::StartExposure2 -------------------------" << endl;
		cout << "sep2.ccd			: " << sep2.ccd 		<< endl;
		cout << "sep2.exposureTime	: " << sep2.exposureTime<< endl;
		cout << "sep2.abgState      : " << sep2.abgState	<< endl;
		cout << "sep2.openShutter	: " << sep2.openShutter	<< endl;
		cout << "sep2.top      		: " << sep2.top			<< endl;
		cout << "sep2.left      	: " << sep2.left		<< endl;
		cout << "sep2.height      	: " << sep2.height		<< endl;
		cout << "sep2.width      	: " << sep2.width		<< endl;
		cout << "sep2.readoutMode	: " << sep2.readoutMode	<< endl;
		cout << "--------------------------------------------------" << endl;
		*/

		return SBIGUnivDrvCommand(CC_START_EXPOSURE2, &sep2, NULL);
	}
	else
	{
		sep.ccd = m_eActiveCCD;
		sep.exposureTime = ulExposure;
		sep.abgState = m_eABGState;
		sep.openShutter = shutterState;
		return SBIGUnivDrvCommand(CC_START_EXPOSURE, &sep, NULL);
	}
}

/*
  
	EndExposure:
		
	End or abort an exposure in the camera.  Should be
	matched to a StartExposure but no damage is done
	by calling it by itself if you don't know if an
	exposure was started for example.
	
*/
PAR_ERROR CSBIGCam::EndExposure(void)
{
	EndExposureParams eep;
	
	eep.ccd = m_eActiveCCD;
	if ( CheckLink() )
		return SBIGUnivDrvCommand(CC_END_EXPOSURE, &eep, NULL);
	else
		return m_eLastError;
}

/*
  
	IsExposueComplete:
		
	Query the camera to see if the exposure in progress is complete.
	This returns TRUE if the CCD is idle (an exposure was never
	started) or if the CCD exposure is complete.
	
*/
PAR_ERROR CSBIGCam::IsExposureComplete(MY_LOGICAL &complete)
{
	QueryCommandStatusParams qcsp;
	QueryCommandStatusResults qcsr;
	
	complete = FALSE;
	if (CheckLink())
	{
		qcsp.command = CC_START_EXPOSURE;
		if (SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr) == CE_NO_ERROR)
		{
			if (m_eActiveCCD == CCD_IMAGING)
			{
				complete = (qcsr.status & 0x03) != 0x02;
			}
			else
			{
				complete = (qcsr.status & 0x0C) != 0x08;
			}
		}
	}
	return m_eLastError;
}

/*
  
	StartReadout:
		
	Start the readout process.  This should be called
	after EndExposure and should be matched with an
	EndExposure call.
	
*/
PAR_ERROR CSBIGCam::StartReadout(StartReadoutParams srp)
{
	if ( CheckLink() )
		return SBIGUnivDrvCommand(CC_START_READOUT, &srp, NULL);
	else
		return m_eLastError;
}

/*
  
	EndReadout:
		
	End a readout started with StartReadout.
	Don't forget to make this call to prepare the
	CCD for idling.
	
*/
PAR_ERROR CSBIGCam::EndReadout(void)
{
	EndReadoutParams erp;
	
	erp.ccd = m_eActiveCCD;
	if ( CheckLink() )
		return SBIGUnivDrvCommand(CC_END_READOUT, &erp, NULL);
	else
		return m_eLastError;
}

/*
  
	ReadoutLine:
		
	Readout a line of data from the camera, optionally
	performing a dark subtraction, placing the data
	at dest.
	
*/
PAR_ERROR CSBIGCam::ReadoutLine(ReadoutLineParams rlp, MY_LOGICAL darkSubtract,
								unsigned short *dest)
{
	if ( CheckLink() ) {
		if ( darkSubtract )
			return SBIGUnivDrvCommand(CC_READ_SUBTRACT_LINE, &rlp, dest);
		else
			return SBIGUnivDrvCommand(CC_READOUT_LINE, &rlp, dest);
	}
	else
		return m_eLastError;
	
}

/*
  
	DumpLines:
		
	Discard data from one or more lines in the camera.
	
*/
PAR_ERROR CSBIGCam::DumpLines(unsigned short noLines)
{
	DumpLinesParams dlp;
	
	dlp.ccd = m_eActiveCCD;
	dlp.lineLength = noLines;
	dlp.readoutMode = m_uReadoutMode;
	if ( CheckLink() )
		return SBIGUnivDrvCommand(CC_DUMP_LINES, &dlp, NULL);
	else
		return m_eLastError;
}

/*
  
	SetTemperatureRegulation:
		
	Enable or disable the temperatre controll at
	the passed setpoint which is the absolute
	(not delta) temperature in degrees C.
	
*/
PAR_ERROR CSBIGCam::SetTemperatureRegulation(MY_LOGICAL enable, double setpoint)
{
	SetTemperatureRegulationParams strp;
	
	if ( CheckLink() ) {
		strp.regulation = enable ? REGULATION_ON : REGULATION_OFF;
		strp.ccdSetpoint = DegreesCToAD(setpoint, TRUE);
		return SBIGUnivDrvCommand(CC_SET_TEMPERATURE_REGULATION, &strp, NULL);
	}
	else
		return m_eLastError;
}

/*
  
	QueryTemperatureStatus:
		
	Get whether the cooling is enabled, the CCD temp
	and setpoint in degrees C and the percent power
	applied to the TE cooler.
	
*/
PAR_ERROR CSBIGCam::QueryTemperatureStatus(MY_LOGICAL &enabled, double &ccdTemp,
										   double &setpointTemp, double &percentTE)
{
	QueryTemperatureStatusResults qtsr;
	
	if ( CheckLink() ) {
		if ( SBIGUnivDrvCommand(CC_QUERY_TEMPERATURE_STATUS, NULL, &qtsr) == CE_NO_ERROR )
		{
			enabled = qtsr.enabled;
			ccdTemp = ADToDegreesC(qtsr.ccdThermistor, TRUE);
			setpointTemp = ADToDegreesC(qtsr.ccdSetpoint, TRUE);
			percentTE = qtsr.power/255.0;
		}
	}
	return m_eLastError;
}

/*
  
	GetCCDTemperature:
		
	Read and return the current CCD temperature.
	
*/
PAR_ERROR CSBIGCam::GetCCDTemperature(double &ccdTemp)
{
	double setpointTemp, percentTE;
	MY_LOGICAL teEnabled;
	
	return QueryTemperatureStatus(teEnabled, ccdTemp, setpointTemp, percentTE);
}

/*
  
	ActivateRelay:
		
	Activate one of the four relays for the passed
	period of time.  Cancel a relay by passing
	zero for the time.
	
*/
PAR_ERROR CSBIGCam::ActivateRelay(CAMERA_RELAY relay, double time)
{
	ActivateRelayParams arp;
	unsigned short ut;
	
	if ( CheckLink() ) {
		arp.tXMinus = arp.tXPlus = arp.tYMinus = arp.tYPlus = 0;
		if ( time >= 655.35 )
			ut = 65535;
		else
			ut = (unsigned short)(time/0.01);
		switch ( relay ){
		case RELAY_XPLUS:  arp.tXPlus = ut; break;
		case RELAY_XMINUS: arp.tXMinus = ut; break;
		case RELAY_YPLUS:  arp.tYPlus = ut; break;
		case RELAY_YMINUS: arp.tYMinus = ut; break;
		}
		return SBIGUnivDrvCommand(CC_ACTIVATE_RELAY, &arp, NULL);
	}
	else
		return m_eLastError;
}

/*

	IsRelayActive:

	Returns tru in the passed parameter if the passed
	relay is active.

*/
PAR_ERROR CSBIGCam::IsRelayActive(CAMERA_RELAY relay, MY_LOGICAL &active)
{
	QueryCommandStatusParams qcsp;
	QueryCommandStatusResults qcsr;

	if ( CheckLink() ) {
		qcsp.command = CC_ACTIVATE_RELAY;
		if ( SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr) == CE_NO_ERROR ) {
			active = FALSE;
			switch ( relay ){
			case RELAY_XPLUS:  active = ((qcsr.status & 8) != 0); break;
			case RELAY_XMINUS: active = ((qcsr.status & 4) != 0); break;
			case RELAY_YPLUS:  active = ((qcsr.status & 2) != 0); break;
			case RELAY_YMINUS: active = ((qcsr.status & 1) != 0); break;
			}
		}
	}
	return m_eLastError;
}

/*
  
	AOTipTilt:
		
	Send a tip/tilt command to the AO-7.
	
*/
PAR_ERROR CSBIGCam::AOTipTilt(AOTipTiltParams attp)
{
	if ( CheckLink() )
		return SBIGUnivDrvCommand(CC_AO_TIP_TILT, &attp, NULL);
	else
		return m_eLastError;
}

/*
  
	CFWCommand:
		
	Send a command to the Color Filter Wheel.
	
*/
PAR_ERROR CSBIGCam::CFWCommand(CFWParams cfwp, CFWResults &cfwr)
{
	if ( CheckLink() )
		return SBIGUnivDrvCommand(CC_CFW, &cfwp, &cfwr);
	else
		return m_eLastError;
}

/*
 
	InitializeShutter:
 
	Initialize the shutter in the camera.

*/
PAR_ERROR CSBIGCam::InitializeShutter(void)
{
	PAR_ERROR res;
	MiscellaneousControlParams mcp;
	QueryCommandStatusParams qcsp;
	QueryCommandStatusResults qcsr;
	
	qcsp.command = CC_MISCELLANEOUS_CONTROL;
	res = SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr);
	if ( res == CE_NO_ERROR ) {
		mcp.fanEnable = (qcsr.status & 0x100) != 0; 
		mcp.shutterCommand = SC_INITIALIZE_SHUTTER;
		mcp.ledState = LED_ON;
		res =SBIGUnivDrvCommand(CC_MISCELLANEOUS_CONTROL, &mcp, NULL);
	}
	return res;
}

/*
  
	EstablishLink:
		
	Once the driver and device are open call this to
	establish a communications link with the camera.
	May be called multiple times without problem.
	
	If there's no error and you want to find out what
	model of camera was found use the GetCameraType()
	function.
	
*/
PAR_ERROR CSBIGCam::EstablishLink(void)
{
	PAR_ERROR res;
	EstablishLinkResults elr;
	EstablishLinkParams elp;
	GetCCDInfoParams gcip;
	GetCCDInfoResults0 gcir0;
	
	res = SBIGUnivDrvCommand(CC_ESTABLISH_LINK, &elp, &elr);
	if ( res == CE_NO_ERROR ) {
		m_eCameraType = (CAMERA_TYPE)elr.cameraType;
		m_nFirmwareVersion = 0;
		gcip.request = CCD_INFO_IMAGING;
		if ( (res = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir0)) == CE_NO_ERROR )
			m_nFirmwareVersion = gcir0.firmwareVersion;
	}
	return res;
}

/*
  
	GetErrorString:
		
	Returns a ANSI C++ standard string object describing
	the error code returned from the lass call to the driver.
	
*/
string CSBIGCam::GetErrorString()
{
	return GetErrorString(m_eLastError);
}

/*
  
	CheckLink:
		
	If a link has been established to a camera return TRUE.
	Otherwise try to establish a link and if successful
	return TRUE.  If fails return FALSE.
	
*/
MY_LOGICAL CSBIGCam::CheckLink(void)
{
	if ( m_eCameraType != NO_CAMERA || EstablishLink() == CE_NO_ERROR )
		return TRUE;
	else
		return FALSE;
}

/*
  
	DegreesCToAD:
		
	Convert temperatures in degrees C to
	camera AD setpoints.
	
*/
unsigned short CSBIGCam::DegreesCToAD(double degC, MY_LOGICAL ccd /* = TRUE */)
{
	double r;
	unsigned short setpoint;
	
	if ( degC < -50.0 )
		degC = -50.0;
	else if ( degC > 35.0 )
		degC = 35.0;
	if ( ccd ) {
		r = R0 * exp(log(RR_CCD)*(T0 - degC)/DT_CCD);
		setpoint = (unsigned short)(MAX_AD/((RB_CCD/r) + 1.0) + 0.5);
	} else {
		r = R0 * exp(log(RR_AMB)*(T0 - degC)/DT_AMB);
		setpoint = (unsigned short)(MAX_AD/((RB_AMB/r) + 1.0) + 0.5);
	}
	return setpoint;
}

/*
  
	ADToDegreesC:
		
	Convert camera AD temperatures to
	degrees C
	
*/
double CSBIGCam::ADToDegreesC(unsigned short ad, MY_LOGICAL ccd /* = TRUE */)
{
	double r, degC;
	
	if ( ad < 1 )
		ad = 1;
	else if ( ad >= MAX_AD - 1 )
		ad = MAX_AD - 1;
	if ( ccd ) {
		r = RB_CCD/(((double)MAX_AD/ad) - 1.0);
		degC = T0 - DT_CCD*(log(r/R0)/log(RR_CCD));
	} else {
		r = RB_AMB/(((double)MAX_AD/ad) - 1.0);
		degC = T0 - DT_AMB*(log(r/R0)/log(RR_AMB));
	}
	return degC;
}

/*

	CFW Functions
	
*/

/*

	SetCFWModel:
	
	Select and try to query the status for the selected
	model CFW.  For a serial base CFW-10 the COM port
	is specified in the second parameter and for all others
	it is ignored.
	
*/
PAR_ERROR CSBIGCam::SetCFWModel(CFW_MODEL_SELECT cfwModel, CFW_COM_PORT comPort /*= CFWPORT_COM1*/)
{
	PAR_ERROR  res = CE_NO_ERROR;
	CFWParams  cfwp;
	CFWResults cfwr;
	
	// close existing port
	m_eCFWError = CFWE_NONE;
	if ( m_eCFWModel != CFWSEL_UNKNOWN )
	{
		cfwp.cfwModel = m_eCFWModel;
		cfwp.cfwCommand = CFWC_CLOSE_DEVICE;
		if ( (res = CFWCommand(cfwp, cfwr)) != CE_NO_ERROR ) {
			m_eCFWError = (CFW_ERROR)cfwr.cfwError;
			return m_eLastError = res;
		}
	}

	// take on this model
	m_eCFWModel = cfwModel;
	
	if ( m_eCFWModel != CFWSEL_UNKNOWN )
	{
		// open new port
		cfwp.cfwModel = m_eCFWModel;
		cfwp.cfwCommand = CFWC_OPEN_DEVICE;
		cfwp.cfwParam1 = comPort;
		if ( (res = CFWCommand(cfwp, cfwr)) != CE_NO_ERROR )
		{
			m_eCFWError = (CFW_ERROR)cfwr.cfwError;
			return m_eLastError = res;
		}
			
		// query it to make sure it's there
		cfwp.cfwCommand = CFWC_QUERY;
		res = CFWCommand(cfwp, cfwr);
		m_eCFWError = (CFW_ERROR)cfwr.cfwError;
	}
	
	return m_eLastError = res;
}

/*

	SetCFWPosition:
	
	Send a command to the CFW to position it then return without
	waiting for it to finish moving.
	
*/
PAR_ERROR CSBIGCam::SetCFWPosition(CFW_POSITION position)
{
	PAR_ERROR res = CE_NO_ERROR;
	CFWParams cfwp;
	CFWResults cfwr;
	
	if ( m_eCFWModel == CFWSEL_UNKNOWN )
	{
		res = CE_CFW_ERROR;
		m_eCFWError = CFWE_DEVICE_NOT_OPEN;
	}
	else
	{
		cfwp.cfwModel = m_eCFWModel;
		cfwp.cfwCommand = CFWC_GOTO;
		cfwp.cfwParam1 = position;
		res = CFWCommand(cfwp, cfwr);
		m_eCFWError = (CFW_ERROR)cfwr.cfwError;
	}
	return m_eLastError = res;
}

/*

	GetCFWPositionAndStatus:
	
	Return the CFW's current position.
	
*/
PAR_ERROR CSBIGCam::GetCFWPositionAndStatus(CFW_POSITION &position, CFW_STATUS &status)
{
	PAR_ERROR res = CE_NO_ERROR;
	CFWParams cfwp;
	CFWResults cfwr;
	
	if ( m_eCFWModel == CFWSEL_UNKNOWN )
	{
		res = CE_CFW_ERROR;
		m_eCFWError = CFWE_DEVICE_NOT_OPEN;
	}
	else
	{
		cfwp.cfwModel = m_eCFWModel;
		cfwp.cfwCommand = CFWC_QUERY;
		res = CFWCommand(cfwp, cfwr);
		m_eCFWError = (CFW_ERROR)cfwr.cfwError;
		position = (CFW_POSITION)cfwr.cfwPosition;
		status = (CFW_STATUS)cfwr.cfwStatus;
	}
	return m_eLastError = res;
}

/*
	GetCFWMaxPosition:
	
	Return the number of filter positions in the
	selected model CFW.
	
*/
PAR_ERROR CSBIGCam::GetCFWMaxPosition(CFW_POSITION &position)
{
	PAR_ERROR res = CE_NO_ERROR;
	CFWParams cfwp;
	CFWResults cfwr;
	
	if ( m_eCFWModel == CFWSEL_UNKNOWN )
	{
		res = CE_CFW_ERROR;
		m_eCFWError = CFWE_DEVICE_NOT_OPEN;
	}
	else
	{
		cfwp.cfwModel = m_eCFWModel;
		cfwp.cfwCommand = CFWC_GET_INFO;
		cfwp.cfwParam1 = CFWG_FIRMWARE_VERSION;
		res = CFWCommand(cfwp, cfwr);
		m_eCFWError = (CFW_ERROR)cfwr.cfwError;
		position = (CFW_POSITION)cfwr.cfwResult2;
	}
	return m_eLastError = res;
}

/*

	GetCFWErrorString:
	
	Return a string describing the last (or passed) CFW Error.
	
*/
//typedef enum { CFWE_NONE, CFWE_BUSY, CFWE_BAD_COMMAND, CFWE_CAL_ERROR, CFWE_MOTOR_TIMEOUT,
//				CFWE_BAD_MODEL, CFWE_DEVICE_NOT_CLOSED, CFWE_DEVICE_NOT_OPEN,
//				CFWE_I2C_ERROR } CFW_ERROR;
static const char *CFW_ERROR_STRINGS[] = {
	"No CFW Error", "CFW Busy Error", "Bad CFW Command", "CFW Calibration Error",
	"CFW Motor Timeout Error", "CFW Model Error", "CFW Device Not Closed", "CFW Device Not Open",
	"CFW I2C Error" };
string CSBIGCam::GetCFWErrorString(CFW_ERROR err)
{
	string s = "";
	if ( err < CFWE_NONE || err > CFWE_I2C_ERROR )
		s = "Unknown CFW Error";
	else
		s = CFW_ERROR_STRINGS[err];
	return s;
}

/*

	GetCFWErrorString:
	
	Return a text based description of the passed
	CFW error.
	
*/
string CSBIGCam::GetCFWErrorString(void)
{
	return GetCFWErrorString(m_eCFWError);
}

/*

	GetFormattedCameraInfo:
	
	Return a formatted string describing
	the Camera's Information.  If htmlFormat
	is TRUE then the string is a two-column
	HTML table, lese it is a tab-deliminated
	rows with a carriage return per row.
	
*/
/* typedef enum { ST7_CAMERA=4, ST8_CAMERA, ST5C_CAMERA, TCE_CONTROLLER,
 ST237_CAMERA, STK_CAMERA, ST9_CAMERA, STV_CAMERA, ST10_CAMERA,
 ST1K_CAMERA, ST2K_CAMERA, STL_CAMERA, ST402_CAMERA, STX_CAMERA,
 ST4K_CAMERA, STT_CAMERA, STI_CAMERA, STF8300_CAMERA, NEXT_CAMERA, NO_CAMERA=0xFFFF } CAMERA_TYPE; */
static const char *CAMERA_TYPE_NAMES[] = {
	"ST-7", "ST-8", "ST-5C", "TCE",
	"ST-237", "STK", "ST-9", "STV", "ST-10",
	"ST-1001", "ST-2000", "STL", "ST-402", "STX",
	"ST-4000", "STT", "ST-i", "STF-8300"};
static const char *AD_RES_NAMES[] = { "Unknown", "12 Bits", "16 Bits"};
static const char *CFW_NAMES[] = {"Unknown", "External", "Shutter Wheel", "CFW-5C"};
PAR_ERROR CSBIGCam::GetFormattedCameraInfo(string &ciStr, MY_LOGICAL htmlFormat /* = TRUE */)
{
	string s, ca, cb, cs, sbr, br, fon, foff;
	PAR_ERROR res = CE_NO_ERROR;
	GetCCDInfoParams gcip;
	GetCCDInfoResults0 gcir0;
	GetCCDInfoResults2 gcir2;
	GetCCDInfoResults3 gcir3;
	GetCCDInfoResults4 gcir4;
	GetDriverInfoParams gdip;
	GetDriverInfoResults0 gdir;
	QueryTemperatureStatusResults qtsr;
	char c[80];
	double d;
	
	if ( htmlFormat ) {
		ca = "<TR><TD valign=\"top\">";
		cb = "</TD></TR>\r";
		cs = "</TD><TD>";
		fon = "<b><u>";
		foff = "</b></u>";
		sbr = "<TR><TD>&nbsp;</TD><TD></TD></TR>\r";
		br = "<br>";
		ciStr = "<TABLE border=\"0\" cellspacing=\"1\" summary = \"\">\r";
	} else {
		ca = fon = foff = "";
		cb = sbr = "\r";
		br = "\r\t";
		cs = "\t";
		ciStr = "";
	}
	
	do {
		ciStr += ca + fon + "Camera Info:" + foff + cb;
		// Camera Information
		gcip.request = CCD_INFO_IMAGING;
		if ( (res = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir0)) != CE_NO_ERROR )
			break;
		if ( gcir0.cameraType >= ST7_CAMERA && gcir0.cameraType <= STF_CAMERA ) {
			s = CAMERA_TYPE_NAMES[gcir0.cameraType - ST7_CAMERA];
			if ( gcir0.cameraType == ST5C_CAMERA || gcir0.cameraType == ST237_CAMERA ) {
				gcip.request = CCD_INFO_EXTENDED_5C;
				if ( (res = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir3)) != CE_NO_ERROR )
						break;
				if ( gcir3.adSize == AD_16_BITS )
					s += "A";
			} else {
				if ( strstr(gcir0.name, "Color") )
					s = "Color " + s;
				gcip.request = CCD_INFO_EXTENDED;
				if ( (res = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir2)) != CE_NO_ERROR )
					break;
				s += (gcir2.imagingABG == ABG_PRESENT) ? " with ABG" : " without ABG";
			}
			
		} else
			s = "Unknown";
		ciStr += ca + "Camera Type:" + cs + s + cb;
		ciStr += ca + "Camera Name:" + cs + gcir0.name + cb;
		d = hex2double(gcir0.firmwareVersion);
		sprintf(c, "%1.2lf", d);
		ciStr += ca + "Firmware Version:" + cs + c + cb;
		gcip.request = CCD_INFO_EXTENDED;
		if ( gcir0.cameraType != ST5C_CAMERA && gcir0.cameraType != ST237_CAMERA )
			ciStr += ca + "Serial Number:" + cs + gcir2.serialNumber + cb;
		if ( gcir0.cameraType == ST5C_CAMERA || gcir0.cameraType == ST237_CAMERA ) {
			if ( gcir3.adSize > AD_16_BITS ) gcir3.adSize = AD_UNKNOWN;
			ciStr += ca + "A/D Resolution:" + cs + AD_RES_NAMES[gcir3.adSize] + cb;
			if ( gcir3.filterType > FW_FILTER_WHEEL ) gcir3.filterType = FW_UNKNOWN;
			ciStr += ca + "Internal CFW:" + cs + CFW_NAMES[gcir3.filterType] + cb;
		} else
			ciStr += ca + "A/D Resolution:" + cs + "16 Bits" + cb;
			
		gcip.request = CCD_INFO_EXTENDED2_TRACKING;
		if ( (res = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir4)) == CE_NO_ERROR )
		{
			sprintf(c, "%s", gcir4.capabilitiesBits & CB_CCD_EXT_TRACKER_MASK ? "Can be used" : "Doesn't Support");
			ciStr += ca + "External Tracker:" + cs + c + cb;
		}

		// CCD Temperature
		if ( (res = GetCCDTemperature(d)) != CE_NO_ERROR )
			break;
		sprintf(c,"%1.2lf Degrees C", d);
		ciStr += ca + "CCD Temperature:" + cs + c + cb;		

		// Ambient Temperature
		if ( (res = SBIGUnivDrvCommand(CC_QUERY_TEMPERATURE_STATUS, NULL, &qtsr)) != CE_NO_ERROR )
			break;
		sprintf(c,"%1.2lf Degrees C", ADToDegreesC(qtsr.ambientThermistor, FALSE));
		ciStr += ca + "Ambient Temp.:" + cs + c + cb;
		
		// Imager Readout Info
		ciStr += sbr + ca +  fon + "Imaging CCD:" + foff + cs + cb;
		sprintf(c, "%d x %d", gcir0.readoutInfo[0].width, gcir0.readoutInfo[0].height);
		ciStr += ca + "Number of Pixels:" + cs + c + cb;
		sprintf(c, "%1.2lf x %1.2lf microns", hex2double(gcir0.readoutInfo[0].pixelWidth),
			hex2double(gcir0.readoutInfo[0].pixelHeight));
		ciStr += ca + "Pixel Size:" + cs + c + cb;
		sprintf(c, "%1.2lf e-/ADU", hex2double(gcir0.readoutInfo[0].gain));
		ciStr += ca + "Electronic Gain:" + cs + c + cb;
		gcip.request = CCD_INFO_EXTENDED2_IMAGING;
		if ( (res = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir4)) != CE_NO_ERROR )
			break;
		sprintf(c,"%s", gcir4.capabilitiesBits & CB_CCD_TYPE_MASK ? "Frame Transfer CCD" : "Full Frame CCD");
		ciStr += ca + "CCD Type:" + cs + c + cb;
		sprintf(c, "%s", gcir4.capabilitiesBits & CB_CCD_ESHUTTER_MASK ? "Yes, Supports ms Exposures" : "No");
		ciStr += ca + "Electronic Shutter:" + cs + c + cb;
		sprintf(c, "%s", gcir4.capabilitiesBits & CB_FRAME_BUFFER_MASK ? "Yes, Camera has Frame Buffer" : "No");
		ciStr += ca + "Frame Buffer:" + cs + c + cb;

		// Tracker Readout Indo
		gcip.request = CCD_INFO_TRACKING;
		if ( (res = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir0)) == CE_NO_ERROR ) {
			ciStr += sbr + ca + fon + "Tracking CCD:" + cs + cb;
			sprintf(c, "%d x %d", gcir0.readoutInfo[0].width, gcir0.readoutInfo[0].height);
			ciStr += ca + "Number of Pixels:" + cs + c + cb;
			sprintf(c, "%1.2lf x %1.2lf microns", hex2double(gcir0.readoutInfo[0].pixelWidth),
				hex2double(gcir0.readoutInfo[0].pixelHeight));
			ciStr += ca + "Pixel Size:" + cs + c + cb;
			sprintf(c, "%1.2lf e-/ADU", hex2double(gcir0.readoutInfo[0].gain));
			ciStr += ca + "Electronic Gain:" + cs + c + cb;
			gcip.request = CCD_INFO_EXTENDED2_TRACKING;
			if ( (res = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir4)) != CE_NO_ERROR )
				break;
			sprintf(c,"%s", gcir4.capabilitiesBits & CB_CCD_TYPE_MASK ? "Frame Transfer CCD" : "Full Frame CCD");
			ciStr += ca + "CCD Type:" + cs + c + cb;
			sprintf(c, "%s", gcir4.capabilitiesBits & CB_CCD_ESHUTTER_MASK ? "Yes, Supports ms Exposures" : "No");
			ciStr += ca + "Electronic Shutter:" + cs + c + cb;
			sprintf(c, "%s", gcir4.capabilitiesBits & CB_FRAME_BUFFER_MASK ? "Yes, Camera has Frame Buffer" : "No");
			ciStr += ca + "Frame Buffer:" + cs + c + cb;
		}
				
		// Driver information
		ciStr += sbr + ca + fon + "Driver Info:" + cs + cb;
		gdip.request = DRIVER_STD;
		if ( (res = SBIGUnivDrvCommand(CC_GET_DRIVER_INFO, &gdip, &gdir)) != CE_NO_ERROR )
			break;
		sprintf(c,"Version %1.2lf", hex2double(gdir.version));
		ciStr += ca + "Driver Info:" + cs + gdir.name + br + c + cb;
		gdip.request = DRIVER_EXTENDED;
		if ( (res = SBIGUnivDrvCommand(CC_GET_DRIVER_INFO, &gdip, &gdir)) != CE_NO_ERROR )
			break;
		sprintf(c,"Version %1.2lf", hex2double(gdir.version));
		ciStr += ca + "Ext. Driver Info:" + cs + gdir.name + br + c + cb;
		gdip.request = DRIVER_USB_LOADER;
		if ( (res = SBIGUnivDrvCommand(CC_GET_DRIVER_INFO, &gdip, &gdir)) == CE_NO_ERROR ) {
			sprintf(c,"Version %1.2lf", hex2double(gdir.version));
			ciStr += ca + "USB Loader Info:" + cs + gdir.name + br + c + cb;
		}
		ciStr += ca + "SBIGCam Class:" + cs + "Version " + VERSION_STR + cb;
	} while ( FALSE );
	if ( htmlFormat )
		ciStr += "</TABLE>";
	return m_eLastError = res;
}

