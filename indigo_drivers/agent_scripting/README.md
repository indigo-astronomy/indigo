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

## ECMAScript > INDIGO bindings

The following low level INDIGO functions can be called from the script: 

```
function indigo_error(message)
function indigo_log(message)
function indigo_debug(message)
function indigo_trace(message)
function indigo_send_message(message)
function indigo_enumerate_properties(device, property)
function indigo_enable_blob(device, property, state)
function indigo_change_text_property(device, property, items)
function indigo_change_number_property(device, property, items)
function indigo_change_switch_property(device, property, items)
```

where ```message``` is any string, ```device``` is device name, ```property``` is property name,  ```items``` is dictionary with item name/value pairs.

The following low level callback functions are called (if present) from the INDIGO: 

```
function indigo_define_property(device, property, items, state, perm, message)
function indigo_update_property(device, property, items, state, message)
function indigo_delete_property(device, property, message)
function indigo_send_message(device, message)
```

where ````device``` is device name, ```property``` is property name,  ```items``` is dictionary with item name/value pairs, ```state``` is "Idle"/"Ok"/"Busy"/"Alert" string, ```perm``` is "RW"/"RO"/"WO" string and ``message``` is any string.
