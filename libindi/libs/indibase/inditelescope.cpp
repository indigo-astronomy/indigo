/*******************************************************************************
  Copyright(c) 2011 Gerry Rozema, Jasem Mutlaq. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/
#include <wordexp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#include "inditelescope.h"
#include "indicom.h"

INDI::Telescope::Telescope()
{
    capability=0;
    last_we_motion = last_ns_motion = -1;
    parkDataType = PARK_NONE;
    Parkdatafile= "~/.indi/ParkData.xml";
    IsParked=false;
    IsLocked=true;

    nSlewRate=0;
    SlewRateS = NULL;

    controller = new INDI::Controller(this);
    controller->setJoystickCallback(joystickHelper);
    controller->setButtonCallback(buttonHelper);

}

INDI::Telescope::~Telescope()
{
    delete (controller);
}

bool INDI::Telescope::initProperties()
{
    DefaultDevice::initProperties();

    // Active Devices
    IUFillText(&ActiveDeviceT[0],"ACTIVE_GPS","GPS","GPS Simulator");
    IUFillText(&ActiveDeviceT[1],"ACTIVE_DOME","DOME","Dome Simulator");
    IUFillTextVector(&ActiveDeviceTP,ActiveDeviceT,2,getDeviceName(),"ACTIVE_DEVICES","Snoop devices",OPTIONS_TAB,IP_RW,60,IPS_IDLE);

    // Address/Port
    IUFillText(&AddressT[0], "ADDRESS", "Address", "");
    IUFillText(&AddressT[1], "PORT",    "Port",    "");
    IUFillTextVector(&AddressTP, AddressT, 2, getDeviceName(), "TCP_ADDRESS_PORT", "TCP Server", OPTIONS_TAB, IP_RW, 60, IPS_IDLE);

    // Use locking if dome is closed (and or) park scope if dome is closing
    IUFillSwitch(&DomeClosedLockT[0],"NO_ACTION","Ignore dome",ISS_ON);
    IUFillSwitch(&DomeClosedLockT[1],"LOCK_PARKING","Dome locks",ISS_OFF);
    IUFillSwitch(&DomeClosedLockT[2],"FORCE_CLOSE","Dome parks",ISS_OFF);
    IUFillSwitch(&DomeClosedLockT[3],"LOCK_AND_FORCE","Both",ISS_OFF);
    IUFillSwitchVector(&DomeClosedLockTP,DomeClosedLockT,4,getDeviceName(),"DOME_POLICY","Dome parking policy",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_IDLE);

    IUFillNumber(&EqN[AXIS_RA],"RA","RA (hh:mm:ss)","%010.6m",0,24,0,0);
    IUFillNumber(&EqN[AXIS_DE],"DEC","DEC (dd:mm:ss)","%010.6m",-90,90,0,0);
    IUFillNumberVector(&EqNP,EqN,2,getDeviceName(),"EQUATORIAL_EOD_COORD","Eq. Coordinates",MAIN_CONTROL_TAB,IP_RW,60,IPS_IDLE);
    lastEqState = IPS_IDLE;

    IUFillNumber(&TargetN[AXIS_RA],"RA","RA (hh:mm:ss)","%010.6m",0,24,0,0);
    IUFillNumber(&TargetN[AXIS_DE],"DEC","DEC (dd:mm:ss)","%010.6m",-90,90,0,0);
    IUFillNumberVector(&TargetNP,TargetN,2,getDeviceName(),"TARGET_EOD_COORD","Slew Target",MOTION_TAB,IP_RO,60,IPS_IDLE);


    IUFillSwitch(&ParkOptionS[0],"PARK_CURRENT","Current",ISS_OFF);
    IUFillSwitch(&ParkOptionS[1],"PARK_DEFAULT","Default",ISS_OFF);
    IUFillSwitch(&ParkOptionS[2],"PARK_WRITE_DATA","Write Data",ISS_OFF);
    IUFillSwitchVector(&ParkOptionSP,ParkOptionS,3,getDeviceName(),"TELESCOPE_PARK_OPTION","Park Options", SITE_TAB,IP_RW,ISR_ATMOST1,60,IPS_IDLE);

    IUFillText(&TimeT[0],"UTC","UTC Time",NULL);
    IUFillText(&TimeT[1],"OFFSET","UTC Offset",NULL);
    IUFillTextVector(&TimeTP,TimeT,2,getDeviceName(),"TIME_UTC","UTC",SITE_TAB,IP_RW,60,IPS_IDLE);

    IUFillNumber(&LocationN[LOCATION_LATITUDE],"LAT","Lat (dd:mm:ss)","%010.6m",-90,90,0,0.0);
    IUFillNumber(&LocationN[LOCATION_LONGITUDE],"LONG","Lon (dd:mm:ss)","%010.6m",0,360,0,0.0 );
    IUFillNumber(&LocationN[LOCATION_ELEVATION],"ELEV","Elevation (m)","%g",-200,10000,0,0 );
    IUFillNumberVector(&LocationNP,LocationN,3,getDeviceName(),"GEOGRAPHIC_COORD","Scope Location",SITE_TAB,IP_RW,60,IPS_OK);

    IUFillSwitch(&CoordS[0],"TRACK","Track",ISS_ON);
    IUFillSwitch(&CoordS[1],"SLEW","Slew",ISS_OFF);
    IUFillSwitch(&CoordS[2],"SYNC","Sync",ISS_OFF);

    if (CanSync())
        IUFillSwitchVector(&CoordSP,CoordS,3,getDeviceName(),"ON_COORD_SET","On Set",MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,60,IPS_IDLE);
    else
        IUFillSwitchVector(&CoordSP,CoordS,2,getDeviceName(),"ON_COORD_SET","On Set",MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,60,IPS_IDLE);

    if (nSlewRate >= 4)
        IUFillSwitchVector(&SlewRateSP, SlewRateS, nSlewRate, getDeviceName(), "TELESCOPE_SLEW_RATE", "Slew Rate", MOTION_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&ParkS[0],"PARK","Park",ISS_OFF);
    IUFillSwitch(&ParkS[1],"UNPARK","UnPark",ISS_OFF);
    IUFillSwitchVector(&ParkSP,ParkS,2,getDeviceName(),"TELESCOPE_PARK","Parking",MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,60,IPS_IDLE);

    IUFillSwitch(&AbortS[0],"ABORT","Abort",ISS_OFF);
    IUFillSwitchVector(&AbortSP,AbortS,1,getDeviceName(),"TELESCOPE_ABORT_MOTION","Abort Motion",MAIN_CONTROL_TAB,IP_RW,ISR_ATMOST1,60,IPS_IDLE);

    IUFillText(&PortT[0],"PORT","Port","/dev/ttyUSB0");
    IUFillTextVector(&PortTP,PortT,1,getDeviceName(),"DEVICE_PORT","Ports",OPTIONS_TAB,IP_RW,60,IPS_IDLE);

    IUFillSwitch(&BaudRateS[0], "9600", "", ISS_ON);
    IUFillSwitch(&BaudRateS[1], "19200", "", ISS_OFF);
    IUFillSwitch(&BaudRateS[2], "38400", "", ISS_OFF);
    IUFillSwitch(&BaudRateS[3], "57600", "", ISS_OFF);
    IUFillSwitch(&BaudRateS[4], "115200", "", ISS_OFF);
    IUFillSwitch(&BaudRateS[5], "230400", "", ISS_OFF);
    IUFillSwitchVector(&BaudRateSP, BaudRateS, 6, getDeviceName(),"TELESCOPE_BAUD_RATE", "Baud Rate", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    IUFillSwitch(&MovementNSS[DIRECTION_NORTH], "MOTION_NORTH", "North", ISS_OFF);
    IUFillSwitch(&MovementNSS[DIRECTION_SOUTH], "MOTION_SOUTH", "South", ISS_OFF);
    IUFillSwitchVector(&MovementNSSP, MovementNSS, 2, getDeviceName(),"TELESCOPE_MOTION_NS", "Motion N/S", MOTION_TAB, IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    IUFillSwitch(&MovementWES[DIRECTION_WEST], "MOTION_WEST", "West", ISS_OFF);
    IUFillSwitch(&MovementWES[DIRECTION_EAST], "MOTION_EAST", "East", ISS_OFF);
    IUFillSwitchVector(&MovementWESP, MovementWES, 2, getDeviceName(),"TELESCOPE_MOTION_WE", "Motion W/E", MOTION_TAB, IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    IUFillNumber(&ScopeParametersN[0],"TELESCOPE_APERTURE","Aperture (mm)","%g",10,5000,0,0.0);
    IUFillNumber(&ScopeParametersN[1],"TELESCOPE_FOCAL_LENGTH","Focal Length (mm)","%g",10,10000,0,0.0 );
    IUFillNumber(&ScopeParametersN[2],"GUIDER_APERTURE","Guider Aperture (mm)","%g",10,5000,0,0.0);
    IUFillNumber(&ScopeParametersN[3],"GUIDER_FOCAL_LENGTH","Guider Focal Length (mm)","%g",10,10000,0,0.0 );
    IUFillNumberVector(&ScopeParametersNP,ScopeParametersN,4,getDeviceName(),"TELESCOPE_INFO","Scope Properties",OPTIONS_TAB,IP_RW,60,IPS_OK);

    // Lock Axis
    IUFillSwitch(&LockAxisS[0],"LOCK_AXIS_1","West/East",ISS_OFF);
    IUFillSwitch(&LockAxisS[1],"LOCK_AXIS_2","North/South",ISS_OFF);
    IUFillSwitchVector(&LockAxisSP,LockAxisS,2,getDeviceName(),"JOYSTICK_LOCK_AXIS","Lock Axis", "Joystick" ,IP_RW,ISR_ATMOST1,60,IPS_IDLE);

    controller->initProperties();

    TrackState=SCOPE_IDLE;

    setDriverInterface(TELESCOPE_INTERFACE);

    IDSnoopDevice(ActiveDeviceT[0].text,"GEOGRAPHIC_COORD");
    IDSnoopDevice(ActiveDeviceT[0].text,"TIME_UTC");

    IDSnoopDevice(ActiveDeviceT[1].text,"DOME_PARK");
    IDSnoopDevice(ActiveDeviceT[1].text,"DOME_SHUTTER");

    return true;
}

void INDI::Telescope::ISGetProperties (const char *dev)
{
    //  First we let our parent populate
    DefaultDevice::ISGetProperties(dev);

    defineText(&PortTP);
    loadConfig(true, "DEVICE_PORT");

    defineSwitch(&BaudRateSP);
    loadConfig(true, "TELESCOPE_BAUD_RATE");

    defineText(&AddressTP);
    loadConfig(true, "TCP_IP_ADDRESS");

    defineText(&ActiveDeviceTP);
    loadConfig(true, "ACTIVE_DEVICES");

    defineSwitch(&DomeClosedLockTP);
    loadConfig(true, "DOME_POLICY");

    if(isConnected())
    {
        //  Now we add our telescope specific stuff
        defineSwitch(&CoordSP);
        defineNumber(&EqNP);
        if (CanAbort())
            defineSwitch(&AbortSP);

        if (HasTime())
            defineText(&TimeTP);
        if (HasLocation())
            defineNumber(&LocationNP);

        if (CanPark())
        {
            defineSwitch(&ParkSP);
            if (parkDataType != PARK_NONE)
            {
                defineNumber(&ParkPositionNP);
                defineSwitch(&ParkOptionSP);
            }
        }
        defineSwitch(&MovementNSSP);
        defineSwitch(&MovementWESP);

        if (nSlewRate >= 4)
            defineSwitch(&SlewRateSP);

        defineNumber(&ScopeParametersNP);        
        defineNumber(&TargetNP);        

    }

    controller->ISGetProperties(dev);

}

bool INDI::Telescope::updateProperties()
{

    if(isConnected())
    {
        controller->mapController("MOTIONDIR", "N/S/W/E Control", INDI::Controller::CONTROLLER_JOYSTICK, "JOYSTICK_1");
        if (nSlewRate >= 4)
        {
            controller->mapController("SLEWPRESET", "Slew Rate", INDI::Controller::CONTROLLER_JOYSTICK, "JOYSTICK_2");
            controller->mapController("SLEWPRESETUP", "Slew Rate Up", INDI::Controller::CONTROLLER_BUTTON, "BUTTON_5");
            controller->mapController("SLEWPRESETDOWN", "Slew Rate Down", INDI::Controller::CONTROLLER_BUTTON, "BUTTON_6");
        }
        if (CanAbort())
            controller->mapController("ABORTBUTTON", "Abort", INDI::Controller::CONTROLLER_BUTTON, "BUTTON_1");
        if (CanPark())
        {
            controller->mapController("PARKBUTTON", "Park", INDI::Controller::CONTROLLER_BUTTON, "BUTTON_2");
            controller->mapController("UNPARKBUTTON", "UnPark", INDI::Controller::CONTROLLER_BUTTON, "BUTTON_3");
        }

        //  Now we add our telescope specific stuff
        defineSwitch(&CoordSP);
        defineNumber(&EqNP);
        if (CanAbort())
            defineSwitch(&AbortSP);
        defineSwitch(&MovementNSSP);
        defineSwitch(&MovementWESP);
        if (nSlewRate >= 4)
            defineSwitch(&SlewRateSP);

        if (HasTime())
            defineText(&TimeTP);
        if (HasLocation())
            defineNumber(&LocationNP);
        if (CanPark())
        {
            defineSwitch(&ParkSP);
            if (parkDataType != PARK_NONE)
            {
                defineNumber(&ParkPositionNP);
                defineSwitch(&ParkOptionSP);
            }
        }
        defineNumber(&ScopeParametersNP);
        defineNumber(&TargetNP);
    }
    else
    {
        deleteProperty(CoordSP.name);
        deleteProperty(EqNP.name);
        if (CanAbort())
            deleteProperty(AbortSP.name);
        deleteProperty(MovementNSSP.name);
        deleteProperty(MovementWESP.name);
        if (nSlewRate >= 4)
            deleteProperty(SlewRateSP.name);

        if (HasTime())
            deleteProperty(TimeTP.name);
        if (HasLocation())
            deleteProperty(LocationNP.name);

        if (CanPark())
        {
            deleteProperty(ParkSP.name);
            if (parkDataType != PARK_NONE)
            {
                deleteProperty(ParkPositionNP.name);
                deleteProperty(ParkOptionSP.name);
            }
        }
        deleteProperty(ScopeParametersNP.name);
    }

    controller->updateProperties();
    ISwitchVectorProperty *useJoystick = getSwitch("USEJOYSTICK");
    if (useJoystick)
    {
        if (isConnected())
        {
            if (useJoystick->sp[0].s == ISS_ON)
            {
                defineSwitch(&LockAxisSP);
                loadConfig(true, "LOCK_AXIS");
            }
            else
                deleteProperty(LockAxisSP.name);
        }
        else
            deleteProperty(LockAxisSP.name);
    }

    return true;
}

bool INDI::Telescope::ISSnoopDevice(XMLEle *root)
{
    controller->ISSnoopDevice(root);

    XMLEle *ep=NULL;
    const char *propName = findXMLAttValu(root, "name");

    if (isConnected())
    {
        if (HasLocation() && !strcmp(propName, "GEOGRAPHIC_COORD"))
        {
            // Only accept IPS_OK state
            if (strcmp(findXMLAttValu(root, "state"), "Ok"))
                return false;

            double longitude=-1, latitude=-1, elevation=-1;

            for (ep = nextXMLEle(root, 1) ; ep != NULL ; ep = nextXMLEle(root, 0))
            {
                const char *elemName = findXMLAttValu(ep, "name");

                if (!strcmp(elemName, "LAT"))
                    latitude = atof(pcdataXMLEle(ep));
                else if (!strcmp(elemName, "LONG"))
                    longitude = atof(pcdataXMLEle(ep));
                else if (!strcmp(elemName, "ELEV"))
                    elevation = atof(pcdataXMLEle(ep));

            }

            return processLocationInfo(latitude, longitude, elevation);
        }
        else if (HasTime() && !strcmp(propName, "TIME_UTC"))
        {
            // Only accept IPS_OK state
            if (strcmp(findXMLAttValu(root, "state"), "Ok"))
                return false;

            char utc[MAXINDITSTAMP], offset[MAXINDITSTAMP];

            for (ep = nextXMLEle(root, 1) ; ep != NULL ; ep = nextXMLEle(root, 0))
            {
                const char *elemName = findXMLAttValu(ep, "name");

                if (!strcmp(elemName, "UTC"))
                    strncpy(utc, pcdataXMLEle(ep), MAXINDITSTAMP);
                else if (!strcmp(elemName, "OFFSET"))
                    strncpy(offset, pcdataXMLEle(ep), MAXINDITSTAMP);
            }

            return processTimeInfo(utc, offset);
        } else if (!strcmp(propName, "DOME_PARK") || !strcmp(propName, "DOME_SHUTTER"))
        {
            if (strcmp(findXMLAttValu(root, "state"), "Ok"))
            {
                // Dome options is dome parks or both and dome is parking.
                if ((DomeClosedLockT[2].s == ISS_ON || DomeClosedLockT[3].s == ISS_ON) && !IsLocked && !IsParked)
                {
                    Park();
                    DEBUG(INDI::Logger::DBG_SESSION, "Dome is closing, parking mount...");
                }
            } // Dome is changing state and Dome options is lock or both. d
            else if (!strcmp(findXMLAttValu(root, "state"), "Ok"))
            {
                bool prevState = IsLocked;
                for (ep = nextXMLEle(root, 1) ; ep != NULL ; ep = nextXMLEle(root, 0))
                {
                    const char *elemName = findXMLAttValu(ep, "name");

                    if (!IsLocked && (!strcmp(elemName, "PARK")) && !strcmp(pcdataXMLEle(ep), "On"))
                        IsLocked = true;
                    else if (IsLocked && (!strcmp(elemName, "UNPARK")) && !strcmp(pcdataXMLEle(ep), "On"))
                        IsLocked = false;
                }
                if (prevState != IsLocked && (DomeClosedLockT[1].s == ISS_ON || DomeClosedLockT[3].s == ISS_ON))
                    DEBUGF(INDI::Logger::DBG_SESSION, "Dome status changed. Lock is set to: %s"
                        , IsLocked ? "locked" : "unlock");
            }
            return true;
        }
    }

    return INDI::DefaultDevice::ISSnoopDevice(root);
}

void INDI::Telescope::triggerSnoop(const char *driverName, const char *snoopedProp)
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "Active Snoop, driver: %s, property: %s", driverName, snoopedProp);
    IDSnoopDevice(driverName, snoopedProp);
}

bool INDI::Telescope::saveConfigItems(FILE *fp)
{
    DefaultDevice::saveConfigItems(fp);

    IUSaveConfigText(fp, &ActiveDeviceTP);
    IUSaveConfigSwitch(fp, &DomeClosedLockTP);
    IUSaveConfigText(fp, &PortTP);
    IUSaveConfigSwitch(fp, &BaudRateSP);
    // Exists and not empty
    if (AddressT[0].text && AddressT[0].text[0] && AddressT[1].text && AddressT[1].text[0])
        IUSaveConfigText(fp, &AddressTP);
    if (HasLocation())
        IUSaveConfigNumber(fp,&LocationNP);
    IUSaveConfigNumber(fp, &ScopeParametersNP);

    controller->saveConfigItems(fp);

    IUSaveConfigSwitch(fp, &LockAxisSP);

    return true;
}

void INDI::Telescope::NewRaDec(double ra,double dec)
{
    switch(TrackState)
    {
       case SCOPE_PARKED:
       case SCOPE_IDLE:
        EqNP.s=IPS_IDLE;
        break;

       case SCOPE_SLEWING:
        EqNP.s=IPS_BUSY;
        break;

       case SCOPE_TRACKING:
        EqNP.s=IPS_OK;
        break;

      default:
        break;
    }

    if (EqN[AXIS_RA].value != ra || EqN[AXIS_DE].value != dec || EqNP.s != lastEqState)
    {
        EqN[AXIS_RA].value=ra;
        EqN[AXIS_DE].value=dec;
        lastEqState = EqNP.s;
        IDSetNumber(&EqNP, NULL);
    }

}

bool INDI::Telescope::Sync(double ra,double dec)
{
    //  if we get here, our mount doesn't support sync
    DEBUG(Logger::DBG_ERROR, "Telescope does not support Sync.");
    return false;
}

bool INDI::Telescope::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    INDI_UNUSED(dir);
    INDI_UNUSED(command);
    DEBUG(Logger::DBG_ERROR, "Telescope does not support North/South motion.");
    IUResetSwitch(&MovementNSSP);
    MovementNSSP.s = IPS_IDLE;
    IDSetSwitch(&MovementNSSP, NULL);
    return false;
}

bool INDI::Telescope::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    INDI_UNUSED(dir);
    INDI_UNUSED(command);
    DEBUG(Logger::DBG_ERROR,"Telescope does not support West/East motion.");
    IUResetSwitch(&MovementWESP);
    MovementWESP.s = IPS_IDLE;
    IDSetSwitch(&MovementWESP, NULL);
    return false;
}

/**************************************************************************************
** Process Text properties
***************************************************************************************/
bool INDI::Telescope::ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
{
    //  first check if it's for our device
    if(!strcmp(dev,getDeviceName()))
    {
        // Serial Port
        if(!strcmp(name,PortTP.name))
        {
            IUUpdateText(&PortTP,texts,names,n);
            PortTP.s=IPS_OK;
            IDSetText(&PortTP,NULL);
            return true;
        }

        // TCP Server settings
        if (!strcmp(name, AddressTP.name))
        {
            IUUpdateText(&AddressTP, texts, names, n);
            AddressTP.s = IPS_OK;
            IDSetText(&AddressTP, NULL);
            return true;
        }

        if(!strcmp(name,TimeTP.name))
        {
            int utcindex   = IUFindIndex("UTC", names, n);
            int offsetindex= IUFindIndex("OFFSET", names, n);

            return processTimeInfo(texts[utcindex], texts[offsetindex]);
        }

        if(!strcmp(name,ActiveDeviceTP.name))
        {
            ActiveDeviceTP.s=IPS_OK;
            IUUpdateText(&ActiveDeviceTP,texts,names,n);
            //  Update client display
            IDSetText(&ActiveDeviceTP,NULL);

            IDSnoopDevice(ActiveDeviceT[0].text,"GEOGRAPHIC_COORD");
            IDSnoopDevice(ActiveDeviceT[0].text,"TIME_UTC");

            IDSnoopDevice(ActiveDeviceT[1].text,"DOME_PARK");
            IDSnoopDevice(ActiveDeviceT[1].text,"DOME_SHUTTER");
            return true;
        }

    }

    controller->ISNewText(dev, name, texts, names, n);

    return DefaultDevice::ISNewText(dev,name,texts,names,n);
}

/**************************************************************************************
**
***************************************************************************************/
bool INDI::Telescope::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device
    if(strcmp(dev,getDeviceName())==0)
    {
        if(strcmp(name,"EQUATORIAL_EOD_COORD")==0)
        {
            //  this is for us, and it is a goto
            bool rc=false;
            double ra=-1;
            double dec=-100;

            for (int x=0; x<n; x++)
            {

                //IDLog("request stuff %s %4.2f\n",names[x],values[x]);

                INumber *eqp = IUFindNumber (&EqNP, names[x]);
                if (eqp == &EqN[AXIS_RA])
                {
                    ra = values[x];
                } else if (eqp == &EqN[AXIS_DE])
                {
                    dec = values[x];
                }
            }
            if ((ra>=0)&&(ra<=24)&&(dec>=-90)&&(dec<=90))
            {
                // Check if it is already parked.
                if (CanPark())
                {
                    if (isParked())
                    {
                        DEBUG(INDI::Logger::DBG_WARNING, "Please unpark the mount before issuing any motion/sync commands.");
                        EqNP.s = lastEqState = IPS_IDLE;
                        IDSetNumber(&EqNP, NULL);
                        return false;
                    }
                }

                // Check if it can sync
                if (CanSync())
                {
                    ISwitch *sw;
                    sw=IUFindSwitch(&CoordSP,"SYNC");
                    if((sw != NULL)&&( sw->s==ISS_ON ))
                    {
                       rc=Sync(ra,dec);
                       if (rc)
                           EqNP .s = lastEqState = IPS_OK;
                       else
                           EqNP.s = lastEqState = IPS_ALERT;
                       IDSetNumber(&EqNP, NULL);
                       return rc;
                    }
                }

                // Issue GOTO
                rc=Goto(ra,dec);
                if (rc) {
                    EqNP .s = lastEqState = IPS_BUSY;
		    //  Now fill in target co-ords, so domes can start turning
		    TargetN[AXIS_RA].value=ra;
		    TargetN[AXIS_DE].value=dec;
                    IDSetNumber(&TargetNP, NULL);

		} else {
                    EqNP.s = lastEqState = IPS_ALERT;
		}
                IDSetNumber(&EqNP, NULL);

            }
            return rc;
        }

        if(strcmp(name,"GEOGRAPHIC_COORD")==0)
        {
            int latindex = IUFindIndex("LAT", names, n);
            int longindex= IUFindIndex("LONG", names, n);
            int elevationindex = IUFindIndex("ELEV", names, n);

            if (latindex == -1 || longindex==-1 || elevationindex == -1)
            {
                LocationNP.s=IPS_ALERT;
                IDSetNumber(&LocationNP, "Location data missing or corrupted.");
            }

            double targetLat  = values[latindex];
            double targetLong = values[longindex];
            double targetElev = values[elevationindex];

            return processLocationInfo(targetLat, targetLong, targetElev);

        }

        if(strcmp(name,"TELESCOPE_INFO")==0)
        {
            ScopeParametersNP.s = IPS_OK;

            IUUpdateNumber(&ScopeParametersNP,values,names,n);
            IDSetNumber(&ScopeParametersNP,NULL);

            return true;
        }

      if(strcmp(name, ParkPositionNP.name) == 0)
      {
        IUUpdateNumber(&ParkPositionNP, values, names, n);
        ParkPositionNP.s = IPS_OK;

        Axis1ParkPosition = ParkPositionN[AXIS_RA].value;
        Axis2ParkPosition = ParkPositionN[AXIS_DE].value;

        IDSetNumber(&ParkPositionNP, NULL);
        return true;
      }

    }

    return DefaultDevice::ISNewNumber(dev,name,values,names,n);
}

/**************************************************************************************
**
***************************************************************************************/
bool INDI::Telescope::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if(strcmp(dev,getDeviceName())==0)
    {
        //  This one is for us
        if(!strcmp(name,CoordSP.name))
        {
            //  client is telling us what to do with co-ordinate requests
            CoordSP.s=IPS_OK;
            IUUpdateSwitch(&CoordSP,states,names,n);
            //  Update client display
            IDSetSwitch(&CoordSP, NULL);
            return true;
        }

        // Slew Rate
        if (!strcmp (name, SlewRateSP.name))
        {
          int preIndex = IUFindOnSwitchIndex(&SlewRateSP);
          IUUpdateSwitch(&SlewRateSP, states, names, n);
          int nowIndex = IUFindOnSwitchIndex(&SlewRateSP);
          if (SetSlewRate(nowIndex) == false)
          {
              IUResetSwitch(&SlewRateSP);
              SlewRateS[preIndex].s = ISS_ON;
              SlewRateSP.s = IPS_ALERT;
          }
          else
            SlewRateSP.s = IPS_OK;
          IDSetSwitch(&SlewRateSP, NULL);
          return true;
        }

        if(!strcmp(name,ParkSP.name))
        {
            if (TrackState == SCOPE_PARKING)
            {
                IUResetSwitch(&ParkSP);
                ParkSP.s = IPS_ALERT;
                Abort();
                DEBUG(INDI::Logger::DBG_SESSION, "Parking/Unparking aborted.");
                IDSetSwitch(&ParkSP, NULL);
                return true;
            }

            int preIndex = IUFindOnSwitchIndex(&ParkSP);
            IUUpdateSwitch(&ParkSP, states, names, n);

            bool toPark = (ParkS[0].s == ISS_ON);

            if (toPark == false && TrackState != SCOPE_PARKED)
            {
                IUResetSwitch(&ParkSP);
                ParkS[1].s = ISS_ON;
                ParkSP.s = IPS_IDLE;
                DEBUG(INDI::Logger::DBG_SESSION, "Telescope already unparked.");
                IDSetSwitch(&ParkSP, NULL);
                return true;
            }

            if (toPark && TrackState == SCOPE_PARKED)
            {
                IUResetSwitch(&ParkSP);
                ParkS[0].s = ISS_ON;
                ParkSP.s = IPS_IDLE;
                DEBUG(INDI::Logger::DBG_SESSION, "Telescope already parked.");
                IDSetSwitch(&ParkSP, NULL);
                return true;
            }

            IUResetSwitch(&ParkSP);
            bool rc =  toPark ? Park() : UnPark();
            if (rc)
            {
                if (TrackState == SCOPE_PARKING)
                {
                     ParkS[0].s = toPark ? ISS_ON : ISS_OFF;
                     ParkS[1].s = toPark ? ISS_OFF : ISS_ON;
                     ParkSP.s = IPS_BUSY;
                }
                else
                {
                    ParkS[0].s = toPark ? ISS_ON : ISS_OFF;
                    ParkS[1].s = toPark ? ISS_OFF : ISS_ON;
                    ParkSP.s = IPS_OK;
                }
            }
            else
            {
                ParkS[preIndex].s = ISS_ON;
                ParkSP.s = IPS_ALERT;
            }

            IDSetSwitch(&ParkSP, NULL);
            return true;
        }

        if(!strcmp(name,MovementNSSP.name))
        {            
            // Check if it is already parked.
            if (CanPark())
            {
                if (isParked())
                {
                    DEBUG(INDI::Logger::DBG_WARNING, "Please unpark the mount before issuing any motion/sync commands.");
                    MovementNSSP.s = IPS_IDLE;
                    IDSetSwitch(&MovementNSSP, NULL);
                    return false;
                }
            }

            IUUpdateSwitch(&MovementNSSP,states,names,n);

            int current_motion = IUFindOnSwitchIndex(&MovementNSSP);

            // if same move requested, return
            if ( MovementNSSP.s == IPS_BUSY && current_motion == last_ns_motion)
                return true;

            // Time to stop motion
            if (current_motion == -1 || (last_ns_motion != -1 && current_motion != last_ns_motion))
            {
                if (MoveNS(last_ns_motion == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_STOP))
                {
                    IUResetSwitch(&MovementNSSP);
                    MovementNSSP.s = IPS_IDLE;
                    last_ns_motion = -1;
                }
                else
                    MovementNSSP.s = IPS_ALERT;
            }
            else
            {
                if (MoveNS(current_motion == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_START))
                {
                    MovementNSSP.s = IPS_BUSY;
                    last_ns_motion = current_motion;
                }
                else
                {
                    IUResetSwitch(&MovementNSSP);
                    MovementNSSP.s = IPS_ALERT;
                    last_ns_motion = -1;
                }
            }

            IDSetSwitch(&MovementNSSP, NULL);

            return true;
        }

        if(!strcmp(name,MovementWESP.name))
        {
            // Check if it is already parked.
            if (CanPark())
            {
                if (isParked())
                {
                    DEBUG(INDI::Logger::DBG_WARNING, "Please unpark the mount before issuing any motion/sync commands.");
                    MovementWESP.s = IPS_IDLE;
                    IDSetSwitch(&MovementWESP, NULL);
                    return false;
                }
            }

            IUUpdateSwitch(&MovementWESP,states,names,n);

            int current_motion = IUFindOnSwitchIndex(&MovementWESP);

            // if same move requested, return
            if ( MovementWESP.s == IPS_BUSY && current_motion == last_we_motion)
                return true;

            // Time to stop motion
            if (current_motion == -1 || (last_we_motion != -1 && current_motion != last_we_motion))
            {
                if (MoveWE(last_we_motion == 0 ? DIRECTION_WEST : DIRECTION_EAST, MOTION_STOP))
                {
                    IUResetSwitch(&MovementWESP);
                    MovementWESP.s = IPS_IDLE;
                    last_we_motion = -1;
                }
                else
                    MovementWESP.s = IPS_ALERT;
            }
            else
            {
                if (MoveWE(current_motion == 0 ? DIRECTION_WEST : DIRECTION_EAST, MOTION_START))
                {
                    MovementWESP.s = IPS_BUSY;
                    last_we_motion = current_motion;
                }
                else
                {
                    IUResetSwitch(&MovementWESP);
                    MovementWESP.s = IPS_ALERT;
                    last_we_motion = -1;
                }
            }

            IDSetSwitch(&MovementWESP, NULL);

            return true;
        }

        if(!strcmp(name,AbortSP.name))
        {
            IUResetSwitch(&AbortSP);

            if (Abort())
            {
                AbortSP.s = IPS_OK;

                if (ParkSP.s == IPS_BUSY)
                {
                    IUResetSwitch(&ParkSP);
                    ParkSP.s = IPS_ALERT;
                    IDSetSwitch(&ParkSP, NULL);

                    DEBUG(INDI::Logger::DBG_SESSION, "Parking aborted.");
                }
                if (EqNP.s == IPS_BUSY)
                {
                    EqNP.s = lastEqState = IPS_IDLE;
                    IDSetNumber(&EqNP, NULL);
                    DEBUG(INDI::Logger::DBG_SESSION, "Slew/Track aborted.");
                }
                if (MovementWESP.s == IPS_BUSY)
                {
                    IUResetSwitch(&MovementWESP);
                    MovementWESP.s = IPS_IDLE;
                    IDSetSwitch(&MovementWESP, NULL);
                }
                if (MovementNSSP.s == IPS_BUSY)
                {
                    IUResetSwitch(&MovementNSSP);
                    MovementNSSP.s = IPS_IDLE;
                    IDSetSwitch(&MovementNSSP, NULL);
                }

                last_ns_motion=last_we_motion=-1;
                if (TrackState != SCOPE_PARKED)
                    TrackState = SCOPE_IDLE;
            }
            else
                AbortSP.s = IPS_ALERT;

            IDSetSwitch(&AbortSP, NULL);

            return true;
        }

      if (!strcmp(name, ParkOptionSP.name))
      {
        IUUpdateSwitch(&ParkOptionSP, states, names, n);
        ISwitch *sp = IUFindOnSwitch(&ParkOptionSP);
        if (!sp)
          return false;

        IUResetSwitch(&ParkOptionSP);

        if ( (TrackState != SCOPE_IDLE && TrackState != SCOPE_TRACKING) || MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY)
        {
          DEBUG(INDI::Logger::DBG_SESSION, "Can not change park position while slewing or already parked...");
          ParkOptionSP.s=IPS_ALERT;
          IDSetSwitch(&ParkOptionSP, NULL);
          return false;
        }

        if (!strcmp(sp->name, "PARK_CURRENT"))
        {
            SetCurrentPark();
        }
        else if (!strcmp(sp->name, "PARK_DEFAULT"))
        {
            SetDefaultPark();
        }
        else if (!strcmp(sp->name, "PARK_WRITE_DATA"))
        {
          if (WriteParkData())
            DEBUG(INDI::Logger::DBG_SESSION, "Saved Park Status/Position.");
          else
            DEBUG(INDI::Logger::DBG_WARNING, "Can not save Park Status/Position.");
        }

        ParkOptionSP.s = IPS_OK;
        IDSetSwitch(&ParkOptionSP, NULL);

        return true;
      }

      if (!strcmp(name, BaudRateSP.name))
      {
          IUUpdateSwitch(&BaudRateSP, states, names, n);
          BaudRateSP.s = IPS_OK;
          IDSetSwitch(&BaudRateSP, NULL);
          return true;
      }

      // Dome parking policy
      if (!strcmp(name, DomeClosedLockTP.name))
      {
          if (n == 1)
          {
              if (!strcmp(names[0], DomeClosedLockT[0].name))
                  DEBUG(INDI::Logger::DBG_SESSION, "Dome parking policy set to: Ignore dome");
              else if (!strcmp(names[0], DomeClosedLockT[1].name))
                  DEBUG(INDI::Logger::DBG_SESSION, "Warning: Dome parking policy set to: Dome locks. This disallows the scope from unparking when dome is parked");
              else if (!strcmp(names[0], DomeClosedLockT[2].name))
                  DEBUG(INDI::Logger::DBG_SESSION, "Warning: Dome parking policy set to: Dome parks. This tells scope to park if dome is parking. This will disable the locking for dome parking, EVEN IF MOUNT PARKING FAILS");
              else if (!strcmp(names[0], DomeClosedLockT[3].name))
                  DEBUG(INDI::Logger::DBG_SESSION, "Warning: Dome parking policy set to: Both. This disallows the scope from unparking when dome is parked, and tells scope to park if dome is parking. This will disable the locking for dome parking, EVEN IF MOUNT PARKING FAILS.");
          }
          IUUpdateSwitch(&DomeClosedLockTP, states, names, n);
          DomeClosedLockTP.s = IPS_OK;
          IDSetSwitch(&DomeClosedLockTP, NULL);

          triggerSnoop(ActiveDeviceT[1].text, "DOME_PARK");
          return true;
      }

      // Lock Axis
      if (!strcmp(name, LockAxisSP.name))
      {
          IUUpdateSwitch(&LockAxisSP, states, names, n);
          LockAxisSP.s = IPS_OK;
          IDSetSwitch(&LockAxisSP, NULL);
          if (LockAxisS[AXIS_RA].s == ISS_ON)
              DEBUG(INDI::Logger::DBG_SESSION, "Joystick motion is locked to West/East axis only.");
          else if (LockAxisS[AXIS_DE].s == ISS_ON)
              DEBUG(INDI::Logger::DBG_SESSION, "Joystick motion is locked to North/South axis only.");
          else
              DEBUG(INDI::Logger::DBG_SESSION, "Joystick motion is unlocked.");
          return true;
      }
    }

    bool rc = controller->ISNewSwitch(dev, name, states, names, n);
    if (rc)
    {
        ISwitchVectorProperty *useJoystick = getSwitch("USEJOYSTICK");
        if (useJoystick && useJoystick->sp[0].s == ISS_ON)
            defineSwitch(&LockAxisSP);
        else
            deleteProperty(LockAxisSP.name);
    }

    //  Nobody has claimed this, so, ignore it
    return DefaultDevice::ISNewSwitch(dev,name,states,names,n);
}


bool INDI::Telescope::Connect()
{
    bool rc=false;

    if(isConnected() || isSimulation())
        return true;

    // Check if TCP Address exists and not empty
    if (AddressT[0].text && AddressT[0].text[0] && AddressT[1].text && AddressT[1].text[0])
        rc = Connect(AddressT[0].text, AddressT[1].text);
    else
        rc = Connect(PortT[0].text, atoi(IUFindOnSwitch(&BaudRateSP)->name));

    if(rc)
        SetTimer(updatePeriodMS);
    return rc;
}


bool INDI::Telescope::Connect(const char *port, uint32_t baud)
{
    //  We want to connect to a port
    //  For now, we will assume it's a serial port
    int connectrc=0;
    char errorMsg[MAXRBUF];
    bool rc;

    DEBUGF(Logger::DBG_DEBUG, "INDI::Telescope connecting to %s",port);

    if ( (connectrc = tty_connect(port, baud, 8, 0, 1, &PortFD)) != TTY_OK)
    {
        tty_error_msg(connectrc, errorMsg, MAXRBUF);

        DEBUGF(Logger::DBG_ERROR,"Failed to connect to port %s. Error: %s", port, errorMsg);

        return false;

    }

    DEBUGF(Logger::DBG_DEBUG, "Port FD %d",PortFD);

    /* Test connection */
    rc=ReadScopeStatus();
    if(rc)
    {
        //  We got a valid scope status read
        DEBUG(Logger::DBG_SESSION,"Telescope is online.");
        return rc;
    }

    //  Ok, we didn't get a valid read
    //  So, we need to close our handle and send error messages
    tty_disconnect(PortFD);

    return false;
}

bool INDI::Telescope::Connect(const char *hostname, const char *port)
{
    if (sockfd != -1)
        close(sockfd);

    struct timeval ts;
    ts.tv_sec = SOCKET_TIMEOUT;
    ts.tv_usec=0;

    DEBUGF(INDI::Logger::DBG_SESSION, "Connecting to %s@%s ...", hostname, port);

    struct sockaddr_in serv_addr;
    struct hostent *hp = NULL;
    int ret = 0;

    // Lookup host name or IPv4 address
    hp = gethostbyname(hostname);
    if (!hp)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to lookup IP Address or hostname.");
        return false;
    }

    memset (&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr_list[0]))->s_addr;
    serv_addr.sin_port = htons(atoi(port));

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to create socket.");
        return false;
    }

    // Connect to the mount
    if ( (ret = ::connect (sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))) < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Failed to connect to mount %s@%s: %s.", hostname, port, strerror(errno));
        close(sockfd);
        sockfd=-1;
        return false;
    }

    // Set the socket receiving and sending timeouts
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&ts,sizeof(struct timeval));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&ts,sizeof(struct timeval));

    DEBUGF(INDI::Logger::DBG_SESSION, "Connected successfuly to %s.", getDeviceName());

    // now let the rest of INDI::Telescope use our socket as if it were a serial port
    PortFD = sockfd;

    return true;
}

bool INDI::Telescope::Disconnect()
{
    DEBUG(Logger::DBG_SESSION,"Telescope is offline.");

    if (isSimulation())
        return true;

    if (sockfd != -1)
    {
        close(sockfd);
        sockfd = -1;
    }
    else
        tty_disconnect(PortFD);

    return true;
}


void INDI::Telescope::TimerHit()
{
    if(isConnected())
    {
        bool rc;

        rc=ReadScopeStatus();

        if(rc == false)
        {
            //  read was not good
            EqNP.s= lastEqState = IPS_ALERT;
            IDSetNumber(&EqNP, NULL);
        }

        SetTimer(updatePeriodMS);
    }
}


bool INDI::Telescope::Park()
{
    DEBUG(INDI::Logger::DBG_WARNING, "Parking is not supported.");
    return false;
}

bool  INDI::Telescope::UnPark()
{
    DEBUG(INDI::Logger::DBG_WARNING, "UnParking is not supported.");
    return false;
}

void INDI::Telescope::SetCurrentPark()
{
    DEBUG(INDI::Logger::DBG_WARNING, "Parking is not supported.");
}

void INDI::Telescope::SetDefaultPark()
{
    DEBUG(INDI::Logger::DBG_WARNING, "Parking is not supported.");
}

bool INDI::Telescope::processTimeInfo(const char *utc, const char *offset)
{
    struct ln_date utc_date;
    double utc_offset=0;

    if (extractISOTime(utc, &utc_date) == -1)
    {
      TimeTP.s = IPS_ALERT;
      IDSetText(&TimeTP, "Date/Time is invalid: %s.", utc);
      return false;
    }

    utc_offset = atof(offset);

    if (updateTime(&utc_date, utc_offset))
    {
        IUSaveText(&TimeT[0], utc);
        IUSaveText(&TimeT[1], offset);
        TimeTP.s = IPS_OK;
        IDSetText(&TimeTP, NULL);
        return true;
    }
    else
    {
        TimeTP.s = IPS_ALERT;
        IDSetText(&TimeTP, NULL);
        return false;
    }
}

bool INDI::Telescope::processLocationInfo(double latitude, double longitude, double elevation)
{
    // Do not update if not necessary
    if (latitude == LocationN[LOCATION_LATITUDE].value && longitude == LocationN[LOCATION_LONGITUDE].value && elevation == LocationN[LOCATION_ELEVATION].value)
    {
        LocationNP.s=IPS_OK;
        IDSetNumber(&LocationNP,NULL);
    }

    if (updateLocation(latitude, longitude, elevation))
    {
        LocationNP.s=IPS_OK;
        LocationN[LOCATION_LATITUDE].value  = latitude;
        LocationN[LOCATION_LONGITUDE].value = longitude;
        LocationN[LOCATION_ELEVATION].value = elevation;
        //  Update client display
        IDSetNumber(&LocationNP,NULL);

        return true;
    }
    else
    {
        LocationNP.s=IPS_ALERT;
        //  Update client display
        IDSetNumber(&LocationNP,NULL);

        return false;
    }
}

bool INDI::Telescope::updateTime(ln_date *utc, double utc_offset)
{
    INDI_UNUSED(utc);
    INDI_UNUSED(utc_offset);

    return true;
}

bool INDI::Telescope::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(latitude);
    INDI_UNUSED(longitude);
    INDI_UNUSED(elevation);

    return true;
}

void INDI::Telescope::SetTelescopeCapability(uint32_t cap, uint8_t slewRateCount)
{
    capability = cap;
    nSlewRate = slewRateCount;

    if (CanSync())
        IUFillSwitchVector(&CoordSP,CoordS,3,getDeviceName(),"ON_COORD_SET","On Set",MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,60,IPS_IDLE);
    else
        IUFillSwitchVector(&CoordSP,CoordS,2,getDeviceName(),"ON_COORD_SET","On Set",MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,60,IPS_IDLE);

    if (nSlewRate >= 4)
    {
        free(SlewRateS);
        SlewRateS = (ISwitch *) malloc(sizeof(ISwitch) * nSlewRate);
        int step = nSlewRate / 4;
        for (int i=0; i < nSlewRate; i++)
        {
            char name[4];
            snprintf(name, 4, "%dx", i+1);
            IUFillSwitch(SlewRateS+i, name, name, ISS_OFF);
        }

        strncpy( (SlewRateS+(step*0))->name, "SLEW_GUIDE", MAXINDINAME);
        strncpy( (SlewRateS+(step*1))->name, "SLEW_CENTERING", MAXINDINAME);
        strncpy( (SlewRateS+(step*2))->name, "SLEW_FIND", MAXINDINAME);
        strncpy( (SlewRateS+(nSlewRate-1))->name, "SLEW_MAX", MAXINDINAME);

        // By Default we set current Slew Rate to 0.5 of max
        (SlewRateS+(nSlewRate/2))->s = ISS_ON;

        IUFillSwitchVector(&SlewRateSP, SlewRateS, nSlewRate, getDeviceName(), "TELESCOPE_SLEW_RATE", "Slew Rate", MOTION_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    }
}

void INDI::Telescope::SetParkDataType(TelescopeParkData type)
{
    parkDataType = type;

    if (parkDataType != PARK_NONE)
    {
        switch (parkDataType)
        {
            case PARK_RA_DEC:
            IUFillNumber(&ParkPositionN[AXIS_RA],"PARK_RA","RA (hh:mm:ss)","%010.6m",0,24,0,0);
            IUFillNumber(&ParkPositionN[AXIS_DE],"PARK_DEC","DEC (dd:mm:ss)","%010.6m",-90,90,0,0);
            break;

            case PARK_AZ_ALT:
            IUFillNumber(&ParkPositionN[AXIS_AZ],"PARK_AZ","AZ D:M:S", "%10.6m", 0.0, 360.0, 0.0, 0);
            IUFillNumber(&ParkPositionN[AXIS_ALT],"PARK_ALT", "Alt  D:M:S", "%10.6m", -90., 90.0, 0.0, 0);
            break;

            case PARK_RA_DEC_ENCODER:
            IUFillNumber(&ParkPositionN[AXIS_RA],"PARK_RA" ,"RA Encoder","%.0f" ,0,16777215,1,0);
            IUFillNumber(&ParkPositionN[AXIS_DE],"PARK_DEC","DEC Encoder","%.0f",0,16777215,1,0);
            break;

            case PARK_AZ_ALT_ENCODER:
            IUFillNumber(&ParkPositionN[AXIS_RA],"PARK_AZ" ,"AZ Encoder","%.0f" ,0,16777215,1,0);
            IUFillNumber(&ParkPositionN[AXIS_DE],"PARK_ALT","ALT Encoder","%.0f",0,16777215,1,0);
            break;

        default:
            break;
        }

        IUFillNumberVector(&ParkPositionNP,ParkPositionN,2,getDeviceName(),"TELESCOPE_PARK_POSITION","Park Position", SITE_TAB,IP_RW,60,IPS_IDLE);
    }
}

void INDI::Telescope::SetParked(bool isparked)
{
  IsParked=isparked;
  IUResetSwitch(&ParkSP);

  ParkSP.s = IPS_OK;
  if (IsParked)
  {      
      ParkS[0].s = ISS_ON;
      TrackState = SCOPE_PARKED;
      DEBUG(INDI::Logger::DBG_SESSION, "Mount is parked.");
  }
  else
  {
      ParkS[1].s = ISS_ON;
      TrackState = SCOPE_IDLE;
      DEBUG(INDI::Logger::DBG_SESSION, "Mount is unparked.");
  }

  IDSetSwitch(&ParkSP, NULL);

  if (parkDataType != PARK_NONE)
    WriteParkData();
}

bool INDI::Telescope::isParked()
{
  return IsParked;
}

bool INDI::Telescope::InitPark()
{
  char *loadres;
  loadres=LoadParkData();
  if (loadres)
  {
    DEBUGF(INDI::Logger::DBG_SESSION, "InitPark: No Park data in file %s: %s", Parkdatafile, loadres);
    SetParked(false);
    return false;
  }

  SetParked(isParked());

  ParkPositionN[AXIS_RA].value = Axis1ParkPosition;
  ParkPositionN[AXIS_DE].value = Axis2ParkPosition;
  IDSetNumber(&ParkPositionNP, NULL);

  return true;
}

char *INDI::Telescope::LoadParkData()
{
  wordexp_t wexp;
  FILE *fp;
  LilXML *lp;
  static char errmsg[512];

  XMLEle *parkxml;
  XMLAtt *ap;
  bool devicefound=false;

  ParkDeviceName = getDeviceName();
  ParkstatusXml=NULL;
  ParkdeviceXml=NULL;
  ParkpositionXml = NULL;
  ParkpositionAxis1Xml = NULL;
  ParkpositionAxis2Xml = NULL;

  if (wordexp(Parkdatafile, &wexp, 0))
  {
    wordfree(&wexp);
    return (char *)("Badly formed filename.");
  }

  if (!(fp=fopen(wexp.we_wordv[0], "r")))
  {
    wordfree(&wexp);
    return strerror(errno);
  }
  wordfree(&wexp);

 lp = newLilXML();

 if (ParkdataXmlRoot)
    delXMLEle(ParkdataXmlRoot);

  ParkdataXmlRoot = readXMLFile(fp, lp, errmsg);

  delLilXML(lp);
  if (!ParkdataXmlRoot)
      return errmsg;

  if (!strcmp(tagXMLEle(nextXMLEle(ParkdataXmlRoot, 1)), "parkdata"))
      return (char *)("Not a park data file");

  parkxml=nextXMLEle(ParkdataXmlRoot, 1);

  while (parkxml)
  {
    if (strcmp(tagXMLEle(parkxml), "device"))
    {
        parkxml=nextXMLEle(ParkdataXmlRoot, 0);
        continue;
    }
    ap = findXMLAtt(parkxml, "name");
    if (ap && (!strcmp(valuXMLAtt(ap), ParkDeviceName)))
    {
        devicefound = true;
        break;
    }
    parkxml=nextXMLEle(ParkdataXmlRoot, 0);
  }

  if (!devicefound)
      return (char *)"No park data found for this device";

  ParkdeviceXml=parkxml;
  ParkstatusXml = findXMLEle(parkxml, "parkstatus");
  ParkpositionXml = findXMLEle(parkxml, "parkposition");
  ParkpositionAxis1Xml = findXMLEle(ParkpositionXml, "axis1position");
  ParkpositionAxis2Xml = findXMLEle(ParkpositionXml, "axis2position");
  IsParked=false;

  if (ParkstatusXml == NULL || ParkpositionAxis1Xml == NULL || ParkpositionAxis2Xml == NULL)
  {
      return (char *)("Park data invalid or missing.");
  }

  if (!strcmp(pcdataXMLEle(ParkstatusXml), "true"))
      IsParked=true; 

  int rc=0;
  rc = sscanf(pcdataXMLEle(ParkpositionAxis1Xml), "%lf", &Axis1ParkPosition);
  if (rc != 1)
  {
      return (char *)("Unable to parse Park Position Axis 1.");
  }
  rc = sscanf(pcdataXMLEle(ParkpositionAxis2Xml), "%lf", &Axis2ParkPosition);
  if (rc != 1)
  {
      return (char *)("Unable to parse Park Position Axis 2.");
  }

  return NULL;
}

bool INDI::Telescope::WriteParkData()
{
  wordexp_t wexp;
  FILE *fp;
  char pcdata[30];
  ParkDeviceName = getDeviceName();

  if (wordexp(Parkdatafile, &wexp, 0))
  {
    wordfree(&wexp);
    DEBUGF(INDI::Logger::DBG_SESSION, "WriteParkData: can not write file %s: Badly formed filename.", Parkdatafile);
    return false;
  }

  if (!(fp=fopen(wexp.we_wordv[0], "w")))
  {
    wordfree(&wexp);
    DEBUGF(INDI::Logger::DBG_SESSION, "WriteParkData: can not write file %s: %s", Parkdatafile, strerror(errno));
    return false;
  }

  if (!ParkdataXmlRoot)
      ParkdataXmlRoot=addXMLEle(NULL, "parkdata");

  if (!ParkdeviceXml)
  {
    ParkdeviceXml=addXMLEle(ParkdataXmlRoot, "device");
    addXMLAtt(ParkdeviceXml, "name", ParkDeviceName);
  }

  if (!ParkstatusXml)
      ParkstatusXml=addXMLEle(ParkdeviceXml, "parkstatus");
  if (!ParkpositionXml)
      ParkpositionXml=addXMLEle(ParkdeviceXml, "parkposition");
  if (!ParkpositionAxis1Xml)
      ParkpositionAxis1Xml=addXMLEle(ParkpositionXml, "axis1position");
  if (!ParkpositionAxis2Xml)
      ParkpositionAxis2Xml=addXMLEle(ParkpositionXml, "axis2position");

  editXMLEle(ParkstatusXml, (IsParked?"true":"false"));

  snprintf(pcdata, sizeof(pcdata), "%f", Axis1ParkPosition);
  editXMLEle(ParkpositionAxis1Xml, pcdata);
  snprintf(pcdata, sizeof(pcdata), "%f", Axis2ParkPosition);
  editXMLEle(ParkpositionAxis2Xml, pcdata);

  prXMLEle(fp, ParkdataXmlRoot, 0);
  fclose(fp);

  return true;
}

double INDI::Telescope::GetAxis1Park()
{
  return Axis1ParkPosition;
}
double INDI::Telescope::GetAxis1ParkDefault()
{
  return Axis1DefaultParkPosition;
}
double INDI::Telescope::GetAxis2Park()
{
  return Axis2ParkPosition;
}
double INDI::Telescope::GetAxis2ParkDefault()
{
  return Axis2DefaultParkPosition;
}

void INDI::Telescope::SetAxis1Park(double value)
{
  Axis1ParkPosition=value;
  ParkPositionN[AXIS_RA].value = value;
  IDSetNumber(&ParkPositionNP, NULL);
}

void INDI::Telescope::SetAxis1ParkDefault(double value)
{
  Axis1DefaultParkPosition=value;
}

void INDI::Telescope::SetAxis2Park(double value)
{
  Axis2ParkPosition=value;
  ParkPositionN[AXIS_DE].value = value;
  IDSetNumber(&ParkPositionNP, NULL);
}

void INDI::Telescope::SetAxis2ParkDefault(double value)
{
  Axis2DefaultParkPosition=value;
}

bool INDI::Telescope::isLocked()
{
    return (DomeClosedLockT[1].s == ISS_ON || DomeClosedLockT[3].s == ISS_ON) && IsLocked;
}

bool INDI::Telescope::SetSlewRate(int index)
{
    INDI_UNUSED(index);
    return true;
}

void INDI::Telescope::processButton(const char *button_n, ISState state)
{
    //ignore OFF
    if (state == ISS_OFF)
        return;

    if (!strcmp(button_n, "ABORTBUTTON"))
    {
        ISwitchVectorProperty *trackSW = getSwitch("TELESCOPE_TRACK_RATE");
        // Only abort if we have some sort of motion going on
        if (ParkSP.s == IPS_BUSY || MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY || EqNP.s == IPS_BUSY || (trackSW && trackSW->s == IPS_BUSY))
        {
            // Invoke parent processing so that INDI::Telescope takes care of abort cross-check
            ISState states[1] = { ISS_ON };
            char *names[1] = { AbortS[0].name};
            ISNewSwitch(getDeviceName(), AbortSP.name, states, names, 1);
        }
    }
    else if (!strcmp(button_n, "PARKBUTTON"))
    {
        ISState states[2] = { ISS_ON, ISS_OFF };
        char *names[2] = { ParkS[0].name, ParkS[1].name };
        ISNewSwitch(getDeviceName(), ParkSP.name, states, names, 2);
    }
    else if (!strcmp(button_n, "UNPARKBUTTON"))
    {
        ISState states[2] = { ISS_OFF, ISS_ON };
        char *names[2] = { ParkS[0].name, ParkS[1].name };
        ISNewSwitch(getDeviceName(), ParkSP.name, states, names, 2);
    }
    else if (!strcmp(button_n, "SLEWPRESETUP"))
    {
       processSlewPresets(1, 270); 
    }
    else if (!strcmp(button_n, "SLEWPRESETDOWN"))
    {
       processSlewPresets(1, 90); 
    }   
}

void INDI::Telescope::processJoystick(const char * joystick_n, double mag, double angle)
{
    if (!strcmp(joystick_n, "MOTIONDIR"))
    {
        if ((TrackState == SCOPE_PARKING) || (TrackState == SCOPE_PARKED))
        {
          DEBUG(INDI::Logger::DBG_WARNING, "Can not slew while mount is parking/parked.");
          return;
        }

        processNSWE(mag, angle);
    }
    else if (!strcmp(joystick_n, "SLEWPRESET"))
        processSlewPresets(mag, angle);
}

void INDI::Telescope::processNSWE(double mag, double angle)
{
    if (mag < 0.5)
    {
        // Moving in the same direction will make it stop
        if (MovementNSSP.s == IPS_BUSY)
        {
            if (MoveNS( MovementNSSP.sp[0].s == ISS_ON ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_STOP))
            {
                IUResetSwitch(&MovementNSSP);
                MovementNSSP.s = IPS_IDLE;
                IDSetSwitch(&MovementNSSP, NULL);
            }
            else
            {
                MovementNSSP.s = IPS_ALERT;
                IDSetSwitch(&MovementNSSP, NULL);
            }
        }

        if (MovementWESP.s == IPS_BUSY)
        {
            if (MoveWE( MovementWESP.sp[0].s == ISS_ON ? DIRECTION_WEST : DIRECTION_EAST, MOTION_STOP))
            {
                IUResetSwitch(&MovementWESP);
                MovementWESP.s = IPS_IDLE;
                IDSetSwitch(&MovementWESP, NULL);
            }
            else
            {
                MovementWESP.s = IPS_ALERT;
                IDSetSwitch(&MovementWESP, NULL);
            }
        }
    }
    // Put high threshold
    else if (mag > 0.9)
    {
        // Only one axis can move at a time
        if (LockAxisS[AXIS_RA].s == ISS_ON)
        {
            // West
            if (angle >= 90 && angle <= 270)
                angle = 180;
            // East
            else
                angle = 0;
        }
        else if (LockAxisS[AXIS_DE].s == ISS_ON)
        {
            // North
            if (angle >= 0 && angle<= 180)
                angle = 90;
            // South
            else
                angle = 270;            
        }

        // North
        if (angle > 0 && angle < 180)
        {
            // Don't try to move if you're busy and moving in the same direction
            if (MovementNSSP.s != IPS_BUSY || MovementNSS[0].s != ISS_ON)
                MoveNS(DIRECTION_NORTH, MOTION_START);

            // If angle is close to 90, make it exactly 90 to reduce noise that could trigger east/west motion as well
            if (angle > 80 && angle < 110)
                angle = 90;

            MovementNSSP.s = IPS_BUSY;
            MovementNSSP.sp[DIRECTION_NORTH].s = ISS_ON;
            MovementNSSP.sp[DIRECTION_SOUTH].s = ISS_OFF;
            IDSetSwitch(&MovementNSSP, NULL);
        }
        // South
        if (angle > 180 && angle < 360)
        {
            // Don't try to move if you're busy and moving in the same direction
            if (MovementNSSP.s != IPS_BUSY  || MovementNSS[1].s != ISS_ON)
                MoveNS(DIRECTION_SOUTH, MOTION_START);

            // If angle is close to 270, make it exactly 270 to reduce noise that could trigger east/west motion as well
            if (angle > 260 && angle < 280)
                angle = 270;

            MovementNSSP.s = IPS_BUSY;
            MovementNSSP.sp[DIRECTION_NORTH].s = ISS_OFF;
            MovementNSSP.sp[DIRECTION_SOUTH].s = ISS_ON;
            IDSetSwitch(&MovementNSSP, NULL);
        }
        // East
        if (angle < 90 || angle > 270)
        {
            // Don't try to move if you're busy and moving in the same direction
            if (MovementWESP.s != IPS_BUSY  || MovementWES[1].s != ISS_ON)
                MoveWE(DIRECTION_EAST, MOTION_START);

            MovementWESP.s = IPS_BUSY;
            MovementWESP.sp[DIRECTION_WEST].s = ISS_OFF;
            MovementWESP.sp[DIRECTION_EAST].s = ISS_ON;
            IDSetSwitch(&MovementWESP, NULL);
        }

        // West
        if (angle > 90 && angle < 270)
        {

            // Don't try to move if you're busy and moving in the same direction
            if (MovementWESP.s != IPS_BUSY  || MovementWES[0].s != ISS_ON)
                MoveWE(DIRECTION_WEST, MOTION_START);

            MovementWESP.s = IPS_BUSY;
            MovementWESP.sp[DIRECTION_WEST].s = ISS_ON;
            MovementWESP.sp[DIRECTION_EAST].s = ISS_OFF;
            IDSetSwitch(&MovementWESP, NULL);
        }
    }
}

void INDI::Telescope::processSlewPresets(double mag, double angle)
{
    // high threshold, only 1 is accepted
    if (mag != 1)
        return;

    int currentIndex = IUFindOnSwitchIndex(&SlewRateSP);

    // Up
    if (angle > 0 && angle < 180)
    {
        if (currentIndex <= 0)
            return;

        IUResetSwitch(&SlewRateSP);
        SlewRateS[currentIndex-1].s = ISS_ON;
        SetSlewRate(currentIndex-1);
    }
    // Down
    else
    {
        if (currentIndex >= SlewRateSP.nsp-1)
            return;

         IUResetSwitch(&SlewRateSP);
         SlewRateS[currentIndex+1].s = ISS_ON;
         SetSlewRate(currentIndex-1);
    }

    IDSetSwitch(&SlewRateSP, NULL);
}

void INDI::Telescope::joystickHelper(const char * joystick_n, double mag, double angle, void *context)
{
    static_cast<INDI::Telescope*>(context)->processJoystick(joystick_n, mag, angle);
}

void INDI::Telescope::buttonHelper(const char * button_n, ISState state, void *context)
{
    static_cast<INDI::Telescope*>(context)->processButton(button_n, state);
}
