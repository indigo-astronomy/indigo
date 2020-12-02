# Imager agent

Backend implementation of image capture process

## Supported devices

N/A

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_imager indigo_ccd_...

## Status: Stable

## Notes on Sequencer Setup

The sequencer is controlled by "AGENT_IMAGER_SEQUENCE" property. It has "SEQUENCE" item describing sequence itself and "01", "02", ..., "15", "16" items describing sequence batches. Each item contains one or more commands separated by semicolons.

"SEQUENCE" item can contain the following commands:
| Command | Meaning |
| ----- | ----- |
| focus=XXX | execute autofocus routine with XXX seconds exposure time |
| park | park mount |
| YY | execute batch YY |

Example: "focus=5;1;2;3;1;2;3;park"

"01", "02", ..., "15", "16" items can contain the following commands:
| Command | Meaning |
| ----- | ----- |
| focus=XXX | execute autofocus routine with XXX seconds exposure time |
| count=XXX | set AGENT_IMAGER_BATCH.COUNT to XXX |
| exposure=XXX | set AGENT_IMAGER_BATCH.EXPOSURE to XXX |
| delay=XXX | set AGENT_IMAGER_BATCH.DELAY to XXX |
| filter=XXX | set WHEEL_SLOT.SLOT to the value corresponding to name XXX |
| mode=XXX | find and select CCD_MODE item with label XXX |
| name=XXX | set CCD_LOCAL_MODE.PREFIX to XXX |
| gain=XXX | set CCD_GAIN.GAIN to XXX |
| offset=XXX | set CCD_OFFSET.OFFSET to XXX |
| gamma=XXX | set CCD_GAMMA.GAMMA to XXX |
| temperature=XXX | set CCD_TEMPERATURE.TEMPERATURE to XXX |
| cooler=XXX | find and select CCD_COOLER item with label XXX |
| frame=XXX | find and select CCD_FRAME_TYPE item with label XXX |
| aperture=XXX | find and select DSLR_APERTURE item with label XXX |
| shutter=XXX | find and select DSLR_SHUTTER item with label XXX |
| iso=XXX | find and select DSLR_ISO item with label XXX |

Example: "exposure=1.0;count=10;name=M31_XXX;filter=Red"
