# Basics of INDIGO Client Development
Version: 0.1

Author: **Rumen G.Bogdanovski**

e-mail: *rumen@skyarchive.org*

## INDIGO Concepts

INDIGO is a platform for the communication between software entities over a software **bus**. These entities are typically either in a **device** or a **client** role, but there are special entities often referred as **agents** which are in both **device** and **client** role. A piece of code registering one or more **devices** on the **bus** is referred as the **driver**.

INDIGO is asynchronous in its nature so to be able to communicate over the **bus**, the client and device have to register a structure containing pointers to **callback functions** which are called by the INDIGO bus upon specific events.

The communication between different entities attached to the bus is done by INDIGO **messages** which contain **property** events. Properties on the other hand contain one or more **items** of a specified type. One can think of the properties as both set of variables and routines. Set of variables as they may store values like the *width* and *height* of the CCD frame or routines like *start 1 second exposure*. Messages sent over the INDIGO bus are abstraction of INDI messages.

The messages sent from the **device** to a **client** may contain one of the following events:
- **definition** of a property - lets the client know that the property can be used, the message contains the property definition: name, type, list of items etc...
- **update** of property item values - lets the client know that certain property items have changed their values.
- **deletion** of a property - lets the client know that the property is removed and can not be used any more.

The messages sent from a **client** to the **device** can be:
- **request for definition** of available properties - client can request the definitions of one or all available properties associated with the device.
- **request for change** of property item values - client can request change of the values of one or several property items.

There are three classes of properties: mandatory, optional, and device specific. Each device class has a set of mandatory properties that should always be defined, a set of optional properties that may or may not be defined and a set of **device** specific properties which may not exist if there is no need. The list of the well known properties is available in [PROPERTIES.md](https://github.com/indigo-astronomy/indigo/blob/master/PROPERTIES.md).

Different instances of the INDIGO bus can be connected in a hierarchical structure, but from a **driver** or a **client** point of view it is fully transparent.

INDIGO uses XML or JSON transport protocols for message passing. The description of the INDIGO protocols used for communication is available in [PROTOCOLS.md](https://github.com/indigo-astronomy/indigo/blob/master/PROTOCOLS.md).

## Basics of INDIGO API

A basic common API shared by both **driver** and **client** roles is defined in [indigo_bus.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_bus.h). The most important structures are:
- *indigo_item* - definition of property item container
- *indigo_property* - definition of property container
- *indigo_device* - definition of a logical device made available by *driver* containing both driver private data and pointers to callback functions
- *indigo_client* - definition of a client containing both client private data and pointers to callback functions

The **bus** instance should be initialized and started by *indigo_start()* call and stopped by *indigo_stop()* call.
A **client** should be attached to the buss by *indigo_attach_client()* or *indigo_detach_client()* call.
There are two calls that can be used by the **client** to send messages to the **device**:
- *indigo_enumerate_properties()* - send request for definition of available properties
- *indigo_change_property()* - request for change of property item values

In order to communicate over TCP the client must use the following calls:
- *indigo_connect_server()* - connect to remote indigo service
- *indigo_disconnect_server()* - disconnect from the remote service

For structure definitions and function prototypes please refer to [indigo_bus.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_bus.h) and [indigo_client.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_client.h).

## Anatomy of the INDIGO client

The indigo client should define several callbacks which will be called by the bus on one of the events:
- **attach** - called when client is attached to the bus
- **define property** - called when the device broadcasts property definition
- **update property** - called when the device broadcasts property value change
- **delete property** - called when the device broadcasts property removal
- **send message** - called when the device broadcasts a human readable text message
- **detach** - called when client is detached from the bus

Here is an example of callbacks that we will use in this example:

```C
static indigo_result my_attach(indigo_client *client) {
	indigo_log("attached to INDIGO bus...");

	/* Request property definitions */
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);

	return INDIGO_OK;
}

static indigo_result my_define_property(indigo_client *client,
	indigo_device *device, indigo_property *property, const char *message) {

		// do something useful here ;)

		return INDIGO_OK;
}

static indigo_result my_update_property(indigo_client *client,
	indigo_device *device, indigo_property *property, const char *message) {

		// do something useful here

		return INDIGO_OK;
}

static indigo_result my_detach(indigo_client *client) {
	indigo_log("detached from INDIGO bus...");
	return INDIGO_OK;
}
```

Once the callbacks are defined the *indigo_client* structure should be defined and initialized:

```C
static indigo_client my_client = {
	"MyClient",                     // client name
	false,                          // is this a remote client "no" - this is us
	NULL,                           // we do not have client specific data
	INDIGO_OK,                      // result of last bus operation - we just initialize it with ok
	INDIGO_VERSION_CURRENT,         // the client speaks current indigo version
	NULL,                           // BLOB mode records -> Set this to NULL
	my_attach,
	my_define_property,
	my_update_property,
	NULL,
	NULL,
	my_detach
};
```  
Please note we do not care about **delete property** and **send message** events in our example so we set them to *NULL*.

Now as we have everything set up and ready to go we need our *main()* function:
```C
int main(int argc, const char * argv[]) {
	/* We need to pass argv and argc to INDIGO */
	indigo_main_argc = argc;
	indigo_main_argv = argv;

	indigo_start();

	/* We want to see debug messages on the screen */
	indigo_set_log_level(INDIGO_LOG_DEBUG);

	indigo_attach_client(&my_client);

	/* We want to connect to a remote indigo host indigosky.local:7624 */
	indigo_server_entry *server;
	indigo_connect_server("indigosky", "indigosky.local", 7624, &server);

	/* We can do whatever we want here while we are waiting for
	   the client to complete. For example we can call some GUI
	   framework main loop etc...
	   Instead we will just sleep for 10 seconds.
	*/
	indigo_usleep(10 * ONE_SECOND_DELAY);

	indigo_disconnect_server(server);
	indigo_detach_client(&my_client);
	indigo_stop();

	return 0;
}
```

## Device and Property Handling

Devices, Properties and property items are treated as strings and should be matched with *strcmp()* call.
Properties can be in one of the four states:
- *INDIGO_IDLE_STATE* - the values may not be initialized
- *INDIGO_OK_STATE* - the property item values are valid and it is save to read or set them
- *INDIGO_BUSY_STATE* - the values are not reliable, some operation is in progress (like exposure is in progress)
- *INDIGO_ALERT_STATE* - the values are not reliable, some operation has failed or the values set are not valid

Each property has predefined type which is one of the following:
- *INDIGO_TEXT_VECTOR* - strings of limited width
-	*INDIGO_NUMBER_VECTOR* - floating point numbers with defined min and max values and increment
-	*INDIGO_SWITCH_VECTOR* - logical values representing “on” and “off” state
-	*INDIGO_LIGHT_VECTOR* - status values with four possible values INDIGO_IDLE_STATE, INDIGO_OK_STATE, INDIGO_BUSY_STATE and INDIGO_ALERT_STATE
-	*INDIGO_BLOB_VECTOR* - binary data of any type and any length usually image data

In case the client needs to check the values of some property item of a specified device it is always a good idea to check if the property is in OK state:
```C
if (!strcmp(device->name, "CCD Imager Simulator @ indigosky") &&
    !strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) &&
    property->state == INDIGO_OK_STATE) {
			...
}
```
The above code snippet checks if the image from "CCD Imager Simulator @ indigosky" camera is ready to be used.

And if the client needs to change some item value this code may help:
```C
static const char * items[] = { CCD_IMAGE_FORMAT_FITS_ITEM_NAME };
static bool values[] = { true };
indigo_change_switch_property(
	client,
	CCD_SIMULATOR,
	CCD_IMAGE_FORMAT_PROPERTY_NAME,
	1,
	items,
	values
);
```
The above code snippet requests the CCD driver to change the image format in to FITS by setting the fits switch item to true.

## Binary Large Objects aka Image Properties

Binary Large OBject (BLOB) properties are a special case. They usually contain image data and they should be handled differently. Their updates are not received by default. The client should enable updates for each BLOB property. It is a good idea to do so in the **define property** callback like so:
```C
if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
	if (device->version >= INDIGO_VERSION_2_0)
		indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_URL);
	else
		indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_ALSO);
}
```
In the above example the VERSION of the protocol is checked and if it is INDIGO the new more efficient way of blob transfer is requested - *INDIGO_ENABLE_BLOB_URL*. If it is legacy INDI protocol the old method is requested - *INDIGO_ENABLE_BLOB_ALSO*. To disable blob transfer again one should use *INDIGO_ENABLE_BLOB_NEVER*.

On the other hand if the BLOB transfer is *INDIGO_ENABLE_BLOB_URL* the data is not transferred, but only the location where it can be retrieved in raw binary form without any encoding and decoding. This is necessary because INDIGO (and INDI) are text based protocols but the BLOB data is binary. To overcome this INDI uses base64 data encoding to transfer BLOBs. This approach leads to a significant slowdown as data is 30% bigger and the BLOBS are encoded in the driver and decoded in the client. For INDY compatibility INDIGO also uses this approach if the client or the server are legacy INDI and this mode is enabled by *INDIGO_ENABLE_BLOB_ALSO*. Due to these diferencies the INDIGO BLOB data should be retrieved explicitly by the client by using *indigo_populate_http_blob_item()*:
```C
if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
		if (*property->items[0].blob.url) {
			indigo_populate_http_blob_item(&property->items[0]);
		}
}
```  

## INDIGO Imaging Client - Example

The following code is a working example of INDIGO client that connects to "*indigosky.local:7624*", and uses camera named "**CCD Imager Simulator @ indigosky**" to take 10 exposures, 3 seconds each and save the images in FITS format. Please make sure that **indigo_ccd_simulator** driver is loaded on *indigosky* otherwise the example will not work because it does not check if the camera is present for simplicity.

```C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <indigo/indigo_bus.h>
#include <indigo/indigo_client.h>

#define CCD_SIMULATOR "CCD Imager Simulator @ indigosky"

static bool connected = false;
static int count = 10;

static indigo_result test_attach(indigo_client *client) {
	indigo_log("attached to INDIGO bus...");
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

static indigo_result test_define_property(indigo_client *client,
	indigo_device *device, indigo_property *property, const char *message) {

	if (strcmp(property->device, CCD_SIMULATOR))
		return INDIGO_OK;
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		indigo_device_connect(client, CCD_SIMULATOR);
		return INDIGO_OK;
	}
	if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
		if (device->version >= INDIGO_VERSION_2_0)
			indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_URL);
		else
			indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_ALSO);
	}
	if (!strcmp(property->name, CCD_IMAGE_FORMAT_PROPERTY_NAME)) {
		static const char * items[] = { CCD_IMAGE_FORMAT_FITS_ITEM_NAME };
		static bool values[] = { true };
		indigo_change_switch_property(client, CCD_SIMULATOR,
			CCD_IMAGE_FORMAT_PROPERTY_NAME, 1, items, values);
	}
	return INDIGO_OK;
}

static indigo_result test_update_property(indigo_client *client,
	indigo_device *device, indigo_property *property, const char *message) {

	if (strcmp(property->device, CCD_SIMULATOR)) return INDIGO_OK;

	static const char * items[] = { CCD_EXPOSURE_ITEM_NAME };
	static double values[] = { 3.0 };
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME) &&
		property->state == INDIGO_OK_STATE) {

		if (indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
			if (!connected) {
				connected = true;
				indigo_log("connected...");
				indigo_change_number_property(client, CCD_SIMULATOR,
					CCD_EXPOSURE_PROPERTY_NAME, 1, items, values);
			}
		} else {
			if (connected) {
				indigo_log("disconnected...");
				indigo_stop();
				connected = false;
			}
		}
		return INDIGO_OK;
	}
	if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) &&
		property->state == INDIGO_OK_STATE) {

		if (*property->items[0].blob.url &&
			indigo_populate_http_blob_item(&property->items[0])) {

			indigo_log("image URL received (%s, %d bytes)...",
				property->items[0].blob.url, property->items[0].blob.size);
		}
		char name[32];
		sprintf(name, "img_%02d.fits", count);
		FILE *f = fopen(name, "wb");
		fwrite(property->items[0].blob.value, property->items[0].blob.size, 1, f);
		fclose(f);
		indigo_log("image saved to %s...", name);
		free(property->items[0].blob.value);
		property->items[0].blob.value = NULL;
	}
	if (!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME)) {
		if (property->state == INDIGO_BUSY_STATE) {
			indigo_log("exposure %gs...", property->items[0].number.value);
		} else if (property->state == INDIGO_OK_STATE) {
			indigo_log("exposure done...");
			if (--count > 0) {
				indigo_change_number_property(
					client,
					CCD_SIMULATOR,
					CCD_EXPOSURE_PROPERTY_NAME,
					1,
					items,
					values
				);
			} else {
				indigo_device_disconnect(client, CCD_SIMULATOR);
			}
		}
		return INDIGO_OK;
	}
	return INDIGO_OK;
}

static indigo_result test_detach(indigo_client *client) {
	indigo_log("detached from INDIGO bus...");
	return INDIGO_OK;
}

static indigo_client test = {
	"Test", false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
	test_attach,
	test_define_property,
	test_update_property,
	NULL,
	NULL,
	test_detach
};

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;

	indigo_start();
	indigo_set_log_level(INDIGO_LOG_INFO);

	indigo_server_entry *server;
	indigo_attach_client(&test);
	indigo_connect_server("indigosky", "indigosky.local", 7624, &server);
	while (count > 0) {
		  indigo_usleep(ONE_SECOND_DELAY);
	}
	indigo_disconnect_server(server);
	indigo_detach_client(&test);
	indigo_stop();
	return 0;
}
```

## Several Notes About the Drivers

There are three groups of structures and functions – for management of standard **local dynamic drivers** (loaded as dynamic libraries and connected to the local **bus**), **local executable drivers** (loaded as executable, e.g. legacy INDI drivers, connected to the local bus over pipes) and **remote servers** (connected to the local bus over a network). The above example uses the third approach using **remote servers**.

## More Client Examples
An open source examples of client API usage are the following pieces of code:

[indigo_test/client.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_test/client.c) - Basic API example

[indigo_tools/indigo_prop_tool.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_tools/indigo_prop_tool.c) - Command line property management tool

[INDIGO Control Panel](https://github.com/indigo-astronomy/control-panel) - Official Linux and Windows Control panel using QT framework

[PixInsight INDIGO client project](https://github.com/PixInsight/PCL/tree/master/src/modules/processes/contrib/kkretzschmar/INDIClient)
