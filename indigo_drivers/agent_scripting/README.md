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
function indigo_save_blob(file_name, blob_item)
function indigo_populate_blob(blob_item)
function indigo_enumerate_properties(device_name, property_name)
function indigo_enable_blob(device_name, property_name, state)
function indigo_change_text_property(device_name, property_name, items)
function indigo_change_number_property(device_name, property_name, items)
function indigo_change_switch_property(device_name, property_name, items)
function indigo_define_text_property(device_name, property_name, property_group, property_label, items, item_defs, state, perm, message)
function indigo_define_number_property(device_name, property_name, property_group, property_label, items, item_defs, state, perm, message)
function indigo_define_switch_property(device_name, property_name, property_group, property_label, items, item_defs, state, perm, rule, message)
function indigo_define_light_property(device_name, property_name, property_group, property_label, items, item_defs, state, message)
function indigo_update_text_property(device_name, property_name, items, state, message)
function indigo_update_number_property(device_name, property_name, items, state, message)
function indigo_update_switch_property(device_name, property_name, items, state, message)
function indigo_update_light_property(device_name, property_name, items, state, message)
function indigo_delete_property(device_name, property_name, message)
function indigo_dtos(value, format);
function indigo_stod(value);
function indigo_set_timer(function, delay);
function indigo_cancel_timer(timer);
function indigo_utc_to_time(utc);
function indigo_utc_to_delay(utc);
function indigo_time_to_delay(time);
function indigo_delay_to_utc(delay);
function indigo_time_to_utc(time);
```

where ``message`` is any string, ``device`` is device name, ``property`` is property name,  ``items`` is dictionary with item name/value pairs.

The following low level callback functions are called (if present) from the INDIGO: 

```
function indigo_on_define_property(device_name, property_name, items, item_defs, state, perm, message)
function indigo_on_update_property(device_name, property_name, items, state, message)
function indigo_on_delete_property(device_name, property_name, message)
function indigo_on_send_message(device_name, message)
function indigo_on_enumerate_properties(device_name, property_name)
function indigo_on_change_property(device_name, property_name, items, state)
```

where ``device`` is device name, ``property`` is property name,  ``items`` is dictionary with item name/value pairs, ``state`` is "Idle"/"Ok"/"Busy"/"Alert" string, ``perm`` is "RW"/"RO"/"WO" string and ``message`` is any string.

The following script is executed on agent load and later will contain high level API definition: [boot.js](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/agent_scripting/boot.js)

## High level API examples

The following script is an example how to register even handlers with property objects cached within high level API implementation and manipulate the properties.

```
// The example script:
// - define MY_COORDS property in "My GPS" group and in "Busy" state on "Scripting Agent" 
// - load indigo_gps_simulator driver if not loaded,
// - connect "GPS Simulator" device,
// - wait GEOGRAPHIC_COORDINATES property to become OK and sends,
// - set LATITUDE and LONGITUDE and update MY_COORDS in "Ok" state,
// - disconnect the device.

// Register the event handler and get notifications for "GPS Simulator" properties only.

indigo_event_handlers.my_handler = {
  devices: [ "GPS Simulator" ],

// Coordinate values for MY_COORDS property

  latitude: 0,
  longitude: 0,

// Define process_coordinates method which in case if property is in OK state
// sends its LATITUDE and LONGITUDE item values to the client as MY_COORDS property update
// and then disconnect the device.

  process_coordinates: function(property) {
    if (property.state == "Ok") {
      this.latitude = property.items.LATITUDE;
      this.longitude = property.items.LONGITUDE;
      indigo_update_number_property("Scripting Agent", "MY_COORDS", { LATITUDE: this.latitude, LONGITUDE: this.longitude }, "Ok", null);
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
  
  // When queried, define MY_COORDS property.

  on_enumerate_properties: function(property) {
    if (property.device == "Scripting Agent") {
      if (property.name == null || property.name == "MY_COORDS") {
        indigo_define_number_property("Scripting Agent", "MY_COORDS", "My GPS", "Coordinates", { LATITUDE: this.latitude, LONGITUDE: this.longitude }, { LATITUDE: { label: "Latitude", format: "%m", min: -90, max: 90 }, LONGITUDE: { label: "Longitude", format: "%m", min:-180, max: 360 }}, "Busy", "RO", null);
      }
    }
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
indigo_delete_property("Scripting Agent", "MY_COORDS", null);
```

The following script is an example how to schedule function execution with INDIGO timers

```
var timer;

function timer_event_handler() {
  indigo_log('timer_event_handler() excuted');
}

timer = indigo_set_timer(timer_event_handler, 1);
```

and the following script is an example how to cancel  function scheduled by the previous example

```
indigo_cancel_timer(timer);
```
