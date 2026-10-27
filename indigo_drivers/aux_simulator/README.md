# Auxiliary Simulators

A virtual INDIGO 3.0 driver for developing and debugging auxiliary-device clients without hardware. Load `indigo_aux_simulator`, then connect any of its four devices independently. No serial port or external simulator process is needed. The standard `SIMULATION` property is enabled and read-only.

## Supported devices

- **PowerBox Simulator**: four individually switched power outlets with current and state, four USB ports with state, two heater outputs (0–100%) with current and state, manual/automatic dew control, voltage/current/power and ambient temperature/humidity/dew point. Each enabled power outlet draws 0.5 A times its outlet number, each heater draws up to 1 A, and the box draws 0.2 A itself at 12.5 V. Automatic dew control sets both heaters to 35%; changing a heater manually returns to manual mode. USB state does not contribute to the modeled supply current.
- **Weather Station Simulator**: ambient and sky temperature, humidity, dew point, wind speed/direction, pressure and rain; wind, rain and cloud condition lights. Edit `X_SIMULATION_PARAMETERS` in the Simulation group to change the readings. Dew point uses the Magnus approximation, clamping humidity to 0.1% for this calculation. Wind is calm below 3 m/s, moderate below 10 m/s, otherwise strong. Rain is dry at zero, wet below 1 mm/h, otherwise raining. Ambient-minus-sky temperature differences classify clear (ambient at least 20 C above sky), cloudy (at least 10 C), otherwise overcast. Unselected lights are IDLE; selected safe lights are OK and other selected lights are ALERT. These simple deterministic conditions are debugging scenarios, not a forecast or safety system.
- **SQM Simulator**: sky brightness, sensor temperature and Bortle class computed by the standard INDIGO helper. Edit `X_SIMULATION_PARAMETERS` to change brightness or temperature.
- **Flat Panel Simulator**: light on/off, intensity (0–100%) and a cover. `AUX_COVER` is a standard one-of-many switch with `OPEN` and `CLOSE`. Motion takes one second and reports BUSY, then OK. An overlapping cover request is ignored by the framework BUSY guard. Disconnect cancels motion and restores the last completed position on reconnect. Light and intensity are independent of cover position, so a client can test each control separately.

Readings are deterministic and update when their inputs change. State is retained across disconnect/reconnect within a loaded driver; unloading restores defaults. Initial state: power outlets and USB enabled, heaters off, manual dew control, clear/dry/calm weather, SQM 21.5 mag/arcsec², flat light off, intensity 50%, cover closed.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_simulator

## Status: Stable

## Notes

Use with Auxiliary Agent: Select the devices in AUX #1 through AUX #4. The agent mirrors standard properties under the corresponding prefixes, for example `AUX_1_AUX_COVER`. Exercise both explicit `OPEN`/`CLOSE` selection and a client toggle that sends `OPEN=false` to close the panel.

## Testing

- 2026-10-20 11:02 3.0.0.1 mac arm64 simulator 13/13 OK
