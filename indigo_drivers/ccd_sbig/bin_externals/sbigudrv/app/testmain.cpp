/*
	main.cpp - This file is a test of the CSBIGCam and CSBIGImage classes.

	1. This software (c)2004 Santa Barbara Instrument Group.
	2. This free software is provided as an example of how 
	    to communicate with SBIG cameras.  It is provided AS-IS
	    without any guarantees by SBIG of suitability for a 
	    particular purpose and without any guarantee to be 
	    bug-free.  If you use it you agree to these terms and
	    agree to do so at your own risk.
 3.  Any distribution of this source code to include these terms.

	Revision History
	Date	 - Modification
	
  2012/11/23 - Updated version of the testmain application tested on Debian 6.0.6 (JS)
               This application grabs number of requested images and stores them
               in FITS format. User has to install the cfitsio library. This
               application also uses a new SBIG's universal driver/library which
               utilizes a new libusb ver. 1.0 library. So, user has to install
               it too. On Debian & Ubuntu distros use the following command:
               sudo apt-get install libusb-1.0-0-dev

  2004/04/26 - Initial release - Matt Longmire (SBIG)

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <string>
#include <libusb.h>

#include "lpardrv.h"
#include "csbigcam.h"
#include "csbigimg.h"

using namespace std;

#define LINE_LEN 	80

// TODO
PAR_ERROR openDriver();
PAR_ERROR closeDriver();
PAR_ERROR openDevice(SBIG_DEVICE_TYPE dev);
PAR_ERROR closeDevice();
PAR_ERROR establishLink();

PAR_ERROR startExposure(bool			dualChannelMode,
			bool			fastReadout,
			CCD_REQUEST 		ccd,
			double 			expTime,
			SHUTTER_COMMAND 	shutterCommand,
			READOUT_BINNING_MODE 	readoutMode,
			unsigned short 		top,
			unsigned short 		left,
			unsigned short 		height,
			unsigned short 		width);

PAR_ERROR endExposure(CCD_REQUEST ccd);

PAR_ERROR monitorExposure();

PAR_ERROR startReadout(CCD_REQUEST 		ccd,
                       READOUT_BINNING_MODE	readoutMode,
                       unsigned short 		top,
		       unsigned short 		left,
		       unsigned short 		height,
		       unsigned short 		width);

PAR_ERROR	endReadout(CCD_REQUEST ccd);

PAR_ERROR	ccdReadout(CSBIGImg*		pImg,
			   bool 		dualChannelMode,
			   CCD_REQUEST 		ccd,
			   READOUT_BINNING_MODE	readoutMode,
			   unsigned short 	top,
			   unsigned short 	left,
			   unsigned short 	height,
			   unsigned short 	width);


PAR_ERROR takeImage(int argc, char *argv[]);
PAR_ERROR grabImageTest(int argc, char *argv[]);
PAR_ERROR fpsTest(int argc, char *argv[]);
PAR_ERROR queryUsbTest();
PAR_ERROR queryEthernetTest();
PAR_ERROR checkEthernetCameras();
PAR_ERROR cfwTest();
PAR_ERROR cfwInit (unsigned short cfwModel, CFWResults* pRes);
PAR_ERROR cfwGoto (unsigned short cfwModel, int cfwPosition, CFWResults* pRes);
PAR_ERROR checkQueryTemperatureStatus();
void  	  cfwShowResults(CFWResults* pRes);
PAR_ERROR readEeprom();
PAR_ERROR getCcdInfo0(GetCCDInfoResults0* res);

float 	  bcd2float(unsigned short bcd);

int main(int argc, char *argv[])
{
  grabImageTest(argc, argv);	// grabs image and save it in FITS or SBIG file format
  //takeImage (argc, argv);	// just grab an image into memory
  //fpsTest(argc, argv);	// measures frames per seconds
  //queryUsbTest();		// tests query usb command
  //queryEthernetTest();        
  //checkEthernetCameras();
  //cfwTest();			// tests the color filter wheel
  //checkQueryTemperatureStatus();
}
//==============================================================
// GRAB IMAGE TEST
//==============================================================
PAR_ERROR grabImageTest(int argc, char *argv[])
{
	PAR_ERROR 	err = CE_NO_ERROR;
	string		filePath, filePathName;
	bool		bFitsType;
	int		numOfImages;
	bool		bLightFrame;
	double		expTime;
	int		rm;
	int             top, left, width, fullWidth, height, fullHeight;
	bool		bFastReadout, bDualChannelMode;
	SBIG_FILE_ERROR	ferr;

  if (argc != 13)
	{
		cout << "Application startup error: Bad number of input parameters. " << endl;
		cout << "Format : ./testapp filePath               fileType imgCount imgType expTime rm  top left width heigh fr dcm" << endl;
		cout << "Example: ./testapp /observations/img/raw/  FITS     100       LF     0.001  1x1  0    0  1000  1000   1  1 " << endl;
		cout << "Example: ./testapp /observations/img/raw/  SBIG      5        DF     15.5   2x2  0    0   0     0     1  1 " << endl;
		return CE_BAD_PARAMETER;
	}

	filePath = argv[1];
	int len  = filePath.length();
	if (filePath[len-1] != '/')
	{
		filePath.append("/");
	}

	//bFileType
	bFitsType = false;
	if (strcmp(argv[2], "FITS") == 0)
	{
		bFitsType = true;
	}

	numOfImages = atoi(argv[3]);

	//imgType
	bLightFrame = false; //suppose DF
	if (strcmp(argv[4], "LF") == 0)
	{
		bLightFrame = true;
	}

	expTime = atof(argv[5]);

	//readout mode
	rm = 0; //suppose 1x1
	if (strcmp(argv[6], "2x2") == 0)
	{
		rm = 1;
	}
	else if (strcmp(argv[6], "3x3") == 0)
	{
		rm = 2;
	}

	// image size: if all params, ie. top, left, width and height are zero,
	// the full size of the CCD image is used.

	// top
	top = atoi(argv[7]);

	// left
	left = atoi(argv[8]);

	// width
	width = atoi(argv[9]);

	// height
	height = atoi(argv[10]);

	//bFastReadout
	bFastReadout = false;
	if (strcmp(argv[11], "1") == 0)
	{
		bFastReadout = true;
	}

	//bDualChannelMode
	bDualChannelMode = false;
	if (strcmp(argv[12], "1") == 0)
	{
		bDualChannelMode = true;
	}

	cout << "Input parameters      : "
	     << "filePath = " 	    << filePath
	     << ", bFitsType = "    << bFitsType
	     << ", numOfImages = " 	<< numOfImages
	     << ", bLightFrame = "  << bLightFrame
	     << ", exposure time = "<< expTime
	     << ", readout mode = " << rm
	     << ", top = " << top
	     << ", left = " << left
	     << ", width = " << width
	     << ", height = " << height
	     << ", bFastReadout = " << bFastReadout
	     << ", bDualChannelMode = " << bDualChannelMode
	     << endl;

	// update filepath:
	if (bLightFrame)
	{
		filePath += "LF_";
	}
	else
	{
		filePath += "DF_";
	}

	// create SBIG Img object
	CSBIGImg* pImg = new CSBIGImg;

	// create SBIG camera object
	CSBIGCam* pCam = new CSBIGCam(DEV_USB1);

	if ((err = pCam->GetError()) != CE_NO_ERROR)
	{
		cout << "CSBIGCam error        : " << pCam->GetErrorString(err) << endl;
		return err;
	}

	// establish link to the camera
	if ((err = pCam->EstablishLink()) != CE_NO_ERROR)
	{
		cout << "Establish link error  : " << pCam->GetErrorString(err) << endl;
		return err;
	}
	cout << "Link established to   : " << pCam->GetCameraTypeString() << endl;

	// set camera params
	pCam->SetActiveCCD(CCD_IMAGING);
	pCam->SetExposureTime(expTime);
	pCam->SetReadoutMode(rm);
	pCam->SetABGState(ABG_LOW7);
	pCam->SetFastReadout(bFastReadout);
	pCam->SetDualChannelMode(bDualChannelMode);

	// update width and height of the image
	pCam->GetFullFrame(fullWidth, fullHeight);

	if (width == 0)
	{
		width = fullWidth;
	}

	if (height == 0)
	{
		height = fullHeight;
	}

	pCam->SetSubFrame(left, top, width, height);
	pImg->AllocateImageBuffer(height, width);

	// take series of images
	for (int i = 1; i <= numOfImages; i++)
	{
		do
		{
			if (bLightFrame)
			{

				#ifndef CHECK_STI_FRAME_RATES
				cout << "Taking light frame no.: " << i << endl;
				#endif

				if ((err = pCam->GrabImage(pImg, SBDF_LIGHT_ONLY)) != CE_NO_ERROR)
				{
					cout << "CSBIGCam error        : " << pCam->GetErrorString(err) << endl;
					break;
				}
			}
			else
			{

				#ifndef CHECK_STI_FRAME_RATES
				cout << "Taking dark  frame no.: " << i << endl;
				#endif

				if ((err = pCam->GrabImage(pImg, SBDF_DARK_ONLY)) != CE_NO_ERROR)
				{
					cout << "CSBIGCam error        : " << pCam->GetErrorString(err) << endl;
					break;
				}
			}

			char             timeBuf[128];
			struct  tm*      pTm;
			struct  timeval  tv;
			struct  timezone tz;
			
			gettimeofday(&tv, &tz);
			pTm = localtime(&(tv.tv_sec));
			sprintf(timeBuf, "%04d-%02d-%02dT%02d:%02d:%02d.%03ld",
				pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday,
				pTm->tm_hour,        pTm->tm_min,     pTm->tm_sec, (tv.tv_usec/1000));

			// create filePathName:
			filePathName = filePath;
			filePathName += timeBuf;

			if (bFitsType)
			{
				filePathName += ".fits";
				if ((ferr = pImg->SaveImage(filePathName.c_str(), SBIF_FITS)) != SBFE_NO_ERROR)
				{
					cout << "SBIF_FITS format save error: " << ferr << endl;
					break;
				}
			}
			else
			{
				filePathName += ".sbig";
				if ((ferr = pImg->SaveImage(filePathName.c_str(), SBIF_COMPRESSED)) != SBFE_NO_ERROR)
				{
					cout << "SBIF_COMPRESSED format save error: " << ferr << endl;
					break;
				}
			}	

			#ifndef CHECK_STI_FRAME_RATES
			cout << "File saved as         : " << filePathName << endl;
			#endif

		}
		while(0);

		if (err != CE_NO_ERROR)
		{
			break;
		}

	} // for

	// close sbig device
	if ((err = pCam->CloseDevice()) != CE_NO_ERROR)
	{
		cout << "CSBIGCam error: " << pCam->GetErrorString(err) << endl;
	}

	// close sbig driver
	if((err = pCam->CloseDriver()) != CE_NO_ERROR)
	{
		cout << "CSBIGCam error: " << pCam->GetErrorString(err) << endl;
	}

	// delete objects
	if(pImg)
	{
		delete pImg;
	}

	if(pCam)
	{
		delete pCam;
	}

	cout << "The End..." << endl;
	return (err);
}
//==============================================================
// FPS TEST
//==============================================================

// TODO

PAR_ERROR fpsTest(int argc, char *argv[])
{
	PAR_ERROR 	err = CE_NO_ERROR;
	string		filePath, filePathName;
	bool		bFitsType;
	int		numOfImages;
	bool		bLightFrame;
	double		expTime;
	int             top, left, width, fullWidth, height, fullHeight;
	bool		bFastReadout, bDualChannelMode;
	int		rm;
	SBIG_FILE_ERROR	ferr;
	double 		fps;
	struct timeval 	begin, end;

	if (argc != 13)
	{
		cout << "Application startup error: Bad number of input parameters. " << endl;
		cout << "Format : ./testmain filePath               fileType imgCount imgType expTime rm  top left width heigh fr dcm" << endl;
		cout << "Example: ./testmain /observations/img/raw/  FITS     100       LF     0.001  1x1  0    0  1000  1000   1  1 " << endl;
		cout << "Example: ./testmain /observations/img/raw/  SBIG      5        DF     15.5   2x2  0    0   0     0     1  1 " << endl;
		return CE_BAD_PARAMETER;
	}
	
	filePath = argv[1];
	int len  = filePath.length();
	if (filePath[len-1] != '/')
	{
		filePath.append("/");
	}
	
	//bFileType
	bFitsType = false;
	if (strcmp(argv[2], "FITS") == 0)
	{
		bFitsType = true;
	}

	numOfImages = atoi(argv[3]);
	
	//imgType
	bLightFrame = false; //suppose DF
	if (strcmp(argv[4], "LF") == 0)
	{
		bLightFrame = true;
	}
	
	expTime = atof(argv[5]);
		
	//readout mode
	rm = 0; //suppose 1x1
	if (strcmp(argv[6], "2x2") == 0)
	{
		rm = 1;
	}
	else if (strcmp(argv[6], "3x3") == 0)
	{
		rm = 2;
	}

	// top
	top = atoi(argv[7]);

	// left
	left = atoi(argv[8]);

	// width
	width = atoi(argv[9]);

	// height
	height = atoi(argv[10]);

	//bFastReadout
	bFastReadout = false;
	if (strcmp(argv[11], "1") == 0)
	{
		bFastReadout = true;
	}

	//bDualChannelMode
	bDualChannelMode = false;
	if (strcmp(argv[12], "1") == 0)
	{
		bDualChannelMode = true;
	}

	cout << "Input parameters      : "
	     << "filePath = " 	    << filePath
	     << ", bFitsType = "    << bFitsType
	     << ", numOfImages = " 	<< numOfImages
	     << ", bLightFrame = "  << bLightFrame
	     << ", exposure time = "<< expTime
	     << ", readout mode = " << rm
	     << ", top = " << top
	     << ", left = " << left
	     << ", width = " << width
	     << ", height = " << height
	     << ", bFastReadout = " << bFastReadout
	     << ", bDualChannelMode = " << bDualChannelMode
	     << endl;

	// update filepath:
	if (bLightFrame)
	{
		filePath += "LF_";
	}
	else
	{
		filePath += "DF_";
	}

	// create SBIG Img object
	CSBIGImg* pImg = new CSBIGImg;

	// create SBIG camera object
	CSBIGCam* pCam = new CSBIGCam(DEV_USB1);

	if ((err = pCam->GetError()) != CE_NO_ERROR)
	{
		cout << "CSBIGCam error        : " << pCam->GetErrorString(err) << endl;
		return err;
	}

	// establish link to the camera
	if ((err = pCam->EstablishLink()) != CE_NO_ERROR)
	{	
		cout << "Establish link error  : " << pCam->GetErrorString(err) << endl;
		return err;
	}						
	cout << "Link established to   : " << pCam->GetCameraTypeString() << endl;
	
	// set camera params
	pCam->SetExposureTime(expTime);
	pCam->SetReadoutMode(rm);
	pCam->SetABGState(ABG_LOW7);
	pCam->SetFastReadout(bFastReadout);
	pCam->SetDualChannelMode(bDualChannelMode);

	// update width and height of the image
	pCam->GetFullFrame(fullWidth, fullHeight);

	if (width == 0)
	{
		width = fullWidth;
	}

	if (height == 0)
	{
		height = fullHeight;
	}

	pCam->SetSubFrame(left, top, width, height);
	pImg->AllocateImageBuffer(height, width);

	gettimeofday(&begin, NULL);

	// take series of images
	for (int i = 1; i <= numOfImages; i++)
	{			
		do
		{ 
			if (bLightFrame)
			{
				if ((err = pCam->GrabImage(pImg, SBDF_LIGHT_ONLY)) != CE_NO_ERROR)
				{
					cout << "CSBIGCam error        : " << pCam->GetErrorString(err) << endl;
					break;
				}
			}
			else
			{
				if ((err = pCam->GrabImage(pImg, SBDF_DARK_ONLY)) != CE_NO_ERROR)
				{
					cout << "CSBIGCam error        : " << pCam->GetErrorString(err) << endl;
					break;
				}
			}
		
			char             timeBuf[128];
			struct  tm*      pTm;

			struct  timeval  tv;
			struct  timezone tz;
			gettimeofday(&tv, &tz);
			pTm = localtime(&(tv.tv_sec));
			sprintf(timeBuf, "%04d-%02d-%02dT%02d:%02d:%02d.%03ld",
	                pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday,
	                pTm->tm_hour,        pTm->tm_min,     pTm->tm_sec, (tv.tv_usec/1000));

			// create filePathName:
			filePathName = filePath;
			filePathName += timeBuf;

			if (bFitsType)
			{
				filePathName += ".fits";
				if ((ferr = pImg->SaveImage(filePathName.c_str(), SBIF_FITS)) != SBFE_NO_ERROR)
				{
					cout << "SBIF_FITS format save error: " << ferr << endl;
					break;
				}
			}
			else
			{
				filePathName += ".sbig";
				if ((ferr = pImg->SaveImage(filePathName.c_str(), SBIF_COMPRESSED)) != SBFE_NO_ERROR)
				{
					cout << "SBIF_COMPRESSED format save error: " << ferr << endl;
					break;
				}
			}
		}
		while(0);
		
		if (err != CE_NO_ERROR)
		{
			break;
		}
		
	} // for
	
	gettimeofday(&end, NULL);

	// close sbig device
	if ((err = pCam->CloseDevice()) != CE_NO_ERROR)
	{
		cout << "CSBIGCam error: " << pCam->GetErrorString(err) << endl;
	}
		
	// close sbig driver	
	if((err = pCam->CloseDriver()) != CE_NO_ERROR)
	{
		cout << "CSBIGCam error: " << pCam->GetErrorString(err) << endl;
	}

	// delete objects
	if(pImg)
	{
		delete pImg;
	}

	if(pCam)
	{
		delete pCam;
	}
		
  double timeDiff = (((end.tv_sec  - begin.tv_sec) * 1000000) +
  		                (end.tv_usec - begin.tv_usec) + 500) / 1000000;

  fps = 1.0 / (timeDiff / numOfImages);

  cout << "-----------------------------------------------"      << endl;
  cout << "Application name                   : testmain utilizing sbigudrv C library" << endl;
  cout << "Camera frame rates measurement     : NO ERROR"        << endl;
  cout << "Average of                         : " << numOfImages << " FITS frames saved to disk." << endl;
  cout << "Exposure time                      : " << expTime     << endl;
  switch (rm)
  {
     case RM_1X1:
       	 	cout << "Readout mode                       : 1x1"              << endl;
          break;
     case RM_2X2:
		cout << "Readout mode                       : 2x2"              << endl;
          break;
     default:
    	 	cout << "Readout mode                       : bad parameter"    << endl;
          break;
  }

  std::_Ios_Fmtflags originalFlags = cout.flags();
  int  originalPrecision = cout.precision(2);
  cout.setf(ios::fixed,ios::floatfield);

  cout << "Frames per second                  : " << fps << " [fps]" << endl;

  cout.flags(originalFlags);
  cout.precision(originalPrecision);

  cout << "The End..." << endl;
  return (err);
}
//==============================================================
// CFW TEST
//==============================================================
// TODO

PAR_ERROR cfwTest()
{
	PAR_ERROR	 err;
	CFWResults cfwr;

	// open driver
	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_OPEN_DRIVER err: %d\n", err);
	if (err != CE_NO_ERROR)
	{
			return err;
	}

	// open device
	OpenDeviceParams odp;
	odp.deviceType = DEV_USB1;

	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_OPEN_DEVICE, &odp, NULL);
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_OPEN_DEVICE err: %d\n", err);
	if (err != CE_NO_ERROR)
	{
			return err;
	}

	// establish link
	EstablishLinkParams  elp;
	EstablishLinkResults elr;
	elp.sbigUseOnly = 0;

	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_ESTABLISH_LINK, &elp, &elr);
	if (err != CE_NO_ERROR)
	{
			fprintf(stderr, "----------------------------------------------\n");
			fprintf(stderr, "CC_ESTABLISH_LINK err: %d\n", err);
			return err;
	}
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_ESTABLISH_LINK err: %d, cameraType: %d\n", err, elr.cameraType);

	unsigned long  position;
	unsigned short cfwModel;

	switch (elr.cameraType)
	{
			case 	STL_CAMERA:
						cfwModel = CFWSEL_CFWL;
						break;

			case	STF_CAMERA:
						cfwModel = CFWSEL_FW5_8300;
						break;

			default:
						cfwModel = CFWSEL_AUTO;
						break;
	}

	do
	{
		// cfwInit
		err = cfwInit(cfwModel, &cfwr);
		if (err != CE_NO_ERROR)
		{
				break;
		}
		fprintf(stderr, "----------------------------------------------\n");
		fprintf(stderr, "cfwInit err: %d\n", err);

		cfwModel = cfwr.cfwModel;

		sleep(5);

		// cfwGoto
		position = 3;
		err = cfwGoto(cfwModel, position, &cfwr);
		if (err != CE_NO_ERROR)
		{
				break;
		}
		fprintf(stderr, "----------------------------------------------\n");
		fprintf(stderr, "cfwGoto requested position: %ld, err: %d\n", position, err);
		cfwShowResults(&cfwr);

		sleep(5);

		// cfwGoto
		position = 1;
		err = cfwGoto(cfwModel, position, &cfwr);
		if (err != CE_NO_ERROR)
		{
				break;
		}
		fprintf(stderr, "----------------------------------------------\n");
		fprintf(stderr, "cfwGoto requested position: %ld, err: %d\n", position, err);
		cfwShowResults(&cfwr);

		sleep(5);

		// cfwGoto
		position = 1;
		err = cfwGoto(cfwModel, position, &cfwr);
		if (err != CE_NO_ERROR)
		{
				break;
		}
		fprintf(stderr, "----------------------------------------------\n");
		fprintf(stderr, "cfwGoto requested position: %ld, err: %d\n", position, err);
		cfwShowResults(&cfwr);

		sleep(5);

		// cfwGoto
		position = 4;
		err = cfwGoto(cfwModel, position, &cfwr);
		if (err != CE_NO_ERROR)
		{
				break;
		}
		fprintf(stderr, "----------------------------------------------\n");
		fprintf(stderr, "cfwGoto requested position: %ld, err: %d\n", position, err);
		cfwShowResults(&cfwr);
	}
	while (0);

	// close device
	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CLOSE_DEVICE, NULL, NULL);
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_CLOSE_DEVICE err: %d\n", err);

	// close driver
	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CLOSE_DRIVER, NULL, NULL);
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_CLOSE_DRIVER err: %d\n", err);

	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "The End...\n");
	return err;
}
//==============================================================
PAR_ERROR cfwInit(unsigned short cfwModel, CFWResults* pRes)
{
	PAR_ERROR	err;
	CFWParams cfwp;

	cfwp.cfwModel   = cfwModel;
	cfwp.cfwCommand = CFWC_INIT;
	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CFW, &cfwp, pRes);

	if (err != CE_NO_ERROR)
	{
			return err;
	}

	do
	{
			cfwp.cfwCommand = CFWC_QUERY;
			err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CFW, &cfwp, pRes);

			if (err != CE_NO_ERROR)
			{
					continue;
			}

			if (pRes->cfwStatus != CFWS_IDLE)
			{
	  			sleep(1);
			}
	}
	while (pRes->cfwStatus != CFWS_IDLE);

	return err;
}
//==============================================================
PAR_ERROR cfwGoto(unsigned short cfwModel, int cfwPosition, CFWResults* pRes)
{
	PAR_ERROR err;
	CFWParams cfwp;

	cfwp.cfwModel = cfwModel;

	do
	{
			cfwp.cfwCommand = CFWC_QUERY;
			err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CFW, &cfwp, pRes);

			if (err != CE_NO_ERROR)
			{
					continue;
			}

			if (pRes->cfwStatus != CFWS_IDLE)
			{
	  			sleep(1);
			}
	}
	while (pRes->cfwStatus != CFWS_IDLE);

	cfwp.cfwCommand = CFWC_GOTO;
	cfwp.cfwParam1  = cfwPosition;
	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CFW, &cfwp, pRes);
	if (err != CE_NO_ERROR)
	{
			return err;
	}

	do
	{
			cfwp.cfwCommand = CFWC_QUERY;
			err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CFW, &cfwp, pRes);

			if (err != CE_NO_ERROR)
			{
					continue;
			}

			if (pRes->cfwStatus != CFWS_IDLE)
			{
	  			sleep(1);
			}
	}
	while (pRes->cfwStatus != CFWS_IDLE);

	return err;
}
//==============================================================
void cfwShowResults(CFWResults* pRes)
{
	fprintf(stderr, "CFWResults->cfwModel      : %d\n", pRes->cfwModel);
	fprintf(stderr, "CFWResults->cfwPosition   : %d\n", pRes->cfwPosition);
	fprintf(stderr, "CFWResults->cfwStatus     : %d\n", pRes->cfwStatus);
	fprintf(stderr, "CFWResults->cfwError      : %d\n", pRes->cfwError);
}
//==============================================================
// QUERY USB TEST
//==============================================================
// TODO
PAR_ERROR queryUsbTest()
{
	PAR_ERROR				err;
	QueryUSBResults res;
	QUERY_USB_INFO	usbInfo;

	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
	printf ("CC_OPEN_DRIVER status: %d\n", err);
	if (err != CE_NO_ERROR)
	{
		return err;
	}

	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_QUERY_USB, NULL, &res);
	if (err == CE_NO_ERROR)
	{
		printf ("camerasFound: %d\n\n", res.camerasFound);

		for (int i = 0; i < 4; i++)
		{
			usbInfo = res.usbInfo[i];
			if (usbInfo.cameraFound)
			{
				printf ("\n");
				printf ("QUERY_USB_INFO no: %d\n", i);
				printf ("cameraFound      : %d\n", usbInfo.cameraFound);
				printf ("cameraType       : %d\n", usbInfo.cameraType);
				printf ("name             : %s\n", usbInfo.name);
				printf ("S/N              : %s\n", usbInfo.serialNumber);
			}
		}
	}

	printf ("The End...\n");
	return err;
}
//==============================================================
// QUERY ETHERNET TEST
//==============================================================
// TODO
PAR_ERROR queryEthernetTest()
{
	PAR_ERROR							err;
	QueryEthernetResults 	res;
	QUERY_ETHERNET_INFO		ethInfo;

	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
	printf ("CC_OPEN_DRIVER status: %d\n", err);
	if (err != CE_NO_ERROR)
	{
			return err;
	}

	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_QUERY_ETHERNET, NULL, &res);
	if (err == CE_NO_ERROR)
	{
			printf ("camerasFound: %d\n\n", res.camerasFound);

			for (int i = 0; i < 4; i++)
			{
					ethInfo = res.ethernetInfo[i];
					if (ethInfo.cameraFound)
					{
							printf ("\n");
							printf ("QUERY_ETHERNET_INFO no: %d\n", 	i);
							printf ("cameraFound           : %d\n", 	ethInfo.cameraFound);
							printf ("ipAddress             : %0lX\n",	ethInfo.ipAddress);
							printf ("cameraType            : %d\n", 	ethInfo.cameraType);
							printf ("name                  : %s\n", 	ethInfo.name);
							printf ("S/N                   : %s\n", 	ethInfo.serialNumber);
					}
			}
	}

	printf ("The End...\n");
	return err;
}
//==============================================================
float bcd2float(unsigned short bcd)
{
	unsigned char  b1 = bcd >> 8;
	unsigned char  b2 = bcd;

	float          f1 = 10 * (b1 >> 4);
	float          f2 = b1 & 0x0F;
	float          f3 = 0.10 * (b2 >> 4);
	float          f4 = 0.01 * (b2 & 0x0F);

	return (f1 + f2 + f3 + f4);
}
//==============================================================
PAR_ERROR readEeprom()
{
	PAR_ERROR 				err = CE_NO_ERROR;
	UserEEPROMParams 	par;
  UserEEPROMResults res;

  par.writeData = FALSE;

  err = (PAR_ERROR)SBIGUnivDrvCommand(CC_USER_EEPROM, &par, &res);

  for (int i = 0; i < 32; i++)
  {
  		printf("EEPROM[%02d]               : %0x\n", i, res.data[i]);
  }

  printf("CC_USER_EEPROM          err: %d\n", err);
  return err;
}
//==============================================================
PAR_ERROR openDriver()
{
	PAR_ERROR err = (PAR_ERROR)SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "CC_OPEN_DRIVER          err: %d\n", err);
	return err;
}
//==============================================================
PAR_ERROR closeDriver()
{
	PAR_ERROR err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CLOSE_DRIVER, NULL, NULL);
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "CC_CLOSE_DRIVER         err: %d\n", err);
	return err;
}
//==============================================================
PAR_ERROR openDevice(SBIG_DEVICE_TYPE dev)
{
	OpenDeviceParams odp;
	odp.deviceType = dev;

	PAR_ERROR err = (PAR_ERROR)SBIGUnivDrvCommand(CC_OPEN_DEVICE, &odp, NULL);
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "CC_OPEN_DEVICE          err: %d\n", err);
	return err;
}
//==============================================================
PAR_ERROR closeDevice()
{
	PAR_ERROR err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CLOSE_DEVICE, NULL, NULL);
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "CC_CLOSE_DEVICE         err: %d\n", err);
	return err;
}
//==============================================================
PAR_ERROR establishLink()
{
	EstablishLinkParams  elp;
	EstablishLinkResults elr;

	elp.sbigUseOnly = 0;

	PAR_ERROR err = (PAR_ERROR)SBIGUnivDrvCommand(CC_ESTABLISH_LINK, &elp, &elr);
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "CC_ESTABLISH_LINK       err: %d\n", err);
	return err;
}
//==============================================================
PAR_ERROR getCcdInfo0(GetCCDInfoResults0* res)
{
	PAR_ERROR 				err = CE_NO_ERROR;
	GetCCDInfoParams	par;

  par.request = 1;

  err = (PAR_ERROR)SBIGUnivDrvCommand(CC_GET_CCD_INFO, &par, res);

  if (err == CE_NO_ERROR)
  {
			fprintf(stderr, "===================================\n");
			fprintf(stderr, "CC_GET_CCD_INFO         err: %d\n", 	 err);
			fprintf(stderr, "firmwareVersion            : %.2f\n", bcd2float(res->firmwareVersion));
			fprintf(stderr, "cameraType                 : %d\n", 	 res->cameraType);
			fprintf(stderr, "name                       : %s\n", 	 res->name);
			fprintf(stderr, "readoutModes               : %d\n",   res->readoutModes);

			for (int i = 0; i < res->readoutModes ; i++)
			{
				fprintf(stderr, "-----------------------------------\n");
				fprintf(stderr, "mode                       : %d\n",   res->readoutInfo[i].mode);
				fprintf(stderr, "width                      : %d\n",   res->readoutInfo[i].width);
				fprintf(stderr, "height                     : %d\n",   res->readoutInfo[i].height);
				fprintf(stderr, "gain                       : %.2f\n", bcd2float(res->readoutInfo[i].gain));
				fprintf(stderr, "pixelWidth                 : %.2f\n", bcd2float(res->readoutInfo[i].pixelWidth));
				fprintf(stderr, "pixelHeight                : %.2f\n", bcd2float(res->readoutInfo[i].pixelHeight));
			}
  }
  return err;
}
//==============================================================

// TODO

PAR_ERROR startExposure(bool									dualChannelMode,
		                    bool									fastReadout,
												CCD_REQUEST 					ccd,
												double 								expTime,
												SHUTTER_COMMAND 			shutterCommand,
												READOUT_BINNING_MODE 	readoutMode,
												unsigned short 				top,
												unsigned short 				left,
												unsigned short 				height,
												unsigned short 				width)
{
	PAR_ERROR 						err = CE_NO_ERROR;
	StartExposureParams2 	sep2;

	sep2.ccd = ccd;

	// For exposures less than 0.01 seconds use millisecond exposures
	// Note: This assumes the caller has used the GetCCDInfo command
	//       to insure the camera supports millisecond exposures
	if (expTime < 0.01)
	{
			// MS exposure time
			sep2.exposureTime = (unsigned long)(expTime * 1000.0 + 0.5);
			if (sep2.exposureTime < 1)
			{
					sep2.exposureTime = 1;
			}
			sep2.exposureTime |= EXP_MS_EXPOSURE;
	}
	else
	{
			sep2.exposureTime = (unsigned long)(expTime * 100.0 + 0.5);
			if (sep2.exposureTime < 1)
			{
					sep2.exposureTime = 1;
			}
	}

	if (fastReadout)
	{
			sep2.exposureTime |= EXP_FAST_READOUT;
	}

	if (dualChannelMode)
	{
			sep2.exposureTime |= EXP_DUAL_CHANNEL_MODE;
	}

	sep2.abgState 		= ABG_LOW7;
	sep2.openShutter 	= shutterCommand;
	sep2.readoutMode  = readoutMode;
	sep2.top          = top;
	sep2.left         = left;
	sep2.height       = height;
	sep2.width        = width;

  err = (PAR_ERROR)SBIGUnivDrvCommand(CC_START_EXPOSURE2, &sep2, NULL);
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "CC_START_EXPOSURE2      err: %d\n", err);
	fprintf(stderr, "ccd         : %d\n",  sep2.ccd);
	fprintf(stderr, "eposureTime : %lX\n", sep2.exposureTime);
	fprintf(stderr, "abgState    : %d\n",  sep2.abgState);
	fprintf(stderr, "openShutter : %d\n",  sep2.openShutter);
	fprintf(stderr, "readoutMode : %d\n",  sep2.readoutMode);
	fprintf(stderr, "top         : %d\n",  sep2.top);
	fprintf(stderr, "left        : %d\n",  sep2.left);
	fprintf(stderr, "height      : %d\n",  sep2.height);
	fprintf(stderr, "width       : %d\n",  sep2.width);

  return err;
}
//==============================================================
PAR_ERROR endExposure(CCD_REQUEST ccd)
{
  EndExposureParams eep;
  eep.ccd = ccd;

  PAR_ERROR err = (PAR_ERROR)SBIGUnivDrvCommand(CC_END_EXPOSURE, &eep, NULL);
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "CC_END_EXPOSURE         err: %d\n", err);
  return err;
}
//==============================================================
PAR_ERROR monitorExposure()
{
	PAR_ERROR 								err = CE_NO_ERROR;
	QueryCommandStatusParams	qcsp;
	QueryCommandStatusResults	qcsr;

	qcsp.command = CC_START_EXPOSURE;

	do
	{
	  	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr);
	  	if (err != CE_NO_ERROR)
	  	{
	      	break;
	  	}
	}
	while ((qcsr.status & 0x03) == 0x02);

	fprintf(stderr, "===================================\n");
	fprintf(stderr, "CC_QUERY_COMMAND_STATUS err: %d\n", err);
	return err;
}
//==============================================================
PAR_ERROR startReadout(CCD_REQUEST 					ccd,
											 READOUT_BINNING_MODE readoutMode,
											 unsigned short 			top,
											 unsigned short 			left,
											 unsigned short 			height,
											 unsigned short 			width)
{
	PAR_ERROR 					err = CE_NO_ERROR;
  StartReadoutParams 	srp;

  srp.ccd 				= ccd;
  srp.readoutMode = readoutMode;
	srp.top         = top;
	srp.left        = left;
	srp.height      = height;
	srp.width       = width;

  err = (PAR_ERROR)SBIGUnivDrvCommand(CC_START_READOUT, &srp, NULL);
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "CC_START_READOUT        err: %d\n", err);
  return err;
}
//==============================================================
PAR_ERROR	endReadout(CCD_REQUEST ccd)
{
	PAR_ERROR 			 err = CE_NO_ERROR;
  EndReadoutParams erp;

  erp.ccd	= ccd;

  err = (PAR_ERROR)SBIGUnivDrvCommand(CC_END_READOUT, &erp, NULL);
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "CC_END_READOUT          err: %d\n", err);
  return err;
}
//==============================================================
PAR_ERROR	ccdReadout(CSBIGImg*							pImg,
										 bool 									dualChannelMode,
										 CCD_REQUEST 						ccd,
										 READOUT_BINNING_MODE		readoutMode,
										 unsigned short 				top,
										 unsigned short 				left,
										 unsigned short 				height,
										 unsigned short 				width)
{
	PAR_ERROR 			 	err  = CE_NO_ERROR;
	ReadoutLineParams	rlp;
	int	i;

	fprintf(stderr, "===================================\n");
	fprintf(stderr, "ccdReadout\n");
	fprintf(stderr, "dualChannelMode: %d\n", dualChannelMode);
	fprintf(stderr, "ccd            : %d\n", ccd);
	fprintf(stderr, "readoutMode    : %d\n", readoutMode);
	fprintf(stderr, "top            : %d\n", top);
	fprintf(stderr, "left           : %d\n", left);
	fprintf(stderr, "height         : %d\n", height);
	fprintf(stderr, "width          : %d\n", width);

	rlp.ccd 				= ccd;
	rlp.readoutMode = readoutMode;
	rlp.pixelStart	= left;
	rlp.pixelLength = width;

	for (i = 0; i < height && err == CE_NO_ERROR; i++)
	{
			err = (PAR_ERROR)SBIGUnivDrvCommand(CC_READOUT_LINE, &rlp, pImg->GetImagePointer() + (long)i * width);
			if (err != CE_NO_ERROR)
			{
					fprintf(stderr, "CC_READOUT_LINE         err: %d, row: %d\n", err, i);
					break;
			}
	}

	return err;
}
//==============================================================
// take image
//==============================================================

// TODO

PAR_ERROR takeImage(int argc, char *argv[])
{
	PAR_ERROR 						err = CE_NO_ERROR;
  CCD_REQUEST						ccd							= CCD_IMAGING;
	SHUTTER_COMMAND 			shutterCommand	= SC_LEAVE_SHUTTER; /*SC_OPEN_SHUTTER;*/
	GetCCDInfoResults0		gcir0;
  struct timeval 				t1, t2, t3, t4;
  double								dt;
	string								filePath, filePathName;
	bool									bFitsType;
	int										numOfImages;
	bool									bLightFrame;
	double								expTime;
	int 									rm;
	int             			top, left, width, fullWidth, height, fullHeight;
	bool									bFastReadout, bDualChannelMode;
	SBIG_FILE_ERROR				ferr;

  if (argc != 13)
	{
		cout << "Application startup error: Bad number of input parameters. " << endl;
		cout << "Format : ./testmain filePath               fileType imgCount imgType expTime rm  top left width heigh fr dcm" << endl;
		cout << "Example: ./testmain /observations/img/raw/  FITS     100       LF     0.001  1x1  0    0  1000  1000   1  1 " << endl;
		cout << "Example: ./testmain /observations/img/raw/  SBIG      5        DF     15.5   2x2  0    0   0     0     1  1 " << endl;
		return CE_BAD_PARAMETER;
	}

	filePath = argv[1];
	int len  = filePath.length();
	if (filePath[len-1] != '/')
	{
		filePath.append("/");
	}

	//bFileType
	bFitsType = false;
	if (strcmp(argv[2], "FITS") == 0)
	{
		bFitsType = true;
	}

	numOfImages = atoi(argv[3]);

	//imgType
	bLightFrame = false; //suppose DF
	if (strcmp(argv[4], "LF") == 0)
	{
		bLightFrame = true;
	}

	expTime = atof(argv[5]);

	//readout mode
	rm = 0; //suppose 1x1
	if (strcmp(argv[6], "2x2") == 0)
	{
		rm = 1;
	}
	else if (strcmp(argv[6], "3x3") == 0)
	{
		rm = 2;
	}

	// top
	top = atoi(argv[7]);

	// left
	left = atoi(argv[8]);

	// width
	width = atoi(argv[9]);

	// height
	height = atoi(argv[10]);

	//bFastReadout
	bFastReadout = false;
	if (strcmp(argv[11], "1") == 0)
	{
		bFastReadout = true;
	}

	//bDualChannelMode
	bDualChannelMode = false;
	if (strcmp(argv[12], "1") == 0)
	{
		bDualChannelMode = true;
	}

	cout << "Input parameters      : "
	     << "filePath = " 	    << filePath
	     << ", bFitsType = "    << bFitsType
	     << ", numOfImages = " 	<< numOfImages
	     << ", bLightFrame = "  << bLightFrame
	     << ", exposure time = "<< expTime
	     << ", readout mode = " << rm
	     << ", top = " << top
	     << ", left = " << left
	     << ", width = " << width
	     << ", height = " << height
	     << ", bFastReadout = " << bFastReadout
	     << ", bDualChannelMode = " << bDualChannelMode
	     << endl;

/*
//#define DUAL_FULL_IMG
//#define DUAL_HALF_IMG
//#define DUAL_HALF_IMG_256
//#define DUAL_LEFT_IMG
#define DUAL_RIGHT_IMG

#ifdef DUAL_FULL_IMG
	unsigned short 				top							= 0;
	unsigned short 				left						= 0;
	unsigned short 				height					= 2495;  	// if 0 => use CCD height
	unsigned short 				width						= 3320;		// if 0 => use CCD width
#endif

#ifdef DUAL_HALF_IMG
	unsigned short 				top							= 624;
	unsigned short 				left						= 830;
	unsigned short 				height					= 1248;  	// if 0 => use CCD height
	unsigned short 				width						= 1660;		// if 0 => use CCD width
#endif

#ifdef DUAL_HALF_IMG_256
	unsigned short 				top							= 736;
	unsigned short 				left						= 830;
	unsigned short 				height					= 1024;  	// if 0 => use CCD height
	unsigned short 				width						= 1660;		// if 0 => use CCD width
#endif

#ifdef DUAL_LEFT_IMG
	unsigned short 				top							= 0;
	unsigned short 				left						= 0;
	unsigned short 				height					= 1000;  	// if 0 => use CCD height
	unsigned short 				width						= 1000;		// if 0 => use CCD width
#endif

#ifdef DUAL_RIGHT_IMG
	unsigned short 				top							= 0;
	unsigned short 				left						= 1660;
	unsigned short 				height					= 1023;  	// if 0 => use CCD height
	unsigned short 				width						= 1023;		// if 0 => use CCD width
#endif
*/

	CSBIGImg* 						pImg 						= new CSBIGImg;

	// open driver
	if ((err = openDriver()) != CE_NO_ERROR)
	{
			return err;
	}

	// open device
	if ((err = openDevice(DEV_USB1)) != CE_NO_ERROR)
	{
			closeDriver();
			return err;
	}

	do
	{
			// establish link
			if ((err = establishLink()) != CE_NO_ERROR)
			{
					break;
			}

			// get full frame 1x1
			if ((err = getCcdInfo0(&gcir0)) != CE_NO_ERROR)
			{
					break;
			}

			if (height == 0)
			{
					height = gcir0.readoutInfo[rm].height;
			}

			if (width == 0)
			{
					width = gcir0.readoutInfo[rm].width;
			}

			pImg->AllocateImageBuffer(height, width);

			// start exposure
			err = startExposure(bDualChannelMode,
													bFastReadout,
													ccd,
													expTime,
													shutterCommand,
													(READOUT_BINNING_MODE)rm,
													top,
													left,
													height,
													width);
			if (err != CE_NO_ERROR)
			{
					break;
			}

			gettimeofday(&t1, NULL);

			// monitor exposure
			err = monitorExposure();
			if (err != CE_NO_ERROR)
			{
					endExposure(ccd);
					break;
			}

			gettimeofday(&t2, NULL);

			// end exposure
			err = endExposure(ccd);
			if (err != CE_NO_ERROR)
			{
					break;
			}

			gettimeofday(&t3, NULL);

			// start readout
		  err = startReadout(ccd, (READOUT_BINNING_MODE)rm, top, left, height, width);
		  if (err != CE_NO_ERROR)
		  {
		      break;
		  }

			// ccd readout
		  err = ccdReadout(pImg, bDualChannelMode, ccd, (READOUT_BINNING_MODE)rm, top, left, height, width);
		  if (err != CE_NO_ERROR)
		  {
		      break;
		  }

		  // end readout
		  err = endReadout(ccd);
		  if (err != CE_NO_ERROR)
		  {
		      break;
		  }

		  gettimeofday(&t4, NULL);

			dt = (((t2.tv_sec  - t1.tv_sec) * 1000000.0) + (t2.tv_usec - t1.tv_usec)) / 1000000.0;
			fprintf(stderr, "exposureTime               : %.3f [sec]\n", dt);

			dt = (((t4.tv_sec  - t3.tv_sec) * 1000000.0) + (t4.tv_usec - t3.tv_usec)) / 1000000.0;
			fprintf(stderr, "readoutTime                : %.3f [sec]\n", dt);

			dt = (((t4.tv_sec  - t1.tv_sec) * 1000000.0) + (t4.tv_usec - t1.tv_usec)) / 1000000.0;
			fprintf(stderr, "totalTime                  : %.3f [sec]\n", dt);

	}
	while (0);

	// close device
	closeDevice();

	// close driver
	closeDriver();

	if (pImg)
	{
			delete pImg;
	}

	fprintf(stderr, "===================================\n");
	fprintf(stderr, "The End...\n");
	return err;
}
//==============================================================
PAR_ERROR queryTemperatureStatus()
{
	QueryTemperatureStatusParams   qtsp;
	QueryTemperatureStatusResults2 qtsr2;

	qtsp.request = TEMP_STATUS_ADVANCED2;

	EstablishLinkParams  elp;
	EstablishLinkResults elr;

	elp.sbigUseOnly = 0;

	PAR_ERROR err = (PAR_ERROR)SBIGUnivDrvCommand(CC_ESTABLISH_LINK, &elp, &elr);
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "CC_ESTABLISH_LINK       err: %d\n", err);
	return err;
}

//==============================================================
// CheckTemperature
//==============================================================
// TODO
PAR_ERROR checkQueryTemperatureStatus()
{
	PAR_ERROR	 err;
	QueryTemperatureStatusParams   qtsp;
	QueryTemperatureStatusResults2 qtsr2;

	// open driver
	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_OPEN_DRIVER err: %d\n", err);
	if (err != CE_NO_ERROR)
	{
			return err;
	}

	// open device
	OpenDeviceParams odp;
	odp.deviceType = DEV_USB1;

	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_OPEN_DEVICE, &odp, NULL);
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_OPEN_DEVICE err: %d\n", err);
	if (err != CE_NO_ERROR)
	{
			return err;
	}

	// establish link
	EstablishLinkParams  elp;
	EstablishLinkResults elr;
	elp.sbigUseOnly = 0;

	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_ESTABLISH_LINK, &elp, &elr);
	if (err != CE_NO_ERROR)
	{
			fprintf(stderr, "----------------------------------------------\n");
			fprintf(stderr, "CC_ESTABLISH_LINK err: %d\n", err);
			return err;
	}
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_ESTABLISH_LINK err: %d, cameraType: %d\n", err, elr.cameraType);


	qtsp.request = TEMP_STATUS_ADVANCED2;

	for (int i = 0; i < 3; i++)
	{
		sleep(5);

		PAR_ERROR err = (PAR_ERROR)SBIGUnivDrvCommand(CC_QUERY_TEMPERATURE_STATUS, &qtsp, &qtsr2);
		fprintf(stderr, "===================================\n");
		fprintf(stderr, "CC_QUERY_TEMPERATURE_STATUS   err: %d\n",   err);

		if (err == CE_NO_ERROR)
		{
			fprintf(stderr, "coolingEnabled:                    %d\n",   qtsr2.coolingEnabled);
			fprintf(stderr, "fanEnabled    :                    %d\n",   qtsr2.fanEnabled);
			fprintf(stderr, "ccdSetpoint   :                    %.2f\n", qtsr2.ccdSetpoint);
			fprintf(stderr, "imagingCCDTemperature:             %.2f\n", qtsr2.imagingCCDTemperature);
			fprintf(stderr, "trackingCCDTemperature:            %.2f\n", qtsr2.trackingCCDTemperature);
			fprintf(stderr, "externalTrackingCCDTemperature:    %.2f\n", qtsr2.externalTrackingCCDTemperature);
			fprintf(stderr, "ambientTemperature:                %.2f\n", qtsr2.ambientTemperature);
			fprintf(stderr, "imagingCCDPower:                   %.2f\n", qtsr2.imagingCCDPower);
			fprintf(stderr, "trackingCCDPower:                  %.2f\n", qtsr2.trackingCCDPower);
			fprintf(stderr, "externalTrackingCCDPower:          %.2f\n", qtsr2.externalTrackingCCDPower);
			fprintf(stderr, "heatsinkTemperature:               %.2f\n", qtsr2.heatsinkTemperature);
			fprintf(stderr, "fanPower:                          %.2f\n", qtsr2.fanPower);
			fprintf(stderr, "fanSpeed:                          %.2f\n", qtsr2.fanSpeed);
			fprintf(stderr, "trackingCCDSetpoint:               %.2f\n", qtsr2.trackingCCDSetpoint);
		}
	}

	// close device
	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CLOSE_DEVICE, NULL, NULL);
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_CLOSE_DEVICE err: %d\n", err);

	// close driver
	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CLOSE_DRIVER, NULL, NULL);
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_CLOSE_DRIVER err: %d\n", err);

	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "The End...\n");
	return err;
}
//==============================================================
// CheckTemperature
//==============================================================
// TODO
PAR_ERROR checkEthernetCameras()
{
	PAR_ERROR	 						err;
	QueryEthernetResults 	qer;
	QUERY_ETHERNET_INFO		qei;
	GetCCDInfoResults0 		gcir0;

	// open driver
	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_OPEN_DRIVER err: %d\n", err);
	if (err != CE_NO_ERROR)
	{
			return err;
	}

	// query ethernet
	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_QUERY_ETHERNET, NULL, &qer);
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_QUERY_ETHERNET err: %d\n", err);
	if (err != CE_NO_ERROR)
	{
			return err;
	}

	printf ("Number of Ethernet cameras found: %d\n\n", qer.camerasFound);

	for (int i = 0; i < 4; i++)
	{
			qei = qer.ethernetInfo[i];
			if (qei.cameraFound)
			{
					printf ("\n");
					printf ("QUERY_ETHERNET_INFO no: %d\n", 	i);
					printf ("cameraFound           : %d\n", 	qei.cameraFound);
					printf ("ipAddress             : %0lX\n",	qei.ipAddress);
					printf ("cameraType            : %d\n", 	qei.cameraType);
					printf ("name                  : %s\n", 	qei.name);
					printf ("S/N                   : %s\n", 	qei.serialNumber);

					// open device
					OpenDeviceParams odp;
					odp.deviceType = DEV_ETH;
					odp.ipAddress  = qei.ipAddress;

					err = (PAR_ERROR)SBIGUnivDrvCommand(CC_OPEN_DEVICE, &odp, NULL);
					fprintf(stderr, "----------------------------------------------\n");
					fprintf(stderr, "CC_OPEN_DEVICE err: %d\n", err);
					if (err != CE_NO_ERROR)
					{
							continue;
					}

					// establish link
					EstablishLinkParams  elp;
					EstablishLinkResults elr;
					elp.sbigUseOnly = 0;

					err = (PAR_ERROR)SBIGUnivDrvCommand(CC_ESTABLISH_LINK, &elp, &elr);
					if (err != CE_NO_ERROR)
					{
							fprintf(stderr, "----------------------------------------------\n");
							fprintf(stderr, "CC_ESTABLISH_LINK err: %d\n", err);
							continue;
					}
					fprintf(stderr, "----------------------------------------------\n");
					fprintf(stderr, "CC_ESTABLISH_LINK err: %d, cameraType: %d\n", err, elr.cameraType);

					// get CCD info 0

					err = getCcdInfo0(&gcir0);

					// close device
					err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CLOSE_DEVICE, NULL, NULL);
					fprintf(stderr, "----------------------------------------------\n");
					fprintf(stderr, "CC_CLOSE_DEVICE err: %d\n", err);

			}
	}

	// close driver
	err = (PAR_ERROR)SBIGUnivDrvCommand(CC_CLOSE_DRIVER, NULL, NULL);
	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "CC_CLOSE_DRIVER err: %d\n", err);

	fprintf(stderr, "----------------------------------------------\n");
	fprintf(stderr, "The End...\n");
	return err;
}
//==============================================================
