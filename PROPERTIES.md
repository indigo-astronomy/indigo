# INDIGO protocols

## Introduction

INDIGO properties and items are abstraction of INDI properties and items. As far as INDIGO uses software bus instead of XML messages,
properties are first of all defined memory structures wich are, if needed, mapped to XML or JSON textual representation.

## Common properties

| Property |  |  |  |  | Items |  |  | Comments |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Name | Legacy mapping | Type | RO | Required | Name | Legacy mapping | Required | |
| CONNECTION | CONNECTION | switch | no | yes | CONNECTED | CONNECT | yes | Item values are undefined if state is not Idle or Ok. |
|  |  |  |  |  | DISCONNECTED | DISCONNECT | yes | |
| INFO | DRIVER_INFO | text | yes | yes | DEVICE_NAME | DRIVER_NAME | yes | "Device in INDIGO strictly represents device itself and not device driver. Valid DEVICE_INTERFACE values are defined in indigo_driver.h as indigo_device_interface enumeration." |
|  |  |  |  |  | DEVICE_VERSION | DRIVER_VERSION | yes |  |
|  |  |  |  |  | DEVICE_INTERFACE | DRIVER_INTERFACE | yes |  |
|  |  |  |  |  | FRAMEWORK_NAME |  | no |  |
|  |  |  |  |  | FRAMEWORK_VERSION |  | no | | 
|  |  |  |  |  | DEVICE_MODEL |  | no |  |
|  |  |  |  |  | DEVICE_FIRMWARE_REVISION |  | no |  |
|  |  |  |  |  | DEVICE_HARDWARE_REVISION |  | no |  |
|  |  |  |  |  | DEVICE_SERIAL_NUMBER |  | no |  |
| SIMULATION | SIMULATION | switch | no | no | ENABLED | ENABLE | yes |  |
|  |  |  |  |  | DISABLED | DISABLE | yes |  |
| CONFIG | CONFIG_PROCESS | switch | no | yes | LOAD | CONFIG_LOAD | yes | DEFAULT is not implemented by INDIGO yet. |
|  |  |  |  |  | SAVE | CONFIG_SAVE | yes |  |
|  |  |  |  |  | DEFAULT | CONFIG_DEFAULT | yes |  |
| PROFILE |  | switch | no | yes | PROFILE_0,... |  | yes | Select the profile number for subsequent CONFIG operation |
| DEVICE_PORT | DEVICE_PORT | text | no | no | PORT | PORT | no | Either device path like "/dev/tty0" or url like "lx200://host:port". |
| DEVICE_PORTS |  | switch | no | no | valid serial port name |  |  | When selected, it is copied to DEVICE_PORT property. |

Properties are implemented by driver base class in [indigo_libs/indigo_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_driver.c).
