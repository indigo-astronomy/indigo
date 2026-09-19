# astap_cli agent

Backend implementation of ASTAP plate solver

## Supported devices

N/A

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_astap indigo_agent_imager indigo_agent_mount indigo_ccd_... indigo_mount_...

## Status: Stable

## Notes on agent setup

### Agent configuration
1. in ASTAP Agent > Index Management select some indexes fitting field size produced by the camera
2. in ASTAP Agent > Plate Solver select indexes to use and hints for subsequent plate solving
3. in ASTAP Agent > Main select related Imager or Guider agent and optionally Mount Agent
4. if Mount Agent is selected, configure also Sync mode
5. Trigger exposure

### Polar alignment procedure
1. In "ASTAP Agent > Main -> Related Agents" select related Imager or Guider agent and a Mount Agent
2. In "ASTAP Agent > Plate solver > Polar alignment settings" configure exposure time and hour angle move (15-20 degrees is ok)
3. In "ASTAP Agent > Plate solver > Sync mode" select "Calculate polar alignment error"
4. Point the telescope over 40 deg Altitude (preferably just passed meridian to avoid meridian flip) and trigger exposure.
5. The agent will take 3 exposures separated in Hour angle as specified in point 2 and give you instructions how to correct the polar error:
```
23:06:04.694 Polar error: 68.03'
23:06:04.698 Azimuth error: +30.69', move C.W. (use azimuth adjustment knob)
23:06:04.699 Altitude error: +60.71', move Up (use altitude adjustment knob)
```
6. Move the mount pole using polar alignment knobs in the advised directions.
7. in "ASTAP Agent > Plate solver > Sync mode" select "Recalculate polar alignment error" and trigger exposure. The agent will solve the frame and recalculate the error.
8. Repeat points 6 and 7 until the error is acceptable.

NB: C.W. is Clockwise, C.C.W. is Counterclockwise.
