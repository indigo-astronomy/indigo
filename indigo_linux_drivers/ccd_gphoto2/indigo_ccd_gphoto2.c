// Copyright (c) 2018 Thomas Stibor
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
// 2.0 by Thomas Stibor <thomas@stibor.net>

/** INDIGO GPHOTO2 CCD driver
 \file indigo_ccd_gphoto2.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_ccd_gphoto2"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-list.h>
#include <gphoto2/gphoto2-version.h>
#include <gphoto2/gphoto2-list.h>
#include <libusb-1.0/libusb.h>
#include "indigo_ccd_gphoto2.h"
#include "dslr_model_info.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define INDIGO_NAME_DSLR			"GPhoto2"
#define INDIGO_NAME_SHUTTER			"Shutter time"
#define INDIGO_NAME_ISO				"ISO"
#define INDIGO_NAME_COMPRESSION			"Compression"
#define INDIGO_NAME_MIRROR_LOCKUP		"Use mirror lockup"
#define INDIGO_NAME_MIRROR_LOCKUP_LOCK		"Enable"
#define INDIGO_NAME_MIRROR_LOCKUP_UNLOCK	"Disable"
#define INDIGO_NAME_LIBGHOTO2			"Gphoto2 library"
#define INDIGO_NAME_LIBGHOTO2_VERSION		"Version"

#define DSLR_LIBGHOTO2_VERSION_PROPERTY_NAME    "DSLR_LIBGHOTO2_VERSION"
#define DSLR_LIBGHOTO2_VERSION_ITEM_NAME        "LIBGHOTO2_VERSION"

#define NIKON_BULB				"Bulb"
#define NIKON_IMGFORMAT				"imagequality"
#define NIKON_COMPRESSION			NIKON_IMGFORMAT
#define NIKON_SHUTTERSPEED			"shutterspeed2"

#define EOS_BULB				"bulb"
#define EOS_ISO					"iso"
#define EOS_IMGFORMAT				"imageformat"
#define EOS_COMPRESSION				EOS_IMGFORMAT
#define EOS_SHUTTERSPEED			"shutterspeed"
#define EOS_CUSTOMFUNCEX			"customfuncex"
#define EOS_MIRROR_LOCKUP_ENABLE		"20,1,3,14,1,60f,1,1"
#define EOS_MIRROR_LOCKUP_DISABLE		"20,1,3,14,1,60f,1,0"

#define UNUSED(x)			(void)(x)
#define MAX_DEVICES			8
#define PRIVATE_DATA			((gphoto2_private_data *)device->private_data)
#define DSLR_ISO_PROPERTY		(PRIVATE_DATA->dslr_iso_property)
#define DSLR_SHUTTER_PROPERTY		(PRIVATE_DATA->dslr_shutter_property)
#define DSLR_COMPRESSION_PROPERTY	(PRIVATE_DATA->dslr_compression_property)
#define DSLR_MIRROR_LOCKUP_PROPERTY     (PRIVATE_DATA->dslr_mirror_lockup_property)
#define DSLR_MIRROR_LOCKUP_LOCK_ITEM    (PRIVATE_DATA->dslr_mirror_lockup_property->items + 0)
#define DSLR_MIRROR_LOCKUP_UNLOCK_ITEM  (PRIVATE_DATA->dslr_mirror_lockup_property->items + 1)
#define DSLR_LIBGHOTO2_VERSION_PROPERTY	(PRIVATE_DATA->dslr_libgphoto2_version_property)
#define DSLR_LIBGHOTO2_VERSION_ITEM	(PRIVATE_DATA->dslr_libgphoto2_version_property->items)
#define is_connected			gp_bits

typedef struct {
	Camera *camera;
	GPContext *context;
	char *name;
	char *value;
	char *libgphoto2_version;
	indigo_property *dslr_shutter_property;
	indigo_property *dslr_iso_property;
	indigo_property *dslr_compression_property;
	indigo_property *dslr_mirror_lockup_property;
	indigo_property *dslr_libgphoto2_version_property;
} gphoto2_private_data;

static indigo_device *devices[MAX_DEVICES] = {NULL};
static libusb_hotplug_callback_handle callback_handle;

/*
  Note: Nikon D50 has gphoto2 widget: "shutterspeed" and "shutterspeed2"
  and list these widgets as:

  gphoto2 --get-config=shutterspeed
  Label: Shutter Speed
  Type: RADIO
  Current: 0.0002s
  Choice: 0 0.0002s
  Choice: 1 0.0003s
  ...
  ...

  gphoto2 --get-config=shutterspeed2
  Label: Shutter Speed 2
  Type: RADIO
  Current: 1/4000
  Choice: 0 1/4000
  Choice: 1 1/3200
  ...
  ...

  However 0.0002s != 1/4000s = 0.00025s
          0.0003s != 1/3200s = 0.0003125s

  It is recommended to use shutterspeed2 widget.
 */
static double parse_shutterspeed(char *str)
{
	const char *delim = "/'";
	uint8_t cnt = 0;
	char *token;
	double nom_denom[2] = {0, 0};

	token = strtok(str, delim);
	while (token != NULL) {
		nom_denom[cnt++ % 2] = atof(token);
		token = strtok(NULL, delim);
	}

	return (nom_denom[1] == 0 ? nom_denom[0] :
		nom_denom[0] / nom_denom[1]);
}

static int lookup_widget(CameraWidget *widget, const char *key,
			 CameraWidget **child)
{
	int rc;

	rc = gp_widget_get_child_by_name(widget, key, child);
	if (rc < GP_OK)
		rc = gp_widget_get_child_by_label(widget, key, child);

	return rc;
}

int widget_enumerate(const char *key, indigo_device *device,
		     indigo_property *property)
{
	CameraWidget *widget = NULL, *child = NULL;
	CameraWidgetType type;
	int rc;

	rc = gp_camera_get_config(PRIVATE_DATA->camera, &widget,
				  PRIVATE_DATA->context);
	if (rc < GP_OK) {
		goto cleanup;
	}

	rc = lookup_widget(widget, key, &child);
	if (rc < GP_OK) {
		goto cleanup;
	}

	/* This type check is optional, if you know what type the label
	 * has already. If you are not sure, better check. */
	rc = gp_widget_get_type(child, &type);
	if (rc < GP_OK) {
		goto cleanup;
	}
	if (type != GP_WIDGET_RADIO) {
		rc = GP_ERROR_BAD_PARAMETERS;
		goto cleanup;
	}

	rc = gp_widget_count_choices(child);
	if (rc < GP_OK) {
		goto cleanup;
	}

	/* If property is NULL we are interested in number of choices only
	   which is stored in rc. */
	if (!property)
		goto cleanup;

	const int n_choices = rc;
	const char *widget_choice;

	for (int i = 0; i < n_choices; i++) {
		rc = gp_widget_get_choice(child, i, &widget_choice);
		if (rc < GP_OK) {
			goto cleanup;
		}
		indigo_init_switch_item(property->items + i,
					widget_choice,
					widget_choice, i == 0);
	}

cleanup:
	gp_widget_free(widget);

	return rc;
}

int set_key_val_dslr(const char *key, const char *val, indigo_device *device)
{
	CameraWidget *widget = NULL, *child = NULL;
	CameraWidgetType type;
	int rc;

	rc = gp_camera_get_config(PRIVATE_DATA->camera, &widget,
				  PRIVATE_DATA->context);
	if (rc < GP_OK) {
		return rc;
	}
	rc = lookup_widget(widget, key, &child);
	if (rc < GP_OK) {
		goto cleanup;
	}

	/* This type check is optional, if you know what type the label
	 * has already. If you are not sure, better check. */
	rc = gp_widget_get_type(child, &type);
	if (rc < GP_OK) {
		goto cleanup;
	}
	switch (type) {
	case GP_WIDGET_MENU:
	case GP_WIDGET_RADIO:
	case GP_WIDGET_TEXT:
		break;
	default:
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[camera:%p,context:%p] widget has bad type",
				    PRIVATE_DATA->camera, PRIVATE_DATA->context);
		rc = GP_ERROR_BAD_PARAMETERS;
		goto cleanup;
	}

	/* This is the actual set call. Note that we keep
	 * ownership of the string and have to free it if necessary.
	 */
	rc = gp_widget_set_value(child, val);
	if (rc < GP_OK) {
		goto cleanup;
	}
	/* This stores it on the camera again */
	rc = gp_camera_set_config(PRIVATE_DATA->camera, widget,
				  PRIVATE_DATA->context);
	if (rc < GP_OK) {
		return rc;
	}

cleanup:
	gp_widget_free(widget);

	return rc;
}

int get_key_val_dslr(const char *key, char **str, indigo_device *device)
{
	CameraWidget *widget = NULL, *child = NULL;
	CameraWidgetType type;
	int rc;
	char *val;

	rc = gp_camera_get_config(PRIVATE_DATA->camera, &widget,
				  PRIVATE_DATA->context);
	if (rc < GP_OK) {
		return rc;
	}
	rc = lookup_widget(widget, key, &child);
	if (rc < GP_OK) {
		goto cleanup;
	}

	/* This type check is optional, if you know what type the label
	 * has already. If you are not sure, better check. */
	rc = gp_widget_get_type(child, &type);
	if (rc < GP_OK) {
		goto cleanup;
	}
	switch (type) {
	case GP_WIDGET_MENU:
	case GP_WIDGET_RADIO:
	case GP_WIDGET_TEXT:
		break;
	default:
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[camera:%p,context:%p] widget has bad type",
				    PRIVATE_DATA->camera, PRIVATE_DATA->context);
		rc = GP_ERROR_BAD_PARAMETERS;
		goto cleanup;
	}

	/* This is the actual query call. Note that we just
	 * a pointer reference to the string, not a copy... */
	rc = gp_widget_get_value(child, &val);
	if (rc < GP_OK) {
		goto cleanup;
	}

	/* Create a new copy for our caller. */
	*str = strdup(val);

cleanup:
	gp_widget_free (widget);

	return rc;
}

static void ctx_error_func(GPContext *context, const char *str, void *data)
{
	UNUSED(context);
	UNUSED(data);

	INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", str);
}

static void ctx_status_func(GPContext *context, const char *str, void *data)
{
	UNUSED(context);
	UNUSED(data);

	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s", str);
}

static int eos_mirror_lockup(const bool enable, indigo_device *device)
{
	return set_key_val_dslr(EOS_CUSTOMFUNCEX, enable ?
				EOS_MIRROR_LOCKUP_ENABLE :
				EOS_MIRROR_LOCKUP_DISABLE,
				device);
}

static int find_slot_of_device(const char *name, const char *value)
{
	if (!name || !value)
		return -1;

	for (uint8_t slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];

		if (devices[slot] && !strcmp(name, PRIVATE_DATA->name) &&
		    !strcmp(value, PRIVATE_DATA->value))
			return slot;
	}

	return -1;
}

static int find_free_slot(const char *name, const char *value)
{
	if (!name || !value)
		return -1;

	for (uint8_t slot = 0; slot < MAX_DEVICES; slot++)
		if (!devices[slot])
			return slot;

	return -1;
}

static int attach_device_in_slot(const int slot, const char *name,
				 const char *value,
				 indigo_device *gphoto2_template)
{
	int rc;
	gphoto2_private_data *private_data = NULL;
	indigo_device *device = NULL;

	private_data = calloc(1, sizeof(gphoto2_private_data));
	if (!private_data) {
		rc = -errno;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", strerror(errno));
		goto err_cleanup;
	}

	/* Create new context to be used by frontend. */
	private_data->context = gp_context_new();
	gp_context_set_error_func(private_data->context,
				  ctx_error_func, NULL);
	gp_context_set_message_func(private_data->context,
				    ctx_status_func, NULL);

	/* Allocate memory for camera. */
	rc = gp_camera_new(&private_data->camera);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "gp_camera_new failed: %d", rc);
		goto err_cleanup;
	}

	/* Initiate a connection to the camera. */
	rc = gp_camera_init(private_data->camera, private_data->context);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "gp_camera_init failed: %d", rc);
		goto err_cleanup;
	}

	private_data->name = strdup(name);
	private_data->value = strdup(value);
	private_data->libgphoto2_version = strdup(
		*gp_library_version(GP_VERSION_SHORT));

	device = calloc(1, sizeof(indigo_device));
	if (!device) {
		rc = -errno;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", strerror(errno));
		goto err_cleanup;
	}
	indigo_device *master_device = device;

	memcpy(device, gphoto2_template, sizeof(indigo_device));
	device->master_device = master_device;

	sprintf(device->name, "%s %s", name, value);
	device->private_data = private_data;

	rc = indigo_attach_device(device);
	if (rc != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_attach_device failed: %d", rc);
		rc = -rc;
		goto err_cleanup;
	}

	devices[slot] = device;

	return rc;

err_cleanup:
	if (private_data) {
		if (private_data->name) {
			free(private_data->name);
			private_data->name = NULL;
		}
		if (private_data->value) {
			free(private_data->value);
			private_data->value = NULL;
		}
		if (private_data->libgphoto2_version) {
			free(private_data->libgphoto2_version);
			private_data->libgphoto2_version = NULL;
		}
		free(private_data);
		private_data = NULL;
	}
	if (device) {
		free(device);
		device = NULL;
	}

	return rc;
}

static int add_camera_device(CameraList *camera_list, indigo_device *device)
{
	int rc = -1;

	for (int n = 0; n < gp_list_count(camera_list); n++) {

		int slot;
		const char *name = NULL;
		const char *value = NULL;

		gp_list_get_name(camera_list, n, &name);
		gp_list_get_value(camera_list, n, &value);

		/* Device already exists in devices[...]. */
		if (find_slot_of_device(name, value) >= 0)
			continue;

		slot = find_free_slot(name, value);
		if (slot < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME,
					    "no free slot for plugged "
					    "device %s %s found", name, value);
			return -1;
		}

		rc = attach_device_in_slot(slot, name, value, device);
		if (rc < 0)
			return rc;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s %s in slot: %d",
				    name, value, slot);
	}

	return rc;
}

static int remove_camera_device(CameraList *camera_list)
{
	int rc = -1;
	bool devices_keep[MAX_DEVICES] = {false};

	for (int c = 0; c < gp_list_count(camera_list); c++) {

		int slot;
		const char *name = NULL;
		const char *value = NULL;

		gp_list_get_name(camera_list, c, &name);
		gp_list_get_value(camera_list, c, &value);

		slot = find_slot_of_device(name, value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "slot: %d %s %s",
				    slot, name, value);
		if (slot >= 0)
			devices_keep[slot] = true;
	}

	gphoto2_private_data *private_data = NULL;

	for (uint8_t slot = 0; slot < MAX_DEVICES; slot++) {
		if (!devices_keep[slot] && devices[slot]) {

			indigo_device **device = &devices[slot];

			indigo_detach_device(*device);
			if ((*device)->private_data) {
				private_data = (*device)->private_data;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "remove camera: %s %s",
						    private_data->name,
						    private_data->value);
			}
			if (private_data) {
				gp_camera_exit(private_data->camera, private_data->context);
				gp_camera_unref(private_data->camera);
				private_data->camera = NULL;
				gp_context_unref(private_data->context);
				private_data->context = NULL;
				free(private_data->name);
				private_data->name = NULL;
				free(private_data->value);
				private_data->value = NULL;
				free(private_data->libgphoto2_version);
				private_data->libgphoto2_version = NULL;
				free(private_data);
			}
			free(*device);
			*device = NULL;
			rc = 0;
		}
	}

	return rc;
}

static int detect_camera_device(indigo_device *device,
				const libusb_hotplug_flag flag)
{
	int rc;
	CameraList *list = NULL;
	const char *name = NULL;
	const char *value = NULL;

	rc = gp_list_new(&list);
	if (rc < GP_OK)
		goto out;

	rc = gp_camera_autodetect(list, NULL);
	if (rc < GP_OK)
		goto out;

	for (int c = 0; c < gp_list_count(list); c++) {
		gp_list_get_name(list, c, &name);
		gp_list_get_value(list, c, &value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME,
				    "detected camera: %s %s", name, value);
	}

	rc = flag == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED ?
		add_camera_device(list, device) :
		remove_camera_device(list);
out:
	if (list)
		gp_list_free(list);

	return rc;
}

void update_property(indigo_device *device, indigo_property *property,
		     const char *widget)
{
	int rc = GP_ERROR;

	for (int p = 0; p < property->count; p++)
		if (property->items[p].sw.value)
			rc = set_key_val_dslr(widget,
					      property->items[p].name,
					      device);
	if (rc == GP_OK) {
		property->state = INDIGO_OK_STATE;
		indigo_update_property(device, property, NULL);
	}
}

static indigo_result ccd_attach(indigo_device *device)
{
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);

	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {

		/*------------------------- SHUTTER-TIME -----------------------*/
		int count = widget_enumerate(EOS_SHUTTERSPEED, device, NULL);
		DSLR_SHUTTER_PROPERTY = indigo_init_switch_property(NULL,
								    device->name,
								    DSLR_SHUTTER_PROPERTY_NAME,
								    INDIGO_NAME_DSLR,
								    INDIGO_NAME_SHUTTER,
								    INDIGO_IDLE_STATE,
								    INDIGO_RW_PERM,
								    INDIGO_ONE_OF_MANY_RULE,
								    count);
		widget_enumerate(EOS_SHUTTERSPEED, device, DSLR_SHUTTER_PROPERTY);

		/*---------------------------- ISO -----------------------------*/
		count = widget_enumerate(EOS_ISO, device, NULL);
		DSLR_ISO_PROPERTY = indigo_init_switch_property(NULL,
								device->name,
								DSLR_ISO_PROPERTY_NAME,
								INDIGO_NAME_DSLR,
								INDIGO_NAME_ISO,
								INDIGO_IDLE_STATE,
								INDIGO_RW_PERM,
								INDIGO_ONE_OF_MANY_RULE,
								count);
		widget_enumerate(EOS_ISO, device, DSLR_ISO_PROPERTY);

		/*------------------------ COMPRESSION -------------------------*/
		count = widget_enumerate(EOS_COMPRESSION, device, NULL);
		DSLR_COMPRESSION_PROPERTY = indigo_init_switch_property(NULL,
									device->name,
									DSLR_COMPRESSION_PROPERTY_NAME,
									INDIGO_NAME_DSLR,
									INDIGO_NAME_COMPRESSION,
									INDIGO_IDLE_STATE,
									INDIGO_RW_PERM,
									INDIGO_ONE_OF_MANY_RULE,
									count);
		widget_enumerate(EOS_COMPRESSION, device, DSLR_COMPRESSION_PROPERTY);

		/*------------------------ MIRROR-LOCKUP -----------------------*/
		DSLR_MIRROR_LOCKUP_PROPERTY = indigo_init_switch_property(NULL,
									  device->name,
									  DSLR_MIRROR_LOCKUP_PROPERTY_NAME,
									  INDIGO_NAME_DSLR,
									  INDIGO_NAME_MIRROR_LOCKUP,
									  INDIGO_IDLE_STATE,
									  INDIGO_RW_PERM,
									  INDIGO_ONE_OF_MANY_RULE,
									  2);
		indigo_init_switch_item(DSLR_MIRROR_LOCKUP_LOCK_ITEM,
					DSLR_MIRROR_LOCKUP_LOCK_ITEM_NAME,
					INDIGO_NAME_MIRROR_LOCKUP_LOCK,
					false);
		indigo_init_switch_item(DSLR_MIRROR_LOCKUP_UNLOCK_ITEM,
					DSLR_MIRROR_LOCKUP_UNLOCK_ITEM_NAME,
					INDIGO_NAME_MIRROR_LOCKUP_UNLOCK,
					true);

		/*--------------------- LIBGPHOTO2-VERSION --------------------*/
		DSLR_LIBGHOTO2_VERSION_PROPERTY = indigo_init_text_property(NULL,
									    device->name,
									    DSLR_LIBGHOTO2_VERSION_PROPERTY_NAME,
									    INDIGO_NAME_DSLR,
									    INDIGO_NAME_LIBGHOTO2,
									    INDIGO_IDLE_STATE,
									    INDIGO_RO_PERM,
									    1);
		indigo_init_text_item(DSLR_LIBGHOTO2_VERSION_ITEM,
				      DSLR_LIBGHOTO2_VERSION_ITEM_NAME,
				      INDIGO_NAME_LIBGHOTO2_VERSION,
				      "%s",
				      PRIVATE_DATA->libgphoto2_version);

		/*--------------------- CCD_EXPOSURE_ITEM --------------------*/
		double number_min = 3600;
		double number_max = -number_min;
		for (int i = 0; i < DSLR_SHUTTER_PROPERTY->count; i++) {
			/* Skip {B,b}ulb widget. */
			if (DSLR_SHUTTER_PROPERTY->items[i].name[0] == 'b' ||
			    DSLR_SHUTTER_PROPERTY->items[i].name[0] == 'B')
				continue;

			double number_shutter = parse_shutterspeed(
				DSLR_SHUTTER_PROPERTY->items[i].name);

			number_min = MIN(number_shutter, number_min);
			number_max = MAX(number_shutter, number_max);
		}
		CCD_EXPOSURE_ITEM->number.min = number_min;
		CCD_EXPOSURE_ITEM->number.max = number_max;

		/*--------------------- CCD_INFO --------------------*/
		char *name = PRIVATE_DATA->name;
		CCD_INFO_PROPERTY->hidden = true;
		for (int i = 0; name[i]; i++)
                        name[i] = toupper(name[i]);
		for (int i = 0; dslr_model_info[i].name; i++) {
                        if (strstr(name, dslr_model_info[i].name)) {
				CCD_INFO_WIDTH_ITEM->number.value = dslr_model_info[i].width;
				CCD_INFO_HEIGHT_ITEM->number.value = dslr_model_info[i].height;
				CCD_INFO_PIXEL_SIZE_ITEM->number.value = dslr_model_info[i].pixel_size;
				CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
				CCD_INFO_PROPERTY->hidden = false;
			}
		}

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}

	return INDIGO_FAILED;
}

static indigo_result ccd_detach(indigo_device *device)
{
	assert(device != NULL);

	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_release_property(DSLR_SHUTTER_PROPERTY);
	indigo_release_property(DSLR_ISO_PROPERTY);
	indigo_release_property(DSLR_COMPRESSION_PROPERTY);
	indigo_release_property(DSLR_MIRROR_LOCKUP_PROPERTY);
	indigo_release_property(DSLR_LIBGHOTO2_VERSION_PROPERTY);

	return indigo_ccd_detach(device);
}

static indigo_result ccd_enumerate_properties(indigo_device *device,
					      indigo_client *client,
					      indigo_property *property)
{
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result ccd_change_property(indigo_device *device,
					 indigo_client *client,
					 indigo_property *property)
{
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	/*------------------------ CONNECTION --------------------------*/
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property,
					    false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			device->is_connected = !!(PRIVATE_DATA->camera && PRIVATE_DATA->context);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

			indigo_define_property(device, DSLR_SHUTTER_PROPERTY, NULL);
			indigo_define_property(device, DSLR_ISO_PROPERTY, NULL);
			indigo_define_property(device, DSLR_COMPRESSION_PROPERTY, NULL);
			indigo_define_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
			indigo_define_property(device, DSLR_LIBGHOTO2_VERSION_PROPERTY, NULL);
		} else {
			if (device->is_connected) {
				indigo_delete_property(device, DSLR_SHUTTER_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_ISO_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_COMPRESSION_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_LIBGHOTO2_VERSION_PROPERTY, NULL);
				device->is_connected = false;
			}
		}
	}
	/*-------------------------- SHUTTER-TIME ----------------------------*/
	else if (indigo_property_match(DSLR_SHUTTER_PROPERTY, property)) {
			indigo_property_copy_values(DSLR_SHUTTER_PROPERTY, property, false);
			update_property(device, DSLR_SHUTTER_PROPERTY, EOS_SHUTTERSPEED);
			return INDIGO_OK;
	}
	/*------------------------------ ISO ---------------------------------*/
	else if (indigo_property_match(DSLR_ISO_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_ISO_PROPERTY, property, false);
		update_property(device, DSLR_ISO_PROPERTY, EOS_ISO);
		return INDIGO_OK;
	}
	/*--------------------------- COMPRESSION ----------------------------*/
	else if (indigo_property_match(DSLR_COMPRESSION_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_COMPRESSION_PROPERTY, property, false);
		update_property(device, DSLR_COMPRESSION_PROPERTY, EOS_COMPRESSION);
		return INDIGO_OK;
	}
	/*-------------------------- MIRROR-LOCKUP ---------------------------*/
	else if (indigo_property_match(DSLR_MIRROR_LOCKUP_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_MIRROR_LOCKUP_PROPERTY, property, false);
		int rc = eos_mirror_lockup(DSLR_MIRROR_LOCKUP_LOCK_ITEM->sw.value, device);
		if (rc == GP_OK) {
			property->state = INDIGO_OK_STATE;
			indigo_update_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
		}
		return INDIGO_OK;
	}
	/*--------------------------- CCD-EXPOSURE ---------------------------*/
	else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;

		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		/* TODO: */
		return INDIGO_OK;
	}

	return indigo_ccd_change_property(device, client, property);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev,
			    libusb_hotplug_event event, void *user_data) {

	UNUSED(ctx);

	indigo_device *gphoto2_template = (indigo_device *)user_data;
	struct libusb_device_descriptor descriptor;
	memset(&descriptor, 0, sizeof(descriptor));

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			detect_camera_device(gphoto2_template,
					     LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			detect_camera_device(gphoto2_template,
					     LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT);
			break;
		}
	}
	return 0;
}

indigo_result indigo_ccd_gphoto2(indigo_driver_action action, indigo_driver_info *info)
{
	static indigo_device gphoto2_template = INDIGO_DEVICE_INITIALIZER(
		"gphoto2",
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "gphoto2 camera", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL,
							  LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
							  LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
							  LIBUSB_HOTPLUG_NO_FLAGS, /* LIBUSB_HOTPLUG_ENUMERATE, */
							  LIBUSB_HOTPLUG_MATCH_ANY,
							  LIBUSB_HOTPLUG_MATCH_ANY,
							  LIBUSB_HOTPLUG_MATCH_ANY,
							  hotplug_callback,
							  &gphoto2_template,
							  &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME,
				    "libusb_hotplug_register_callback ->  %s",
				    rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
