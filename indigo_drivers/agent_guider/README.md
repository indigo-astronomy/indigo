# Indigo Guider Agent

Backend implementation of guiding process

## Supported devices

This is a meta driver that can control all underlying CCD and Guider drivers.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_guider indigo_ccd_... indigo_guider_...

## Status: Stable

# Notes
## Guiding Setup
### Drift Detection Algorithms
There are 3 algorithms to detect the tracking drift:
1. **Donuts** - This mode uses the entire image and all the stars on the image to detect the drift.
Because of that it has a "built in" scintillation resilience. It can operate nicely with highly
de-focused stars hence the name Donuts. It will also work nicely with frames with hot pixels,
hot lines and hot columns. However the limitation is that it may not work well if there are
bright stars on the border of the frame. In this case **Selection** mode
should be used. **Donuts** has one configuration parameter *Edge Clipping* (in pixels) and should
be used if the edge of the frame contains artifacts or dark areas.
**Donts** drift detection may be susceptible to bad polar alignment, it may lead to small drifts due to the field rotation.

2. **Selection** - It uses the centroid of a small area around the star with
a specified radius to detect the drift. In multi-star mode an average drift from a given number of stars is used.
This is universal method that should work in most of the cases. It is resilient to hot pixels, hot lines and hot columns.
**Selection** has several configuration parameters, selection center given by *Selection X*, *Selection Y* and *Radius* all in pixels. If single star is used, it has one additional parameter - *Subframe* used for better performance and lower network load. This is an integer number meaning how many radii
around the selection should be downloaded from the camera. It has two main benefits - with a remote setup
it decreases the network load and it also speeds up the image download time from the camera.
In Multi-star mode **Selection** algorithm may be susceptible to bad polar alignment, it may lead to small drifts due to the field rotation.

4. **Centroid** - This is a full frame centroid, useful for bright objects that occupy
large portion of the frame like Moon and planets. It will **not work** with stars.

For better performance sub-frames can be used with all three drift detection modes.
However with **Selection** algorithm, a sub-frame around the current selection can
be automatically used by the agent.

### Drift Controller Settings

Indigo_agent_guider uses *Proportional-Integral* (*PI*) controller to correct for the telescope tracking errors. *Proportional* or *P*
means that it will attempt to correct for any random errors like gusts of wind, random bumps etc. *Integral* or *I* means
that it will look at the last several frames and compensate for any systematic drifts like bad polar alignment, periodic
errors etc.

The drift controller has many parameters that can be configured. Some of them will be configured automatically
during the calibration process. The defaults for other settings should be good enough for most of the cases, however
to get the best performance one may need to tweak them.

* **Exposure time** - Guiding exposure time in seconds. should be long enough to register stars on the frame but not too long, as the accumulated
drift would become an issue. A good exposure time to start is 1 or 2 seconds.

* **Delay time** - Delay between correction cycles cycles in seconds. Use 0 unless this delay is really necessary.

* **Calibration step** - Guider pulse length in seconds used during calibration process. Use 0.2 second for start. If the drift is too much or not enough the value is decreased or increased automatically.

* **Max clear backlash steps** - A max number of steps to clear backlash (fails, if **Min clear backlash drift** is not achieved) during pre-calibration phase of calibration process.

* **Min clear backlash drift** - A min drift to achieve with **Max clear backlash steps** during pre-calibration phase of calibration process.

* **Max calibration steps** - Maximum number of pulses to reach **Min calibration drift** during calibration process. Value between 15-25 should work in most of the cases.

* **Min calibration drift** - Minimum drift in pixels in each direction while calibrating. Value between 15-25 pixels would be enough in most of the cases. The bigger the number the better the estimates of the guiding parameters.

* **Angle** - Camera rotation angle in degrees. Auto set during calibration.

* **Dec backlash** - Declination axis backlash in pixels. Auto set during calibration.

* **RA speed** and **Dec speed** - Guiding speed of the Right Ascension or Declination axis in pixels/second. Auto set during calibration. **RA speed** is the speed at the equator and it will be auto adjusted when declination changes. Declination is read from *AGENT_GUIDER_MOUNT_COORDINATES* property, which is updated by the Mount agent if the Guider gent is selected as related.

* **Side of Pier** - Telescope side of pier at calibration time (-1 = East , 1 = West, 0 = undefined). This is used to adjust guider settings after meridian flip, which is detected by comparing this value with *AGENT_GUIDER_MOUNT_COORDINATES.SIDE_OF_PIER* (is the current side of pier). *AGENT_GUIDER_MOUNT_COORDINATES* is updated by the Mount agent if the Guider gent is selected as related. If one of the two, calibration *SIDE_OF_PIER* or mount *SIDE_OF_PIER* is undefined (0) the guider parameters will not be adjusted if the other one changes, because either the current orientation or the orientation during calibration was not known. Some mounts do not report their side of pier and is set to 0. In this case meridian flip can not be detected and recalibration or manual guider settings adjustment are required after meridian flip.

* **Min error** - Smallest error to attempt to correct in pixels. If the error is less than that, no correction will be attempted. This value is equipment specific but in general 0 is OK.

* **Min pulse** - Minimal pulse length in seconds. If the calculated length of the correction pulse is less than that, no correction will be performed. This is to avoid small corrections that can not be handled by the guider device.

* **Max pulse** - Maximum pulse length for a single correction in seconds. If the calculated correction pulse is longer it will be truncated to this value. With too much drift the guider may loose the star. Also with too long pulse the time between corrections may become enough to cause large tracking errors. Values around 1 second are OK.

* **RA Proportional aggressivity** and **Dec Proportional aggressivity** - They are the only parameters that are related to *P controller*. In most of the cases those two parameters will be enough to set. They represent how many percent of the last measured drift will be corrected.
The **Proportional aggressivity** is your primary term for controlling the error. this directly scales your error, so with a small **Proportional aggressivity** the controller will make small attempts to minimize the error, and with a large **Proportional aggressivity** the controller will make a larger attempt. If the **Proportional aggressivity** is too small you might never minimize the error and not be able to respond to changes affecting your system, and if it is too large you can have an unstable behavior and severely overshoot the desired value. A good initial value is ~90% for both RA and Dec.

* **RA Integral gain** and **Dec Integral gain** - These are the gains of the integral error. Or how strong should be the correction for the residuals  of the systematic (Integral) errors like Periodic error or bad polar alignment. Setting **RA Integral gain** or **Dec Integral gain** to 0 means P-only controller for Right Ascension or Declination respectively. If *PI controller* is needed a good value to start with would be ~0.5 for both RA and Dec. Please note that 1 does not correspond to 100% of the systematic error, there are many reasons for that. For example **Proportional aggressivity** have already corrected most of the systematic error. So think of it as how strong the Integral component should react to the residual errors (after Proportional correction) accumulated over time. Also note that this term is often the cause of instability in your controller, so be conservative with it.

* **Integral stack size** - the history length (in number of frames) to be used for the *Integral* component of the controller. If stack size is 1 (regardless of the values of the **RA Integral gain** and **Dec Integral gain**) the controller is pure *Proportional* as there is no history.
Default value is 1 which means that pure *P controller* is used, but if a *PI controller* is needed a good initial value would be around 10.

* **Dithering offset X** and  **Dithering offset Y** - Add constant offset from the reference during guiding in pixels. The values are reset to 0 when a guiding process is started.

### Fine Tuning the Drift Controller

Here are several tips and guide lines, how to fine tune the *PI controller*:

* The *P Controller* is stable and can be used alone. With sufficient **Proportional aggressivity** and short exposures, will always compensate for any random and will perform good enough with systematic errors. The *P Controller* is easy to tune and gives reasonably good results. This is why it is the default in indigo_guider_agent. However the guiding will not be as smooth as the guiding of a well tuned *PI Controller*.

* The *I Controller* is over-reacting to the random errors, which leads to oscillations around the set point. They will eventually slowly fade but in some cases these oscillations may continue for a long time or not fade at all. This is why a pure *I controller* is not a good idea. The good balance between proportional and integral components is essential for the smooth and accurate guiding. In INDIGO Guider Agent we use modified PI controller which will try to dampen any *I* over-reactions to random errors and in this case *P controller* will be 100% responsible for random error compensation.

* Too much *I* component may result in over-corrections and steady drifts. This means that the real systematic drift is smaller and is being over corrected, in this case **RA Integral gain** or **Dec Integral gain**, depending on the axis, should be decreased.
**NB:** Be conservative with with *Integral* component, as guiding may start drifting or randomly over compensating.

* If the guiding is bumpy and scattered, the random error may be smaller and *P* component may be over reacting, then the corresponding **Proportional aggressivity** should be decreased (take some power from *P*).

* If the error compensation for RA or Dec is too slow (it takes many frames) and is lazily approaching (from one side) the set point, most likely the **Proportional aggressivity** for this axis should be increased.

* If there are oscillations or over corrections then the **Proportional aggressivity** for the corresponding axis is most likely too high and should be decreased.

Fine tuning a *PI controller* is a tricky business and the defaults should produce good results in most cases, so our advise is to change the settings with care.

## Meridian flip
Please, see **Side of Pier** parameter of the **Settings** for details how meridian flip is detected.

If the Declination guiding runs away after meridian flip **Reverse Dec speed after meridian flip** should be enabled. Some mounts track their "side of pier" state and automatically reverse the direction of the declination motor after a meridian flip. Other mounts do not do this. There is no way for INDIGO to know this in advance. This is why INDIGO needs this to be specified.

## Guiding after slew
Please see **RA speed** and **Dec speed** parameters of the **Settings** for details.

As of INDIGO version 2.0-237 recalibration after GOTO or meridian flip is not required. In order for this to work the *Mount agent* needs to push the mount coordinates and orientation to *AGENT_GUIDER_MOUNT_COORDINATES* property. This is achieved by setting the *Guider agent* as related from the *Mount agent*.
