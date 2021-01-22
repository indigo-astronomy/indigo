# Indigo Guider Agent

Backend implementation of guiding process

## Supported devices

This is a meta driver that can control all underlaying CCD and Guider drivers.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_guider indigo_ccd_... indigo_guider_...

## Status: Under development

## Notes on Guiding Setup

### Drift Detection Algorithms
There are 3 algorithms to detect the tracking drift:
1. **Donuts** - This mode uses the entire image and all the stars on the image to detect the drift.
Because of that it has a "built in" scintillation resilience. It can operate nicely with highly
de-focused stars hence the name Donuts. It will also work nicely with frames with hot pixels,
hot lines and hot columns. However the limitation is that it may not work well if there are
stars on the border of the frame. This is especially true for 8-bit cameras. They may not be able to
calibrate with a frame full of stars coming in and out of the frame. In this case **Selection** mode
should be used for calibration and then **Donuts** can be used for the guiding. **Donuts** has one
configuration parameter *Edge Clipping* (in pixels) and should be used if the edge of the frame
contains artifacts or dark areas.

However if **Donuts** does not work for you **Selection** should work.

2. **Selection** - It uses the centroid of a small area around the star with
a specified radius to detect the drift. This is universal method that should work in most of the cases.
It is resilient to hot pixels, hot lines and hot columns. **Selection** has several configuration parameters,
selection center given by *Selection X*, *Selection Y* and *Radius* all in pixels. It has one additional
parameter *Subframe* used for better performance. This is an integer number meaning how many radii
around the selection should be downloaded from the camera. It has two main benefits - with a remote setup
it decreases the network load and it also speeds up the image download time from the camera.

3. **Centroid** - This is a full frame centroid, useful for bright objects that occupy
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

* **RA speed** and **Dec speed** - Guiding speed of the Right Ascension or Declination axis in pixels/second. Auto set during calibration.

* **Min error** - Smallest error to attempt to correct in pixels. If the error is less than that, no correction will be attempted. This value is equipment specific but in general 0 is OK.

* **Min pulse** - Minimal pulse length in seconds. If the calculated length of the correction pulse is less than that, no correction will be performed. This is to avoid small corrections that can not be handled by the guider device.

* **Max pulse** - Maximum pulse length for a single correction in seconds. If the calculated correction pulse is longer it will be truncated to this value. With too much drift the guider may loose the star. Also with too long pulse the time between corrections may become enough to cause large tracking errors. Values around 1 second are OK.

* **RA Aggressivity** and **Dec Aggressivity** - This is how much to compensate of the whole accumulated RA and Dec drifts in one cycle in percents. A good initial value would be 80-90% for both axis.

* **RA Proportional weight**, **Dec Proportional weight** - *P* component weights of RA and Dec axis (*I* weight = 1 - *P* weight). They specify how much of **RA Aggressivity** and **Dec Aggressivity** respectively, should correct for the random (*Proportional*) errors (the rest is used for systematic (*Integral*) errors). **RA Proportional weight** and **Dec Proportional weight** are numbers between 0 and 1 (1 - pure *P Controller*, 0.5 - equally *P* and *I Controller* and 0 - pure *I controller*). If *PI controller* is needed a good value to start with would be 0.7 for both RA and Dec.

* **Integral stacking** - the history length (in number of frames) to be used for the *Integral* component of the controller. If stacking is 1 (regardless of the values of the **RA Proportional weight** and **Dec Proportional weight**) the controller is pure *Proportional* as there is no history.
Default value is 1 which means that pure *P controller* is used, but if a *PI controller* is needed a good initial value would be between 3 and 6.

* **Dithering offset X** and  **Dithering offset Y** - Add constant offset from the reference during guiding in pixels. The values are reset to 0 when a guiding process is started.

### Fine Tuning the Drift Controller

Here are several tips and guide lines, how to fine tune the *PI controller*:

* The *P Controller* is stable and can be used alone. With sufficient **Aggressivity** it will always compensate for any random and systematic errors in a reasonable time. The *P Controller* is easy to tune and gives reasonably good results. This is why it is the default in indigo_guider_agent. However the guiding will not be as smooth as the guiding of a well tuned *PI Controller*.

* The *I Controller* is over-reacting to the random errors, which leads to oscillations around the set point. They will eventually slowly fade but in some cases these oscillations may continue for a long time or not fade at all. This is why a pure *I controller* is not a good idea. The good balance between proportional and integral components is essential for the smooth guiding.

* Too much *I* component may result in over-corrections and oscillations. This means that the real systematic drift is smaller and is being over corrected, in this case **RA Proportional weight** or **Dec Proportional weight**, depending on the axis, should be increased (take some power from *I* and give it to *P*).

* If the guiding is bumpy and scattered, the systematic error may be too large and the *I* component may not be able not compensate
for it, then the corresponding **Proportional weight** should be decreased (take some power from *P* and give it to *I*). **NB:** Be conservative with with *Integral* component, as oscillations may appear.

* If the error compensation for RA or Dec is too slow (it takes many frames) and is lazily approaching (from one side) the set point, most likely the **Aggressivity** for this axis should be increased.

* If there are oscillations or over corrections which can not be compensated by increasing the values of **RA Proportional weight** or **Dec Proportional weight** (removing power from *Integral* component), then the **Aggressivity** for the corresponding axis is most likely to high and should be decreased.

Fine tuning a *PI controller* is a tricky business and the defaults should produce good results in most cases, so our advise is to change the settings with care.
