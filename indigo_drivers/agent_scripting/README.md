# Scripting agent

Backend implementation of INDIGO scripting based on Duktape engine (https://duktape.org).

## Supported devices

N/A

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_scripting ...

## Status: Under development

## Low level API (ECMAScript <-> INDIGO bindings)

The following low level INDIGO functions can be called from the script: 

```
function indigo_error(message)
function indigo_log(message)
function indigo_debug(message)
function indigo_trace(message)
function indigo_send_message(message)
function indigo_enumerate_properties(device_name, property_name)
function indigo_enable_blob(device_name, property_name, state)
function indigo_change_text_property(device_name, property_name, items)
function indigo_change_number_property(device_name, property_name, items)
function indigo_change_switch_property(device_name, property_name, items)
```

where ``message`` is any string, ``device`` is device name, ``property`` is property name,  ``items`` is dictionary with item name/value pairs.

The following low level callback functions are called (if present) from the INDIGO: 

```
function indigo_on_define_property(device_name, property_name, items, state, perm, message)
function indigo_on_update_property(device_name, property_name, items, state, message)
function indigo_on_delete_property(device_name, property_name, message)
function indigo_on_send_message(device_name, message)
```

where ``device`` is device name, ``property`` is property name,  ``items`` is dictionary with item name/value pairs, ``state`` is "Idle"/"Ok"/"Busy"/"Alert" string, ``perm`` is "RW"/"RO"/"WO" string and ``message`` is any string.

The following script is executed on agent load and later will contain high level API definition: [boot.js](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/agent_scripting/boot.js)

## High level API examples

The following script is an example how to register even handlers for define, update, delete and message events with property objects cached within high level API implementation and manipulate the properties.

```
// The example script:
// - load indigo_gps_simulator driver if not loaded,
// - connect "GPS Simulator" device,
// - wait GEOGRAPHIC_COORDINATES property to become OK and sends,
//   LATITUDE and LONGITUDE as a message to the client,
// - disconnect the device.

// Register the event handler and get notifications for "GPS Simulator" properties only.

indigo_event_handlers.my_handler = {
  devices: [ "GPS Simulator" ],

// Define process_coordinates method which in case if property is in OK state
// sends its LATITUDE and LONGITUDE item values to the client as the message
// and then disconnect the device.

  process_coordinates: function(property) {
    if (property.state == "Ok") {
      indigo_send_message("Coordinates " + property.items.LATITUDE + " " + property.items.LONGITUDE);
      indigo_devices["GPS Simulator"].CONNECTION.change({ DISCONNECTED: true });
    }
  },

// Define handler called when the property is defined.
// Dump the property, if the property is named CONNECTION, set it CONNECTED item
// to true (connect the device), if the property is named GEOGRAPHIC_COORDINATES
// call process_coordinates method to process it.

  on_define: function(property) { 
    indigo_log_with_property("Defined ", property);
    if (property.name=='CONNECTION') {
      property.change({ CONNECTED: true });
    } else if (property.name=='GEOGRAPHIC_COORDINATES') {
      this.process_coordinates(property);
    }
  },
  
// Define handler called when the property is updated.
// Dump the property, if the property is named GEOGRAPHIC_COORDINATES
// call process_coordinates method to process it.

  on_update: function(property) { 
    indigo_log_with_property("Updated ", property); 
    if (property.name=='GEOGRAPHIC_COORDINATES') {
      this.process_coordinates(property);
    }
  },
  
// Define handler called when the property is deleted.
// Dump the property.

  on_delete: function(property) {
    indigo_log_with_property("Deleted ", property); 
  },
  
// Define handler called when the message is received.
// Write the message to server log.

  on_message: function(message) {
    indigo_log_with_property("Message ", message);
  }
};

// if indigo_gps_simulator driver is not loaded, load it.
// Otherwise force server to reenumerate (define) all properties.

if (!indigo_devices["Server"].DRIVERS.items.indigo_gps_simulator) {
  indigo_devices["Server"].DRIVERS.change({ indigo_gps_simulator: true });
} else {
  indigo_enumerate_properties();
}
```

The following script is an example how to unregister even handlers for define, update, delete and message events:

```
delete indigo_event_handlers.my_handler;
```
