# INDIGO Device Access Control and Exclusive Lock System
Revision: 27.03.2020 (draft)

Author: **Rumen G.Bogdanovski**

e-mail: *rumen@skyarchive.org*

## Introduction

As of version 2.0-120 INDIGO provides device access control and exclusive locking system based on tokens.

In INDIGO devices on the server can be:
- **Protected** - devices that are protected by token on the server.
- **Locked** - device that is locked by some client (temporarily protected).
- **Public** - devices that are publicly controllable.

## Tokens
There are two types of tokens in INDIGO:

### Master Tokens
**Master tokens** are used differently by the client and the server.

On the server **master token** is used to override the device locks in 2 cases:
- To unlock devices **locked** by failed clients.
- To override the lock of the devices by higher priority tasks.

It is important to note that if the server has no **master token** set, **public devices** can not be locked and no devices can be **protected** even if they have **device token** set. In other words to enable access control on the server it must have **master token** set.

On the client the **master token** can be used in the following situations:
- To get exclusive lock of the **public devices** that do not have **device tokens** set on the client.
- To get high priority access to the devices on the server if matches the server **master token**.

Any client that has the server **master token** can override the any exclusive locks or device protection and can control any device.

### Device Tokens
**Device tokens** are shared between the server and the client. The client must know the particular device token, set at the server, to be able to access this device, unless it has the **master token**.

It is important to note that **protected devices** can not be exclusively locked. Only **public devices** can be locked at connect, if the connection request has **token** set. This will set temporary **device token** on the server that will be valid until the device is disconnected.

## INDIGO Access Control List

INDIGO uses internal Access Control List (ACL) to manage device access. Each property change request must have the **token** option correctly set to access **protected** or **locked** devices. This is done automatically by the INDIGO framework. First the framework looks up the **device token** in the client ACL and if found it is used. If not found it looks if there is a **master token** set on the client and uses it. If none found it sends basic request with the hoping that the device is neither **protected** nor **locked**.

## INDIGO Access Control List API:

INDIGO supports internal token based ACL that can be handled by several calls. It is important to mention that servers and clients have separate Access Control lists that are handled by the same functions:

- *indigo_add_device_token(device_name, token)* - add device to the ACL

- *indigo_remove_device_token(device_name)* - remove the device from the ACL, on server it means that the device will not be protected.

- *indigo_get_device_token(device_name)* - get the device token from the ACL, if not found 0 is returned.

- *indigo_get_device_or_master_token(device_name)* - get device token if set else get master token or 0 if none set.

- *indigo_get_master_token()* - get master token if set 0 if not.

- *indigo_set_master_token(token)* - set master token.

- *indigo_clear_device_tokens()* - clear ACL

- *indigo_load_device_tokens_from_file(file_name)* - Load ACL from file.

## Access Control File Format
TBD
