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
indigo_error(message)
indigo_log(message)
indigo_debug(message)
indigo_trace(message)
indigo_send_message(message)
indigo_enumerate_properties(device, property)
indigo_enable_blob(device, property, state)
indigo_change_text_property(device, property, { name: value, ... })
indigo_change_number_property(device, property, { name: value, ... })
indigo_change_switch_property(device, property, { name: value, ... })
```

where ```message``` is any string, ```device``` is device name, ```property``` is property name, ```name``` is item name and ```value``` is item value.
