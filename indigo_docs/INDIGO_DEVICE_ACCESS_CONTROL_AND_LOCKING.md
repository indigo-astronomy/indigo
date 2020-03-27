# INDIGO Device Access Control and Exclusive Lock System
Revision: 27.03.2020 (draft)

Author: **Rumen G.Bogdanovski**

e-mail: *rumen@skyarchive.org*

## Introduction

As of version 2.0-120 INDIGO provides device access control and exclusive locking system based on tokens. INDIGO access control and locking are not meant to be security features.
Both only limit clients' ability to modify the properties of certain devices. In other words device control them, but the property states and values can be monitored by all clients.

## Device Access
In INDIGO server the devices can be in one of the three access states:
- **Protected** - devices that are protected by token on the server.
- **Public** - devices that are publicly controllable.
- **Locked** - device that is locked by some client (temporarily protected).

### Protected Devices
If the device has **device token** set on the server this device is controllable only by the clients that have the this token or have the server **master token**. Note that the server must have **master token** set otherwise **device tokens** will have no effect. Protected devices can not be locked exclusively by the clients.

### Public Devices
This is the default access for the INDIGO device. If there is no **device token** set on the server this device is controllable by every client. However every client can claim exclusive control access of the Public device.

### Locked Devices
These are public devices that are locked by some client with a client specified token. The lock is obtained by setting the token option in the device connect property request and released when the device is disconnected.


## Tokens
There are two types of tokens in INDIGO that have slightly different meaning on the client and the server:

- **Master tokens**
- **Device tokens**

### Master Tokens
**Master tokens** are used differently by the client and the server.

On the server **master token** is used to override the device locks in 2 cases:
- To unlock devices **locked** by failed clients.
- To override the lock of the devices by clients with higher priority tasks.

It is important to note that if the server has no **master token** set, **public devices** can not be locked and no devices can be **protected** even if they have **device token** set. In other words to enable access control on the server it must have **master token** set.

On the client the **master token** can be used in the following situations:
- To get exclusive lock of the **public devices** that do not have **device tokens** set in the client.
- To get high priority access to the devices on the server if it matches the server **master token**. Any client that has the server **master token** can override any exclusive locks or device protection and can control any device.

### Device Tokens
**Device tokens** are shared between the server and the client. The client must know the particular device token, set at the server, to be able to control this device, unless it has the **master token**.

It is important to note that **protected devices** can not be exclusively locked. Only **public devices** can be locked at connect, if the connection request has **token** set. This will set temporary **device token** on the server that will be valid until the device is disconnected.

## INDIGO Access Control List

INDIGO uses internal Access Control List (ACL) to manage device access. Each property change request must have the **token** option correctly set to access **protected** or **locked** devices. This is done automatically by the INDIGO framework. First the framework looks up the **device token** in the client ACL and if found it is used. If not found it looks if there is a **master token** set on the client and uses it. If none of the 2 tokens are found it sends basic request hoping that the device is neither **protected** nor **locked**.

On the client the ACL is like an internal password storage that will be automatically used bu the framework, to request access to a specific device. However on the server it has a different meaning. The server ACL is the list of tokens against which the client provided token will be verified and if they match the access will be granted.

## INDIGO Access Control List API:

INDIGO supports internal token based ACL that can be handled by several calls. As mentioned above servers and clients have separate ACLs but they are handled by the same framework functions:

- *indigo_add_device_token(device_name, token)* - add device and token to the ACL

- *indigo_remove_device_token(device_name)* - remove the device from the ACL, on server it means that the device will not be protected.

- *indigo_get_device_token(device_name)* - get the device token from the ACL, if not set 0 is returned.

- *indigo_get_device_or_master_token(device_name)* - get device token if set, if not get the master token or 0 if none of the two is set.

- *indigo_get_master_token()* - get master token if set or 0 if not.

- *indigo_set_master_token(token)* - set master token.

- *indigo_clear_device_tokens()* - clear the whole ACL

- *indigo_load_device_tokens_from_file(file_name)* - Load ACL from file.

Example of their usage can be found in *indigo_server*, *indigo_prop_tool* or *indigo_control_panel* source code.

## Access Control File Format
The ACL file has a very simple syntax containing two fields:
- *token* - hexadecimal token
- *Device name* - device name.

Rules:
- Lines starting with '#' are ignored.
- '@' is a special device name meaning that this is the master token of the server or the client.
- '@ server' can be used on the client to specify the master token used for authentication against the specified server.
- remote devices should be specified with the host name like this "Device @ server"

Client ACL example:
```
# Client master token
12345678 @

# Master token to be used for indigosky
12FA0101 @ indigosky

# Some Devices
12FA3213 Dome Dragonfly @ indigosky
12FA3213 Dragonfly Controller @ indigosky
```

Server ACL Example:
```
# Server master token
12FA0101 @

# Some Devices
12FA3213 Dome Dragonfly
12FA3213 Dragonfly Controller
```
