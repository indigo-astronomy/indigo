// Copyright (c) 2018 Rumen G. Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Rumen Bogdanovski <rumen@skyarchive.org>


/** INDIGO CCD driver for Apogee
 \file indigo_ccd_apogee.cpp
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME		"indigo_ccd_apogee"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include <sstream>
//#include <iostream>
//#include <fstream>
#include <stdexcept>

#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <libapogee/ApogeeCam.h>
#include <libapogee/FindDeviceEthernet.h>
#include <libapogee/FindDeviceUsb.h>
#include <libapogee/Alta.h>
#include <libapogee/AltaF.h>
#include <libapogee/Ascent.h>
#include <libapogee/Aspen.h>
#include <libapogee/Quad.h>
#include <libapogee/ApgLogger.h>
#include <libapogee/CamHelpers.h>
#include <libapogee/versionNo.h>

#include "indigo_driver_xml.h"
#include "indigo_ccd_apogee.h"

#define MAXCAMERAS				32

#define PRIVATE_DATA              ((apogee_private_data *)device->private_data)

#undef INDIGO_DEBUG_DRIVER
#define INDIGO_DEBUG_DRIVER(c) c

typedef struct {
	ApogeeCam *camera;
	std::string discovery_string;
	bool available;
	indigo_timer *exposure_timer, *temperature_timer;
	long int buffer_size;
	unsigned short *buffer;
	char serial[255];
	bool can_check_temperature;
} apogee_private_data;

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		PRIVATE_DATA->can_check_temperature = true;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' attached", device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' detached.", device->name);
	return indigo_ccd_detach(device);
}


std::vector<std::string> GetDeviceVector( const std::string & msg ) {
	std::vector<std::string> devices;
	const std::string startDelim("<d>");
	const std::string stopDelim("</d>");

	size_t pos = 0;
	bool find = true;
	while( find ) {
		size_t posStart = msg.find( startDelim, pos );
		if( std::string::npos == posStart ) {
			break;
		}
		size_t posStop = msg.find( stopDelim, posStart+1 );
		if( std::string::npos == posStop ) {
			break;
		}
		size_t strLen = (posStop - posStart) - startDelim.size();
		std::string sub = msg.substr( posStart+startDelim.size(), strLen );
		devices.push_back( sub );
		pos = 1+posStop;
	}
	return devices;
}

std::vector<std::string> MakeTokens(const std::string &str, const std::string &separator)
{
	std::vector<std::string> returnVector;
	std::string::size_type start = 0;
	std::string::size_type end = 0;

	while( (end = str.find(separator, start)) != std::string::npos)
	{
		returnVector.push_back (str.substr (start, end-start));
		start = end + separator.size();
	}

	returnVector.push_back( str.substr(start) );

	return returnVector;
}

///////////////////////////
//	GET    ITEM    FROM     FIND       STR
std::string GetItemFromFindStr( const std::string & msg,
				const std::string & item )
{

	//search the single device input string for the requested item
    std::vector<std::string> params = MakeTokens( msg, "," );
	std::vector<std::string>::iterator iter;

	for(iter = params.begin(); iter != params.end(); ++iter)
	{
	   if( std::string::npos != (*iter).find( item ) )
	   {
		 std::string result = MakeTokens( (*iter), "=" ).at(1);

		 return result;
	   }
	} //for

	std::string noOp;
	return noOp;
}

////////////////////////////
//	GET		USB  ADDRESS
std::string GetUsbAddress( const std::string & msg )
{
    return GetItemFromFindStr( msg, "address=" );
}


////////////////////////////
//	GET		ETHERNET  ADDRESS
std::string GetEthernetAddress( const std::string & msg )
{
    std::string addr = GetItemFromFindStr( msg, "address=" );
    addr.append(":");
    addr.append( GetItemFromFindStr( msg, "port=" ) );
    return addr;
}
////////////////////////////
//	GET		ID
uint16_t GetID( const std::string & msg )
{
    std::string str = GetItemFromFindStr( msg, "id=" );
    uint16_t id = 0;
    std::stringstream ss;
    ss << std::hex << std::showbase << str.c_str();
    ss >> id;

    return id;
}

////////////////////////////
//	GET		FRMWR       REV
uint16_t GetFrmwrRev( const std::string & msg )
{
    std::string str = GetItemFromFindStr(  msg, "firmwareRev=" );

    uint16_t rev = 0;
    std::stringstream ss;
    ss << std::hex << std::showbase << str.c_str();
    ss >> rev;

    return rev;
}

CamModel::PlatformType GetModel(const std::string &msg)
{
    return CamModel::GetPlatformType(GetItemFromFindStr(msg, "model="));
}

////////////////////////////
//	        IS      DEVICE      FILTER      WHEEL
bool IsDeviceFilterWheel( const std::string & msg )
{
    std::string str = GetItemFromFindStr(  msg, "deviceType=" );

    return ( 0 == str.compare("filterWheel" ) ? true : false );
}

////////////////////////////
//	        IS  	ASCENT
bool IsAscent( const std::string & msg )
{
	std::string model = GetItemFromFindStr(  msg, "model=" );
	std::string ascent("Ascent");
    return( 0 == model .compare( 0, ascent.size(), ascent ) ? true : false );
}

////////////////////////////
//	        IS  	ASPEN
bool IsAspen( const std::string & msg )
{
	std::string model = GetItemFromFindStr(  msg, "model=" );
	std::string aspen("Aspen");
    return( 0 == model .compare( 0, aspen.size(), aspen ) ? true : false );
}

////////////////////////////
//		CHECK	STATUS
void checkStatus( const Apg::Status status )
{
	switch( status )
	{
		case Apg::Status_ConnectionError:
		{
			std::string errMsg("Status_ConnectionError");
			std::runtime_error except( errMsg );
			throw except;
		}
		break;

		case Apg::Status_DataError:
		{
			std::string errMsg("Status_DataError");
			std::runtime_error except( errMsg );
			throw except;
		}
		break;

		case Apg::Status_PatternError:
		{
			std::string errMsg("Status_PatternError");
			std::runtime_error except( errMsg );
			throw except;
		}
		break;

		case Apg::Status_Idle:
		{
			std::string errMsg("Status_Idle");
			std::runtime_error except( errMsg );
			throw except;
		}
		break;

		default:
			//no op on purpose
		break;
	}
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;


std::vector<std::string> device_strings;
static indigo_device *devices[MAXCAMERAS] = {NULL};

static void hotplug(void *param) {
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);
	std::string msg;
	std::string discovery_string;

	sleep(3);
	try {
		FindDeviceUsb lookUsb;
		msg = lookUsb.Find();
		//msg  = std::string("<d>address=0,interface=usb,deviceType=camera,id=0x49,firmwareRev=0x21,model=AltaU-"
		//                   "4020ML,interfaceStatus=NA</d><d>address=1,interface=usb,model=Filter "
		//                   "Wheel,deviceType=filterWheel,id=0xFFFF,firmwareRev=0xFFEE</d>");
		device_strings = GetDeviceVector( msg );
	} catch (std::runtime_error err) {
		std::string text = err.what();
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Hot plug failed  %s", text.c_str());
		return;
	}

	std::vector<std::string>::iterator iter;
	int i = 0;
	for(iter = device_strings.begin(); iter != device_strings.end(); ++iter, ++i) {
		discovery_string = (*iter);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "camera[%d]: desc = %s serial = %s", i, discovery_string.c_str());
		bool found = false;
		for (int j = 0; j < MAXCAMERAS; j++) {
			indigo_device *device = devices[j];
			if (device) {
				if (discovery_string.compare(PRIVATE_DATA->discovery_string) == 0) {
					found = true;
					break;
				}
			}
		}
		if (found) continue;

		apogee_private_data *private_data = (apogee_private_data *)malloc(sizeof(apogee_private_data));
		assert(private_data != NULL);
		memset(private_data, 0, sizeof(apogee_private_data));
		indigo_device *device = (indigo_device *)malloc(sizeof(indigo_device));
		assert(device != NULL);
		memcpy(device, &ccd_template, sizeof(indigo_device));
		PRIVATE_DATA->discovery_string = discovery_string;
		std::string model = GetItemFromFindStr(discovery_string, "model=");
		strncpy(device->name, model.c_str(), INDIGO_NAME_SIZE);
		device->private_data = private_data;
		for (int j = 0; j < MAXCAMERAS; j++) {
			if (devices[j] == NULL) {
				indigo_async((void *(*)(void *))indigo_attach_device, devices[j] = device);
				break;
			}
		}
	}
}

static void hotunplug(void *param) {
	std::string discovery_string;
	std::string msg;
	sleep(3);
	try {
		FindDeviceUsb lookUsb;
		msg = lookUsb.Find();
		device_strings = GetDeviceVector( msg );
	} catch (std::runtime_error err) {
		std::string text = err.what();
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Hot unplug failed  %s", text.c_str());
		return;
	}

	for (int j = 0; j < MAXCAMERAS; j++) {
		indigo_device *device = devices[j];
		if (device) {
			PRIVATE_DATA->available = false;
		}
	}

	std::vector<std::string>::iterator iter;
	int i = 0;
	for(iter = device_strings.begin(); iter != device_strings.end(); ++iter, ++i) {
		discovery_string = (*iter);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "camera[%d]: serial = %s", i, discovery_string.c_str());
		for (int j = 0; j < MAXCAMERAS; j++) {
			indigo_device *device = devices[j];
			if (!device || (discovery_string.compare(PRIVATE_DATA->discovery_string) == 0)) continue;
			PRIVATE_DATA->available = true;
			break;
		}
	}
	for (int j = 0; j < MAXCAMERAS; j++) {
		indigo_device *device = devices[j];
		if (device && !PRIVATE_DATA->available) {
			indigo_detach_device(device);
			free(device->private_data);
			free(device);
			devices[j] = NULL;
		}
	}
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	pthread_mutex_lock(&device_mutex);
	libusb_get_device_descriptor(dev, &descriptor);
	if (descriptor.idVendor == UsbFrmwr::APOGEE_VID) {
		switch (event) {
			case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Hot-plug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
				indigo_async((void *(*)(void *))hotplug, NULL);
				break;
			}
			case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Hot-unplug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
				indigo_async((void *(*)(void *))hotunplug, NULL);
				break;
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return 0;
};

static void remove_all_devices() {
	for (int i = 0; i < MAXCAMERAS; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL)
			continue;
		indigo_detach_device(*device);
		if (((apogee_private_data *)(*device)->private_data)->buffer)
			free(((apogee_private_data *)(*device)->private_data)->buffer);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
}

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_apogee(indigo_driver_action action, indigo_driver_info *info) {
		static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

		SET_DRIVER_INFO(info, "Apogee Camera", __FUNCTION__, DRIVER_VERSION, last_action);

		if (action == last_action)
			return INDIGO_OK;

		switch (action) {
			case INDIGO_DRIVER_INIT: {
				for (int i = 0; i < MAXCAMERAS; i++) {
					devices[i] = NULL;
				}
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libapogee version: %d.%d.%d", APOGEE_MAJOR_VERSION, APOGEE_MINOR_VERSION, APOGEE_PATCH_VERSION);
				last_action = action;
				indigo_start_usb_event_handler();
				int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, UsbFrmwr::APOGEE_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
				return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
			}
			case INDIGO_DRIVER_SHUTDOWN: {
				last_action = action;
				libusb_hotplug_deregister_callback(NULL, callback_handle);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
				remove_all_devices();
				break;
			}
			case INDIGO_DRIVER_INFO: {
				break;
			}
		}

		return INDIGO_OK;
	}
