# EQMac guider driver

http://eqmac.hulse.id.au

## Supported devices

Any EQDir compatible mount controlled by EQMac application

## Supported platforms

This driver is MacOS specific. Doesn't work in sandboxed applications on Mojave because of security issues with Apple Events.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_guider_eqmac

## Status: Stable

This driver has only guider interface (IPC messages), the rest is compatible with LX200 driver (localhost at port 4030).
