# NexStar mount driver

## Supported devices

Any NexStar protocol compatible mount (SkyWatcher; Celestron; Orion; ...) connected over the serial port of the hand controller or network.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_nexstar

## Status: Stable

Driver is developed and tested with:
* SkyWatcher AZ-EQ6 GT with SynScan controller
* SkyWatcher EQ6 Pro with SynScan controller
* Celestron Advanced VX NexStar+ controller
* Celestron NexStar 4/5 SE with NexStar/NexStar+/StarSense controllers

## Comments

Use URL in form tcp://host:port to connect to the mount over network (default port is 9999).
To export the mount over the network one can use Nexbridge https://sourceforge.net/projects/nexbridge, and the serial connection to the mount should be over the hand controler.

This driver uses libnexstar library https://sourceforge.net/projects/libnexstar/

A non-standard switch property "Tracking mode" is provided by this driver for fork mounts. It is set to "auto" by default and in such case driver tries to guess the mode from tracking mode reported by the hand controller. If the mount is not tracking, error is reported.

A non-standard switch property "Guider rate" is provided by this driver.
