# MX-HD mount driver

## Supported devices

* MX-HD equatorial mount

Connection over serial port. USB serial connections typically appear as `/dev/ttyUSB0` or `/dev/ttyACM0`.
Bluetooth serial connections are supported when the operating system exposes the mount as an RFCOMM serial device such as `/dev/rfcomm0`.
The driver reads the mount product name with `:GVP#` and firmware version with `:GVF#` and reflects them in the mount information property when available.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_mxhd

The mount and guider devices share the same serial connection and can be connected in any order.

## Site and time setup

The MX-HD mount needs valid time and observing-site settings for normal operation.
This driver does not automatically send UTC/time or geographic coordinates on connection, because an INDIGO client may expose default or not-yet-initialized values during startup.

Use the INDIGO client `Configuration Control` profile mechanism to save and load fixed settings such as the serial port, baud rate and observing-site latitude/longitude.
After connecting, use `Set UTC` -> `From host` to copy the Raspberry Pi or host computer time into the INDIGO UTC/offset fields and send it to the mount.

Changing `MOUNT_GEOGRAPHIC_COORDINATES` or `MOUNT_UTC_TIME` manually in the client still sends the updated values to the mount.

## Coordinate epoch

The MX-HD protocol reports RA/Dec derived from the mount axes and the coordinate reference used during alignment or sync.
For this reason the driver keeps `MOUNT_EPOCH` user-configurable instead of assuming a fixed epoch for the hardware.

The default `MOUNT_EPOCH` is `0`, which means current epoch/JNow in INDIGO.
Use the same epoch consistently for alignment/sync, client target coordinates and subsequent gotos.
If a client workflow uses J2000 catalog coordinates, set `MOUNT_EPOCH` to `2000` before sync/goto operations and save it in the INDIGO configuration profile if needed.

## Behavior notes

On MX-HD hardware, a HOME operation normally resumes sidereal tracking after the home position is reached.
For INDIGO HOME semantics, this driver stops the drive after HOME completion by sending `@FD0#`.

UNPARK is implemented by moving the mount to the home position, but it is treated as an observing-ready state.
Unlike HOME, the driver does not send `@FD0#` after UNPARK, so the MX-HD sidereal drive is allowed to start or remain active.

The MX-HD tracking state is not queried from hardware on connection.
The INDIGO Tracking switch reflects the state last commanded or inferred by the driver; on a fresh driver start it is shown as stopped until a driver action such as UNPARK, slew, or explicit Tracking ON changes it.

While the mount is parked, the driver rejects motion, slew, sync, tracking-rate changes and pulse-guide requests with an INDIGO Alert state and a `Mount is parked!` message. UNPARK remains available.
Some clients may keep the rejected motion or guide property shown in Alert/red state until the next valid update of that property; this indicates the last request was rejected and does not mean the mount is still moving or guiding.

MX-HD HOME/PARK operations include an initial sequence which cannot be interrupted by the mount firmware.
To implement Abort during these operations, the driver disables motor excitation.
After aborting HOME/PARK/UNPARK this way, the driver blocks all motion-related commands for 90 seconds.
After this safety interval, the driver re-enables motor excitation with `@ME1#` before the next motion-related command.

## Status: Tested

Tested on Raspberry Pi 5 with real MX-HD hardware:

* serial connection
* RA/Dec polling
* park, unpark and home
* tracking on/off
* sidereal, solar and lunar rate selection
* manual NSEW motion
* goto and sync
* abort during goto
* abort during park, unpark and home
* `MOUNT_EPOCH` operation with current epoch/JNow and J2000
* guider-device connection
* pulse guide command/state flow
* independent RA/DEC pulse-guide stop behavior
