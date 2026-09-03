# SynScan mount driver

## Supported devices

Any SynScan protocol compatible mount (SkyWatcher; Celestron; Orion; ...) connected over serial port or network.

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_synscan

## Status: Stable

Driver is developed and tested with:
* SkyWatcher NEQ6 Pro
* SkyWatcher EQAZ6
* SkyWatcher EQ8
* SkyWatcher AZ-GTi

## Comments

Use URL in form synscan://host:port to connect to the mount over UDP (default port is 11880). Use just synscan:// for UDP autodetection.

The mount, guider and AUX shutter devices share one physical SynScan connection. Guide pulses are serialized through the mount queue; overlapping pulses in the same direction extend the active pulse, while an opposite-direction pulse on the same axis cancels the active pulse and starts the new direction.

SynScan-specific optional properties are defined only when supported by the connected controller: polarscope brightness, operating mode, auxiliary encoders and autohome settings. On controllers with a snap port, the `Mount SynScan (aux)` device exposes the standard AUX shutter `CCD_EXPOSURE` and `CCD_ABORT_EXPOSURE` properties.

If the mount is not stationary (i.e. it is not part of a permanent setup, such as in an observatory) removing the existing alignment points should be part of the installation process: clear them when setting up the mount, before performing any sync and before using it. Otherwise the old alignment points, which correspond to the previous physical setup, will interfere with the new ones and the pointing accuracy will deteriorate.

## Refactored driver behavior

The SynScan driver was refactored to use the current INDIGO driver architecture while keeping the same physical connection and device names. For users this should mostly be visible as more predictable state reporting, cleaner recovery from connection problems, and a few newly exposed controller capabilities.

The mount now reports its high-level activity through `MOUNT_STATE`, including slewing, parking, homing and tracking. Coordinate properties stay busy while the mount is moving during slew, park, home and autohome operations, so clients can follow motion progress more reliably.

After a coordinate slew, the driver honors the selected `MOUNT_ON_COORDINATES_SET` action. If tracking is requested, tracking is started automatically when the slew completes. Home and autohome operations stop tracking when they finish, matching the expected mount state after a home procedure.

Autohome is exposed only when both axes report home-indexer support. During autohome the driver searches for the home index on both axes, reports the operation as a home process, and keeps the autohome switch active until the procedure finishes.

When auxiliary encoders are enabled and supported by the controller, reported coordinates are read from the auxiliary encoders instead of the motor positions. Movement commands still use motor positions internally, so goto, park, home and autohome keep using the motor controller's commanded-position model.

The driver recognizes additional recent Sky-Watcher model codes and exposes optional properties only when the connected controller reports the matching capability. Supported snap-port mounts also get a separate `Mount SynScan (aux)` logical device for remote shutter control.

Serial and network communication handling is more defensive. If the driver loses the physical connection, it disconnects the shared mount/guider/AUX devices instead of repeatedly logging low-level I/O errors.
