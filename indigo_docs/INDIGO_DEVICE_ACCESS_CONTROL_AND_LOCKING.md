# INDIGO Device Access Control and Exclusive Lock System
Revision: 06.04.2020 (draft)

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

**IMPORTANT: This functionality is still considered experimental and may be a subject to changes.**

## Introduction

As of version 2.0-120 INDIGO provides device access control and exclusive locking system based on tokens. INDIGO access control and locking are not meant to be security features. Both only limit clients' ability to modify the properties of certain devices. In other words some clients can control the devices, but all can monitor property states and values.

## Device Access
In INDIGO server the devices can be in one of the three access states:
- **Protected** - devices that can be controlled with a token
- **Public** - devices that are publicly controllable
- **Locked** - devices that are locked by some client (temporarily protected).

### Protected Devices
If the device has **device token** set on the server, this device is controllable only by the clients, who have this token or have the server **master token**. Note that the server must have **master token** set, otherwise **device tokens** will have no effect. Protected devices can not be locked exclusively by the clients.

### Public Devices
This is the default access for the INDIGO device. If there is no **device token** set on the server this device is controllable by every client. However every client can claim exclusive control access of the public device.

### Locked Devices
These are public devices that are locked by some client with a client specified token. The lock is obtained by setting the *token* attribute in the device connect property request (see [PROTOCOLS.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/PROTOCOLS.md)) and released when the device is disconnected. The server must have **master token** set, otherwise exclusive lock will have no effect. On the other hand the exclusive lock can be overridden by using the server **master token**.


## Tokens
There are two types of tokens in INDIGO that have slightly different meaning on the client and the server:

- **Master tokens**
- **Device tokens**

### Master Tokens
**Master token** is used differently by the client and the server.

On the server **master token** is used to override the device locks in two cases:
- To unlock devices **locked** by failed clients.
- To override the lock of the devices by clients with higher priority tasks.

It is important to note that if the server has no **master token** set, **public devices** can not be locked and no devices can be **protected**, even if they have **device token** set. In other words to enable access control on the server, it must have **master token** set.

On the client the **master token** has no strict dedication. It can be used in the following situations:
- To get exclusive lock of the **public devices** which do not have **device tokens** set on the client.
- To get high priority access to the devices on the server, if it matches the server **master token**. Any client that has the server **master token** can override any exclusive locks or device protection and can control any device.

### Device Tokens
**Device tokens** are shared between the server and the client. The client must know the particular device token, set on the server, to be able to control this device, unless the client has the server **master token**.

It is important to note that **protected devices** can not be exclusively locked. Only **public devices** can be locked at connect, if the connection request has *token* attribute set. This will set temporary **device token** on the server which will be valid until the device is disconnected.

If the device is public on the server and has a **device token** set on the client, the client will lock the device upon connect with this token.

## INDIGO Device Access Control List

INDIGO uses internal device access control list (ACL) to manage device access. Each property change request must have the *token* attribute correctly set to access **protected** or **locked** devices. This is done automatically by the INDIGO framework. First the framework looks up the **device token** in the client device ACL and if found it is used. If not found, it looks if there is a **master token** set on the client and uses it. If none of the two tokens are found, it sends basic request (without *token* attribute set) hoping that the device is neither **protected** nor **locked**.

On the client the device ACL is like an internal password storage, that will be automatically used by the framework, to request access to a specific device. However on the server it has a different meaning. The server device ACL is the list of tokens against which the client provided token will be verified and if they match the access will be granted.

## INDIGO Device Access Control List API:

INDIGO supports internal token based device ACL that can be handled by several calls. As mentioned above servers and clients have separate device ACLs but they are handled by the same framework functions:

- *indigo_add_device_token(device_name, token)* - add device and token to the DACL. If the device exists, the token will be updated.

- *indigo_remove_device_token(device_name)* - remove the device from the DACL. On server it means that the device will not be protected.

- *indigo_get_device_token(device_name)* - get the device token from the DACL. If not set, returns 0.

- *indigo_get_device_or_master_token(device_name)* - get the device token from DACL if set. If not, get the master token. If none of the two is set return 0.

- *indigo_get_master_token()* - get the master token if set. Otherwise returns 0.

- *indigo_set_master_token(token)* - set master token.

- *indigo_clear_device_tokens()* - clear the whole device ACL

- *indigo_load_device_tokens_from_file(file_name)* - Load device ACL from file. Existing list will not be removed, read tokens will be added to the list (or updated if exist).

- *indigo_save_device_tokens_to_file(file_name)* - save device ACL to file.

The formal function declarations are available in [indigo_token.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_token.h)

### Usage Examples

Example of functions usage can be found in *indigo_server*, *indigo_prop_tool* or *indigo_control_panel* source code.

1. [indigo_server/indigo_server.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_server/indigo_server.c) - the INDIGO server

1. [indigo_tools/indigo_prop_tool.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_tools/indigo_prop_tool.c) - Command line property management tool

1. [INDIGO Control Panel](https://github.com/indigo-astronomy/control-panel) - Official Linux and Windows Control panel using QT framework

## Device Access Control File Format
The device ACL file has a very simple syntax. Lines starting with '#' or empty lines are ignored.
All other lines contain two fields separated by space:
- *token* - hexadecimal token
- *Device name* - a string that may have spaces

Special cases of device names:
- '@' - a special device name meaning that this is the master token of the server or the client.
- '@ servername' - can be used on the client to specify the master token used for authentication on the specified server.
- 'Device @ hostname' - remote devices should be specified with device name and host name

Client ACL file example:
```
# Client master token
# <token> @
12345678 @

# Master token to be used for devices on indigosky host
# <token> <@ host>
12FA0101 @ indigosky

# Some Remote Device Tokens
# <token> <device name @ host>
12FA3213 Dome Dragonfly @ indigosky
12FA3213 Dragonfly Controller @ indigosky
```

Server ACL file example:
```
# Server master token
12FA0101 @

# Some Devices
12FA3213 Dome Dragonfly
12FA3213 Dragonfly Controller
```

The proposed device ACL file name extension is '.idac' standing for 'INDIGO Device Access Control'
