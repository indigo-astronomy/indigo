# Basics of INDIGO Client Development
Revision: 13.10.2024 (draft)

Author: **Rumen G.Bogdanovski**

e-mail: *rumenastro@gmail.com*

## INDIGO Concepts

INDIGO is a platform for the communication between software entities over a software **bus**. These entities are typically either in a **device** or a **client** role, but there are special entities often referred as **agents** which are in both **device** and **client** role. A piece of code registering one or more **devices** on the **bus** is referred as the **driver**.

INDIGO is asynchronous in its nature, so to be able to communicate over the **bus**, the client and device have to register a structure containing pointers to **callback functions** which are called by the INDIGO **bus** upon specific events.

The communication between different entities attached to the **bus** is done by INDIGO **messages** which contain **property** events. Properties on the other hand contain one or more **items** of a specified type. One can think of the properties as both set of variables and routines. Set of variables as they may store values like the *width* and *height* of the CCD frame or routines like *start 1 second exposure*. Messages sent over the INDIGO **bus** are abstraction of INDI messages.

The messages sent from the **device** to a **client** may contain one of the following events:
- **definition** of a property - lets the client know that the property can be used, the message contains the property definition: name, type, list of items etc...
- **update** of property item values - lets the client know that certain property items have changed their values.
- **deletion** of a property - lets the client know that the property is removed and can not be used any more.

The messages sent from a **client** to the **device** can be:
- **request for enumeration** of available properties - client can request the definitions of one or all available properties associated with the device.
- **request for change** of property item values - client can request change of the values of one or several property items.

There are three classes of properties: mandatory, optional, and device specific. Each device class has a set of mandatory properties that should always be defined, a set of optional properties that may or may not be defined and a set of **device** specific properties which may not exist if there is no need. The list of the well known properties is available in [PROPERTIES.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/PROPERTIES.md).

Different instances of the INDIGO **bus** can be connected in a hierarchical structure, but from a **driver** or a **client** point of view it is fully transparent.

INDIGO uses XML or JSON transport protocols for message passing. The description of the INDIGO protocols used for communication is available in [PROTOCOLS.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/PROTOCOLS.md).

## Basics of INDIGO API

A basic common API shared by both **driver** and **client** roles is defined in [indigo_bus.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_bus.h). The most important structures are:
- *indigo_item* - definition of property item container
- *indigo_property* - definition of property container
- *indigo_device* - definition of a logical device made available by *driver* containing both driver private data and pointers to callback functions
- *indigo_client* - definition of a client containing both client private data and pointers to callback functions

The **bus** instance should be initialized and started by *indigo_start()* call and stopped by *indigo_stop()* call.
A **client** should be attached to the **bus** by calling *indigo_attach_client()* and detached with *indigo_detach_client()* call.
There are several functions that can be used by the **client** to send messages to the **device**:
- *indigo_enumerate_properties()* - send request for definition of available properties
- *indigo_change_property()* - request for change of property item values
- *indigo_enable_blob()* - enable and disable BLOB transfer.

There are also higher level functions to request a property change, which do not require properly initialized *indigo_property* structure:
- *indigo_change_text_property()* - change multiple items of a text property
- *indigo_change_switch_property()* - change multiple items of a switch property
- *indigo_change_number_property()* - change multiple items of a number property
- *indigo_change_text_property_1()* - change one item of a text property
- *indigo_change_switch_property_1()* - change one item of a switch property
- *indigo_change_number_property_1()* - change one item of a number property

**NOTE:** *indigo_change_XXX_property()* functions internally construct *indigo_property* structure and call *indigo_change_property()*

In order to communicate over TCP the client must use the following calls:
- *indigo_connect_server()* - connect to remote indigo service
- *indigo_connection_status()* - check connection status returns *true* of connected (introduced in INDIGO 2.0-112)
- *indigo_disconnect_server()* - disconnect from the remote service

Because of the asynchronous nature of INDIGO *indigo_connect_server()* is asynchronous too. When it returns success it means that the connection request has been successful, but not that the connection is established. *indigo_connect_server()* will try to keep the connection alive and reconnect until *indigo_disconnect_server()* is called. In case if there is a need to monitor connection status, as of INDIGO 2.0-112 there is a function *indigo_connection_status()* that returns *true* if connected and *false* if not. The optional *last_error* parameter will contain the message of the last error.

**NOTE!** We **DO NOT encourage** developers to use *indigo_connection_status()* in their new projects, however it is **absolutely safe** to use it. This function is added upon several requests from existing projects which adopt INDIGO and their logic requires this information. We encourage developers to design asynchronous clients abstracted from the connection status.

For structure definitions and function prototypes please refer to [indigo_bus.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_bus.h) and [indigo_client.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_client.h).

## Anatomy of the INDIGO client

The indigo client should define several callbacks which will be called by the **bus** on one of the events:
- **attach** - called when client is attached to the **bus**
- **define property** - called when the device broadcasts property definition
- **update property** - called when the device broadcasts property value change
- **delete property** - called when the device broadcasts property removal
- **send message** - called when the device broadcasts a human readable text message
- **detach** - called when client is detached from the **bus**

Every **define property**, **delete property** and **update property** have an optional human readable message associated with them. In case of an error this message may contain the reason of the failure. If there is no message associated with the event it will be set to *NULL*. Here are some templates of the callbacks that we will use in the client example:

```C
static indigo_result my_attach(indigo_client *client) {
	indigo_log("attached to INDIGO bus...");

	// Request property definitions
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
	"MyClient",                 // client name
	false,                      // is this a remote client "no" - this is us
	NULL,                       // we do not have client specific data
	INDIGO_OK,                  // result of last bus operation
	                            // - we just initialize it with ok
	INDIGO_VERSION_CURRENT,     // the client speaks current indigo version
	NULL,                       // BLOB mode records -> Set this to NULL
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

In case the connection status has to be monitored by the client (not encouraged to do so) *indigo_connection_status()* should be used. The code may look something like this:

```C
char last_error_message[256];
if (indigo_connection_status(server, last_error_message)) {
	/* Connected everything is ok */
} else {
	/* If not connected yet, last_error_message will be empty sting,
	   if connect failed it will contain the reason.
	*/
	if (last_error_message[0] == '\0') {
		/* not connected yet, should wait a bit more */
	} else {
		/* connect failed for reason stored in last_error_message */
	}
}
```

## Device and Property Handling

Devices, Properties and property items are treated as strings and should be matched with *strcmp()* call.

```C
if (!strcmp(device->name, "CCD Imager Simulator @ indigosky") {
	...
}
...
if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) {
	...
}
```

There is a special **delete property** event, which means that the device and all its properties are removed. In this case *property->name* is an empty string and *device->name* is the name of the removed device. Upon receiving such event the INDIGO client framework will automatically release all resources used by the device. Therefore if the client keeps property, device or any other pointers to resources of this device, they should not be used any more. It is a good idea such pointers to be be invalidated in the **delete property** handler callback by setting them to *NULL*.

### Properties

Standard property names are defined in [indigo_names.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_names.h)

Properties can be in one of the four states:
- *INDIGO_IDLE_STATE* - the values may not be initialized
- *INDIGO_OK_STATE* - the property item values are valid and it is save to read or set them
- *INDIGO_BUSY_STATE* - the values are not reliable, some operation is in progress (like exposure is in progress)
- *INDIGO_ALERT_STATE* - the values are not reliable, some operation has failed or the values set are not valid

Each property has predefined type which is one of the following:
- *INDIGO_TEXT_VECTOR* - strings of limited width
- *INDIGO_NUMBER_VECTOR* - floating point numbers with defined min and max values and increment
- *INDIGO_SWITCH_VECTOR* - logical values representing “on” and “off” state, there are several behavior rules for this type: *INDIGO_ONE_OF_MANY_RULE* (only one switch can be "on" at a time), *INDIGO_AT_MOST_ONE_RULE* (none or one switch can be "on" at a time) and *INDIGO_ANY_OF_MANY_RULE* (independent checkbox group)
- *INDIGO_LIGHT_VECTOR* - status values with four possible values *INDIGO_IDLE_STATE*, *INDIGO_OK_STATE*, *INDIGO_BUSY_STATE* and *INDIGO_ALERT_STATE*
- *INDIGO_BLOB_VECTOR* - binary data of any type and any length usually image data

Properties have permissions assigned to them:
- *INDIGO_RO_PERM* - Read only permission, which means that the client can not modify their item values
- *INDIGO_RW_PERM* - Read and write permission, client can read the value and can change it
- *INDIGO_WO_PERM* - Write only permission, client can change it but can not read its value (can be used for passwords)

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

### Handling Properties Asynchronously

In some casess properties should be hangled asynchronoysly. In case of dynamic or static driver linking by the client and you need to send a change request. Functions that send property change requests, such as *indigo_change_number_property()*, cannot be called directly from the property handler callbacks. These functions must be called asynchronously (see [indigo_examples/dynamic_driver_client.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_examples/dynamic_driver_client.c)). To facilitate this, several functions are provided:

In some cases, properties should be handled asynchronously. When dynamically or statically linking a driver by the client, and you need to send a change request, functions that send property change requests, such as *indigo_change_number_property()*, cannot be called directly from the property handler callbacks. These functions must be called asynchronously (see [indigo_examples/dynamic_driver_client.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_examples/dynamic_driver_client.c)). To facilitate this, several functions are provided:

- *indigo_handle_property_async()* - Executes a callback in a new thread, receiving pointers to *indigo_device*, *indigo_client*, and *indigo_property* as parameters.
- *indigo_async()* - Executes a callback in a new thread without parameters.
- *indigo_set_timer()* - Executes a callback in a separate thread after a delay. The callback receives a pointer to *indigo_device*.
- *indigo_set_timer_with_data()* - Executes a callback in a separate thread after a delay. The callback receives a pointer to *indigo_device* and a *void* pointer to the provided user data.

Here is an example code:

```C
static void start_exposure(indigo_device *device, indigo_client *client, indigo_property *property) {
	(void*)(device);
	(void*)(property);
	static const char * items[] = { CCD_EXPOSURE_ITEM_NAME };
	static const double values[] = { 3.0 };
	indigo_change_number_property(client, CCD_SIMULATOR, CCD_EXPOSURE_PROPERTY_NAME, 1, items, values);
}

static indigo_result client_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	...
	if (!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			...
			indigo_handle_property_async(start_exposure, device, client, NULL);
		}
	}
	...
	return INDIGO_OK;
}
```
It is important to note that *client*, *device*, and *property* objects may be invalidated during the execution of *start_exposure()*. It is the developer's responsibility to ensure proper synchronization of threads to handle these potential invalidations.

### Value vs Target in the Numeric Properties
The items of the numeric INDIGO properties have several fields like *min*, *max* etc. However the most used are *target* and *value*. If a client requests a value change, the update response from the driver will have the requested value stored in *target* and the *value* set to the current value (as read from the device). For example if the client requests CCD to be cooled to -20<sup>0</sup>C and the current CCD temperature is +10<sup>0</sup>C, the property update to the client will have *target* set to -20<sup>0</sup>C and *value* set to +10<sup>0</sup>C.

### Binary Large Objects aka Image Properties

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

On the other hand if the BLOB transfer is *INDIGO_ENABLE_BLOB_URL* the data is not transferred, but only the location where it can be retrieved in raw binary form without any encoding and decoding. This is the default INDIGO way of BLOB transfer. The fact that INDIGO (and INDI) are text based protocols but the BLOB data is binary poses a problem. To overcome this INDI uses Base64 data encoding to transfer BLOBs. This approach leads to a significant slowdown because the transferred data is approximately 30% larger than the raw data and additional encoding and decoding is performed in the driver and the client. INDIGO uses this way of transfer for INDI compatibility, if the client or the server are only legacy INDI capable. This legacy mode is enabled by *INDIGO_ENABLE_BLOB_ALSO*. Due to these differences the INDIGO BLOB data should be retrieved explicitly by the client by using *indigo_populate_http_blob_item()* call:

```C
if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) &&
   (property->state == INDIGO_OK_STATE)) {

    if (*property->items[0].blob.url) {
    	indigo_populate_http_blob_item(&property->items[0]);
    }
}
```  

### Devices
There are several things to be considered while handling devices.

First thing is the device interface (aka device type like CCD, Focuser, Mount etc...). This determines what are the mandatory properties of each device, but there are several properties mandatory for every INDIGO device.
Each INDIGO device has a read only property 'INFO' (*INFO_PROPERTY_NAME*), which contains items with the basic information of the device like name, version, serial number etc. However probably the most important item is 'DEVICE_INTERFACE' (*INFO_DEVICE_INTERFACE_ITEM_NAME*) which determines the device type. This is a sting containing an integer number with a bitmap of the supported interfaces (devices). It is a bitmap because there are devices that host more than one device in a single package like CCD camera with a guider port. However, unlike INDI, INDIGO always exposes one device per each logical device, so in the case of CCD + Guider, INDIGO will present one guider device and one CCD. There is a catch with 'DEVICE_INTERFACE' item, because 'INFO' is a text property and the integer bitmap is in decimal string format, and should be converted to int with *atoi()* before checking the bits.

```C
uint32_t device_interface;

if (!strcmp(property->name, INFO_PROPERTY_NAME)) {
	indigo_item *item;
	item = indigo_get_item(property, INFO_DEVICE_INTERFACE_ITEM_NAME);
	device_interface = atoi(item->text.value);
}

if (device_interface & INDIGO_INTERFACE_CCD) {
	// The device is CCD camera
}
```

The second important thing to consider about devices is that they need to be in connected state in order to be used. There is a switch property with one of many rule 'CONNECTION' (*CONNECTION_PROPERTY_NAME*) available to every device. It has two items: 'CONNECTED' (*CONNECTION_CONNECTED_ITEM_NAME*) and 'DISCONNECTED' (*CONNECTION_DISCONNECTED_ITEM_NAME*). So if the device is in connected state `CONNECTION.CONNECTED == 1`, if in disconnected `CONNECTION.DISCONNECTED == 1`.
In order to connect the device the client should request `CONNECTION.CONNECTED = 1`, and to disconnect respectively `CONNECTION.DISCONNECTED = 1`. To make the life easier there are utility functions for this - *indigo_device_connect()* and *indigo_device_disconnect()* For example:

```C
indigo_device_connect(client, "CCD Imager Simulator @ indigosky");
...
indigo_device_disconnect(client, "CCD Imager Simulator @ indigosky");
```

Please remember calling these functions only requests connection or disconnection. If the device is not connected the client has to wait for 'CONNECTION' property update and to check the state and item values as shown below:

```C
static indigo_result my_update_property(indigo_client *client,
	indigo_device *device, indigo_property *property, const char *message) {
	...
	if (!strcmp(property->device, "CCD Imager Simulator @ indigosky") &&
		!strcmp(property->name, CONNECTION_PROPERTY_NAME) &&
		property->state == INDIGO_OK_STATE) {

		if (indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
			// "CCD Imager Simulator @ indigosky" is connected
		} else {
			// "CCD Imager Simulator @ indigosky" is disconnected
		}
	}
	...
}
```

If the device is already connected, the second connection attempt will be ignored and device property enumeration, and CONNECTION property update will not happen. The client should check the state of connection property before trying to connect. If the device is already connected and the client logic relays on property enumeration on device connect, the client can trigger property enumeration by calling *indigo_enumerate_properties()* like this:
```C
indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
```
However, it is advised to design the client to use the cached properties, defined at client attach, rather than requesting property enumeration.

## Several Notes About the Drivers

There are three groups of structures and functions – for management of standard **local dynamic drivers** (loaded as dynamic libraries and connected to the local **bus**), **local executable drivers** (loaded as executable, e.g. legacy INDI drivers, connected to the local **bus** over pipes) and **remote servers** (connected to the local **bus** over a network). The above example uses the third approach using **remote servers**.

More information about available driver types in INDIGO is available in [INDIGO_SERVER_AND_DRIVERS_GUIDE.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/INDIGO_SERVER_AND_DRIVERS_GUIDE.md)

In INDIGO drivers are dynamically loaded and unloaded. This means that device can be attached to the **bus** or detached at any given time. The client should be able to handle these events for one more reason: most USB devices are hot-plug devices. Which means that these devices will be attached to the **bus** when plugged to the host computer and detached from the **bus** when unplugged (provided the corresponding driver is loaded).

## Serverless Operation - Applications Loading INDIGO Drivers

INDIGO framework can operate in an environment without a server. In this case the application must load the INDIGO driver instead of connecting to the server. The rest of the code will remain the same. Examples how to do this are available in indigo_examples folder of the INDIGO source tree:

1. [indigo_examples/executable_driver_client.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_examples/executable_driver_client.c) - Example of serverless operation using executable INDIGO driver.

2. [indigo_examples/dynamic_driver_client.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_examples/dynamic_driver_client.c) - Example of serverless operation using dynamic INDIGO driver.

The main difference, in terms of code, between the examples above and the *INDIGO Imaging Client - Example*, shown below, is the *main()* function. In terms of supported platforms, only the the remote server example shown below can work on all supported operating systems. The two examples above can work only on Linux and MacOSX. The reason for this is that INDIGO drivers can run only on Linux and MacOSX but not on Windows.

It is important to note a key consideration regarding the dynamic or static driver linking by the client. Functions that send property change requests, such as *indigo_change_number_property()*, cannot be called directly from the property handler callbacks. These functions must be called asynchronously as described in [Handling Properties Asynchronously](#handling-properties-asynchronously) (see [indigo_examples/dynamic_driver_client.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_examples/dynamic_driver_client.c)).

## INDIGO service Discovery
INDIGO server provides automatic service discovery using mDNS/DNS-SD (known as Bonjour in Apple ecosystem, and Avahi on Linux). This makes it easy for the client to identify INDIGO services. The INDIGO services can by identified by **_indigo._tcp**.

As of INDIGO 2.0-226 several functions for automatic service discovery are provided:
- *indigo_resolve_service()* - this function resolves a service by name and network interface, and if the service is resolved it will call a user defined callback.
- *indigo_start_service_browser()* - this function starts the service browser synchronously and returns. Each time a service is discovered or removed the user defined callback will be called. It will also be called when all available services are listed. In the call back the service name, the network interface where the service is available and the event on which it is called.
- *indigo_stop_service_browser()* - stops the service browser

### Discovery callback
This function is user defined and will be called by the framework when a service is discovered, removed and when all the services available at the moment are listed.
```C
void discover_callback(indigo_service_discovery_event event, const char *service_name, uint32_t interface_index);
```
*event* can be:
- *INDIGO_SERVICE_ADDED* - service is added and may be reported several times if available on several network interfaces
- *INDIGO_SERVICE_ADDED_GROUPED* - added service is reported once, interface is set to INDIGO_INTERFACE_ANY
- *INDIGO_SERVICE_REMOVED* - service is removed and may be reported several times, if available on several network interfaces
- *INDIGO_SERVICE_REMOVED_GROUPED* - removed service is reported once, interface is set to INDIGO_INTERFACE_ANY
- *INDIGO_SERVICE_END_OF_RECORD* - end of record, no more visible services at this time

*service_name* is the name of the discovered or removed service and *interface_index* is the number of the network interface where the service is available.

### Resolver callback
This function is user defined and will be called by the framework when a service is resolved by *indigo_resolve_service()*
```C
void resolve_callback(const char *service_name, uint32_t interface_index, const char *host, int port);
```
*service_name* is the name of the service being resolved, *interface_index* is the number of the network interface where the service is available, *host* is the host name and *port* is the TCP port where indigo service is accessible. Use *host* and *port* to connect to this service. In case of resolutuin error the callbck will have *host = NULL* and *port = 0*.

### Service discovery example
Working example can be found in [indigo_examples/service_ddiscovery.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_examples/service_discovery.c):
```C
#include <stdio.h>
#include <indigo/indigo_service_discovery.h>

/* This function will be called every time a service is resolved by indigo_resolve_service() */
void resolve_callback(const char *service_name, uint32_t interface_index, const char *host, int port) {
	if (host != NULL) {
		printf("= %s -> %s:%u (interface %d)\n", service_name, host, port, interface_index);
	} else {
		fprintf(stderr, "! %s -> service can not be resolved\n", service_name);
	}
}

/* This function will be called every time a service is discovered, removed or at the end of the record */
void discover_callback(indigo_service_discovery_event event, const char *service_name, uint32_t interface_index) {
	switch (event) {
		case INDIGO_SERVICE_ADDED:
			printf("+ %s (interface %d)\n", service_name, interface_index);
			/* we have a new indigo service discovered, we need to resolve it
			   to get the hostname and the port of the service
			*/
			indigo_resolve_service(service_name, interface_index, resolve_callback);
			break;
		case INDIGO_SERVICE_REMOVED:
			/* service is gone indigo_server providing it is stopped */
			printf("- %s (interface %d)\n", service_name, interface_index);
			break;
		case INDIGO_SERVICE_END_OF_RECORD:
			/* these are all the sevises available at the moment,
			   but we will continue listening for updates
			*/
			printf("___end___\n");
			break;
		default:
			break;
	}
}

int main() {
	/* start service browser, the discover_callback() will be called
	   everytime a service is discoveres, removed or end of the record occured
	*/
	indigo_start_service_browser(discover_callback);

	/* give it 10 seconds to work, in a real life
	   it may work while the application is working
	   to get imediate updates on the available services.
	*/
	indigo_sleep(10);

	/* stop the service browser and cleanuo the memory it used,
	   as we do not need it any more
	*/
	indigo_stop_service_browser();
	return 0;
}
```

### Third party libraries that can be used for INDIGO service discovery

- MacOS, Linux & Windows - **[QtZeroConf](https://github.com/jbagg/QtZeroConf)**, uses Qt framework ([INDIGO Control Panel](https://github.com/indigo-astronomy/control-panel) can be used as an example)
- Linux - **[Avahi](https://www.avahi.org/doxygen/html/index.html)**
- MacOS, Windows - **[Bonjour](https://developer.apple.com/bonjour/)**

Dealing with the service discovery is beyond the scope of this document.

## INDIGO Imaging Client - Example

The following code is a working example of INDIGO client that connects to "*indigosky.local:7624*", and uses camera named "**CCD Imager Simulator @ indigosky**" to take 5 exposures, 3 seconds each and save the images in FITS format. Please make sure that **indigo_ccd_simulator** driver is loaded on *indigosky* otherwise the example will not work because it does not check if the camera is present for simplicity.

```C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <indigo/indigo_bus.h>
#include <indigo/indigo_client.h>

#define CCD_SIMULATOR "CCD Imager Simulator @ indigosky"

static bool connected = false;
static int count = 5;

static indigo_result client_attach(indigo_client *client) {
	indigo_log("attached to INDIGO bus...");
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

static indigo_result client_define_property(
	indigo_client *client,
	indigo_device *device,
	indigo_property *property,
	const char *message
) {
	if (strcmp(property->device, CCD_SIMULATOR))
		return INDIGO_OK;
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		if (indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
			connected = true;
			indigo_log("already connected...");
			static const char * items[] = { CCD_EXPOSURE_ITEM_NAME };
			static double values[] = { 3.0 };
			indigo_change_number_property(client, CCD_SIMULATOR, CCD_EXPOSURE_PROPERTY_NAME, 1, items, values);
		} else {
			indigo_device_connect(client, CCD_SIMULATOR);
			return INDIGO_OK;
		}
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
		indigo_change_switch_property(client, CCD_SIMULATOR, CCD_IMAGE_FORMAT_PROPERTY_NAME, 1, items, values);
	}
	return INDIGO_OK;
}

static indigo_result client_update_property(
	indigo_client *client,
	indigo_device *device,
	indigo_property *property,
	const char *message
) {
	if (strcmp(property->device, CCD_SIMULATOR))
		return INDIGO_OK;
	static const char * items[] = { CCD_EXPOSURE_ITEM_NAME };
	static double values[] = { 3.0 };
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
		if (indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
			if (!connected) {
				connected = true;
				indigo_log("connected...");
				indigo_change_number_property(client, CCD_SIMULATOR, CCD_EXPOSURE_PROPERTY_NAME, 1, items, values);
			}
		} else {
			if (connected) {
				indigo_log("disconnected...");
				connected = false;
			}
		}
		return INDIGO_OK;
	}
	if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
		/* URL blob transfer is available only in client - server setup.
		   This will never be called in case of a client loading a driver. */
		if (*property->items[0].blob.url && indigo_populate_http_blob_item(&property->items[0]))
			indigo_log("image URL received (%s, %d bytes)...", property->items[0].blob.url, property->items[0].blob.size);

		if (property->items[0].blob.value) {
			char name[32];
			sprintf(name, "img_%02d.fits", count);
			FILE *f = fopen(name, "wb");
			fwrite(property->items[0].blob.value, property->items[0].blob.size, 1, f);
			fclose(f);
			indigo_log("image saved to %s...", name);
			/* In case we have URL BLOB transfer we need to release the blob ourselves */
			if (*property->items[0].blob.url) {
				free(property->items[0].blob.value);
				property->items[0].blob.value = NULL;
			}
		}
	}
	if (!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME)) {
		if (property->state == INDIGO_BUSY_STATE) {
			indigo_log("exposure %gs...", property->items[0].number.value);
		} else if (property->state == INDIGO_OK_STATE) {
			indigo_log("exposure done...");
			if (--count > 0) {
				indigo_change_number_property(
					client, CCD_SIMULATOR,
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

static indigo_result client_detach(indigo_client *client) {
	indigo_log("detached from INDIGO bus...");
	return INDIGO_OK;
}

static indigo_client client = {
	"Remote server client", false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
	client_attach,
	client_define_property,
	client_update_property,
	NULL,
	NULL,
	client_detach
};

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
#if defined(INDIGO_WINDOWS)
	freopen("indigo.log", "w", stderr);
#endif

	indigo_set_log_level(INDIGO_LOG_INFO);
	indigo_start();

	indigo_server_entry *server;
	indigo_attach_client(&client);
	indigo_connect_server("indigosky", "indigosky.local", 7624, &server); // Check correct host name in 2nd arg!!!
	while (count > 0) {
		  indigo_sleep(1);
	}
	indigo_disconnect_server(server);
	indigo_detach_client(&client);
	indigo_stop();
	return 0;
}
```

## More Client Examples

An open source examples of client API usage are the following pieces of code:

1. [indigo_examples/remote_server_client.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_examples/remote_server_client.c) - Basic API example which takes exposures with the camera

1. [indigo_examples/remote_server_client_mount.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_examples/remote_server_client_mount.c) - Basic API example which unparks and moves the mount

1. [indigo_tools/indigo_prop_tool.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_tools/indigo_prop_tool.c) - Command line property management tool

1. [INDIGO Control Panel](https://github.com/indigo-astronomy/indigo_control_panel) - Official Linux and Windows Control panel using QT framework

1. [PixInsight INDIGO client project](https://github.com/PixInsight/PCL/tree/master/src/modules/processes/contrib/kkretzschmar/INDIClient)
