// Copyright (c) 2019 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO PTP implementation
 \file indigo_ptp.h
 */

#ifndef indigo_ptp_h
#define indigo_ptp_h

#include <indigo/indigo_driver.h>

#define PRIVATE_DATA    ((ptp_private_data *)device->private_data)
#define DRIVER_VERSION 	0x0001
#define DRIVER_NAME 		"indigo_ccd_ptp"

#define PTP_TIMEOUT							3000

#define PTP_DUMP_CONTAINER(c) INDIGO_DEBUG(ptp_dump_container(__LINE__, __FUNCTION__, c))

typedef enum {
	ptp_container_command =	0x0001,
	ptp_container_data = 0x0002,
	ptp_container_response = 0x0003,
	ptp_container_event = 0x0004
} ptp_contaner_code;

typedef enum {
	ptp_request_undefined = 0x1000,
	ptp_request_getdeviceinfo = 0x1001,
	ptp_request_opensession = 0x1002,
	ptp_request_closesession = 0x1003,
	ptp_request_getstorageids = 0x1004,
	ptp_request_getstorageinfo = 0x1005,
	ptp_request_getnumobjects = 0x1006,
	ptp_request_getobjecthandles = 0x1007,
	ptp_request_getobjectinfo = 0x1008,
	ptp_request_getobject = 0x1009,
	ptp_request_getthumb = 0x100a,
	ptp_request_deleteobject = 0x100b,
	ptp_request_sendobjectinfo = 0x100c,
	ptp_request_sendobject = 0x100d,
	ptp_request_initiatecapture = 0x100e,
	ptp_request_formatstore = 0x100f,
	ptp_request_resetdevice = 0x1010,
	ptp_request_selftest = 0x1011,
	ptp_request_setobjectprotection = 0x1012,
	ptp_request_powerdown = 0x1013,
	ptp_request_getdevicepropdesc = 0x1014,
	ptp_request_getdevicepropvalue = 0x1015,
	ptp_request_setdevicepropvalue = 0x1016,
	ptp_request_resetdevicepropvalue = 0x1017,
	ptp_request_terminateopencapture = 0x1018,
	ptp_request_moveobject = 0x1019,
	ptp_request_copyobject = 0x101a,
	ptp_request_getpartialobject = 0x101b,
	ptp_request_initiateopencapture = 0x101c,
	ptp_request_getnumdownloadableobjects = 0x9001,
	ptp_request_getallobjectinfo = 0x9002,
	ptp_request_getuserassigneddevicename = 0x9003,
	ptp_request_mtpgetobjectpropssupported = 0x9801,
	ptp_request_mtpgetobjectpropdesc = 0x9802,
	ptp_request_mtpgetobjectpropvalue = 0x9803,
	ptp_request_mtpsetobjectpropvalue = 0x9804,
	ptp_request_mtpgetobjproplist = 0x9805,
	ptp_request_mtpsetobjproplist = 0x9806,
	ptp_request_mtpgetinterdependendpropdesc = 0x9807,
	ptp_request_mtpsendobjectproplist = 0x9808,
	ptp_request_mtpgetobjectreferences = 0x9810,
	ptp_request_mtpsetobjectreferences = 0x9811,
	ptp_request_mtpupdatedevicefirmware = 0x9812,
	ptp_request_mtpskip = 0x9820,
} ptp_request_code;

typedef enum {
	ptp_response_undefined = 0x2000,
	ptp_response_ok = 0x2001,
	ptp_response_generalerror = 0x2002,
	ptp_response_sessionnotopen = 0x2003,
	ptp_response_invalidtransactionid = 0x2004,
	ptp_response_operationnotsupported = 0x2005,
	ptp_response_parameternotsupported = 0x2006,
	ptp_response_incompletetransfer = 0x2007,
	ptp_response_invalidstorageid = 0x2008,
	ptp_response_invalidobjecthandle = 0x2009,
	ptp_response_devicepropnotsupported = 0x200a,
	ptp_response_invalidobjectformatcode = 0x200b,
	ptp_response_storefull = 0x200c,
	ptp_response_objectwriteprotected = 0x200d,
	ptp_response_storereadonly = 0x200e,
	ptp_response_accessdenied = 0x200f,
	ptp_response_nothumbnailpresent = 0x2010,
	ptp_response_selftestfailed = 0x2011,
	ptp_response_partialdeletion = 0x2012,
	ptp_response_storenotavailable = 0x2013,
	ptp_response_specificationbyformatunsupported = 0x2014,
	ptp_response_novalidobjectinfo = 0x2015,
	ptp_response_invalidcodeformat = 0x2016,
	ptp_response_unknownvendorcode = 0x2017,
	ptp_response_capturealreadyterminated = 0x2018,
	ptp_response_devicebusy = 0x2019,
	ptp_response_invalidparentobject = 0x201a,
	ptp_response_invaliddevicepropformat = 0x201b,
	ptp_response_invaliddevicepropvalue = 0x201c,
	ptp_response_invalidparameter = 0x201d,
	ptp_response_sessionalreadyopen = 0x201e,
	ptp_response_transactioncancelled = 0x201f,
	ptp_response_specificationofdestinationunsupported = 0x2020,
	ptp_response_mtpundefined = 0xa800,
	ptp_response_mtpinvalidobjectpropcode = 0xa801,
	ptp_response_mtpinvalidobjectprop_format = 0xa802,
	ptp_response_mtpinvalidobjectprop_value = 0xa803,
	ptp_response_mtpinvalidobjectreference = 0xa804,
	ptp_response_mtpinvaliddataset = 0xa806,
	ptp_response_mtpspecificationbygroupunsupported = 0xa807,
	ptp_response_mtpspecificationbydepthunsupported = 0xa808,
	ptp_response_mtpobjecttoolarge = 0xa809,
	ptp_response_mtpobjectpropnotsupported = 0xa80a,
} ptp_response_code;

typedef struct {
	uint32_t length;
	uint16_t type;
	uint16_t code;
	uint32_t transaction_id;
	uint32_t params[5];
} ptp_container;

typedef struct {
	int vendor;
	int product;
	const char *name;
	indigo_device_interface iface;
} ptp_camera_model;

typedef struct {
	libusb_device *dev;
	libusb_device_handle *handle;
	int ep_in, ep_out, ep_int;
	int device_count;
	pthread_mutex_t mutex;
	uint32_t session_id;
	uint32_t transaction_id;
} ptp_private_data;

void ptp_dump_container(int line, const char *function, ptp_container *container);
bool ptp_open(indigo_device *device);
bool ptp_request(indigo_device *device, uint16_t code, int count, ...);
bool ptp_read(indigo_device *device, void *data, int length, int *actual);
bool ptp_response(indigo_device *device, uint16_t *code, int count, ...);
void ptp_close(indigo_device *device);

#endif /* indigo_ptp_h */
