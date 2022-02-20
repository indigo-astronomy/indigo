# INDIGO Astrometry / ASTAP Agent - Polar Alignment Guide

Revision: 02.20.2022 (draft)

Author: **Rumen G.Bogdanovski**

e-mail: *rumen@skyarchive.org*

## Introduction

Equatorial mounts have two axis - Right Ascension / Polar axis and Declination axis. The good polar alignment is critical for these mounts for accurate tracking. This means that the RA / Polar axis should be parallel to earth's rotation axis.

INDIGO uses 3 point polar alignment (3PPA) procedure. This method is derived from the drift alignment method and it does not require a celestial pole view. It relays on the mount GOTO functionality and plate solving. 3PPA method will move the telescope twice in Hour Angle with a specified amount and will take one exposure at start, one in the middle and one at the end. Then it will calculate the polar error based on the drift in Declination and will give clear instructions how to correct it. Currently Atmospheric refraction is not taken in to account and for better results the mount should be trained above 35-40 degrees in altitude. A good starting point is 45-50 degrees in Altitude, just passed the meridian. It is not mandatory but this way meridian flip will be avoided and usually the mount can safely move the required distance in Hour Angle.

## Configuration
- **Exposure time** - This is the exposure time in seconds used for the frame acquisition. Usually several seconds is enough.
- **Hour angle move** - This is how much the mount should move in Hour angle (-Right Ascension) between the exposures in degrees (15 degrees = 1 hour). In the northern hemisphere positive value will move the mount Clockwise, negative - Counterclockwise or vice versa in southern hemisphere. The recommended value is between 15 and 25 degrees (See the safety note below).
- **Related Agents** - The polar alignment will work only if image source agent and the mount agent is selected. This is usually done in the client. Here is how it looks in Ain INDIGO Imager - "Mount Agent @ vega" is selected to control "Mount Simulator" mount. Images will be taken by "Imager Agent":

 ![](POLAR_ALIGNMENT/pa_config.png)

## Running the process
1. Point the mount above 35-40 degrees in altitude and preferably close to the meridian. If **Hour angle move** is negative you should be before the meridian and if positive past the meridian to avoid meridian flip. Please note if the final position is close to 90 degrees azimuth (due east) or 270 degrees azimuth (dew west) the polar alignment calculation will not be accurate. This is another reason to start close to the meridian.
2. Click on "Start alignment". This will take some time. The agent will take three exposures and solve them, and move mount between them. When it is done it will show the error and give you clear instructions how to correct the error:

 ![](POLAR_ALIGNMENT/pa_start.png)

 And will print in the log messages like this:

 ```
 03:39:57.092 Polar error: 67.41'
 03:39:57.092 Azimuth error: +30.72', move C.W. (use azimuth adjustment knob)
 03:39:57.092 Altitude error: +60.00', move Up (use altitude adjustment knob)
 ```

3. Follow the instructions and move the mount polar axis using the altitude and azimuth polar alignment knobs. **Do not touch Right ascension and Declination**. Moving in Right ascension or Declination will lead to wrong polar error estimation and wrong polar alignment. In this example we should move clockwise (C.W.) in azimuth and up (Up) in altitude.

4. Now click "Recalculate error". This will take another exposure and give you another polar error estimate:

 ```
 03:40:34.664 Polar error: 18.89'
 03:40:34.665 Azimuth error: +11.64', move C.W. (use azimuth adjustment knob)
 03:40:34.665 Altitude error: +14.88', move Up (use altitude adjustment knob)
 ```

5. Repeat steps 3 and 4 until the polar error is acceptable:

 ```
 03:41:36.800 Polar error: 0.87'
 03:41:36.801 Azimuth error: +0.84', move C.W. (use azimuth adjustment knob)
 03:41:36.801 Altitude error: -0.24', move Down (use altitude adjustment knob)
 ```

## Notes
1. If the initial error is larger than 5 degrees the process will fail and ask for better initial polar alignment.
2. If the initial error is more than 3 degrees it is recommended to repeat the process for better accuracy.
3. Make sure the the mount is above 35-40 degrees in altitude at its lowest during the polar alignment process to minimize the effect of Atmospheric Refraction. The current implementation does not take refraction in to account. It will be enhanced in the future.
4. Polar alignment end position should not be close to 90 (due East) and 270 (due West) degrees in azimuth. Close to these azimuths error estimation is inaccurate and turning the altitude knob will mostly change the azimuth and not the altitude.
5. The polar error estimate may vary between different runs with several arc minutes. There are many reasons for that - cone error, periodic error, backlash, camera pixel scale etc. Do not be too picky on that.
6. Polar error of several arc minutes (even up to 10') is ok for most of the cases.

## *Safety Note*
*Please make sure the mount has enough room to move twice the **Hour angle move** in the specified direction. The mount will move automatically and it may hit something which may result in damage. So, if you are in the northern hemisphere and the **Hour angle move** is set to -20 degrees, make sure the mount can safely move 40 degrees counterclockwise.*

Clear skies!  
