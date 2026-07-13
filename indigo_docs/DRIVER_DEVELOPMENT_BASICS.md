# Basics of INDIGO Driver Development
Revision: 13.07.2026 (draft)

Author: **Rumen G. Bogdanovski**

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
A **device** should be attached to the **bus** by calling *indigo_attach_device()* and detached with *indigo_detach_device()* call.

There are four functions that can be used by the **device** to send messages to the **client**:
- *indigo_define_property()* - defines a new property
- *indigo_update_property()* - one or more item values of the property are changed
- *indigo_delete_property()* - property is not needed any more and shall not be used
- *indigo_send_message()*  - broadcast a human readable text message

Calls *indigo_define_property()*, *indigo_update_property()* and *indigo_delete_property()* can also send an optional human readable text message associated with the event. A common use for this message is in case of failure. For example, if the client request fails, the driver must set the property state to indicate error (ALERT state) and optionally can send the reason for the failure as a text message along with the update.

Properties and items within the driver are defined with a group of functions:
- *indigo_init_XXX_property()*
- *indigo_init_XXX_item()*

and released with:
- *indigo_release_property()*

For structure definitions and function prototypes please refer to [indigo_bus.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_bus.h) and [indigo_driver.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_driver.h).

## The INDIGO 3.0 (indigo3) API

INDIGO 3.0 raised the API version to **3.0** and introduced several framework wide changes that a driver author should be aware of. The API stays backward source compatible with the vast majority of INDIGO 2.0 drivers - existing drivers keep building and working - but new drivers should be written against the 3.0 additions described below. The most important ones are:

- **Unified (portable) I/O layer** - a single, platform independent I/O abstraction ([indigo_uni_io.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_uni_io.h)) that replaces the direct use of platform specific socket, serial and file calls. It is what makes the same driver source build and run on Linux, macOS **and Windows**. See [Communication with the Hardware](#communication-with-the-hardware).
- **Per device asynchronous handler queues with priorities** - instead of handling every property change on the bus thread (or spawning ad-hoc threads), each device owns a serialized task queue on which change handlers, finalizers and polling callbacks run in the background. Tasks can be scheduled with a priority so that time critical operations (guiding, abort) overtake ordinary ones. See [Timers, Handler Queues and Prolonged Operations](#timers-handler-queues-and-prolonged-operations).
- **Property state in messages** - `indigo_define_property()` and `indigo_update_property()` now carry the property state to the client together with the optional human readable message, so a client learns about a state change and its reason in a single event.
- **Driver code generator** - most base drivers are now produced by a code generator from a compact device description. The generator emits exactly the 3.0 style code shown in this document (uni I/O + handler queues). Migration is documented in [DRIVER_GENERATOR_MIGRATION.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/DRIVER_GENERATOR_MIGRATION.md). Understanding the API in this chapter is a prerequisite for understanding the generated code.

Two convenience macros used pervasively by 3.0 style drivers are worth introducing early, they are defined in [indigo_bus.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_bus.h):

- *INDIGO_UPDATE_PROPERTY_STATE(property, state, message, ...)* - set the property state and notify the clients in one step (optionally with a `printf` style message).
- *INDIGO_COPY_VALUES_PROCESS_CHANGE(property, handler)* - the canonical body of a **change property** branch: it copies the requested values into the property, sets it to <span style="color:orange">**BUSY**</span>, notifies the clients and schedules *handler* on the device queue - but only if the property is not already <span style="color:orange">**BUSY**</span> (which protects against overlapping requests). A `..._ANYTIME` variant omits the busy guard and `..._SYNC_CHANGE`/`..._TARGETS_...` variants run the handler synchronously or copy targets instead of values.

## Properties

Standard property names are defined in [indigo_names.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_names.h)

### Property States, Types and Permissions

Properties can be in one of the four states:
- *INDIGO_IDLE_STATE* - the values may not be initialized
- *INDIGO_OK_STATE* - the property item values are valid and it is safe to read or set them
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

The properties have a *hidden* flag, if set to *true* the property will not be enumerated, This is useful for non mandatory standard properties which are defined in the base device class but not applicable for some specific device. There are many examples for this in the INDIGO driver base.

### Property Life Cycle

In general the life cycle of a property is:
1. *indigo_init_XXX_property()*, *indigo_init_XXX_item()* ... *indigo_init_XXX_item()* - allocate resources and initialize the property
2. *indigo_define_property()* - notify clients of a new property
3. *indigo_update_property()* - notify clients of a value change or the state is changed etc.
4. *indigo_delete_property()* - notify clients that the property can not be used any more
5. *indigo_release_property()* - release the driver resources used by the propriety

### State Descriptions and State Transitions

As mentioned above properties can be in one of the 4 states: <span style="color:grey">**IDLE**</span>, <span style="color:green">**OK**</span>, <span style="color:orange">**BUSY**</span> and <span style="color:red">**ALERT**</span>.

State <span style="color:grey">**IDLE**</span> is rarely used, it is intended for properties that are not always applicable. For example some focusers have temperature sensor that can be plugged and unplugged while device is connected. If the sensor is plugged and gives readings the property showing the temperature must be <span style="color:green">**OK**</span>, and if the the sensor is unplugged it must be in <span style="color:grey">**IDLE**</span> state.
In this state the property data is not reliable.

State <span style="color:green">**OK**</span> is used when the property is in established stable state without any errors. For example image is retrieved from the camera and can be read by the client. <span style="color:green">**OK**</span> state is the only state when the property data is valid.

State <span style="color:orange">**BUSY**</span> is used for lengthy operations still in progress. For example if the exposure is in progress, the image data property must be <span style="color:orange">**BUSY**</span>. In this state the property data can be valid but it is transient. Good example for this is a mount slewing to coordinates. The property will contain the current position but it is not final.

State <span style="color:red">**ALERT**</span> indicates an error. For example image download from the camera failed. In this state the property data must be considered garbage.

Pretty much all possible state transitions are permitted in INDIGO but the normal and most common state transition is:

- <span style="color:green">**OK**</span> -> <span style="color:orange">**BUSY**</span> -> <span style="color:green">**OK**</span>

If the operation is instant, <span style="color:orange">**BUSY**</span> state can be skipped for example setting a variable:

- <span style="color:green">**OK**</span> -> <span style="color:green">**OK**</span>

However in case of error the state transition can be:

- <span style="color:green">**OK**</span> -> <span style="color:orange">**BUSY**</span> -> <span style="color:red">**ALERT**</span>

And respectively for instant operations:

- <span style="color:green">**OK**</span> -> <span style="color:red">**ALERT**</span>

In case of error recovery the following state transitions are allowed:
- <span style="color:red">**ALERT**</span> -> <span style="color:orange">**BUSY**</span> -> <span style="color:green">**OK**</span>
- <span style="color:red">**ALERT**</span> -> <span style="color:green">**OK**</span>

If the error is persistent on update retry the state transtions can be:
- <span style="color:red">**ALERT**</span> -> <span style="color:orange">**BUSY**</span> -> <span style="color:red">**ALERT**</span>
- <span style="color:red">**ALERT**</span> -> <span style="color:red">**ALERT**</span>

In case of a persistent error on update retry <span style="color:red">**ALERT**</span> -> <span style="color:green">**OK**</span> -> <span style="color:red">**ALERT**</span> is **NOT** permitted!

It is **MANDATORY** to use the property states as intended. The states are used by the client software to determine errors, and when the data in the property is valid. Using property states improperly will result in erratic behavior of the client, like reading wrong data etc.

### Item Value vs Item Target
The items of the numeric INDIGO properties have several fields like *min*, *max* etc. However the most used by the driver are *target* and *value*. If a client requests change of some property item, the requested value should be stored in the *target* filed on the drivers end, and the *value* should be set to the current value (as read from the device). For example if the client requests CCD to be cooled to -20<sup>0</sup>C and the current CCD temperature is +10<sup>0</sup>C the update to the client must have *target* set to -20<sup>0</sup>C and *value* set to +10<sup>0</sup>C. On the other hand when a property update is received from the client both *value* and *target* are set to the requested value. So in general the driver does not need to update the *target*, but only the *value*. Comparing *target* and *value* can be used to determine if the operation is complete, as shown in the Atik filter wheel driver:
```C
/* get the current filter */
libatik_wheel_query(
	PRIVATE_DATA->handle,
	&PRIVATE_DATA->slot_count,
	&PRIVATE_DATA->current_slot
);

/* assign current value to the item value */
WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;

/* if target == value => requested filter is set and
   we change property state to OK
*/
if (WHEEL_SLOT_ITEM->number.value == WHEEL_SLOT_ITEM->number.target) {
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
}
...
/* no matter if we are done or not we let the clients know what is \
   going on with the request.
*/
indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
```

## Types of INDIGO Drivers

There are three types of INDIGO drivers. The build system of INDIGO automatically produces the three of them:
- **dynamic driver** - driver that can be loaded and unloaded at any given time
- **static driver** - this driver can be statically linked in the application it can not be unloaded or loaded however they can be enabled and disabled on the fly.
- **executable driver** - these are standard executable programs that communicate over pipes (*stdin* and *stdout*), **legacy** INDI style drivers that can be loaded on startup and they are supported only for INDI compatibility.

The **dynamic drivers** and the **static drivers** are orders of magnitude more effective, as they use shared memory for communication compared to **executable drivers** which use pipes. Therefore they should always be preferred.

In INDIGO the drivers can be used remotely using the INDIGO server or the **clients** can load them locally which makes the communication even faster sacrificing the network capability. However this is not exactly correct as the **client** can also act as a **server** for the locally connected devices and some remote **client** can also use these devices.

## Anatomy of the INDIGO Driver
### Driver Entry Point
Like every software program the INDIGO drivers have and entry point. As **executable drivers** are standalone programs their entry point is *int main()*, but this is not the case with **dynamic drivers** and **static drivers**. They need to have a function called with the name of the driver for example if the driver is a CCD driver by convention the driver name should start with **indigo_ccd_** flowed by the model or the vendor name or abbreviation for example **indigo_ccd_atik** - the driver for Atik cameras has an entry point *indigo_ccd_atik()* or **indigo_ccd_asi** - the driver for ZWO ASI cameras has an entry point *indigo_ccd_asi()*. The prototype of the driver entry point is:

```C
indigo_result indigo_XXX_YYY(indigo_driver_action action,
	indigo_driver_info *info);
```

It accepts 2 parameters *action* which can be *INDIGO_DRIVER_INIT*, *INDIGO_DRIVER_SHUTDOWN* or *INDIGO_DRIVER_INFO*. The framework calls this function with those parameters at driver loading, unloading and when only driver info is requested. it is a good idea to populate *indigo_driver_info* sructure at every call. Here is an example code how this function may look like for the Atik filter wheel driver(**indigo_wheel_atik**):

```C
indigo_result indigo_wheel_atik(indigo_driver_action action,
	indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(
		info,
		"Atik Filter Wheel",
		__FUNCTION__,
		DRIVER_VERSION,
		false,
		last_action
	);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;

		/* Initialize the exported devices here with indigo_attach_device(),
		   if the driver does not support hot-plug or register hot-plug
		   callback which will be called when device is connected.
		*/

		break;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;

		/* Detach devices if hot-plug is not supported with
		   indigo_detach_device() or deregister the hot-plug callback
		   in case hot-plug is supported and release all driver resources.
		*/

		break;

	case INDIGO_DRIVER_INFO:
		/* info is already initialized at the beginning */
		break;
	}

	return INDIGO_OK;
}
```
One of the few synchronous parts of INDIGO are the driver entry points. They are executed synchronously and if they block or take too long to return, this will block or slowdown the entire INDIGO **bus**. For that reason the driver entry point functions must return immediately. If a prolonged operation should be performed, it must be started in a background thread or process. The easiest way to achieve this is via *indigo_async()* call.

The main function for the **executable driver** is pretty much a boiler plate code that connects to the INDIGO **bus** and calls the driver entry point with *INDIGO_DRIVER_INIT* and when done it calls it again with *INDIGO_DRIVER_SHUTDOWN*:

```C
int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	indigo_client *protocol_adapter = indigo_xml_device_adapter(indigo_stdin_handle, indigo_stdout_handle);
	indigo_start();
	indigo_wheel_atik(INDIGO_DRIVER_INIT, NULL);
	indigo_attach_client(protocol_adapter);
	indigo_xml_parse(NULL, protocol_adapter);
	indigo_wheel_atik(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
	return 0;
}
```

### Device Initialization and Attaching

As mentioned above the devices that the driver will handle should be initialized and attached to the INDIGO **bus**. As indigo is asynchronous the device should register several callbacks to handle several events:
- **device attach** - device is attached to the bus
- **enumerate properties** - client requests enumerate (define) properties
- **change property** - client requests property change
- **enable BLOB** - client requests enableBLOB mode change (BLOBs are explained in [CLIENT_DEVELOPMENT_BASICS.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/CLIENT_DEVELOPMENT_BASICS.md))
- **device detach** - device is detached from the bus

For each of these events a callback should be registered and if ignored the callback shall be set to *NULL*, however there are predefined callbacks for each device class that can be used if the driver does not need to handle this event. Also if the device does not need to handle some event it can be left to the base class handler. It is important to note that if the device registers its own callback it should call the base class callback for all not handled cases.
The following example illustrates this:

```C
static indigo_result atik_wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result atik_wheel_change_property(indigo_device *device,
	indigo_client *client, indigo_property *property) {

	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// ---------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {

			... // connect device here

			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			// disconnect device here
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
		// do not return here, let the baseclass callback do its job...
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		// ---------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);

		... // change wheel slot here

		// notify the clients about the property update
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		// we fully handled the situation so we can return
		return INDIGO_OK;
		// ---------------------------------------------------------------------
	}
	// let the base calss do its job
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result atik_wheel_detach(indigo_device *device) {
	assert(device != NULL);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}
```

In the example above we handle only  **device attach**, **change property** and **device detach**. Since we do not have any custom properties, we will rely on the base class handler for the property enumeration; we also do not have any BLOBs so we will ignore them.
We need to provide our callbacks in the *indigo_device* structure. There is an initializer macro, where we also provide the device name:

```C
static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
	"ATIK Filter Wheel",
	atik_wheel_attach,
	indigo_wheel_enumerate_properties,
	atik_wheel_change_property,
	NULL,
	atik_wheel_detach
);
```

Now as we have everything set up we need to attach our device. Here is an example code to do this:

```C
device = malloc(sizeof(indigo_device));
assert(device != NULL);
memcpy(device, &wheel_template, sizeof(indigo_device));

... /* if the driver has private data here
       device->private_data should be initialized too
    */
indigo_attach_device(device);
```

And once we are done with the device we need release all the resources:

```C
indigo_detach_device(device);
... /* release private data if any with free(device->private_data); */
free(device);
device = NULL;
```

Attaching and detaching of the devices can be done either in the driver entry point for not hot-plug devices or in the hot-plug callback for hot-plug devices.
There are many examples for that in the available INDIGO drivers.

Please note, function *indigo_property_match()* is used to match properties against the requests. This function will match read only (RO), read/write (RW) and write only (WO) properties. However if the property changes its permissions, like in the case when two cameras are handled by the same driver and one has temperature control and second has only temperature sensor for the second camera CCD_TEMPERATURE property should be RO and for the first RW. It is driver's responsibility to enforce property permissions and for that case INDIGO 2.0-136 provides a second function that matches properties if they are not RO - *indigo_property_match_changeable()*. Since INDIGO 2.0-180 there is also *indigo_property_match_writable()* which matches only writable properties changed since INDIGO 2.0-179 to *indigo_property_match_changeable()* which matches defined and writable properties.

There is one important note, in order to use the property macros like *CONNECTION_PROPERTY*, *WHEEL_SLOT_PROPERTY* or items like *CONNECTION_CONNECTED_ITEM* the parameter names of the callbacks must be *device*, *client* and *property*. These macros are defined in the header files of the base classes for convenience.

In case your device needs custom properties there are many device drivers that use such. A good and simple example for this is [indigo_aux_rts](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/aux_rts) driver.

### Timers, Handler Queues and Prolonged Operations

Timers in INDIGO are managed with several calls:

- *indigo_set_timer()* - schedule callback to be called after a certain amount of time. The callback will be executed in a separate thread (prototype changed in INDIGO 2.0-122).
- *indigo_reschedule_timer()* - reschedule already scheduled timer, can be used for recurring operations.
- *indigo_cancel_timer()* - request cancellation of a scheduled timer and return, the timer may finish after the function returned.
- *indigo_cancel_timer_sync()* - request cancellation of a scheduled timer and wait until canceled (introduced in INDIGO 2.0-122).

The timer callback should be a void function that accepts pointer to *indigo_device*. The following function illustrates how to poll the Atik filter wheel until the desired filter is set:

```C
static void wheel_timer_callback(indigo_device *device) {
	libatik_wheel_query(
		PRIVATE_DATA->handle,
		&PRIVATE_DATA->slot_count,
		&PRIVATE_DATA->current_slot
	);

	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (WHEEL_SLOT_ITEM->number.value == WHEEL_SLOT_ITEM->number.target) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_set_timer(device, 0.5, wheel_timer_callback, NULL);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}
```

When the filter change request is sent, the timer can be scheduled to fire after 0.5 seconds by calling *indigo_set_timer()* and start polling:

```C
indigo_set_timer(device, 0.5, wheel_timer_callback, NULL);
```

However there is a better way to use timers that gives more flexibility (like rescheduling and canceling the timer). Preserving the timer object gives this flexibility:

```C
indigo_timer *wheel_timer;
...
	indigo_set_timer(device, 0.5, wheel_timer_callback, &wheel_timer);
```

Using the timer object the above callback can be rewritten by using *indigo_reschedule_timer()*:

```C
static void wheel_timer_callback(indigo_device *device) {
	libatik_wheel_query(
		PRIVATE_DATA->handle,
		&PRIVATE_DATA->slot_count,
		&PRIVATE_DATA->current_slot
	);

	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (WHEEL_SLOT_ITEM->number.value == WHEEL_SLOT_ITEM->number.target) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &wheel_timer);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}
```

And finally the scheduled timer can be canceled by calling:

```C
indigo_cancel_timer(device, &wheel_timer);
```

Rather than using a global timer objects, as shown above, it is a good idea to store them in the device private data. Good example for this is [indigo_wheel_asi.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/wheel_asi/indigo_wheel_asi.c).

As of INDIGO 2.0-122 new call is introduced -  *indigo_cancel_timer_sync()*. This call is useful in an event of device disconnect and device detach to prevent releasing of the device resources before the timer is canceled. It should not be called directly in the **change property** callback, as it may deadlock this thread. So if some timers need to be canceled at disconnect it is a good idea to handle the connection property asynchronously with *indigo_async()*, *indigo_handle_property_async()* or *indigo_set_timer()* (with 0 time delay). There are examples of this in almost every driver.

Blocking or prolonged operations executed in the driver main thread may block the whole INDIGO framework. Because of this they should be executed asynchronously in a separate thread. Asynchronous operations can be executed with:

- *indigo_async()* - execute a function asynchronously in a separate thread.
- *indigo_handle_property_async()* - utility function that provides a convenient way to execute prolonged or blocking property change operations in a separate thread. The callback function accepts the same parameters as the driver's **change property** callback.

#### Asynchronous Handler Queues (INDIGO 3.0)

Spawning a fresh thread (with *indigo_async()*) for every property change works, but it has two drawbacks: threads for the same device may run concurrently and step on each other's hardware access, and there is no ordering or prioritization between requests. INDIGO 3.0 solves this with a **per device handler queue**. Every *indigo_device* owns a serialized queue (the *device->queue* field) served by a single worker thread. Handlers scheduled on that queue run **one at a time, in the background**, in the order they were submitted - so a driver no longer needs its own mutex to serialize hardware access between change handlers, and the bus thread is never blocked.

Handlers are scheduled with the *indigo_execute_handler()* family declared in [indigo_driver.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_driver.h). A handler is an ordinary *indigo_timer_callback* - a `void` function taking an *indigo_device \**:

- *indigo_execute_handler()* - run the handler on the device queue as soon as the worker is free (ASAP).
- *indigo_execute_handler_with_data()* - same, passing an arbitrary `void *` payload to the handler.
- *indigo_execute_handler_in()* - run the handler after a delay (in seconds). This is the idiomatic way to poll: a handler reschedules itself with a small delay until the operation completes.
- *indigo_execute_handler_with_data_in()* - delayed variant carrying a data payload.
- *indigo_cancel_pending_handlers()* - drop all not yet started tasks queued for the device (typically called on disconnect).
- *indigo_cancel_pending_handler()* - drop only the pending tasks bound to a specific handler function.

A typical 3.0 style **change property** callback does no hardware work itself. It validates and copies the request, flips the property to <span style="color:orange">**BUSY**</span>, and defers the actual work to a handler running on the queue. The *INDIGO_COPY_VALUES_PROCESS_CHANGE()* macro introduced earlier encapsulates exactly this pattern:

```C
static void wheel_slot_handler(indigo_device *device) {
	WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
	int slot = (int)WHEEL_SLOT_ITEM->number.target;
	if (slot == PRIVATE_DATA->slot) {
		INDIGO_UPDATE_PROPERTY_STATE(WHEEL_SLOT_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		/* talk to the hardware here - we are on the device queue,
		   so this cannot race with any other handler for this device */
		if (move_to_slot(device, slot)) {
			/* poll for completion by rescheduling on the queue after 0.5 s */
			indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
		} else {
			INDIGO_UPDATE_PROPERTY_STATE(WHEEL_SLOT_PROPERTY, INDIGO_ALERT_STATE, NULL);
		}
	}
}

static indigo_result wheel_change_property(indigo_device *device,
	indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, wheel_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		/* copy values, set BUSY, notify clients and queue wheel_slot_handler */
		INDIGO_COPY_VALUES_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	}
	return indigo_wheel_change_property(device, client, property);
}
```

This is the recommended pattern for all new drivers and is exactly what the driver code generator emits. Because everything for one device runs on a single queue, the connection handler can safely call *indigo_cancel_pending_handlers()* on disconnect to discard any still queued polling before it closes the hardware.

#### Handler Priorities

Because the queue is serialized, an ordinary long running task would delay everything queued after it. That is unacceptable for operations that are time critical - a guiding pulse must fire on time, an **abort** must be honored immediately even while a slew handler is queued. INDIGO 3.0 therefore lets a task be submitted with a **priority**: higher priority tasks are pulled from the queue ahead of lower priority ones. The predefined levels are defined in [indigo_timer.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_timer.h):

| Constant | Value | Intended use |
|---|---|---|
| *INDIGO_TASK_PRIORITY_NORMAL* | 0 | ordinary property changes (the default used by *indigo_execute_handler()*) |
| *INDIGO_TASK_PRIORITY_HIGH* | 5 | tasks that should overtake ordinary work |
| *INDIGO_TASK_PRIORITY_TIME* | 10 | time critical tasks, e.g. guiding pulses |
| *INDIGO_TASK_PRIORITY_URGENT* | 20 | urgent tasks, more urgent than time critical, e.g. abort/stop |

The priority aware variants take the priority as their second argument:

- *indigo_execute_priority_handler()* - run ASAP with the given priority.
- *indigo_execute_priority_handler_with_data()* - same, with a data payload.
- *indigo_execute_priority_handler_in()* - run after a delay with the given priority.
- *indigo_execute_priority_handler_with_data_in()* - delayed variant with a data payload.

Note that a numeric priority may be passed directly. For example the guider on a mount uses an urgent priority so that a pulse guide command and its "stop after N milliseconds" finalizer are never delayed by a queued slew or a property poll:

```C
/* abort must jump the queue ahead of any pending slew handler */
indigo_execute_priority_handler(device, INDIGO_TASK_PRIORITY_URGENT, mount_abort_callback);
...
/* start the pulse now, and schedule its stop exactly after `north` ms,
   both at urgent priority so guiding stays accurate */
indigo_execute_priority_handler(device, INDIGO_TASK_PRIORITY_URGENT, guider_guide_dec_callback);
indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT,
	((double)north) / 1000.0, guider_guide_dec_finish_callback);
```

A driver that needs a queue outside of the standard device queue (for example an auxiliary control channel) can create and manage one directly with *indigo_queue_create()*, *indigo_queue_add()* / *indigo_queue_add_with_data()*, *indigo_queue_remove()* and *indigo_queue_delete()*, all declared in [indigo_timer.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_timer.h). Most drivers never need this - the device queue reached through the *indigo_execute_handler()* family is sufficient.

### Communication with the Hardware

Most of the devices use USB connection, for communicating with them the standard libusb library is used. Lubusb is well documented and will not be covered in this document.

Other devices use serial communication over RS-232 port or over TCP or UDP network protocols. Since INDIGO 3.0 all of these are handled through the **Unified I/O** layer defined in [indigo_uni_io.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_uni_io.h). This layer abstracts serial ports, TCP and UDP sockets, plain files and even HID devices behind a single opaque handle, so that the very same driver source compiles and runs unchanged on Linux, macOS and Windows. New drivers should always use the uni I/O functions rather than raw POSIX/Winsock calls.

> The older *indigo_io.h* functions (*indigo_open_serial()*, *indigo_open_tcp()*, *indigo_read()*, *indigo_write()* ...) still exist for source compatibility, but they are not portable to Windows and should not be used in new code. The uni I/O functions are their portable replacements.

#### The uni I/O handle

Every open connection is represented by an *indigo_uni_handle \** (an opaque pointer), not a raw file descriptor. It internally knows whether it wraps a file, serial port, TCP socket, UDP socket or HID device, and it carries its own logging level and last error. Instead of the traditional integer handle, a driver typically stores this pointer in its private data:

```C
typedef struct {
	indigo_uni_handle *handle;
	...
} device_private_data;
```

When opening a handle you pass a **log level** (one of *INDIGO_LOG_ERROR*, *INDIGO_LOG_INFO*, *INDIGO_LOG_DEBUG*, ...) which controls how the traffic on that handle is logged. For devices that exchange binary (non text) data, OR the level with the *BINARY_LOG* flag so that the log is rendered as a hex dump instead of text.

Open functions:

- *indigo_uni_open_serial()* - open serial communication at 9600-8N1
- *indigo_uni_open_serial_with_speed()* - open serial at a given speed (`XXXX`-8N1)
- *indigo_uni_open_serial_with_config()* - open serial with a configuration string of the form "9600-8N1"
- *indigo_uni_open_client_socket()* / *indigo_uni_client_tcp_socket()* / *indigo_uni_client_udp_socket()* - open a TCP or UDP client socket
- *indigo_uni_open_url()* - open a TCP or UDP connection from a URL (see below)
- *indigo_uni_open_file()* / *indigo_uni_create_file()* - open or create a file
- *indigo_uni_open_hid()* - open a HID device by vendor/product id

Input functions:

- *indigo_uni_read()* - read a fixed number of bytes into a buffer
- *indigo_uni_read_available()* / *indigo_uni_peek_available()* - read (or peek without consuming) whatever is currently available
- *indigo_uni_read_section()* / *indigo_uni_read_section2()* - read up to a terminator character, optionally ignoring some characters, with a timeout
- *indigo_uni_read_line()* - convenience macro reading a line (terminated by `\n`, ignoring `\r\n`)
- *indigo_uni_scanf_line()* - read a formatted line
- *indigo_uni_wait_for_data()* - wait (with a microsecond timeout) until data is available
- *indigo_uni_discard()* - discard all pending input

Output functions:

- *indigo_uni_write()* - write a buffer
- *indigo_uni_printf()* / *indigo_uni_vprintf()* - write a formatted string

Serial line control (*indigo_uni_set_dtr()*, *indigo_uni_set_rts()*, *indigo_uni_set_cts()*), socket options (*indigo_uni_set_socket_read_timeout()*, *indigo_uni_set_socket_write_timeout()*, *indigo_uni_set_socket_nodelay_option()*) and helpers for files, folders, host resolution and gzip (de)compression are also provided - see [indigo_uni_io.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_uni_io.h) for the full list.

Unlike the legacy layer, uni I/O handles **must be closed** with *indigo_uni_close()*, which takes the address of the handle pointer and sets it to `NULL` after closing - so the plain POSIX `close()`/`shutdown()` calls must **not** be used on them:

```C
indigo_uni_close(&PRIVATE_DATA->handle);   /* PRIVATE_DATA->handle becomes NULL */
```

Here is how a small serial driver reads and writes a binary packet using uni I/O (from *indigo_wheel_trutek*):

```C
PRIVATE_DATA->handle = indigo_uni_open_serial(name, INDIGO_LOG_DEBUG | BINARY_LOG);
if (PRIVATE_DATA->handle != NULL) {
	unsigned char buffer[4] = { 0xA5, 0x03, 0x20, 0xA5 + 0x03 + 0x20 };
	if (indigo_uni_write(PRIVATE_DATA->handle, (char *)buffer, 4) == 4 &&
	    indigo_uni_read(PRIVATE_DATA->handle, (char *)buffer, 4) == 4 &&
	    buffer[0] == 0xA5 && buffer[1] == 0x83) {
		/* handshake ok */
	}
}
```

#### Opening serial, TCP and UDP transparently

Some devices can be accessed over a serial port, TCP or UDP. INDIGO follows a URL convention: port names can be prefixed with **tcp://** for TCP connections or **udp://** for UDP connections, or with any other string of the form **xxx://** for a specific protocol; anything without a prefix is considered a serial port. For example drivers like lx200 and nexstar accept **lx200://** and **nexstar://** respectively to indicate a TCP connection. The TCP or UDP port can be specified by a **:port** suffix; if omitted the default port for the device is assumed.

The uni I/O layer resolves all of this for you:
- *indigo_uni_is_url()* - returns true if the name starts with **tcp://**, **udp://** or the given custom prefix
- *indigo_uni_open_url()* - opens the URL as a TCP or UDP connection, applying a default port and a protocol hint

```C
char *name = DEVICE_PORT_ITEM->text.value;
if (!indigo_uni_is_url(name, "nexdome")) {
	/* no known prefix -> treat as a local serial port */
	PRIVATE_DATA->handle = indigo_uni_open_serial(name, INDIGO_LOG_DEBUG);
} else {
	/* tcp://, udp:// or nexdome:// -> network device, default port 8080,
	   TCP unless the URL explicitly uses udp:// */
	PRIVATE_DATA->handle = indigo_uni_open_url(name, 8080, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
}
```

The above considers **tcp://**, **udp://** and **nexdome://** prefixes as network hosts. If the custom prefix argument is `NULL`, only **tcp://** and **udp://** are treated as network devices and anything else is a local serial device. If no port is present in the URL the supplied default (*8080* here) is used. The *protocol_hint* argument (*INDIGO_TCP_HANDLE* or *INDIGO_UDP_HANDLE*) selects the protocol for custom prefixes, while an explicit **udp://** in the URL always forces UDP.

Many hardware vendors provide their own Software Development Kit (SDK) for their products in this case the communication with the devices can be done using the SDK provided by the hardware vendor.

Examples for all types of communication are available in the [INDIGO driver base](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/).

### Conflicting Drivers

In some situations some drivers may interfere with each other like different implementations of the driver for the same hardware. Loading both drivers may result in erratic behavior and both drivers should not be loaded. For that reason INDIGO version 2.0-114 introduces a function that can be used to check if certain drivers are already initialized:

 - *indigo_driver_initialized()* - checks if the specified driver is initialized and returns true if it is.

In the following example the usage of *indigo_driver_initialized()* is illustrated, here *indigo_rotator_lunatico* driver will not load if *indigo_focuser_lunatico* is already loaded. However this will not prevent *indigo_focuser_lunatico* from loading if *indigo_rotator_lunatico* is loaded. For that reason reciprocal actions must be taken in *indigo_focuser_lunatico*.

 ```C
#include <indigo/indigo_client.h>
...
indigo_result indigo_rotator_lunatico(indigo_driver_action action, indigo_driver_info *info) {
	...
	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		if (indigo_driver_initialized("indigo_focuser_lunatico")) {
			INDIGO_DRIVER_LOG(
				DRIVER_NAME,
				"Conflicting driver %s is already loaded", "indigo_focuser_lunatico"
			);
			last_action = INDIGO_DRIVER_SHUTDOWN;
			return INDIGO_FAILED;
		}
		...
		break;
	case INDIGO_DRIVER_SHUTDOWN:
		...
		break;
	case INDIGO_DRIVER_INFO:
		break;
	}
	return INDIGO_OK;
}
 ```

In order to use *indigo_driver_initialized()* the driver must include **indigo_client.h**.

## INDIGO Driver - Example

This example is a working device driver handling Atik Filer Wheels. They are hot-plug devices, therefore the driver attaches and detaches the device in the hot-plug callback function. This driver is chosen as the device is really simple yet supports hot-plug, this way the more complex hot-plug support is illustrated with a simpler code. Another simple driver without a hot-plug is the mentioned above [indigo_aux_rts](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/aux_rts) driver.

The example below is written against the **INDIGO 3.0 API**: it opens the device through the [Unified I/O](#communication-with-the-hardware) layer (*indigo_uni_open_hid()* / *indigo_uni_close()*) and performs every connect, disconnect and filter move on the [device handler queue](#asynchronous-handler-queues-indigo-30) instead of blocking the bus thread. The **change property** callback only validates and copies the request and then defers the real work to a handler. Note that the actual [indigo_wheel_atik.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/wheel_atik/indigo_wheel_atik.c) in the driver base is produced by the driver code generator (see [DRIVER_GENERATOR_MIGRATION.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/DRIVER_GENERATOR_MIGRATION.md)) and therefore contains extra generator markers; the hand written equivalent shown here is functionally identical but easier to read.

### The Atik Filter Wheel driver

Driver header file **indigo_wheel_atik.h**:

```C
/** INDIGO Atik filter wheel driver
 \file indigo_wheel_atik.h
 */

#ifndef wheel_atik_h
#define wheel_atik_h

#include <indigo/indigo_driver.h>
#include <indigo/indigo_wheel_driver.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Register Atik filter wheel hot-plug callback
 */

extern indigo_result indigo_wheel_atik(indigo_driver_action action,
	indigo_driver_info *info);

#ifdef __cplusplus
}
#endif

#endif /* wheel_atik_h */
```

Main driver source file **indigo_wheel_atik.c**:

```C
/** INDIGO Atik filter wheel driver
 \file indigo_wheel_atik.c
 */

#define DRIVER_VERSION 0x03000004
#define DRIVER_NAME "indigo_wheel_atik"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <libatik.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_wheel_atik.h"

// ------------------------------------------- ATIK USB interface implementation

#define ATIK_VENDOR_ID                  0x04D8
#define ATIK_PRODUC_ID                  0x003F

#define PRIVATE_DATA        ((atik_private_data *)device->private_data)

typedef struct {
	indigo_uni_handle *handle;             // unified I/O handle (wraps the HID device)
	int slot_count, current_slot, target_slot;
} atik_private_data;

// ------------------------------------------ low level (unified I/O) helpers

static bool atik_open(indigo_device *device) {
	// open the HID device through the portable unified I/O layer;
	// BINARY_LOG makes the traffic show up as a hex dump in the log
	PRIVATE_DATA->handle = indigo_uni_open_hid(ATIK_VENDOR_ID, ATIK_PRODUC_ID, INDIGO_LOG_DEBUG | BINARY_LOG);
	return PRIVATE_DATA->handle != NULL;
}

static void atik_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);   // closes and sets the handle to NULL
}

// ------------------------------------------ INDIGO Wheel device implementation

// Polling handler: reschedules itself on the device queue until the wheel
// reaches the requested slot, then flips the property to OK.
static void wheel_move_finalizer(indigo_device *device) {
	libatik_wheel_query((hid_device *)PRIVATE_DATA->handle->hid_device, &PRIVATE_DATA->slot_count, &PRIVATE_DATA->current_slot);
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		INDIGO_UPDATE_PROPERTY_STATE(WHEEL_SLOT_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
	}
}

// Connection handler - runs on the device queue, so it may block on I/O.
static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = atik_open(device);
		if (connection_result) {
			connection_result = false;
			for (int i = 0; i < 10; i++) {
				libatik_wheel_query((hid_device *)PRIVATE_DATA->handle->hid_device, &PRIVATE_DATA->slot_count, &PRIVATE_DATA->current_slot);
				if (PRIVATE_DATA->slot_count > 0 && PRIVATE_DATA->slot_count <= 9) {
					connection_result = true;
					break;
				}
				indigo_sleep(1);
			}
			if (connection_result) {
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->slot_count;
				WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot;
			}
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);   // drop any queued move polling
		atik_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	// let the base class finish handling CONNECTION and notify the clients
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

// Filter change handler - runs on the device queue.
static void wheel_slot_handler(indigo_device *device) {
	if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->current_slot) {
		INDIGO_UPDATE_PROPERTY_STATE(WHEEL_SLOT_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		PRIVATE_DATA->target_slot = WHEEL_SLOT_ITEM->number.value;
		libatik_wheel_set((hid_device *)PRIVATE_DATA->handle->hid_device, PRIVATE_DATA->target_slot);
		libatik_wheel_query((hid_device *)PRIVATE_DATA->handle->hid_device, &PRIVATE_DATA->slot_count, &PRIVATE_DATA->current_slot);
		WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
		indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);   // start polling
	}
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_wheel_enumerate_properties(device, client, property);
}

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		WHEEL_SLOT_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

// The change property callback only validates/copies the request and defers
// the actual (blocking) work to a handler running on the device queue.
static indigo_result wheel_change_property(indigo_device *device,
	indigo_client *client, indigo_property *property) {

	assert(device != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// ---------------------------------------------------------- CONNECTION
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, wheel_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		// ---------------------------------------------------------- WHEEL_SLOT
		// copy values, set BUSY, notify the clients and queue wheel_slot_handler
		INDIGO_COPY_VALUES_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);   // synchronous disconnect on detach
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

// --------------------------------------------------------- hot-plug support

static pthread_mutex_t hotplug_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_device *wheel = NULL;

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
	"Atik Filter Wheel",
	wheel_attach,
	wheel_enumerate_properties,
	wheel_change_property,
	NULL,
	wheel_detach
);

static void process_plug_event(libusb_device *dev) {
	pthread_mutex_lock(&hotplug_mutex);
	if (wheel == NULL) {
		char usb_path[INDIGO_NAME_SIZE];
		indigo_get_usb_path(dev, usb_path);
		atik_private_data *private_data = indigo_safe_malloc(sizeof(atik_private_data));
		wheel = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
		snprintf(wheel->name, INDIGO_NAME_SIZE, "%s #%s", "Atik Filter Wheel", usb_path);
		wheel->private_data = private_data;
		indigo_attach_device(wheel);
	}
	pthread_mutex_unlock(&hotplug_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&hotplug_mutex);
	if (wheel != NULL) {
		indigo_detach_device(wheel);
		free(wheel->private_data);
		free(wheel);
		wheel = NULL;
	}
	pthread_mutex_unlock(&hotplug_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev,
	libusb_hotplug_event event, void *user_data) {

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			// attach asynchronously so we do not block the USB event thread
			INDIGO_ASYNC(process_plug_event, dev);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			process_unplug_event(dev);
			break;
		}
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_wheel_atik(indigo_driver_action action,
	indigo_driver_info *info) {

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Atik Filter Wheel", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		wheel = NULL;
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(
			NULL,
			LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED|LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
			LIBUSB_HOTPLUG_ENUMERATE,
			ATIK_VENDOR_ID,
			ATIK_PRODUC_ID,
			LIBUSB_HOTPLUG_MATCH_ANY,
			hotplug_callback,
			NULL,
			&callback_handle
		);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		VERIFY_NOT_CONNECTED(wheel);   // refuse to unload while still connected
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		process_unplug_event(NULL);
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
```

File containg main fuction needed only for the executable driver  **indigo_wheel_atik_main.c**:

```C
/** INDIGO Atik filter wheel driver main
 \file indigo_wheel_atik_main.c
 */

#include <stdio.h>
#include <string.h>

#include <indigo/indigo_driver_xml.h>
#include "indigo_wheel_atik.h"

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	indigo_client *protocol_adapter = indigo_xml_device_adapter(indigo_stdin_handle, indigo_stdout_handle);
	indigo_start();
	indigo_wheel_atik(INDIGO_DRIVER_INIT, NULL);
	indigo_attach_client(protocol_adapter);
	indigo_xml_parse(NULL, protocol_adapter);
	indigo_wheel_atik(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
	return 0;
}
```

## More Driver Examples

An open source examples of driver API usage can be found in INDIGO driver libraries:

1. [INDIGO common drivers](https://github.com/indigo-astronomy/indigo/tree/master/indigo_drivers)
1. [INDIGO Linux specific drivers](https://github.com/indigo-astronomy/indigo/tree/master/indigo_linux_drivers)
1. [INDIGO MacOS specific drivers](https://github.com/indigo-astronomy/indigo/tree/master/indigo_mac_drivers)
1. [INDIGO optional drivers](https://github.com/indigo-astronomy/indigo/tree/master/indigo_optional_drivers)
