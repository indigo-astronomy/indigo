# iOptron mount driver

## Supported devices

Most if not all iOptron mounts are supported over serial port or network. Make sure to read the comments below.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_ioptron

## Comments

Use URL in form ieq://host:port to connect to the mount over network

To find your default port through the hand controller use Menu -> Settings -> Wi-Fi Options -> Wireless Status

* CEM60-EC - Default Port 4030
* HEM27 - Default Port 8899

Also you need to make sure when configuring INDIGO you are using the proper protocol verison. Make sure to read the Protocol version included in this directory structure. An example is Protocol 3.0 for INDIGO settings will work with the GEM28 and HEM27.

## Status: Stable

Driver is tested and verified with:
* CEM60-EC
* iEQ 45 Pro
* GEM28
* HEM27