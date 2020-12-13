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

The following script is an example how to register even handlers for define, update, delete and message events with property objects cached within high level API implementation:

```
indigo_event_handlers.my_handler = {
  devices: [ "GPS Simulator" ],

  on_define: function(property) {
    indigo_log_with_property("Defined ", property);  
  },
  
  on_update: function(property) {
    indigo_log_with_property("Updated ", property);  
  },
  
  on_delete: function(property) {
    indigo_log_with_property("Deleted ", property);  
  },
  
  on_message: function(message) {
    indigo_log_with_property("Message ", message);  
  }
};

```

The following script is an example how to unregister even handlers for define, update, delete and message events:

```
delete indigo_event_handlers.my_handler;
```
