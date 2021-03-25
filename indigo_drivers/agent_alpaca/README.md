# ASCOM ALPACA bridge agent

https://github.com/ASCOMInitiative/ASCOMRemote/blob/master/Documentation/ASCOM%20Alpaca%20API%20Reference.pdf
https://ascom-standards.org/api

## Supported devices

N/A

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_alpaca ...

## Status: Under development

## Notes

### General

Mapping from INDIGO names to ALPACA device numers is maintained and made persistent in AGENT_ALPACA_DEVICES property.

All web configuration requests are redirected to INDIGO Server root.

### Wheel

IFilterWheelV2 implemented, no limitations

### Focuser

IFocuserV1 implemented, no limitations

### Mount

ITelescopeV3 implemented with exception of MoveAxis method group (not compatible with INDIGO substantially)

### Guider

Sufficient subset of ITelescopeV3 implemented, no limitations

### Lightbox

ICoverCalibratorV1 implemented, HaltCover is dummy method (no couterpart in INDIGO)

--------------------------------------------------------------------------------------------------------------

ConformanceCheck ASCOM Device Conformance Checker Version 6.5.7500.22515, Build time: 14/07/2020 13:30:30
ConformanceCheck Running on: ASCOM Platform 6.5 SP1 6.5.1.3234

ConformanceCheck Driver ProgID: ASCOM.AlpacaDynamic5.FilterWheel

Error handling
Error number for "Not Implemented" is: 80040400
Error number for "Invalid Value 1" is: 80040404
Error number for "Value Not Set 1" is: 80040402
Error number for "Value Not Set 2" is: 80040403
Error messages will not be interpreted to infer state.

21:33:29.713 Driver Access Checks              OK
21:33:30.644 AccessChecks                      OK       Successfully created driver using late binding
21:33:30.851 AccessChecks                      OK       Successfully connected using late binding
21:33:30.858 AccessChecks                      INFO     The driver is a COM object
21:33:31.770 AccessChecks                      INFO     Device does not expose interface IFilterWheel
21:33:32.000 AccessChecks                      INFO     Device exposes interface IFilterWheelV2
21:33:32.048 AccessChecks                      OK       Successfully created driver using driver access toolkit
21:33:32.095 AccessChecks                      OK       Successfully connected using driver access toolkit

Conform is using ASCOM.DriverAccess.FilterWheel to get a FilterWheel object
21:33:32.138 ConformanceCheck                  OK       Driver instance created successfully
21:33:32.212 ConformanceCheck                  OK       Connected OK

Common Driver Methods
21:33:32.282 InterfaceVersion                  OK       1
21:33:32.330 Connected                         OK       True
21:33:32.381 Description                       OK       CCD Imager Simulator (wheel)
21:33:32.426 DriverInfo                        OK       ASCOM Dynamic Driver v6.5.1.3234 - REMOTE DEVICE: indigo_ccd_simulator
21:33:32.472 DriverVersion                     OK       2.0.0.17
21:33:32.519 Name                              OK       CCD Imager Simulator (wheel)
21:33:32.562 CommandString                     INFO     Conform cannot test the CommandString method
21:33:32.570 CommandBlind                      INFO     Conform cannot test the CommandBlind method
21:33:32.578 CommandBool                       INFO     Conform cannot test the CommandBool method
21:33:32.587 Action                            INFO     Conform cannot test the Action method
21:33:32.621 SupportedActions                  OK       Driver returned an empty action list

Pre-run Checks
21:33:34.071 Pre-run Check                     OK       Filter wheel is stationary, ready to start tests

Properties
21:33:34.148 FocusOffsets Get                  OK       Found 5 filter offset values
21:33:34.162 FocusOffsets Get                  INFO     Filter 0 Offset: 0
21:33:34.219 FocusOffsets Get                  INFO     Filter 1 Offset: 0
21:33:34.266 FocusOffsets Get                  INFO     Filter 2 Offset: 0
21:33:34.312 FocusOffsets Get                  INFO     Filter 3 Offset: 0
21:33:34.359 FocusOffsets Get                  INFO     Filter 4 Offset: 0
21:33:34.410 Names Get                         OK       Found 5 filter names
21:33:34.420 Names Get                         INFO     Filter 0 Name: Ha
21:33:34.429 Names Get                         INFO     Filter 1 Name: OIII
21:33:34.437 Names Get                         INFO     Filter 2 Name: SII
21:33:34.450 Names Get                         INFO     Filter 3 Name: Clean
21:33:34.459 Names Get                         INFO     Filter 4 Name: Empty
21:33:34.468 Names Get                         OK       Number of filter offsets and number of names are the same: 5
21:33:34.483 Position Get                      OK       Currently at position: 0
21:33:34.643 Position Set                      OK       Reached position: 0 in: 0.1 seconds
21:33:36.287 Position Set                      OK       Reached position: 1 in: 0.6 seconds
21:33:37.928 Position Set                      OK       Reached position: 2 in: 0.6 seconds
21:33:39.574 Position Set                      OK       Reached position: 3 in: 0.6 seconds
21:33:41.183 Position Set                      OK       Reached position: 4 in: 0.6 seconds
21:33:42.280 Position Set                      OK       Correctly rejected bad position: -1
21:33:42.296 Position Set                      OK       Correctly rejected bad position: 5

Conformance test complete

No errors, warnings or issues found: your driver passes ASCOM validation!!

--------------------------------------------------------------------------------------------------------------

ConformanceCheck ASCOM Device Conformance Checker Version 6.5.7500.22515, Build time: 14/07/2020 13:30:30
ConformanceCheck Running on: ASCOM Platform 6.5 SP1 6.5.1.3234

ConformanceCheck Driver ProgID: ASCOM.AlpacaDynamic1.Focuser

Error handling
Error number for "Not Implemented" is: 80040400
Error number for "Invalid Value 1" is: 80040404
Error number for "Value Not Set 1" is: 80040402
Error number for "Value Not Set 2" is: 80040403
Error messages will not be interpreted to infer state.

21:35:11.918 Driver Access Checks              OK
21:35:12.839 AccessChecks                      OK       Successfully created driver using late binding
21:35:13.055 AccessChecks                      OK       Successfully connected using late binding
21:35:13.062 AccessChecks                      INFO     The driver is a COM object
21:35:13.965 AccessChecks                      INFO     Device does not expose IFocuser interface
21:35:14.236 AccessChecks                      INFO     Device exposes IFocuserV2 interface
21:35:14.312 AccessChecks                      INFO     Device does not expose IFocuserV3 interface
21:35:14.385 AccessChecks                      OK       Successfully created driver using driver access toolkit
21:35:14.396 AccessChecks                      OK       Successfully connected using driver access toolkit
21:35:14.406 AccessChecks                      OK       Successfully disconnected using driver access toolkit

Conform is using ASCOM.DriverAccess.Focuser to get a Focuser object
21:35:14.498 ConformanceCheck                  OK       Driver instance created successfully
21:35:14.574 ConformanceCheck                  OK       Connected OK

Common Driver Methods
21:35:14.651 InterfaceVersion                  OK       1
21:35:14.699 Connected                         OK       True
21:35:14.747 Description                       OK       CCD Imager Simulator (focuser)
21:35:14.792 DriverInfo                        OK       ASCOM Dynamic Driver v6.5.1.3234 - REMOTE DEVICE: indigo_ccd_simulator
21:35:14.837 DriverVersion                     OK       2.0.0.17
21:35:14.886 Name                              OK       CCD Imager Simulator (focuser)
21:35:14.927 CommandString                     INFO     Conform cannot test the CommandString method
21:35:14.936 CommandBlind                      INFO     Conform cannot test the CommandBlind method
21:35:14.943 CommandBool                       INFO     Conform cannot test the CommandBool method
21:35:14.952 Action                            INFO     Conform cannot test the Action method
21:35:14.976 SupportedActions                  OK       Driver returned an empty action list

Properties
21:35:15.163 Absolute                          OK       False
21:35:15.176 IsMoving                          OK       False
21:35:15.206 MaxStep                           OK       65535
21:35:15.219 MaxIncrement                      OK       65535
21:35:15.236 Position                          OK       Position must not be implemented for a relative focuser and a PropertyNotImplementedException exception was generated as expected
21:35:15.276 StepSize                          OK       Optional member threw a PropertyNotImplementedException exception.
21:35:15.289 TempCompAvailable                 OK       True
21:35:15.305 TempComp Read                     OK       False
21:35:15.321 TempComp Write                    OK       Successfully turned temperature compensation on
21:35:15.338 TempComp Write                    OK       Successfully turned temperature compensation off
21:35:15.357 Temperature                       OK       25

Methods
21:35:15.440 Halt                              OK       Focuser halted OK
21:35:15.472 Move - TempComp False                      Moving by: 6554
21:35:34.474 Move - TempComp False                      Asynchronous move found
21:35:34.484 Move - TempComp False             OK       Relative move OK
21:35:34.499 Move - TempComp False             INFO     Returning to original position: 0
21:35:41.798 Move - TempComp True                       Moving by: 6554
21:35:41.821 Move - TempComp True              OK       COM Exception correctly raised as expected

Conformance test complete

No errors, warnings or issues found: your driver passes ASCOM validation!!

--------------------------------------------------------------------------------------------------------------

ConformanceCheck ASCOM Device Conformance Checker Version 6.5.7500.22515, Build time: 14/07/2020 13:30:30
ConformanceCheck Running on: ASCOM Platform 6.5 SP1 6.5.1.3234

ConformanceCheck Driver ProgID: ASCOM.AlpacaDynamic1.CoverCalibrator

Error handling
Error number for "Not Implemented" is: 80040400
Error number for "Invalid Value 1" is: 80040405
Error number for "Value Not Set 1" is: 80040402
Error number for "Value Not Set 2" is: 80040403
Error messages will not be interpreted to infer state.

22:31:33.433 Driver Access Checks              OK       
22:31:34.348 AccessChecks                      OK       Successfully created driver using late binding
22:31:37.085 AccessChecks                      OK       Successfully connected using late binding
22:31:37.092 AccessChecks                      INFO     The driver is a COM object
22:31:37.979 AccessChecks                      OK       Successfully created driver using early binding to ICoverCalibratorV1 interface
22:31:40.672 AccessChecks                      OK       Successfully connected using early binding to ICoverCalibratorV1 interface
22:31:40.703 AccessChecks                      OK       Successfully created driver using driver access toolkit
22:31:43.263 AccessChecks                      OK       Successfully connected using driver access toolkit

Conform is using ASCOM.DriverAccess.CoverCalibrator to get a CoverCalibrator object
22:31:43.327 ConformanceCheck                  OK       Driver instance created successfully

Pre-connect checks

Connect
22:31:45.990 ConformanceCheck                  OK       Connected OK

Common Driver Methods
22:31:46.057 InterfaceVersion                  OK       1
22:31:46.110 Connected                         OK       True
22:31:46.160 Description                       OK       Flip-Flat
22:31:46.204 DriverInfo                        OK       ASCOM Dynamic Driver v6.5.1.3234 - REMOTE DEVICE: indigo_aux_flipflat
22:31:46.251 DriverVersion                     OK       2.0.0.4
22:31:46.298 Name                              OK       Flip-Flat
22:31:46.341 CommandString                     INFO     Conform cannot test the CommandString method
22:31:46.349 CommandBlind                      INFO     Conform cannot test the CommandBlind method
22:31:46.356 CommandBool                       INFO     Conform cannot test the CommandBool method
22:31:46.366 Action                            INFO     Conform cannot test the Action method
22:31:46.394 SupportedActions                  OK       Driver returned an empty action list

Properties
22:31:46.604 CalibratorState                   OK       Off
22:31:46.638 CoverState                        OK       Open
22:31:46.666 MaxBrightness                     OK       255
22:31:46.718 Brightness                        OK       255

Methods
22:31:48.856 OpenCover                         OK       OpenCover was successful. The asynchronous open took 2.0 seconds
22:31:50.964 CloseCover                        OK       CloseCover was successful. The asynchronous close took 2.1 seconds
22:31:52.102 HaltCover                         ISSUE    CoverStatus indicates that the device has cover capability and a MethodNotImplementedException exception was thrown, this method must function per the ASCOM specification.
22:31:52.156 CalibratorOn                      OK       CalibratorOn with brightness -1 threw an InvalidValueException as expected
22:31:53.209 CalibratorOn                      OK       CalibratorOn with brightness 0 was successful. The synchronous operation took 1.0 seconds
22:31:53.223 CalibratorOn                      OK       The Brightness property does return the value that was set
22:31:53.276 CalibratorOn                      OK       CalibratorOn with brightness 63 was successful. The synchronous operation took 0.0 seconds
22:31:53.295 CalibratorOn                      OK       The Brightness property does return the value that was set
22:31:53.368 CalibratorOn                      OK       CalibratorOn with brightness 127 was successful. The synchronous operation took 0.0 seconds
22:31:53.383 CalibratorOn                      OK       The Brightness property does return the value that was set
22:31:53.430 CalibratorOn                      OK       CalibratorOn with brightness 191 was successful. The synchronous operation took 0.0 seconds
22:31:53.444 CalibratorOn                      OK       The Brightness property does return the value that was set
22:31:53.492 CalibratorOn                      OK       CalibratorOn with brightness 255 was successful. The synchronous operation took 0.0 seconds
22:31:53.508 CalibratorOn                      OK       The Brightness property does return the value that was set
22:31:53.565 CalibratorOn                      OK       CalibratorOn with brightness 256 threw an InvalidValueException as expected
22:32:08.669 CalibratorOff                     OK       CalibratorOff was successful. The synchronous action took 15.1 seconds
22:32:08.683 CalibratorOff                     OK       Brightness is set to zero when the calibrator is turned off

Conformance test complete

Your driver had 0 errors, 0 warnings and 1 issues


--------------------------------------------------------------------------------------------


ConformanceCheck ASCOM Device Conformance Checker Version 6.5.7500.22515, Build time: 7/14/2020 12:30:30 PM
ConformanceCheck Running on: ASCOM Platform 6.5 SP1 6.5.1.3234

ConformanceCheck Driver ProgID: ASCOM.AlpacaDynamic2.Rotator

Error handling
Error number for "Not Implemented" is: 80040400
Error number for "Invalid Value 1" is: 80040405
Error number for "Value Not Set 1" is: 80040402
Error number for "Value Not Set 2" is: 80040403
Error messages will not be interpreted to infer state.

12:21:04.508 Driver Access Checks              OK       
12:21:05.430 AccessChecks                      OK       Successfully created driver using late binding
12:21:06.117 AccessChecks                      OK       Successfully connected using late binding
12:21:06.117 AccessChecks                      INFO     The driver is a COM object
12:21:07.493 AccessChecks                      INFO     Driver does not expose interface IRotator
12:21:07.508 AccessChecks                      INFO     Driver does not expose interface IRotatorV2
12:21:07.524 AccessChecks                      OK       Successfully created driver using driver access toolkit
12:21:08.198 AccessChecks                      OK       Successfully connected using driver access toolkit

Conform is using ASCOM.DriverAccess.Rotator to get a Rotator object
12:21:08.226 ConformanceCheck                  OK       Driver instance created successfully
12:21:08.851 ConformanceCheck                  OK       Connected OK

Common Driver Methods
12:21:08.930 InterfaceVersion                  OK       1
12:21:08.977 Connected                         OK       True
12:21:09.025 Description                       OK       Field Rotator Simulator
12:21:09.072 DriverInfo                        OK       ASCOM Dynamic Driver v6.5.1.3234 - REMOTE DEVICE: indigo_rotator_simulator
12:21:09.134 DriverVersion                     OK       2.0.0.2
12:21:09.525 Name                              OK       Field Rotator Simulator
12:21:09.606 CommandString                     INFO     Conform cannot test the CommandString method
12:21:09.617 CommandBlind                      INFO     Conform cannot test the CommandBlind method
12:21:09.617 CommandBool                       INFO     Conform cannot test the CommandBool method
12:21:09.633 Action                            INFO     Conform cannot test the Action method
12:21:09.678 SupportedActions                  OK       Driver returned an empty action list

Can Properties
12:21:09.820 CanReverse                        OK       False

Pre-run Checks
12:21:10.024 Pre-run Check                     OK       Rotator is stationary

Properties
12:21:10.119 IsMoving                          OK       False
12:21:10.211 Position                          OK       0
12:21:10.259 TargetPosition                    OK       0
12:21:10.307 StepSize                          OK       Optional member threw a PropertyNotImplementedException exception.
12:21:10.354 Reverse Read                      OK       when CanReverse is False and a PropertyNotImplementedException exception was generated as expected
12:21:10.463 Reverse Write                     OK       when CanReverse is False and a PropertyNotImplementedException exception was generated as expected

Methods
12:21:10.540 Halt                              OK       Halt command successful
12:21:21.602 MoveAbsolute                      OK       Asynchronous move successful to: 45 degrees
12:21:21.633 MoveAbsolute                      OK       Rotator is at the expected position: 45
12:21:42.070 MoveAbsolute                      OK       Asynchronous move successful to: 135 degrees
12:21:42.118 MoveAbsolute                      OK       Rotator is at the expected position: 135
12:22:03.055 MoveAbsolute                      OK       Asynchronous move successful to: 225 degrees
12:22:03.071 MoveAbsolute                      OK       Rotator is at the expected position: 225
12:22:23.883 MoveAbsolute                      OK       Asynchronous move successful to: 315 degrees
12:22:23.914 MoveAbsolute                      OK       Rotator is at the expected position: 315
12:22:24.008 MoveAbsolute                      OK       Movement to large negative angle -405 degrees
12:22:24.133 MoveAbsolute                      OK       Movement to large positive angle 405 degrees
12:22:27.102 Move                              OK       Asynchronous move successful - moved by -10 degrees to: 305 degrees
12:22:27.117 Move                              OK       Rotator is at the expected position: 305
12:22:30.071 Move                              OK       Asynchronous move successful - moved by 10 degrees to: 315 degrees
12:22:30.086 Move                              OK       Rotator is at the expected position: 315
12:22:39.539 Move                              OK       Asynchronous move successful - moved by -40 degrees to: 275 degrees
12:22:39.586 Move                              OK       Rotator is at the expected position: 275
12:22:49.039 Move                              OK       Asynchronous move successful - moved by 40 degrees to: 315 degrees
12:22:49.087 Move                              OK       Rotator is at the expected position: 315
12:23:18.476 Move                              OK       Asynchronous move successful - moved by -130 degrees to: 185 degrees
12:23:18.508 Move                              OK       Rotator is at the expected position: 185
12:23:48.367 Move                              OK       Asynchronous move successful - moved by 130 degrees to: 315 degrees
12:23:48.399 Move                              OK       Rotator is at the expected position: 315
12:25:12.633 Move                              OK       Asynchronous move successful - moved by -375 degrees to: -60 degrees
12:25:12.696 Move                              INFO     Rotator supports angles < 0.0
12:25:12.711 Move                              OK       Rotator is at the expected position: -60
12:26:36.695 Move                              OK       Asynchronous move successful - moved by 375 degrees to: 315 degrees
12:26:36.742 Move                              OK       Rotator is at the expected position: 315

Performance
12:26:41.883 Position                          INFO     Transaction rate: 328.4 per second
12:26:46.899 TargetPosition                    INFO     Transaction rate: 355.4 per second
12:26:46.899 StepSize                          INFO     Skipping test as property is not supported
12:26:51.930 IsMoving                          INFO     Transaction rate: 353.2 per second

Conformance test complete

No errors, warnings or issues found: your driver passes ASCOM validation!!

-------------------------------------------------------------------------------------

ConformanceCheck ASCOM Device Conformance Checker Version 6.5.7500.22515, Build time: 14/07/2020 13:30:30
ConformanceCheck Running on: ASCOM Platform 6.5 SP1 6.5.1.3234

ConformanceCheck Driver ProgID: ASCOM.AlpacaDynamic3.Telescope

Error handling
Error number for "Not Implemented" is: 80040400
Error number for "Invalid Value 1" is: 80040401
Error number for "Invalid Value 2" is: 80040405
Error number for "Value Not Set 1" is: 80040402
Error number for "Value Not Set 2" is: 80040403
Error messages will not be interpreted to infer state.

16:32:14.593 Driver Access Checks              OK       
16:32:15.514 AccessChecks                      OK       Successfully created driver using late binding
16:32:16.250 AccessChecks                      OK       Successfully connected using late binding
16:32:16.256 AccessChecks                      INFO     The driver is a COM object
16:32:17.270 AccessChecks                      INFO     Device does not expose interface ITelescopeV2
16:32:18.694 AccessChecks                      INFO     Device exposes interface ITelescopeV3
16:32:20.251 AccessChecks                      OK       Successfully created driver using driver access toolkit
16:32:21.209 AccessChecks                      OK       Successfully connected using driver access toolkit

Conform is using ASCOM.DriverAccess.Telescope to get a Telescope object
16:32:22.822 ConformanceCheck                  OK       Driver instance created successfully
16:32:23.853 ConformanceCheck                  OK       Connected OK

Common Driver Methods
16:32:23.927 InterfaceVersion                  OK       3
16:32:23.985 Connected                         OK       True
16:32:24.066 Description                       OK       Mount Simulator
16:32:24.118 DriverInfo                        OK       ASCOM Dynamic Driver v6.5.1.3234 - REMOTE DEVICE: indigo_mount_simulator
16:32:24.180 DriverVersion                     OK       2.0.0.6
16:32:24.242 Name                              OK       Mount Simulator
16:32:24.292 CommandString                     INFO     Conform cannot test the CommandString method
16:32:24.301 CommandBlind                      INFO     Conform cannot test the CommandBlind method
16:32:24.308 CommandBool                       INFO     Conform cannot test the CommandBool method
16:32:24.316 Action                            INFO     Conform cannot test the Action method
16:32:24.352 SupportedActions                  OK       Driver returned an empty action list

Can Properties
16:32:24.491 CanFindHome                       OK       False
16:32:24.509 CanPark                           OK       True
16:32:24.527 CanPulseGuide                     OK       False
16:32:24.552 CanSetDeclinationRate             OK       False
16:32:24.587 CanSetGuideRates                  OK       True
16:32:24.622 CanSetPark                        OK       False
16:32:24.693 CanSetPierSide                    OK       False
16:32:24.745 CanSetRightAscensionRate          OK       False
16:32:24.776 CanSetTracking                    OK       True
16:32:24.799 CanSlew                           OK       True
16:32:24.832 CanSlewltAz                       OK       False
16:32:24.857 CanSlewAltAzAsync                 OK       False
16:32:24.882 CanSlewAsync                      OK       True
16:32:24.933 CanSync                           OK       True
16:32:24.960 CanSyncAltAz                      OK       False
16:32:24.999 CanUnPark                         OK       True

Pre-run Checks
16:32:25.135 Mount Safety                      INFO     Scope is parked, so it has been unparked for testing
16:32:25.211 Mount Safety                      INFO     Scope tracking has been enabled
16:32:25.253 TimeCheck                         INFO     PC Time Zone:  W. Europe Standard Time, offset -1 hours.
16:32:25.263 TimeCheck                         INFO     PC UTCDate:    25-Mar-2021 15:32:25.263
16:32:25.318 TimeCheck                         INFO     Mount UTCDate: 25-Mar-2021 15:32:25.000

Properties
16:32:25.418 AlignmentMode                     OK       algPolar
16:32:25.499 Altitude                          OK       49.00
16:32:25.609 ApertureArea                      OK       Optional member threw a PropertyNotImplementedException exception.
16:32:25.673 ApertureDiameter                  OK       Optional member threw a PropertyNotImplementedException exception.
16:32:25.738 AtHome                            OK       False
16:32:25.797 AtPark                            OK       False
16:32:25.856 Azimuth                           OK       0.00
16:32:25.930 Declination                       OK        90:00:00.00
16:32:25.999 DeclinationRate Read              OK       0.00
16:32:26.082 DeclinationRate Write             OK       CanSetDeclinationRate is False and a PropertyNotImplementedException exception was generated as expected
16:32:26.159 DoesRefraction Read               OK       Optional member threw a PropertyNotImplementedException exception.
16:32:26.263 DoesRefraction Write              OK       Optional member threw a PropertyNotImplementedException exception.
16:32:26.339 EquatorialSystem                  OK       equJ2000
16:32:26.402 FocalLength                       OK       Optional member threw a PropertyNotImplementedException exception.
16:32:26.472 GuideRateDeclination Read         OK       50.00
16:32:26.501 GuideRateDeclination Write        OK       Can write Declination Guide Rate OK
16:32:26.571 GuideRateRightAscension Read      OK       50.00
16:32:26.600 GuideRateRightAscension Write     OK       Can set RightAscension Guide OK
16:32:26.661 IsPulseGuiding                    OK       CanPulseGuide is False and a PropertyNotImplementedException exception was generated as expected
16:32:26.717 RightAscension                    OK       22:53:53.32
16:32:26.829 RightAscensionRate Read           OK       0.00
16:32:26.901 RightAscensionRate Write          OK       CanSetRightAscensionRate is False and a PropertyNotImplementedException exception was generated as expected
16:32:26.948 SiteElevation Read                OK       0
16:32:27.002 SiteElevation Write               OK       Invalid Value exception generated as expected on set site elevation < -300m
16:32:27.050 SiteElevation Write               OK       Invalid Value exception generated as expected on set site elevation > 10,000m
16:32:27.086 SiteElevation Write               OK       Legal value 0m written successfully
16:32:27.143 SiteLatitude Read                 OK        49:00:00.00
16:32:27.219 SiteLatitude Write                OK       Invalid Value exception generated as expected on set site latitude < -90 degrees
16:32:27.257 SiteLatitude Write                OK       Invalid Value exception generated as expected on set site latitude > 90 degrees
16:32:27.290 SiteLatitude Write                OK       Legal value  49:00:00.00 degrees written successfully
16:32:27.375 SiteLongitude Read                OK        17:00:00.00
16:32:27.484 SiteLongitude Write               OK       Invalid Value exception generated as expected on set site longitude < -180 degrees
16:32:27.522 SiteLongitude Write               OK       Invalid Value exception generated as expected on set site longitude > 180 degrees
16:32:27.572 SiteLongitude Write               OK       Legal value  17:00:00.00 degrees written successfully
16:32:27.642 Slewing                           OK       False
16:32:27.708 SlewSettleTime Read               OK       Optional member threw a PropertyNotImplementedException exception.
16:32:27.812 SlewSettleTime Write              OK       Optional member threw a PropertyNotImplementedException exception.
16:32:27.837 SlewSettleTime Write              OK       Optional member threw a PropertyNotImplementedException exception.
16:32:27.919 SideOfPier Read                   OK       Optional member threw a PropertyNotImplementedException exception.
16:32:27.941 SiderealTime                      OK       04:53:56.33
16:32:27.953 SiderealTime                      OK       Scope and ASCOM sidereal times agree to better than 1 minute, Scope: 04:53:56.33, ASCOM: 04:53:43.71
16:32:28.059 TargetDeclination Read            OK       COM InvalidOperationException generated on read before write
16:32:28.105 TargetDeclination Write           INFO     Tests moved after the SlewToCoordinates tests so that Conform can check they properly set target coordinates.
16:32:28.139 TargetDeclination Read            OK       COM InvalidOperationException generated on read before write
16:32:28.182 TargetRightAscension Write        INFO     Tests moved after the SlewToCoordinates tests so that Conform can check they properly set target coordinates.
16:32:28.237 Tracking Read                     OK       True
16:32:29.710 Tracking Write                    OK       False
16:32:30.904 TrackingRates                              Found drive rate: driveSidereal
16:32:30.920 TrackingRates                              Found drive rate: driveLunar
16:32:30.947 TrackingRates                              Found drive rate: driveSolar
16:32:30.972 TrackingRates                              Found drive rate: driveKing
16:32:31.008 TrackingRates                     OK       Drive rates read OK
16:32:31.059 TrackingRates                     OK       Disposed tracking rates OK
16:32:31.128 TrackingRates                     OK       Successfully obtained a TrackingRates object after the previous TrackingRates object was disposed
16:32:31.174 TrackingRate Read                 OK       driveSidereal
16:32:31.297 TrackingRate Write                OK       Successfully set drive rate: driveSidereal
16:32:31.392 TrackingRate Write                OK       Successfully set drive rate: driveLunar
16:32:31.484 TrackingRate Write                OK       Successfully set drive rate: driveSolar
16:32:31.568 TrackingRate Write                OK       Successfully set drive rate: driveKing
16:32:31.621 TrackingRate Write                OK       Invalid Value exception generated as expected when TrackingRate is set to an invalid value (5)
16:32:31.689 TrackingRate Write                OK       Invalid Value exception generated as expected when TrackingRate is set to an invalid value (-1)
16:32:31.804 UTCDate Read                      OK       25-Mar-2021 15:32:25.000
16:32:31.842 UTCDate Write                     OK       Optional member threw a PropertyNotImplementedException exception.

Methods
16:32:31.993 CanMoveAxis:Primary               OK       CanMoveAxis:Primary False
16:32:32.079 CanMoveAxis:Secondary             OK       CanMoveAxis:Secondary False
16:32:32.144 CanMoveAxis:Tertiary              OK       CanMoveAxis:Tertiary False
16:32:33.872 Park                              OK       Success
16:32:33.969 Park                              OK       Success if already parked
16:32:34.030 Parked:AbortSlew                  OK       AbortSlew did raise an exception when Parked as required
16:32:34.259 Parked:SlewToCoordinates          OK       SlewToCoordinates did raise an exception when Parked as required
16:32:34.401 Parked:SlewToCoordinatesAsync     OK       SlewToCoordinatesAsync did raise an exception when Parked as required
16:32:34.577 Parked:SlewToTarget               OK       SlewToTarget did raise an exception when Parked as required
16:32:34.816 Parked:SlewToTargetAsync          OK       SlewToTargetAsync did raise an exception when Parked as required
16:32:34.946 Parked:SyncToCoordinates          OK       SyncToCoordinates did raise an exception when Parked as required
16:32:35.144 Parked:SyncToTarget               OK       SyncToTarget did raise an exception when Parked as required
16:32:35.913 UnPark                            OK       Success
16:32:35.964 UnPark                            OK       Success if already unparked
16:32:36.068 AbortSlew                         OK       AbortSlew OK when not slewing
16:32:36.214 AxisRate:Primary                  OK       Empty axis rate returned
16:32:36.237 AxisRate:Primary                  OK       Disposed axis rates OK
16:32:36.305 AxisRate:Secondary                OK       Empty axis rate returned
16:32:36.348 AxisRate:Secondary                OK       Disposed axis rates OK
16:32:36.444 AxisRate:Tertiary                 OK       Empty axis rate returned
16:32:36.472 AxisRate:Tertiary                 OK       Disposed axis rates OK
16:32:36.507 FindHome                          OK       CanFindHome is False and a MethodNotImplementedException exception was generated as expected
16:32:36.686 MoveAxis Primary                  OK       CanMoveAxis Primary is False and a MethodNotImplementedException exception was generated as expected
16:32:36.820 MoveAxis Secondary                OK       CanMoveAxis Secondary is False and a MethodNotImplementedException exception was generated as expected
16:32:36.958 MoveAxis Tertiary                 OK       CanMoveAxis Tertiary is False and a MethodNotImplementedException exception was generated as expected
16:32:37.099 PulseGuide                        OK       CanPulseGuide is False and a MethodNotImplementedException exception was generated as expected
16:32:57.709 SlewToCoordinates                 OK       Slewed OK. RA:   03:54:05.36
16:32:57.727 SlewToCoordinates                 OK       Slewed OK. DEC:  01:00:00.00
16:32:57.774 SlewToCoordinates                 OK       The TargetRightAscension property 03:54:05.36 matches the expected RA OK.
16:32:57.824 SlewToCoordinates                 OK       The TargetDeclination property  01:00:00.00 matches the expected Declination OK.
16:32:57.946 SlewToCoordinates (Bad L)         OK       Correctly rejected bad RA coordinate: -01:00:00.00
16:32:58.019 SlewToCoordinates (Bad L)         OK       Correctly rejected bad Dec coordinate: -100:00:00.00
16:32:58.131 SlewToCoordinates (Bad H)         OK       Correctly rejected bad RA coordinate: 25:00:00.00
16:32:58.186 SlewToCoordinates (Bad H)         OK       Correctly rejected bad Dec coordinate: 100:00:00.00
16:33:03.407 SlewToCoordinatesAsync            OK       Slewed OK. RA:   02:54:26.41
16:33:03.423 SlewToCoordinatesAsync            OK       Slewed OK. DEC:  02:00:00.00
16:33:03.472 SlewToCoordinatesAsync            OK       The TargetRightAscension property 02:54:26.41 matches the expected RA OK.
16:33:03.533 SlewToCoordinatesAsync            OK       The TargetDeclination property  02:00:00.00 matches the expected Declination OK.
16:33:03.628 SlewToCoordinatesAsync (Bad L)    OK       Correctly rejected bad RA coordinate: -01:00:00.00
16:33:03.709 SlewToCoordinatesAsync (Bad L)    OK       Correctly rejected bad Dec coordinate: -100:00:00.00
16:33:03.815 SlewToCoordinatesAsync (Bad H)    OK       Correctly rejected bad RA coordinate: 25:00:00.00
16:33:03.861 SlewToCoordinatesAsync (Bad H)    OK       Correctly rejected bad Dec coordinate: 100:00:00.00
16:33:09.184 SyncToCoordinates                 OK       Slewed to start position OK. RA:   01:54:32.43
16:33:09.199 SyncToCoordinates                 OK       Slewed to start position OK. DEC:  24:30:00.00
16:33:09.286 SyncToCoordinates                 OK       Synced to sync position OK. RA:   01:50:32.43
16:33:09.307 SyncToCoordinates                 OK       Synced to sync position OK. DEC:  23:30:00.00
16:33:09.351 SyncToCoordinates                 OK       The TargetRightAscension property 01:50:32.43 matches the expected RA OK.
16:33:09.405 SyncToCoordinates                 OK       The TargetDeclination property  23:30:00.00 matches the expected Declination OK.
16:33:14.824 SyncToCoordinates                 OK       Slewed back to start position OK. RA:   01:54:32.43
16:33:14.852 SyncToCoordinates                 OK       Slewed back to start position OK. DEC:  24:30:00.00
16:33:14.990 SyncToCoordinates                 OK       Synced to reversed sync position OK. RA:   01:58:32.43
16:33:15.014 SyncToCoordinates                 OK       Synced to reversed sync position OK. DEC:  25:30:00.00
16:33:20.254 SyncToCoordinates                 OK       Slewed back to start position OK. RA:   01:54:32.43
16:33:20.282 SyncToCoordinates                 OK       Slewed back to start position OK. DEC:  24:30:00.00
16:33:20.422 SyncToCoordinates (Bad L)         OK       Correctly rejected bad RA coordinate: -01:00:00.00
16:33:20.479 SyncToCoordinates (Bad L)         OK       Correctly rejected bad Dec coordinate: -100:00:00.00
16:33:20.594 SyncToCoordinates (Bad H)         OK       Correctly rejected bad RA coordinate: 25:00:00.00
16:33:20.658 SyncToCoordinates (Bad H)         OK       Correctly rejected bad Dec coordinate: 100:00:00.00
16:33:20.757 TargetRightAscension Write        OK       Invalid Value exception generated as expected on set TargetRightAscension < 0 hours
16:33:20.796 TargetRightAscension Write        OK       Invalid Value exception generated as expected on set TargetRightAscension > 24 hours
16:33:20.868 TargetRightAscension Write        OK       Target RightAscension is within 1 second of the value set: 00:54:48.47
16:33:20.944 TargetDeclination Write           OK       Invalid Value exception generated as expected on set TargetDeclination < -90 degrees
16:33:20.969 TargetDeclination Write           OK       Invalid Value exception generated as expected on set TargetDeclination < -90 degrees
16:33:21.011 TargetDeclination Write           OK       Legal value  01:00:00.00 DD:MM:SS written successfully
16:33:25.240 SlewToTarget                      OK       Slewed OK. RA:   01:54:49.48
16:33:25.252 SlewToTarget                      OK       Slewed OK. DEC:  03:00:00.00
16:33:25.299 SlewToTarget                      OK       The TargetRightAscension property 01:54:49.48 matches the expected RA OK.
16:33:25.355 SlewToTarget                      OK       The TargetDeclination property  03:00:00.00 matches the expected Declination OK.
16:33:25.507 SlewToTarget (Bad L)              OK       Telescope.TargetRA correctly rejected bad RA coordinate: -01:00:00.00
16:33:25.549 SlewToTarget (Bad L)              OK       Telescope.TargetDeclination correctly rejected bad Dec coordinate: -100:00:00.00
16:33:25.652 SlewToTarget (Bad H)              OK       Telescope.TargetRA correctly rejected bad RA coordinate: 25:00:00.00
16:33:25.690 SlewToTarget (Bad H)              OK       Telescope.TargetDeclination correctly rejected bad Dec coordinate: 100:00:00.00
16:33:31.119 SlewToTargetAsync                 OK       Slewed OK. RA:   00:54:54.49
16:33:31.155 SlewToTargetAsync                 OK       Slewed OK. DEC:  04:00:00.00
16:33:31.204 SlewToTargetAsync                 OK       The TargetRightAscension property 00:54:54.49 matches the expected RA OK.
16:33:31.271 SlewToTargetAsync                 OK       The TargetDeclination property  04:00:00.00 matches the expected Declination OK.
16:33:31.362 SlewToTargetAsync (Bad L)         OK       Telescope.TargetRA correctly rejected bad RA coordinate: -01:00:00.00
16:33:31.410 SlewToTargetAsync (Bad L)         OK       Telescope.TargetDeclination correctly rejected bad Dec coordinate: -100:00:00.00
16:33:31.549 SlewToTargetAsync (Bad H)         OK       Telescope.TargetRA correctly rejected bad RA coordinate: 25:00:00.00
16:33:31.613 SlewToTargetAsync (Bad H)         OK       Telescope.TargetDeclination correctly rejected bad Dec coordinate: 100:00:00.00
16:33:31.712 DestinationSideOfPier                      Test skipped as AligmentMode is not German Polar
16:33:31.817 SlewToAltAz                       OK       CanSlewAltAz is False and a MethodNotImplementedException exception was generated as expected
16:33:31.942 SlewToAltAzAsync                  OK       CanSlewAltAzAsync is False and a MethodNotImplementedException exception was generated as expected
16:33:37.272 SyncToTarget                      OK       Slewed to start position OK. RA:   01:55:00.51
16:33:37.303 SyncToTarget                      OK       Slewed to start position OK. DEC:  24:30:00.00
16:33:37.494 SyncToTarget                      OK       Synced to sync position OK. RA:   01:51:00.51
16:33:37.511 SyncToTarget                      OK       Synced to sync position OK. DEC:  23:30:00.00
16:33:42.758 SyncToTarget                      OK       Slewed back to start position OK. RA:   01:55:00.51
16:33:42.780 SyncToTarget                      OK       Slewed back to start position OK. DEC:  24:30:00.00
16:33:42.912 SyncToTarget                      OK       Synced to reversed sync position OK. RA:   01:59:00.51
16:33:42.936 SyncToTarget                      OK       Synced to reversed sync position OK. DEC:  25:30:00.00
16:33:48.097 SyncToTarget                      OK       Slewed back to start position OK. RA:   01:55:00.51
16:33:48.115 SyncToTarget                      OK       Slewed back to start position OK. DEC:  24:30:00.00
16:33:48.237 SyncToTarget (Bad L)              OK       Telescope.TargetRA correctly rejected bad RA coordinate: -01:00:00.00
16:33:48.300 SyncToTarget (Bad L)              OK       Telescope.TargetDeclination correctly rejected bad Dec coordinate: -100:00:00.00
16:33:48.406 SyncToTarget (Bad H)              OK       Telescope.TargetRA correctly rejected bad RA coordinate: 25:00:00.00
16:33:48.465 SyncToTarget (Bad H)              OK       Telescope.TargetDeclination correctly rejected bad Dec coordinate: 100:00:00.00
16:33:48.632 SyncToAltAz                       OK       CanSyncAltAz is False and a MethodNotImplementedException exception was generated as expected

SideOfPier Model Tests
16:33:48.713 SideOfPier Model Tests            INFO     Tests skipped because this driver does Not support SideOfPier Read

Post-run Checks
16:33:48.866 Mount Safety                      OK       Tracking stopped to protect your mount.

Conformance test complete

No errors, warnings or issues found: your driver passes ASCOM validation!!
