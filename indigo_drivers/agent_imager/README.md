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

## Notes on breakpoint and inter-agent synchronisation

AGENT_IMAGER_BREAKPOINT and AGENT_IMAGER_RESUME_CONDITION properties can be used for various interprocess interactions. If some items of AGENT_IMAGER_BREAKPOINT are set on, exposure_batch_process() will be suspended on the related point waiting for one of the following conditions:

1. aborted by AGENT_ABORT_PROCESS property,
2. resumed by AGENT_PAUSE_PROCESS property,
3. if BARRIER item is selected in AGENT_IMAGER_RESUME_CONDITION and exposure_batch_process() for all related imager agents is suspended. In such case also running process in all related agents is resumed. This state is detected by the value of items of AGENT_IMAGER_BARRIER_STATE mirroring AGENT_PAUSE_PROCESS properties state on all related Imager Agents.

### Configuration for synchronising beginning of capture on multiple Imager Agent instances

On one of Imager Agent instances select other Imager Agent instances as related agents, select PRE-CAPTURE breakpoint in AGENT_IMAGER_BREAKPOINT and BARRIER in AGENT_IMAGER_RESUME_CONDITION.
On other Imager Agent instances don't select related agents, select PRE-CAPTURE breakpoint in AGENT_IMAGER_BREAKPOINT and TRIGGER in AGENT_IMAGER_RESUME_CONDITION.

Set the same COUNT value in AGENT_IMAGER_BATCH and start exposure batch on all Imager Agents in any order. The will wait for each other until all are suspended on PRE-CAPTURE and then resumed by Imager Agent with AGENT_IMAGER_RESUME_CONDITION set to BARRIER.

### Configuration for synchronising dithering on multiple Imager Agent instances
 
On one of Imager Agent instances select other Imager Agent instances and Guider agent as related agents, select PRE-CAPTURE and POST-CAPTURE breakpoints in AGENT_IMAGER_BREAKPOINT and BARRIER in AGENT_IMAGER_RESUME_CONDITION. Configure dithering as usual.
On other Imager Agent instances don't select related agents, select PRE-CAPTURE and CAPTURE breakpoints in AGENT_IMAGER_BREAKPOINT and TRIGGER in AGENT_IMAGER_RESUME_CONDITION.

Set the same COUNT value in AGENT_IMAGER_BATCH and start guiding on Guider Agent and exposure batch on all Imager Agents in any order. The will wait for each other until all are suspended on PRE-DITHER and then resumed by Imager Agent with AGENT_IMAGER_RESUME_CONDITION set to BARRIER.

### Other possible usages

Setting AGENT_IMAGER_BREAKPOINT to any state and AGENT_IMAGER_RESUME_CONDITION to TRIGGER can be used to synchronise any number of AstroImager instances to any other events by INDIGO Script. Among other you can assure the exposure batch is not started before target temperature or weather conditions are met, to synchronise batch of short exposure on one agent with single long exposure on another and many more.  

### Caveats

Remember, than making circular references in related agent chain or setting AGENT_IMAGER_RESUME_CONDITION to BARRIER on more than one Imager agent is not detected and can lead to recursion or deadlock!
