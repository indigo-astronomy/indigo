# iOptron iEAF / iAFS focuser driver

## Supported devices

Existing model identifiers 2 (iEAF) and 3 (iAFS) are supported at 115200 baud. The public device name remains `iOptron iEAF`; INFO reports the detected model and four-digit firmware field. One device is present at startup; additional independent serial instances can be configured. No hot-plug support.

[Manufacturer iEAF manual](https://www.ioptron.com/v/Manuals/8453_iEAF_Manual.pdf) and [iAFS product page](https://www.ioptron.com/product-p/fa20.htm) describe hardware capabilities. The wire details below preserve the existing implementation and require hardware verification; no independent manufacturer wire specification is bundled.

## Controls

Absolute and relative coordinate moves span 0–99999 integer steps. Relative inward decreases the coordinate and outward increases it; reversal toggles the hardware direction mapping. Zero/no-op completes without unnecessary motor movement. `X_FOCUSER_ZERO_SYNC.SYNC` synchronizes to zero without a move command; this replaces legacy `ZERO_SYNC.SYNC` to follow the custom-property naming rule. The switch resets after each request. Arbitrary coordinate SYNC, speed, backlash, configurable limits and automatic compensation are not exposed.

Reverse, zero and abort are checked with a status read because their commands have no ACK. Abort publishes the stopped measured position and allows a new move. Competing movement requests and zero/reverse during motion are rejected. Poll failure or 100 unchanged moving polls (nominally 10 seconds) attempts a stop and reports ALERT; this software policy is not a physical stall measurement. A valid stopped status or successful abort clears uncertain movement state. Disconnect cancels queued work and attempts a stop; failed communication cannot confirm a physical halt.

## Protocol compatibility

- `:DeviceInfo#` → `PPPPPPMMFFFF#`: six-digit position, two-digit model, four-digit firmware.
- `:FI#` → `PPPPPPPMTTTTTD#`: seven-digit position, moving 0/1, temperature in Kelvin hundredths, direction 0=reversed / 1=normal.
- `:FR#`: toggle direction, no reply.
- `:FM%7d#`: move to a seven-column space-padded coordinate, no immediate reply.
- `:FZ#`: synchronize zero, no reply.
- `:FQ#`: abort, no reply.

The earlier README described seven identity position digits and the opposite direction polarity. The grammar above matches the pre-migration driver and simulator. Framing, field widths, digits and flags are now validated. Invalid temperature reports ALERT without replacing the last valid reading. The existing two-second USB startup settling delay is retained on the device handler queue.

## Build and validation

Authoritative source: `indigo_focuser_ioptron.driver`; regenerate C/header/main with `indigo_generator`. Xcode and Windows project integration are included. Migration progress, simulator results and unverified hardware/platform assumptions are recorded in `REFACTOR.md`.

```sh
indigo_server indigo_focuser_ioptron
```

Select the serial port before connecting; generator defaults replace the old platform-specific automatic port choice.

## License

INDIGO Astronomy open-source license.
