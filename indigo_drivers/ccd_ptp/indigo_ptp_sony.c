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

/** INDIGO PTP Sony implementation
 \file indigo_ptp_sony.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <libusb-1.0/libusb.h>

#include "indigo_ptp.h"
#include "indigo_ptp_sony.h"

char *ptp_operation_sony_code_label(uint16_t code) {
	switch (code) {
		case ptp_operation_sony_SDIOConnect: return "sonySDIOConnect_Sony";
		case ptp_operation_sony_GetSDIOGetExtDeviceInfo: return "sonyGetSDIOGetExtDeviceInfo_Sony";
		case ptp_operation_sony_GetDevicePropDesc: return "sonyGetDevicePropDesc_Sony";
		case ptp_operation_sony_GetDevicePropertyValue: return "sonyGetDevicePropertyValue_Sony";
		case ptp_operation_sony_SetControlDeviceA: return "sonySetControlDeviceA_Sony";
		case ptp_operation_sony_GetControlDeviceDesc: return "sonyGetControlDeviceDesc_Sony";
		case ptp_operation_sony_SetControlDeviceB: return "sonySetControlDeviceB_Sony";
		case ptp_operation_sony_GetAllDevicePropData: return "sonyGetAllDevicePropData_Sony";
	}
	return ptp_operation_code_label(code);
}

char *ptp_event_sony_code_label(uint16_t code) {
	switch (code) {
		case ptp_event_sony_ObjectAdded: return "ObjectAdded_Sony";
		case ptp_event_sony_ObjectRemoved: return "ObjectRemoved_Sony";
		case ptp_event_sony_PropertyChanged: return "PropertyChanged_Sony";
	}
	return ptp_event_code_label(code);
}

char *ptp_property_sony_code_name(uint16_t code) {
	static char label[INDIGO_NAME_SIZE];
	sprintf(label, "%04x", code);
	return label;
}


char *ptp_property_sony_code_label(uint16_t code) {
	switch (code) {
		case ptp_property_sony_DPCCompensation: return "DPCCompensation_Sony";
		case ptp_property_sony_DRangeOptimize: return "DRangeOptimize_Sony";
		case ptp_property_sony_ImageSize: return "ImageSize_Sony";
		case ptp_property_sony_ShutterSpeed: return "ShutterSpeed_Sony";
		case ptp_property_sony_ColorTemp: return "ColorTemp_Sony";
		case ptp_property_sony_CCFilter: return "CCFilter_Sony";
		case ptp_property_sony_AspectRatio: return "AspectRatio_Sony";
		case ptp_property_sony_FocusStatus: return "FocusStatus_Sony";
		case ptp_property_sony_ExposeIndex: return "ExposeIndex_Sony";
		case ptp_property_sony_PictureEffect: return "PictureEffect_Sony";
		case ptp_property_sony_ABFilter: return "ABFilter_Sony";
		case ptp_property_sony_ISO: return "ISO_Sony";
		case ptp_property_sony_Autofocus: return "Autofocus_Sony";
		case ptp_property_sony_Capture: return "Capture_Sony";
		case ptp_property_sony_Movie: return "Movie_Sony";
		case ptp_property_sony_StillImage: return "StillImage_Sony";
	}
	return ptp_property_code_label(code);
}

char *ptp_property_sony_value_code_label(indigo_device *device, uint16_t property, uint64_t code) {
	return ptp_property_value_code_label(device, property, code);
}

bool ptp_sony_initialise(indigo_device *device) {
	assert(0);
}

bool ptp_sony_handle_event(indigo_device *device, ptp_event_code code, uint32_t *params) {
	return ptp_handle_event(device, code, params);
}

bool ptp_sony_set_property(indigo_device *device, ptp_property *property) {
	assert(0);
}

bool ptp_sony_exposure(indigo_device *device) {
	assert(0);
}

bool ptp_sony_liveview(indigo_device *device) {
	assert(0);
}

bool ptp_sony_af(indigo_device *device) {
	assert(0);
}

bool ptp_sony_check_compression_has_raw(indigo_device *device) {
	assert(0);
}
