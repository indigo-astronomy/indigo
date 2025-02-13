// Copyright (c) 2024 Rumen G. Bogdanovski
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
// 2.0 by Rumen Bogdanovski <rumenastro@gmail.com>

/** INDIGO USB-Serial Utilities
 \file indigo_usbserial_utils.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(INDIGO_MACOS)
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <CoreFoundation/CoreFoundation.h>
#elif defined(INDIGO_LINUX)
#include <libudev.h>
#elif defined(INDIGO_WINDOWS)
#include <windows.h>
#include <setupapi.h>
#endif

#include <indigo/indigo_usbserial_utils.h>

int indigo_enumerate_usbserial_devices(indigo_serial_info* serial_info, int num_serial_info) {
#if defined(INDIGO_MACOS)
	CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOUSBDeviceClassName);
	if (matchingDict == NULL) {
		return -1;
	}
	io_iterator_t iter;
#if defined(kIOMainPortDefault)
	kern_return_t kr = IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &iter);
#else
	kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &iter);
#endif
	if (kr != KERN_SUCCESS) {
		return -1;
	}
	int count = 0;
	io_service_t device;
	while ((device = IOIteratorNext(iter)) != 0 && count < num_serial_info) {
		CFNumberRef vendorIdAsCFNumber = IORegistryEntryCreateCFProperty(device, CFSTR(kUSBVendorID), kCFAllocatorDefault, 0);
		CFNumberRef productIdAsCFNumber = IORegistryEntryCreateCFProperty(device, CFSTR(kUSBProductID), kCFAllocatorDefault, 0);
		CFNumberRef deviceClassAsCFNumber = IORegistryEntryCreateCFProperty(device, CFSTR(kUSBDeviceClass), kCFAllocatorDefault, 0);
		CFStringRef vendorString = IORegistryEntryCreateCFProperty(device, CFSTR(kUSBVendorString), kCFAllocatorDefault, 0);
		CFStringRef productString = IORegistryEntryCreateCFProperty(device, CFSTR(kUSBProductString), kCFAllocatorDefault, 0);
		CFStringRef serialNumberString = IORegistryEntryCreateCFProperty(device, CFSTR(kUSBSerialNumberString), kCFAllocatorDefault, 0);
		if (vendorIdAsCFNumber && productIdAsCFNumber && deviceClassAsCFNumber) {
			int vendor_id, product_id, device_class;
			CFNumberGetValue(vendorIdAsCFNumber, kCFNumberIntType, &vendor_id);
			CFNumberGetValue(productIdAsCFNumber, kCFNumberIntType, &product_id);
			CFNumberGetValue(deviceClassAsCFNumber, kCFNumberIntType, &device_class);
			// Skip USB hubs
			if (device_class == 0x09) {
				goto cleanup;
			}
			char vendor_str[256] = { 0 };
			char product_str[256] = { 0 };
			char serial_str[256] = { 0 };
			if (vendorString) {
				CFStringGetCString(vendorString, vendor_str, sizeof(vendor_str), kCFStringEncodingUTF8);
			}
			if (productString) {
				CFStringGetCString(productString, product_str, sizeof(product_str), kCFStringEncodingUTF8);
			}
			if (serialNumberString) {
				CFStringGetCString(serialNumberString, serial_str, sizeof(serial_str), kCFStringEncodingUTF8);
			}
			io_iterator_t serialIter;
			IOUSBFindInterfaceRequest request;
			request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
			request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
			request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
			request.bAlternateSetting = kIOUSBFindInterfaceDontCare;
			kr = IORegistryEntryCreateIterator(device, kIOServicePlane, kIORegistryIterateRecursively, &serialIter);
			if (kr != KERN_SUCCESS) {
				goto cleanup;
			}
			io_service_t serialService;
			while ((serialService = IOIteratorNext(serialIter)) != 0) {
				CFTypeRef bsdPathAsCFString = IORegistryEntryCreateCFProperty(serialService, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
				if (bsdPathAsCFString) {
					char path[256];
					if (CFStringGetCString(bsdPathAsCFString, path, sizeof(path), kCFStringEncodingUTF8)) {
						serial_info[count].vendor_id = vendor_id;
						serial_info[count].product_id = product_id;
						indigo_safe_strncpy(serial_info[count].vendor_string, vendor_str, INFO_ITEM_SIZE);
						indigo_safe_strncpy(serial_info[count].product_string, product_str, INFO_ITEM_SIZE);
						indigo_safe_strncpy(serial_info[count].serial_string, serial_str, INFO_ITEM_SIZE);
						indigo_safe_strncpy(serial_info[count].path, path, PATH_MAX);
						count++;
					}
					CFRelease(bsdPathAsCFString);
				}
				IOObjectRelease(serialService);
			}
			IOObjectRelease(serialIter);
		}
	cleanup:
		if (vendorIdAsCFNumber) {
			CFRelease(vendorIdAsCFNumber);
		}
		if (productIdAsCFNumber) {
			CFRelease(productIdAsCFNumber);
		}
		if (deviceClassAsCFNumber) {
			CFRelease(deviceClassAsCFNumber);
		}
		if (vendorString) {
			CFRelease(vendorString);
		}
		if (productString) {
			CFRelease(productString);
		}
		if (serialNumberString) {
			CFRelease(serialNumberString);
		}
		IOObjectRelease(device);
	}
	IOObjectRelease(iter);
	return count;
#elif defined(INDIGO_LINUX)
	struct udev* udev;
	struct udev_enumerate* enumerate;
	struct udev_list_entry* devices, * dev_list_entry;
	struct udev_device* dev;
	udev = udev_new();
	if (!udev) {
		return -1;
	}
	// Create a list of the devices in the 'tty' subsystem
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "tty");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	// Iterate through the list of devices
	int count = 0;
	udev_list_entry_foreach(dev_list_entry, devices) {
		if (count >= num_serial_info) {
			break;
		}
		const char* path;
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);
		// Get the parent device with the 'usb' subsystem
		struct udev_device* parent_dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
		if (parent_dev) {
			const char* dev_node = udev_device_get_devnode(dev);
			const char* vendor = udev_device_get_sysattr_value(parent_dev, "idVendor");
			const char* product = udev_device_get_sysattr_value(parent_dev, "idProduct");
			const char* serial = udev_device_get_sysattr_value(parent_dev, "serial");
			const char* product_name = udev_device_get_sysattr_value(parent_dev, "product");
			const char* manufacturer = udev_device_get_sysattr_value(parent_dev, "manufacturer");
			if (dev_node && vendor && product) {
				serial_info[count].vendor_id = strtol(vendor, NULL, 16);
				serial_info[count].product_id = strtol(product, NULL, 16);
				indigo_safe_strncpy(serial_info[count].vendor_string, manufacturer ? manufacturer : "", INFO_ITEM_SIZE);
				indigo_safe_strncpy(serial_info[count].product_string, product_name ? product_name : "", INFO_ITEM_SIZE);
				indigo_safe_strncpy(serial_info[count].serial_string, serial ? serial : "", INFO_ITEM_SIZE);
				indigo_safe_strncpy(serial_info[count].path, dev_node, PATH_MAX);
				count++;
			}
		}
		udev_device_unref(dev);
	}
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
	return count;
#elif defined(INDIGO_WINDOWS)
GUID guid = { 0x4d36e978, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18} };
HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
if (deviceInfoSet == INVALID_HANDLE_VALUE) {
	indigo_error("Failed to get device information set");
	return 0;
}
SP_DEVINFO_DATA deviceInfoData;
deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
DWORD index = 0;
for (index = 0; SetupDiEnumDeviceInfo(deviceInfoSet, index, &deviceInfoData); index++) {
	char portName[256];
	char friendlyName[256];
	BOOL success = SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, NULL, (PBYTE)portName, sizeof(portName), NULL);
	if (!success) {
		indigo_error("Failed to get port name for device %d", index);
		continue;
	}
	HKEY hKey;
	hKey = SetupDiOpenDevRegKey(deviceInfoSet, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
	if (hKey == INVALID_HANDLE_VALUE) {
		indigo_error("Failed to open registry key for device");
		continue;
	}
	DWORD portNameSize = sizeof(portName);
	DWORD type;
	if (RegQueryValueExA(hKey, "PortName", NULL, &type, (LPBYTE)portName, &portNameSize) == ERROR_SUCCESS) {
		if (type == REG_SZ) {
			DWORD type;
			DWORD portNameSize = sizeof(portName);
			snprintf(serial_info[index].path, PATH_MAX, "%s:", portName);
		}
	}
	RegCloseKey(hKey);
	success = SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)friendlyName, sizeof(friendlyName), NULL);
	if (!success) {
		indigo_error("Failed to get friendly name for device %d", index);
		continue;
	}
	snprintf(serial_info[index].product_string, PATH_MAX, "%s", friendlyName);
}
SetupDiDestroyDeviceInfoList(deviceInfoSet);
return index;
#else
	return 0;
#endif
}

void indigo_usbserial_label(indigo_serial_info* serial_info, char* label) {
	if (strncmp(serial_info->product_string, "", PATH_MAX)) {
#if defined(INDIGO_WINDOWS)
		snprintf(label, INDIGO_VALUE_SIZE, "%s", serial_info->product_string);
#else
		snprintf(label, INDIGO_VALUE_SIZE, "%s (%s)", serial_info->path, serial_info->product_string);
#endif
		return;
	}
	snprintf(label, INDIGO_VALUE_SIZE, "%s", serial_info->path);
}

indigo_serial_info *indigo_usbserial_match(
	indigo_serial_info *serial_info,
	const int num_serial_info,
	indigo_device_match_pattern *patterns,
	const int num_patterns
) {
	if (num_serial_info == 0 || num_patterns == 0 || serial_info == NULL || patterns == NULL) {
		return NULL;
	}

	for (int i = 0; i < num_patterns; i++) {
		indigo_device_match_pattern *pattern = &patterns[i];
		for (int j = 0; j < num_serial_info; j++) {
			indigo_serial_info *info = &serial_info[j];
			bool match = true;

			if (pattern->vendor_id != 0 && pattern->vendor_id != info->vendor_id) {
				match = false;
			}
			if (pattern->product_id != 0 && pattern->product_id != info->product_id) {
				match = false;
			}
			if (pattern->vendor_string[0] != 0) {
				if (pattern->exact_match) {
					if (strcmp(pattern->vendor_string, info->vendor_string) != 0) {
						match = false;
					}
				} else {
					if (strstr(info->vendor_string, pattern->vendor_string) == NULL) {
						match = false;
					}
				}
			}
			if (pattern->product_string[0] != 0) {
				if (pattern->exact_match) {
					if (strcmp(pattern->product_string, info->product_string) != 0) {
						match = false;
					}
				} else {
					if (strstr(info->product_string, pattern->product_string) == NULL) {
						match = false;
					}
				}
			}
			if (pattern->serial_string[0] != 0) {
				if (pattern->exact_match) {
					if (strcmp(pattern->serial_string, info->serial_string) != 0) {
						match = false;
					}
				} else {
					if (strstr(info->serial_string, pattern->serial_string) == NULL) {
						match = false;
					}
				}
			}

			if (match) {
				return info;
			}
		}
	}
	return NULL;
}
