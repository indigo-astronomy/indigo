# PENTAX Camera driver

https://en.wikipedia.org/wiki/Template:Pentax_digital_interchangeable_lens_cameras

## Supported devices

The following cameras are, in theory, supported by this driver. Nevertheless the most of them were never tested.

* Pentax K100D - APS-C 6.1 MP
* Pentax K100D Super - APS-C 6.1 MP
* Pentax K110D - APS-C 6.1 MP
* Pentax K10D - APS-C 10.2 MP
* Pentax K20D - APS-C 14.6 MP
* Pentax K200D - APS-C 10.2 MP
* Pentax K-m - APS-C 10.2 MP
* Pentax K-x - APS-C 12.4 MP
* Pentax K-r - APS-C 12.4 MP
* Pentax K-7 - APS-C 14.6 MP
* Pentax K2000 - APS-C 10.2 MP
* Pentax GX10 - APS-C 10.2 MP (Samsung rebranded)
* Pentax GX20 - APS-C 14.6 MP (Samsung rebranded)
* Pentax K-01 - APS-C 16.3 MP (Mirrorless)
* Pentax K-30 - APS-C 16.3 MP
* Pentax K-50 - APS-C 16.3 MP
* Pentax K-500 - APS-C 16.3 MP
* Pentax K-5 - APS-C 16.3 MP
* Pentax K-5 II - APS-C 16.3 MP
* Pentax K-5 IIs - APS-C 16.3 MP (No AA filter)
* Pentax K-3 - APS-C 24.1 MP
* Pentax K-3 II - APS-C 24.1 MP
* Pentax K-S1 - APS-C 20.12 MP
* Pentax K-S2 - APS-C 20.12 MP
* Pentax K-1 - Full-frame 36.4 MP
* Pentax K-1 Mark II - Full-frame 36.4 MP
* Pentax KP - APS-C 24.3 MP
* Pentax K-70 - APS-C 24 MP
* Pentax KF - APS-C 24 MP
* Pentax K-3 Mark III - APS-C 25.7 MP
* Pentax 645D - Medium Format 40 MP
* Pentax 645Z - Medium Format 51.4 MP

Hot-plug and multiple-device discovery are implemented, but simultaneous cameras and physical USB removal during capture have not been hardware-validated.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_ccd_pentax

## Comments

The following VIDs are recognised by this driver: 0x0A17 (Pentax) a 0x04E8 (rebranded Samsung).

The camera uses USB Mass Storage transport, which can conflict with the operating system's storage driver. On macOS, elevated privileges may be needed to detach that driver; successful connection with the current K-m driver is still unverified, and `sudo` alone did not resolve the observed USB claim error (-99). Linux arm64 capture and transfer have been tested on a Raspberry Pi using `sudo`; operation without elevated privileges has not been verified.

## Limitations and camera setup

Exposure and abort support is currently enabled only for the K-m and K2000 camera identities. Physical capture tests cover a K-m with firmware 1.11.1.8 on Linux arm64. The other listed cameras have not been validated for capture; the list is not a statement of complete tethering support. K2000 hardware validation and Windows runtime testing remain outstanding.

On the tested K-m, program-mode commands do not switch between Manual and Bulb while USB is connected. The camera's e-dial is also locked during the connection. Original pkTriggerCord behaved the same way on this camera.

For bulb exposures:

1. Stop capture and disconnect the camera's USB cable.
2. Set the physical mode dial to **M**, then turn the e-dial past the 30-second shutter setting until **Bulb** is selected.
3. Reconnect USB and connect the camera in INDIGO.

The driver uses a timed Manual exposure for requests shorter than one second and Bulb for requests of one second or longer. Switching between these ranges on the tested K-m therefore requires the corresponding physical camera setup before connecting USB. A refused mode change produces an error rather than an exposure in the wrong mode.

Bulb timing is controlled by host USB commands and is not calibrated to the physical shutter. A short K-m test requested 1.2 s and 3 s but recorded 1.09 s and 2.88 s in JPEG EXIF, respectively (110–120 ms short). These two samples failed a ±50 ms accuracy check; do not assume precision timing from the requested duration. Mechanical shutter timing has not been measured independently.

JPEG, DNG and PEF native image transfer has been tested. Select the desired capture format on the camera before connecting; INDIGO's image-format property controls delivered processing/output and does not select the camera's JPEG/RAW capture format. Streaming, subframes and camera binning are not supported.

## Status: Under development

Driver is developed and tested with:
* Pentax K-m

## Testing

- 2026-10-26 23:38 3.0.0.3 mac arm64 PENTAX K-m 1/0 Failed
- 2026-10-26 23:51 3.0.0.4 mac arm64 fake SDK 36/36 OK
- 2026-10-27 18:24 3.0.0.4 linux arm64 PENTAX K-m (connection baseline) 1/1 OK
- 2026-10-27 20:15 3.0.0.5 linux arm64 fake SDK 44/44 OK
- 2026-10-27 20:49 3.0.0.5 linux arm64 PENTAX K-m 1/0 Failed
- 2026-10-27 20:49 3.0.0.5 linux arm64 PENTAX K-m (bulb) 1/1 OK
- 2026-10-27 20:50 3.0.0.5 linux arm64 PENTAX K-m DNG (single frame) 1/1 OK
- 2026-10-27 20:50 3.0.0.5 linux arm64 PENTAX K-m PEF (single frame) 1/1 OK
