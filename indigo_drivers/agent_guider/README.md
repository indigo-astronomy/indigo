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

### Guiding Algorithms:
There are 3 algorithms to detect drift:
1. **Donuts** - This mode uses full frame images and all stars on the image to detect the drift.
Because of that is has built in scintillation resilience. It can operate nicely with highly
de-focused stars hence the name Donuts. It will also work nicely with frames with many
hot pixels but so far it will struggle with hot columns and lines (working to fix it).

2. **Selection mode** - It uses the centroid of a small area around the star with
specified radius to detect the drift. This is universal method that should work in most of the cases.
It is resilient to hot lines and hot columns as ling as they are not in the selection.

3. **Centroid** - This is a full frame centroid, useful for bright objects that occupy
large portion of the frame like Moon and planets. It will **not work** with stars.

### Fine Tunning the Drift Controller
The drift controller has many parameters that can be configured. Most of them will be configured automatically
during the calibration. The defaults for other settings should be good for most of the cases, however
to get the best performance one may need to tweak them. The most important ones are:
* **Exposure time** - should be long enough to register stars in the frame but not too long so that the accumulated
drift is an issue.

* **RA Aggressivity** and **Dec Aggressivity** - This is how much to compensate in one cycle of the whole accumulated
RA and Dec drifts in percents.

* **Stacking**, **P/I RA**, **P/I Dec** - These parameters are related to the drift controller and are explained below.

Indigo_agent_guider uses *Proportional-Integral* (*PI*) controller to correct for the telescope tracking errors. *Proportional* or *P*
means that it will attempt to correct for any random errors like gusts of wind, random bumps etc. *Integral* or *I* means
that it will look at the last several frames and compensate for any systematic drifts like bad polar alignment, periodic
errors etc. **Stacking** is the history length (in number of frames) to be used for the *I* part of the controller. **P/I RA** and
**P/I Dec** are numbers specifying how much of **RA Aggressivity** and **Dec Aggressivity** respectively, should correct
for random (*P*) errors (the rest is used for systematic errors). **P/I RA** and **P/I Dec** are numbers between
0 and 1 (1 - pure *Proportional*, 0.5 - equally *Proportional* and *Integral* and 0 - pure *Integral* controller).

Here are several guide lines, how to fine tune the *PI* controller:

* Too much *I* component may result in oscillations or over corrections. This means that the real systematic drift is smaller and is being over corrected, in this case **P/I RA** or **P/I Dec** depending on the axis should be increased (take some power from *I* and give it to *P*).

* If the guiding is bumpy and rough, the systematic error may be too large and the *I* component may not be able not compensate
for it, then the corresponding **P/I** value should be decreased (take some power from *P* and give it to *I*).

* If the error compensation is too slow (takes many frames) and is lazily approaching the set point, most likely **Aggressivity**
for this axis should be increased.

* If there are oscillations or over corrections which can not be compensated by increasing the **P/I RA** or **P/I Dec** parameters,
then the **Aggressivity** for the corresponding axis is most likely to high and should be decreased.

Fine tuning a *PI controller* is a tricky busyness and the defaults should produce good results in most cases, so we advise
you to change the settings with care.
