# Guide to indigo_server and INDIGO Drivers
Revision: 25.07.2020 (draft)

Author: **Rumen G.Bogdanovski**

e-mail: *rumen@skyarchive.org*

## The INDIGO server: **indigo_server**
This is the help message of the indigo_server:
```
umen@sirius:~ $ indigo_server -h
INDIGO server v.2.0-123 built on Jul 25 2020 01:06:15.
usage: indigo_server [-h | --help]
       indigo_server [options] indigo_driver_name indigo_driver_name ...
options:
       --  | --do-not-fork
       -l  | --use-syslog
       -p  | --port port                     (default: 7624)
       -b  | --bonjour name                  (default: hostname)
       -T  | --master-token token            (master token for devce access default: 0 = none)
       -a  | --acl-file file
       -b- | --disable-bonjour
       -u- | --disable-blob-urls
       -w- | --disable-web-apps
       -c- | --disable-control-panel
       -v  | --enable-info
       -vv | --enable-debug
       -vvv| --enable-trace
       -r  | --remote-server host[:port]     (default port: 7624)
       -i  | --indi-driver driver_executable
rumen@sirius:~ $
```

The **indigo_server** is highly configurable trough command line options at start and also when already started:

### **--** | **--do-not-fork**
By default **indigo_server** will start a child process called **indigo_worker**, and the drivers will be run in the boundaries of this process. This is done for a reason, if a single driver crashes this will crash **indigo_worker** process indigo server will detect it and start **indigo_worker** again. This switch prevents this behavior and if a driver fails the whole server will crash without an attempt to recover. This is is used for faulty driver debugging.

```
indigo_server───indigo_worker─┬─{indigo_worker}
                              ├─{indigo_worker}
                              ├─{indigo_worker}
                              ├─{indigo_worker}
                              └─{indigo_worker}
```

### -l  | --use-syslog
Debug and error messages are sent to syslog.

### -p  | --port
Set listening port for the indigo_server.

### -b  | --bonjour
INDIGO uses mDNS/Bonjour for service discovery. This will set the name of the service in the network, so that the clients can discover and automatically connect to the INDIGO server without providing host and port.

### -T  | --master-token
Set the server master token for device access control. Please see [INDIGO_DEVICE_ACCESS_CONTROL_AND_LOCKING.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/INDIGO_DEVICE_ACCESS_CONTROL_AND_LOCKING.md) for details.

### -a  | --acl-file file
Use tokens for device access control from a file. Please see [INDIGO_DEVICE_ACCESS_CONTROL_AND_LOCKING.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/INDIGO_DEVICE_ACCESS_CONTROL_AND_LOCKING.md) for details.

### -b- | --disable-bonjour
Do not announce the service with Bonjour. The client should enter host and port to connect.

### -u- | --disable-blob-urls
INDIGO provides 2 ways of BLOB (image) transfer. One is the legacy INDI style where image is base64 encoded and sent to the client in plain text and the client decodes the data on its end. This makes the data ~30% larger and the encoding and decoding is CPU intensive. The second INDIGO style is to use binary image transfer over http protocol avoiding encoding, decoding and data size overhead. By default The client can request any type of BLOB transfer. With this switch you can force the server to accept only legacy INDI style blob transfer.  

### -w- | --disable-web-apps
This switch will disable INDIGO web applications like imager, telescope control etc.

### -c- | --disable-control-panel
This switch will disable the web based control panel.

### -v  | --enable-info
Show some information messages in the log

### -vv | --enable-debug
Show more verbose messages useful for debugging and troubleshooting.

### -vvv| --enable-trace
Show even more messages like low level driver<->device communication and INDIGO protocol messages.

### -r  | --remote-server
INDIGO server can connect to other servers and attach their INDIGO buses to its own bus. With this switch host names and ports of the remote servers to be attached can be provided. This switch can be used several times, once per remote INDIGO server.

### -i  | --indi-driver
Run drivers in separate processes. If a driver name is preceded by this switch it will be run in a separate process. This is the way to run INDI drivers in INDIGO. The drawback of this approach is that the driver communication will be in orders of magnitude slower than running the driver in the **indigo_worker** process and those driver can not be dynamically loaded and unloaded.
```
$ indigo_server indigo_ccd_asi -i indigo_ccd_simulator -i indigo_mount_simulator
```
In this case **indigo_ccd_asi** driver will be loaded in the **indigo_worker** process, but **indigo_ccd_simulator** and **indigo_mount_simulator** will have their own processes forked from **indigo_worker**.
```
indigo_server───indigo_worker─┬─indigo_ccd_simulator
                              ├─indigo_mount_simulator
                              ├─{indigo_worker}
                              ├─{indigo_worker}
                              ├─{indigo_worker}
                              ├─{indigo_worker}
                              ├─{indigo_worker}
                              ├─{indigo_worker}
                              └─{indigo_worker}
```

## INDIGO Drivers
There are three versions of each INDIGO driver, each coming with its benefits and drawbacks. The INDIGO drivers can be used by both **indigo_server** and clients. INDIGO does not require server to operate. Clients can load drivers and use them without the need of a server. In this case only locally attached devices can be accessed (much like in ASCOM). An example how to use INDIGO executable driver without a server can be found in [client.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_test/client.c).

Clients can also be developed to be clients and servers at the same time.

### Dynamically Loaded Drivers
These drivers are loaded by default by the INDIGO server. They are loaded and run in **indigo_worker** process of **indigo_server**.
- PROS:
 * Extremely fast driver-client and driver-server data transfer.
 * Drivers can be dynamically loaded and unloaded at runtime.
- CONS:
 * A faulty driver can bring down the service or the application, however **indigo_server** can partly recover from this, as mentioned in the previous section.

### Static Drivers
These drivers are intended for use in the clients, **indigo_server** can not use them. They can not be dynamically loaded and unloaded. But can be enabled and disabled.
 - PROS:
  * Extremely fast driver-client and driver-server data transfer.
  * As drivers are linked in a monolithic client executable, there is no need to distribute many drivers as a separate files.
 - CONS:
  * Drivers can not be updated without application re linking.
  * A faulty driver can bring down the application.

### Executable Drivers
These are the INDI style drivers, They are run in a separate sub process of **indigo_worker** process. In clients they also run in a separate sub-process. They can be native INDIGO or drivers for INDI.
 - PROS:
  * A faulty driver will not bring down the service or the application. Only the attached to the driver devices will stop working. **indigo_server** can partly recover by reloading the driver.
  * This is how INDIGO can run INDI drivers.
 - CONS:
  * driver-client and driver-server data transfer is in orders of magnitude slower.
  * They require more resources to run.
  * No dynamic loading and unloading in **indigo_server**. These drivers can be loaded only at server startup.
