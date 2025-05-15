// Copyright (c) 2018-2025 CloudMakers, s. r. o.
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

/** INDIGO USB Utilities
 \file indigo_usb_utils.c
 */

#include <indigo/indigo_bus.h>
#include <indigo/indigo_usb_utils.h>

indigo_result indigo_get_usb_path(libusb_device* handle, char *path) {
	uint8_t data[10];
	data[0] = libusb_get_bus_number(handle);
	int n = libusb_get_port_numbers(handle, &data[1], 9);
	if (n != LIBUSB_ERROR_OVERFLOW) {
		for (int i = 0; i <= n; i++) {
			sprintf(path + 2 * i, "%02X", data[i]);
		}
	} else {
		path[0] = '\0';
		return INDIGO_FAILED;
	}
	return INDIGO_OK;
}

#if defined(INDIGO_USB_HOTPLUG_POLLING)

typedef struct hotplug_callback_info {
	libusb_hotplug_callback_handle handle;
	libusb_hotplug_event events;
	libusb_hotplug_flag flags;
	int vendor_id;
	int product_id;
	int dev_class;
	libusb_hotplug_callback_fn cb_fn;
	void *user_data;
	struct hotplug_callback_info *next;
} hotplug_callback_info;

static hotplug_callback_info *hotplug_callback_list = NULL;
static pthread_mutex_t hotplug_callback_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static int hotplug_device_count = 0;
static libusb_device **hotplug_device_list = NULL;

static bool device_matches(libusb_device *device, hotplug_callback_info *info, char *format, int handle) {
	struct libusb_device_descriptor desc;
	int ret = libusb_get_device_descriptor(device, &desc);
	if (ret < 0) {
		return false;
	}
	if (info->vendor_id != LIBUSB_HOTPLUG_MATCH_ANY && info->vendor_id != desc.idVendor) {
		return false;
	}
	if (info->product_id != LIBUSB_HOTPLUG_MATCH_ANY && info->product_id != desc.idProduct) {
		return false;
	}
	if (info->dev_class != LIBUSB_HOTPLUG_MATCH_ANY) {
		uint8_t dev_class;
		libusb_get_device_descriptor(device, &desc);
		dev_class = desc.bDeviceClass;
		if (info->dev_class != dev_class) {
			return false;
		}
	}
	INDIGO_DEBUG(indigo_debug(format, handle, desc.idVendor, desc.idProduct));
	return true;
}

static void update_device_list() {
	pthread_mutex_lock(&hotplug_callback_list_mutex);
	libusb_device **new_list = NULL;
	int new_count = (int)libusb_get_device_list(NULL, &new_list);
	if (new_count < 0) {
		indigo_error("Error getting device list: %s", libusb_error_name(new_count));
		pthread_mutex_unlock(&hotplug_callback_list_mutex);
		return;
	}
	bool differ = new_count !=hotplug_device_count;
	if (!differ) {
		for (int i = 0; i < new_count; i++) {
			if (new_list[i] != hotplug_device_list[i]) {
				differ = true;
				break;
			}
		}
	}
	if (differ) {
		for (int i = 0; i < new_count; i++) {
			bool found = false;
			for (int j = 0; j < hotplug_device_count; j++) {
				if (new_list[i] == hotplug_device_list[j]) {
					found = true;
					break;
				}
			}
			hotplug_callback_info *callback_info = hotplug_callback_list;
			while (callback_info != NULL) {
				if ((callback_info->events & LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) != 0) {
					if (!found && device_matches(new_list[i], callback_info, "Reporting LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED to %d for %d:%d", callback_info->handle)) {
						callback_info->cb_fn(NULL, new_list[i], LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, callback_info->user_data);
					}
				}
				callback_info = callback_info->next;
			}
		}
		for (int i = 0; i < hotplug_device_count; i++) {
			bool found = false;
			for (int j = 0; j < new_count; j++) {
				if (new_list[j] == hotplug_device_list[i]) {
					found = true;
					break;
				}
			}
			hotplug_callback_info *callback_info = hotplug_callback_list;
			while (callback_info != NULL) {
				if ((callback_info->events & LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) != 0) {
					if (!found && device_matches(hotplug_device_list[i], callback_info, "Reporting LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT to %d for %d:%d", callback_info->handle)) {
						callback_info->cb_fn(NULL, hotplug_device_list[i], LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, callback_info->user_data);
					}
				}
				callback_info = callback_info->next;
			}
		}
		if (hotplug_device_list) {
			libusb_free_device_list(hotplug_device_list, 1);
		}
		hotplug_device_list = new_list;
		hotplug_device_count = new_count;
	} else {
		libusb_free_device_list(new_list, 1);
	}
	pthread_mutex_unlock(&hotplug_callback_list_mutex);
}

void *indigo_usb_hotplug_thread(void *arg) {
	while (true) {
		if (hotplug_callback_list != NULL) {
			update_device_list();
		}
		indigo_usleep(INDIGO_DELAY(3));
	}
	return NULL;
}

int libusb_hotplug_register_callback_sim(libusb_context *ctx, libusb_hotplug_event events, libusb_hotplug_flag flags, int vendor_id, int product_id, int dev_class, libusb_hotplug_callback_fn cb_fn, void *user_data, libusb_hotplug_callback_handle *handle) {
	static libusb_hotplug_callback_handle last_handle = 0;
	hotplug_callback_info *callback_info = indigo_safe_malloc(sizeof(hotplug_callback_info));
	callback_info->handle = ++last_handle;
	callback_info->events = events;
	callback_info->flags = flags;
	callback_info->vendor_id = vendor_id;
	callback_info->product_id = product_id;
	callback_info->dev_class = dev_class;
	callback_info->cb_fn = cb_fn;
	callback_info->user_data = user_data;
	callback_info->next = hotplug_callback_list;
	*handle = last_handle;
	INDIGO_DEBUG(indigo_debug("Registered USB hotplug callback %d for %d:%d", last_handle, vendor_id, product_id));
	pthread_mutex_lock(&hotplug_callback_list_mutex);
	hotplug_callback_list = callback_info;
	if ((callback_info->flags & LIBUSB_HOTPLUG_ENUMERATE) != 0) {
		for (int i = 0; i < hotplug_device_count; i++) {
			if (device_matches(hotplug_device_list[i], callback_info, "Reporting LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED to %d for %d:%d", callback_info->handle)) {
				callback_info->cb_fn(NULL, hotplug_device_list[i], LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, callback_info->user_data);
			}
		}
	}
	pthread_mutex_unlock(&hotplug_callback_list_mutex);
	return LIBUSB_SUCCESS;
}

int libusb_hotplug_deregister_callback_poll(libusb_context *ctx, libusb_hotplug_callback_handle handle) {
	pthread_mutex_lock(&hotplug_callback_list_mutex);
	if (hotplug_callback_list != NULL) {
		hotplug_callback_info *callback_info = hotplug_callback_list;
		if (hotplug_callback_list->handle == handle) {
			hotplug_callback_list = hotplug_callback_list->next;
			indigo_safe_free(callback_info);
			INDIGO_DEBUG(indigo_debug("Deregistered USB hotplug callback %d", handle));
			pthread_mutex_unlock(&hotplug_callback_list_mutex);
			return LIBUSB_SUCCESS;
		}
		hotplug_callback_info *callback_next = callback_info->next;
		while (callback_next != NULL) {
			if (callback_next->handle == handle) {
				callback_info->next = callback_next->next;
				indigo_safe_free(callback_next);
				INDIGO_DEBUG(indigo_debug("Deregistered USB hotplug callback %d", handle));
				pthread_mutex_unlock(&hotplug_callback_list_mutex);
				return LIBUSB_SUCCESS;
			}
			callback_info = callback_next;
			callback_next = callback_next->next;
		}
	}
	indigo_error("Deregistered USB hotplug callback %d", handle);
	pthread_mutex_unlock(&hotplug_callback_list_mutex);
	return LIBUSB_ERROR_INVALID_PARAM;
}

#else

void *indigo_usb_hotplug_thread(void *arg) {
	while (true) {
		libusb_handle_events(NULL);
	}
	return NULL;
}
#endif

