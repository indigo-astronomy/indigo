# PegasusAstro Prodigy Microfocuser

Supports the Prodigy focuser and its two power outlets and two USB ports through one serial connection, 19200 baud, 8N1. The protocol reference is the bundled `ProdigyMF-Serial-Command-Table.pdf` (firmware 1.4 and later).

## Connection

Run `indigo_server indigo_focuser_prodigy`. Set the port on **Pegasus Prodigy Focuser**, including when using only **Pegasus Prodigy Powerbox**. Either logical device can connect first; both share one handle and queue. Disconnecting one preserves the other's connection. Additional independent pairs can be configured through the focuser's additional instances property.

## Focuser

Absolute GOTO, relative movement, coordinate SYNC, abort, speed, backlash, temperature and encoder-zero park are supported. Relative inward uses a positive device offset, matching the legacy driver; confirm physical direction on hardware. Reverse is fixed normal; automatic compensation is unsupported and hidden.

Measured position remains separate from the requested target. Both software limits are enforced and update motion ranges (default -999999 to 999999). Limits do not program a hardware endpoint. Speed is read from the controller at connection and after changes. Settings require valid command acknowledgements; failed temperature reads retain the last valid value and report ALERT.

`X_FOCUSER_PARK.PARK` starts motion to the zero encoder and stays BUSY until readback confirms zero and idle. It is rejected if zero is outside the configured software limits or another motion is pending. Motion and park use queued finalizers; 100 unchanged movement polls trigger a stop attempt and ALERT. After uncertain movement, use abort to confirm stopped position before another move. Physical park/encoder behavior needs hardware validation.

## Powerbox

`AUX_POWER_OUTLET` and `AUX_USB_PORT` control both channels independently. Writes validate command acknowledgements and read back the four boolean states with `D`, including partial-write failure. `AUX_OUTLET_NAMES` remains available before connection; labels are persistent.

`X_AUX_REBOOT.REBOOT` sends the no-reply reboot command, then checks identity and port state before reporting completion. It uses at most ten delayed attempts and is rejected during movement or uncertain motion. Actual firmware reboot timing requires hardware verification. The powerbox advertises the powerbox interface; temperature belongs to the focuser.

## Development and validation

`indigo_focuser_prodigy.driver` is authoritative; regenerate C/header/main with `indigo_generator`. The host simulator uses shared `serial_motion`, supports headless/ready-file/trace operation and isolated test profiles. Coverage, intermediate results, reproduction commands and hardware/platform gaps are in `REFACTOR.md` and `../../indigo_test/CHANGES.md`. Windows project integration is included; execution on Windows/Linux remains unverified.

## License

INDIGO Astronomy open-source license.
