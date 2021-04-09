Driver ASCOM Conformance Test Results
-------------------------------------------------------------------------------------------------------------

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

------------------------------------------------------------------------------------

ConformanceCheck ASCOM Device Conformance Checker Version 6.5.7500.22515, Build time: 7/14/2020 12:30:30 PM
ConformanceCheck Running on: ASCOM Platform 6.5 SP1 6.5.1.3234

ConformanceCheck Driver ProgID: ASCOM.AlpacaDynamic1.Dome

Error handling
Error number for "Not Implemented" is: 80040400
Error number for "Invalid Value 1" is: 80040405
Error number for "Value Not Set 1" is: 80040402
Error number for "Value Not Set 2" is: 80040403
Error messages will not be interpreted to infer state.

16:10:55.159 Driver Access Checks              OK
16:10:56.100 AccessChecks                      OK       Successfully created driver using late binding
16:10:56.832 AccessChecks                      OK       Successfully connected using late binding
16:10:56.832 AccessChecks                      INFO     The driver is a COM object
16:10:57.769 AccessChecks                      INFO     Device does not expose interface IDome
16:10:58.457 AccessChecks                      INFO     Device exposes interface IDomeV2
16:10:59.987 AccessChecks                      OK       Successfully created driver using driver access toolkit
16:11:00.659 AccessChecks                      OK       Successfully connected using driver access toolkit

Conform is using ASCOM.DriverAccess.Dome to get a Dome object
16:11:02.191 ConformanceCheck                  OK       Driver instance created successfully
16:11:02.988 ConformanceCheck                  OK       Connected OK

Common Driver Methods
16:11:03.112 InterfaceVersion                  OK       1
16:11:03.194 Connected                         OK       True
16:11:03.238 Description                       OK       Dome Simulator
16:11:03.299 DriverInfo                        OK       ASCOM Dynamic Driver v6.5.1.3234 - REMOTE DEVICE: indigo_dome_simulator
16:11:03.349 DriverVersion                     OK       2.0.0.5
16:11:03.408 Name                              OK       Dome Simulator
16:11:03.506 CommandString                     INFO     Conform cannot test the CommandString method
16:11:03.518 CommandBlind                      INFO     Conform cannot test the CommandBlind method
16:11:03.534 CommandBool                       INFO     Conform cannot test the CommandBool method
16:11:03.565 Action                            INFO     Conform cannot test the Action method
16:11:03.596 SupportedActions                  OK       Driver returned an empty action list

Can Properties
16:11:03.768 CanFindHome                       OK       False
16:11:03.816 CanPark                           OK       True
16:11:03.880 CanSetAltitude                    OK       False
16:11:03.940 CanSetAzimuth                     OK       True
16:11:04.003 CanSetPark                        OK       True
16:11:04.050 CanSetShutter                     OK       True
16:11:04.097 CanSlave                          OK       True
16:11:04.144 CanSyncAzimuth                    OK       False

Pre-run Checks
16:11:04.644 DomeSafety                                 Attempting to open shutter as some tests may fail if it is closed...
16:11:05.097 DomeSafety                        OK       Shutter status: shutterOpen

Properties
16:11:05.253 Altitude                          OK       Optional member threw a PropertyNotImplementedException exception.
16:11:05.347 AtHome                            OK       Optional member threw a PropertyNotImplementedException exception.
16:11:05.409 AtPark                            OK       True
16:11:05.456 Azimuth                           OK       0
16:11:05.534 ShutterStatus                     OK       shutterOpen
16:11:05.581 Slaved Read                       OK       False
16:11:05.675 Slaved Write                      OK       Slave state changed successfully
16:11:05.753 Slewing                           OK       False

Methods
16:11:05.956 AbortSlew                         OK       AbortSlew command issued successfully
16:11:06.037 SlewToAltitude                    OK       Optional member threw a MethodNotImplementedException exception.
16:11:06.800 SlewToAzimuth 0                   OK       Asynchronous slew OK
16:11:22.816 SlewToAzimuth 45                  OK       Asynchronous slew OK
16:11:38.677 SlewToAzimuth 90                  OK       Asynchronous slew OK
16:11:54.799 SlewToAzimuth 135                 OK       Asynchronous slew OK
16:12:10.722 SlewToAzimuth 180                 OK       Asynchronous slew OK
16:12:26.831 SlewToAzimuth 225                 OK       Asynchronous slew OK
16:12:42.646 SlewToAzimuth 270                 OK       Asynchronous slew OK
16:12:58.737 SlewToAzimuth 315                 OK       Asynchronous slew OK
16:13:09.440 SlewToAzimuth                     OK       COM invalid value exception correctly raised for slew to -10 degrees
16:13:09.503 SlewToAzimuth                     OK       COM invalid value exception correctly raised for slew to 370 degrees
16:13:09.643 SyncToAzimuth                     OK       Optional member threw a MethodNotImplementedException exception.
16:13:10.084 CloseShutter                      OK       Shutter closed successfully
16:13:31.815 OpenShutter                       OK       Shutter opened successfully
16:13:53.206 FindHome                          OK       Optional member threw a MethodNotImplementedException exception.
16:13:58.550 Park                              OK       Dome parked successfully
16:14:09.362 SetPark                           OK       SetPark issued OK

Post-run Checks
16:14:09.628 DomeSafety                        INFO     Attempting to close shutter...
16:14:09.943 DomeSafety                        OK       Shutter successfully closed
16:14:09.971 DomeSafety                        INFO     Attempting to park dome...
16:14:10.706 DomeSafety                        OK       Dome successfully parked

Conformance test complete

No errors, warnings or issues found: your driver passes ASCOM validation!!

---------------------------------------------------------------------------------------


ConformanceCheck ASCOM Device Conformance Checker Version 6.5.7500.22515, Build time: 7/14/2020 12:30:30 PM
ConformanceCheck Running on: ASCOM Platform 6.5 SP1 6.5.1.3234

ConformanceCheck Driver ProgID: ASCOM.AlpacaDynamic2.Switch

Error handling
Error number for "Not Implemented" is: 80040400
Error number for "Invalid Value 1" is: 80040405
Error number for "Value Not Set 1" is: 80040402
Error number for "Value Not Set 2" is: 80040403
Error messages will not be interpreted to infer state.

06:04:01.270 Driver Access Checks              OK
06:04:02.254 AccessChecks                      OK       Successfully created driver using late binding
06:04:02.989 AccessChecks                      OK       Successfully connected using late binding
06:04:02.989 AccessChecks                      INFO     The driver is a COM object
06:04:03.943 AccessChecks                      INFO     Device does not expose interface ISwitch
06:04:04.630 AccessChecks                      INFO     Device exposes interface ISwitchV2
06:04:05.614 AccessChecks                      OK       Successfully created driver using driver access toolkit
06:04:06.286 AccessChecks                      OK       Successfully connected using driver access toolkit

Conform is using ASCOM.DriverAccess.Switch to get a Switch object
06:04:06.397 ConformanceCheck                  OK       Driver instance created successfully
06:04:06.974 ConformanceCheck                  OK       Connected OK

Common Driver Methods
06:04:07.067 InterfaceVersion                  OK       2
06:04:07.114 Connected                         OK       True
06:04:07.163 Description                       OK       Dragonfly Controller
06:04:07.225 DriverInfo                        OK       ASCOM Dynamic Driver v6.5.1.3234 - REMOTE DEVICE: indigo_aux_dragonfly
06:04:07.317 DriverVersion                     OK       2.0.0.4
06:04:07.396 Name                              OK       Dragonfly Controller
06:04:07.442 CommandString                     INFO     Conform cannot test the CommandString method
06:04:07.442 CommandBlind                      INFO     Conform cannot test the CommandBlind method
06:04:07.442 CommandBool                       INFO     Conform cannot test the CommandBool method
06:04:07.457 Action                            INFO     Conform cannot test the Action method
06:04:07.473 SupportedActions                  OK       Driver returned an empty action list

Properties
06:04:07.708 MaxSwitch                         OK       16

Methods
06:04:07.926 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID below 0 was used in method: CanWrite
06:04:07.942 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID above MaxSwitch was used in method: CanWrite
06:04:07.942 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID below 0 was used in method: GetSwitch
06:04:07.957 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID above MaxSwitch was used in method: GetSwitch
06:04:07.957 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID below 0 was used in method: GetSwitchDescription
06:04:07.973 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID above MaxSwitch was used in method: GetSwitchDescription
06:04:07.973 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID below 0 was used in method: GetSwitchName
06:04:07.990 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID above MaxSwitch was used in method: GetSwitchName
06:04:08.004 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID below 0 was used in method: GetSwitchValue
06:04:08.021 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID above MaxSwitch was used in method: GetSwitchValue
06:04:08.021 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID below 0 was used in method: MaxSwitchValue
06:04:08.037 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID above MaxSwitch was used in method: MaxSwitchValue
06:04:08.052 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID below 0 was used in method: MinSwitchValue
06:04:08.052 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID above MaxSwitch was used in method: MinSwitchValue
06:04:08.115 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID below 0 was used in method: SetSwitch
06:04:08.130 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID above MaxSwitch was used in method: SetSwitch
06:04:08.145 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID below 0 was used in method: SetSwitchValue
06:04:08.208 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID above MaxSwitch was used in method: SetSwitchValue
06:04:08.238 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID below 0 was used in method: SwitchStep
06:04:08.255 SwitchNumber                      OK       Switch device threw an InvalidOperationException when a switch ID above MaxSwitch was used in method: SwitchStep
06:04:14.208 GetSwitchName                     OK       Found switch 0
06:04:14.238 GetSwitchName                     OK         Name: Relay #1
06:04:14.254 GetSwitchDescription              OK         Description: Relay #1
06:04:14.254 MinSwitchValue                    OK         Minimum: 0
06:04:14.270 MaxSwitchValue                    OK         Maximum: 1
06:04:14.286 SwitchStep                        OK         Step size: 1
06:04:14.286 SwitchStep                        OK         Step size is greater than zero
06:04:14.286 SwitchStep                        OK         Step size is less than the range of possible values
06:04:14.301 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:04:14.301 CanWrite                          OK         CanWrite: True
06:04:14.318 GetSwitch                         OK         False
06:04:14.318 GetSwitchValue                    OK         0
06:04:15.067 SetSwitch                         OK         GetSwitch returned False after SetSwitch(False)
06:04:15.114 SetSwitch                         OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitch(False)
06:04:18.944 SetSwitch                         OK         GetSwitch read True after SetSwitch(True)
06:04:18.973 SetSwitch                         OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitch(True)
06:04:26.113 SetSwitchValue                    OK         GetSwitch returned False after SetSwitchValue(MINIMUM_VALUE)
06:04:26.145 SetSwitchValue                    OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitchValue(MINIMUM_VALUE)
06:04:29.285 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value below SwitchMinimum was set: -1
06:04:33.223 SetSwitchValue                    OK         GetSwitch returned True after SetSwitchValue(MAXIMUM_VALUE)
06:04:33.255 SetSwitchValue                    OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitchValue(MAXIMUM_VALUE)
06:04:36.348 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value above SwitchMaximum was set: 2
06:04:39.458 SetSwitchValue                    INFO       Testing with steps that are 0% offset from integer SwitchStep values
06:04:40.286 SetSwitchValue Offset:   0%       OK         Set and read match: 0
06:04:44.176 SetSwitchValue Offset:   0%       OK         Set and read match: 1
06:04:47.349 SetSwitchValue                    INFO       Testing with steps that are 25% offset from integer SwitchStep values
06:04:48.132 SetSwitchValue Offset:  25%       INFO       Set/Read differ by 20-30% of SwitchStep. Set: 0.25, Read: 0
06:04:51.303 SetSwitchValue                    INFO       Testing with steps that are 50% offset from integer SwitchStep values
06:04:52.020 SetSwitchValue Offset:  50%       INFO       Set/Read differ by 40-50% of SwitchStep. Set: 0.5, Read: 0
06:04:55.192 SetSwitchValue                    INFO       Testing with steps that are 75% offset from integer SwitchStep values
06:04:55.911 SetSwitchValue Offset:  75%       INFO       Set/Read differ by 70-80% of SwitchStep. Set: 0.75, Read: 0
06:04:59.239 SetSwitchValue                    OK         Switch has been reset to its original state

06:05:02.427 GetSwitchName                     OK       Found switch 1
06:05:02.427 GetSwitchName                     OK         Name: Relay #2
06:05:02.442 GetSwitchDescription              OK         Description: Relay #2
06:05:02.442 MinSwitchValue                    OK         Minimum: 0
06:05:02.459 MaxSwitchValue                    OK         Maximum: 1
06:05:02.473 SwitchStep                        OK         Step size: 1
06:05:02.473 SwitchStep                        OK         Step size is greater than zero
06:05:02.473 SwitchStep                        OK         Step size is less than the range of possible values
06:05:02.489 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:05:02.489 CanWrite                          OK         CanWrite: True
06:05:02.505 GetSwitch                         OK         False
06:05:02.520 GetSwitchValue                    OK         0
06:05:03.207 SetSwitch                         OK         GetSwitch returned False after SetSwitch(False)
06:05:03.239 SetSwitch                         OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitch(False)
06:05:07.207 SetSwitch                         OK         GetSwitch read True after SetSwitch(True)
06:05:07.236 SetSwitch                         OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitch(True)
06:05:14.364 SetSwitchValue                    OK         GetSwitch returned False after SetSwitchValue(MINIMUM_VALUE)
06:05:14.395 SetSwitchValue                    OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitchValue(MINIMUM_VALUE)
06:05:17.551 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value below SwitchMinimum was set: -1
06:05:21.458 SetSwitchValue                    OK         GetSwitch returned True after SetSwitchValue(MAXIMUM_VALUE)
06:05:21.473 SetSwitchValue                    OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitchValue(MAXIMUM_VALUE)
06:05:24.536 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value above SwitchMaximum was set: 2
06:05:27.676 SetSwitchValue                    INFO       Testing with steps that are 0% offset from integer SwitchStep values
06:05:28.520 SetSwitchValue Offset:   0%       OK         Set and read match: 0
06:05:32.473 SetSwitchValue Offset:   0%       OK         Set and read match: 1
06:05:35.660 SetSwitchValue                    INFO       Testing with steps that are 25% offset from integer SwitchStep values
06:05:36.412 SetSwitchValue Offset:  25%       INFO       Set/Read differ by 20-30% of SwitchStep. Set: 0.25, Read: 0
06:05:39.600 SetSwitchValue                    INFO       Testing with steps that are 50% offset from integer SwitchStep values
06:05:40.411 SetSwitchValue Offset:  50%       INFO       Set/Read differ by 40-50% of SwitchStep. Set: 0.5, Read: 0
06:05:43.630 SetSwitchValue                    INFO       Testing with steps that are 75% offset from integer SwitchStep values
06:05:44.286 SetSwitchValue Offset:  75%       INFO       Set/Read differ by 70-80% of SwitchStep. Set: 0.75, Read: 0
06:05:47.617 SetSwitchValue                    OK         Switch has been reset to its original state

06:05:50.770 GetSwitchName                     OK       Found switch 2
06:05:50.785 GetSwitchName                     OK         Name: Relay #3
06:05:50.801 GetSwitchDescription              OK         Description: Relay #3
06:05:50.834 MinSwitchValue                    OK         Minimum: 0
06:05:50.848 MaxSwitchValue                    OK         Maximum: 1
06:05:50.848 SwitchStep                        OK         Step size: 1
06:05:50.864 SwitchStep                        OK         Step size is greater than zero
06:05:50.864 SwitchStep                        OK         Step size is less than the range of possible values
06:05:50.879 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:05:50.879 CanWrite                          OK         CanWrite: True
06:05:50.895 GetSwitch                         OK         False
06:05:50.895 GetSwitchValue                    OK         0
06:05:51.629 SetSwitch                         OK         GetSwitch returned False after SetSwitch(False)
06:05:51.660 SetSwitch                         OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitch(False)
06:05:55.599 SetSwitch                         OK         GetSwitch read True after SetSwitch(True)
06:05:55.615 SetSwitch                         OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitch(True)
06:06:02.677 SetSwitchValue                    OK         GetSwitch returned False after SetSwitchValue(MINIMUM_VALUE)
06:06:02.723 SetSwitchValue                    OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitchValue(MINIMUM_VALUE)
06:06:05.817 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value below SwitchMinimum was set: -1
06:06:09.723 SetSwitchValue                    OK         GetSwitch returned True after SetSwitchValue(MAXIMUM_VALUE)
06:06:09.739 SetSwitchValue                    OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitchValue(MAXIMUM_VALUE)
06:06:12.849 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value above SwitchMaximum was set: 2
06:06:15.926 SetSwitchValue                    INFO       Testing with steps that are 0% offset from integer SwitchStep values
06:06:16.770 SetSwitchValue Offset:   0%       OK         Set and read match: 0
06:06:20.676 SetSwitchValue Offset:   0%       OK         Set and read match: 1
06:06:23.802 SetSwitchValue                    INFO       Testing with steps that are 25% offset from integer SwitchStep values
06:06:24.676 SetSwitchValue Offset:  25%       INFO       Set/Read differ by 20-30% of SwitchStep. Set: 0.25, Read: 0
06:06:27.864 SetSwitchValue                    INFO       Testing with steps that are 50% offset from integer SwitchStep values
06:06:28.614 SetSwitchValue Offset:  50%       INFO       Set/Read differ by 40-50% of SwitchStep. Set: 0.5, Read: 0
06:06:31.833 SetSwitchValue                    INFO       Testing with steps that are 75% offset from integer SwitchStep values
06:06:32.520 SetSwitchValue Offset:  75%       INFO       Set/Read differ by 70-80% of SwitchStep. Set: 0.75, Read: 0
06:06:35.802 SetSwitchValue                    OK         Switch has been reset to its original state

06:06:39.021 GetSwitchName                     OK       Found switch 3
06:06:39.021 GetSwitchName                     OK         Name: Relay #4
06:06:39.036 GetSwitchDescription              OK         Description: Relay #4
06:06:39.052 MinSwitchValue                    OK         Minimum: 0
06:06:39.067 MaxSwitchValue                    OK         Maximum: 1
06:06:39.067 SwitchStep                        OK         Step size: 1
06:06:39.083 SwitchStep                        OK         Step size is greater than zero
06:06:39.098 SwitchStep                        OK         Step size is less than the range of possible values
06:06:39.098 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:06:39.114 CanWrite                          OK         CanWrite: True
06:06:39.114 GetSwitch                         OK         False
06:06:39.130 GetSwitchValue                    OK         0
06:06:39.864 SetSwitch                         OK         GetSwitch returned False after SetSwitch(False)
06:06:39.895 SetSwitch                         OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitch(False)
06:06:43.708 SetSwitch                         OK         GetSwitch read True after SetSwitch(True)
06:06:43.739 SetSwitch                         OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitch(True)
06:06:50.821 SetSwitchValue                    OK         GetSwitch returned False after SetSwitchValue(MINIMUM_VALUE)
06:06:50.847 SetSwitchValue                    OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitchValue(MINIMUM_VALUE)
06:06:53.942 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value below SwitchMinimum was set: -1
06:06:57.930 SetSwitchValue                    OK         GetSwitch returned True after SetSwitchValue(MAXIMUM_VALUE)
06:06:57.973 SetSwitchValue                    OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitchValue(MAXIMUM_VALUE)
06:07:01.051 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value above SwitchMaximum was set: 2
06:07:04.129 SetSwitchValue                    INFO       Testing with steps that are 0% offset from integer SwitchStep values
06:07:04.973 SetSwitchValue Offset:   0%       OK         Set and read match: 0
06:07:08.864 SetSwitchValue Offset:   0%       OK         Set and read match: 1
06:07:12.008 SetSwitchValue                    INFO       Testing with steps that are 25% offset from integer SwitchStep values
06:07:12.818 SetSwitchValue Offset:  25%       INFO       Set/Read differ by 20-30% of SwitchStep. Set: 0.25, Read: 0
06:07:15.989 SetSwitchValue                    INFO       Testing with steps that are 50% offset from integer SwitchStep values
06:07:16.786 SetSwitchValue Offset:  50%       INFO       Set/Read differ by 40-50% of SwitchStep. Set: 0.5, Read: 0
06:07:19.926 SetSwitchValue                    INFO       Testing with steps that are 75% offset from integer SwitchStep values
06:07:20.630 SetSwitchValue Offset:  75%       INFO       Set/Read differ by 70-80% of SwitchStep. Set: 0.75, Read: 0
06:07:23.818 SetSwitchValue                    OK         Switch has been reset to its original state

06:07:26.988 GetSwitchName                     OK       Found switch 4
06:07:27.005 GetSwitchName                     OK         Name: Relay #5
06:07:27.020 GetSwitchDescription              OK         Description: Relay #5
06:07:27.020 MinSwitchValue                    OK         Minimum: 0
06:07:27.036 MaxSwitchValue                    OK         Maximum: 1
06:07:27.051 SwitchStep                        OK         Step size: 1
06:07:27.051 SwitchStep                        OK         Step size is greater than zero
06:07:27.067 SwitchStep                        OK         Step size is less than the range of possible values
06:07:27.067 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:07:27.083 CanWrite                          OK         CanWrite: True
06:07:27.083 GetSwitch                         OK         False
06:07:27.099 GetSwitchValue                    OK         0
06:07:27.879 SetSwitch                         OK         GetSwitch returned False after SetSwitch(False)
06:07:27.926 SetSwitch                         OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitch(False)
06:07:31.817 SetSwitch                         OK         GetSwitch read True after SetSwitch(True)
06:07:31.832 SetSwitch                         OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitch(True)
06:07:39.005 SetSwitchValue                    OK         GetSwitch returned False after SetSwitchValue(MINIMUM_VALUE)
06:07:39.036 SetSwitchValue                    OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitchValue(MINIMUM_VALUE)
06:07:42.129 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value below SwitchMinimum was set: -1
06:07:46.053 SetSwitchValue                    OK         GetSwitch returned True after SetSwitchValue(MAXIMUM_VALUE)
06:07:46.082 SetSwitchValue                    OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitchValue(MAXIMUM_VALUE)
06:07:49.206 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value above SwitchMaximum was set: 2
06:07:52.285 SetSwitchValue                    INFO       Testing with steps that are 0% offset from integer SwitchStep values
06:07:53.114 SetSwitchValue Offset:   0%       OK         Set and read match: 0
06:07:57.004 SetSwitchValue Offset:   0%       OK         Set and read match: 1
06:08:00.145 SetSwitchValue                    INFO       Testing with steps that are 25% offset from integer SwitchStep values
06:08:00.958 SetSwitchValue Offset:  25%       INFO       Set/Read differ by 20-30% of SwitchStep. Set: 0.25, Read: 0
06:08:04.114 SetSwitchValue                    INFO       Testing with steps that are 50% offset from integer SwitchStep values
06:08:04.832 SetSwitchValue Offset:  50%       INFO       Set/Read differ by 40-50% of SwitchStep. Set: 0.5, Read: 0
06:08:07.973 SetSwitchValue                    INFO       Testing with steps that are 75% offset from integer SwitchStep values
06:08:08.648 SetSwitchValue Offset:  75%       INFO       Set/Read differ by 70-80% of SwitchStep. Set: 0.75, Read: 0
06:08:11.958 SetSwitchValue                    OK         Switch has been reset to its original state

06:08:15.161 GetSwitchName                     OK       Found switch 5
06:08:15.161 GetSwitchName                     OK         Name: Relay #6
06:08:15.176 GetSwitchDescription              OK         Description: Relay #6
06:08:15.207 MinSwitchValue                    OK         Minimum: 0
06:08:15.208 MaxSwitchValue                    OK         Maximum: 1
06:08:15.224 SwitchStep                        OK         Step size: 1
06:08:15.239 SwitchStep                        OK         Step size is greater than zero
06:08:15.239 SwitchStep                        OK         Step size is less than the range of possible values
06:08:15.254 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:08:15.254 CanWrite                          OK         CanWrite: True
06:08:15.271 GetSwitch                         OK         False
06:08:15.271 GetSwitchValue                    OK         0
06:08:15.974 SetSwitch                         OK         GetSwitch returned False after SetSwitch(False)
06:08:15.989 SetSwitch                         OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitch(False)
06:08:20.020 SetSwitch                         OK         GetSwitch read True after SetSwitch(True)
06:08:20.038 SetSwitch                         OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitch(True)
06:08:27.114 SetSwitchValue                    OK         GetSwitch returned False after SetSwitchValue(MINIMUM_VALUE)
06:08:27.145 SetSwitchValue                    OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitchValue(MINIMUM_VALUE)
06:08:30.271 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value below SwitchMinimum was set: -1
06:08:34.208 SetSwitchValue                    OK         GetSwitch returned True after SetSwitchValue(MAXIMUM_VALUE)
06:08:34.223 SetSwitchValue                    OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitchValue(MAXIMUM_VALUE)
06:08:37.333 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value above SwitchMaximum was set: 2
06:08:40.445 SetSwitchValue                    INFO       Testing with steps that are 0% offset from integer SwitchStep values
06:08:41.270 SetSwitchValue Offset:   0%       OK         Set and read match: 0
06:08:45.207 SetSwitchValue Offset:   0%       OK         Set and read match: 1
06:08:48.332 SetSwitchValue                    INFO       Testing with steps that are 25% offset from integer SwitchStep values
06:08:49.117 SetSwitchValue Offset:  25%       INFO       Set/Read differ by 20-30% of SwitchStep. Set: 0.25, Read: 0
06:08:52.317 SetSwitchValue                    INFO       Testing with steps that are 50% offset from integer SwitchStep values
06:08:53.051 SetSwitchValue Offset:  50%       INFO       Set/Read differ by 40-50% of SwitchStep. Set: 0.5, Read: 0
06:08:56.223 SetSwitchValue                    INFO       Testing with steps that are 75% offset from integer SwitchStep values
06:08:56.895 SetSwitchValue Offset:  75%       INFO       Set/Read differ by 70-80% of SwitchStep. Set: 0.75, Read: 0
06:09:00.161 SetSwitchValue                    OK         Switch has been reset to its original state

06:09:03.349 GetSwitchName                     OK       Found switch 6
06:09:03.364 GetSwitchName                     OK         Name: Relay #7
06:09:03.364 GetSwitchDescription              OK         Description: Relay #7
06:09:03.382 MinSwitchValue                    OK         Minimum: 0
06:09:03.382 MaxSwitchValue                    OK         Maximum: 1
06:09:03.395 SwitchStep                        OK         Step size: 1
06:09:03.395 SwitchStep                        OK         Step size is greater than zero
06:09:03.411 SwitchStep                        OK         Step size is less than the range of possible values
06:09:03.411 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:09:03.411 CanWrite                          OK         CanWrite: True
06:09:03.430 GetSwitch                         OK         False
06:09:03.442 GetSwitchValue                    OK         0
06:09:04.254 SetSwitch                         OK         GetSwitch returned False after SetSwitch(False)
06:09:04.301 SetSwitch                         OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitch(False)
06:09:08.101 SetSwitch                         OK         GetSwitch read True after SetSwitch(True)
06:09:08.145 SetSwitch                         OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitch(True)
06:09:15.333 SetSwitchValue                    OK         GetSwitch returned False after SetSwitchValue(MINIMUM_VALUE)
06:09:15.379 SetSwitchValue                    OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitchValue(MINIMUM_VALUE)
06:09:18.427 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value below SwitchMinimum was set: -1
06:09:22.349 SetSwitchValue                    OK         GetSwitch returned True after SetSwitchValue(MAXIMUM_VALUE)
06:09:22.379 SetSwitchValue                    OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitchValue(MAXIMUM_VALUE)
06:09:25.457 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value above SwitchMaximum was set: 2
06:09:28.567 SetSwitchValue                    INFO       Testing with steps that are 0% offset from integer SwitchStep values
06:09:29.380 SetSwitchValue Offset:   0%       OK         Set and read match: 0
06:09:33.270 SetSwitchValue Offset:   0%       OK         Set and read match: 1
06:09:36.458 SetSwitchValue                    INFO       Testing with steps that are 25% offset from integer SwitchStep values
06:09:37.286 SetSwitchValue Offset:  25%       INFO       Set/Read differ by 20-30% of SwitchStep. Set: 0.25, Read: 0
06:09:40.459 SetSwitchValue                    INFO       Testing with steps that are 50% offset from integer SwitchStep values
06:09:41.192 SetSwitchValue Offset:  50%       INFO       Set/Read differ by 40-50% of SwitchStep. Set: 0.5, Read: 0
06:09:44.349 SetSwitchValue                    INFO       Testing with steps that are 75% offset from integer SwitchStep values
06:09:45.051 SetSwitchValue Offset:  75%       INFO       Set/Read differ by 70-80% of SwitchStep. Set: 0.75, Read: 0
06:09:48.286 SetSwitchValue                    OK         Switch has been reset to its original state

06:09:51.408 GetSwitchName                     OK       Found switch 7
06:09:51.426 GetSwitchName                     OK         Name: Relay #8
06:09:51.442 GetSwitchDescription              OK         Description: Relay #8
06:09:51.458 MinSwitchValue                    OK         Minimum: 0
06:09:51.458 MaxSwitchValue                    OK         Maximum: 1
06:09:51.473 SwitchStep                        OK         Step size: 1
06:09:51.473 SwitchStep                        OK         Step size is greater than zero
06:09:51.489 SwitchStep                        OK         Step size is less than the range of possible values
06:09:51.489 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:09:51.505 CanWrite                          OK         CanWrite: True
06:09:51.505 GetSwitch                         OK         False
06:09:51.520 GetSwitchValue                    OK         0
06:09:52.223 SetSwitch                         OK         GetSwitch returned False after SetSwitch(False)
06:09:52.255 SetSwitch                         OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitch(False)
06:09:56.067 SetSwitch                         OK         GetSwitch read True after SetSwitch(True)
06:09:56.098 SetSwitch                         OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitch(True)
06:10:03.161 SetSwitchValue                    OK         GetSwitch returned False after SetSwitchValue(MINIMUM_VALUE)
06:10:03.192 SetSwitchValue                    OK         GetSwitchValue returned MINIMUM_VALUE after SetSwitchValue(MINIMUM_VALUE)
06:10:06.270 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value below SwitchMinimum was set: -1
06:10:10.177 SetSwitchValue                    OK         GetSwitch returned True after SetSwitchValue(MAXIMUM_VALUE)
06:10:10.208 SetSwitchValue                    OK         GetSwitchValue returned MAXIMUM_VALUE after SetSwitchValue(MAXIMUM_VALUE)
06:10:13.286 SetSwitchValue                    OK         Switch threw an InvalidOperationException when a value above SwitchMaximum was set: 2
06:10:16.381 SetSwitchValue                    INFO       Testing with steps that are 0% offset from integer SwitchStep values
06:10:17.223 SetSwitchValue Offset:   0%       OK         Set and read match: 0
06:10:21.161 SetSwitchValue Offset:   0%       OK         Set and read match: 1
06:10:24.382 SetSwitchValue                    INFO       Testing with steps that are 25% offset from integer SwitchStep values
06:10:25.301 SetSwitchValue Offset:  25%       INFO       Set/Read differ by 20-30% of SwitchStep. Set: 0.25, Read: 0
06:10:28.551 SetSwitchValue                    INFO       Testing with steps that are 50% offset from integer SwitchStep values
06:10:29.301 SetSwitchValue Offset:  50%       INFO       Set/Read differ by 40-50% of SwitchStep. Set: 0.5, Read: 0
06:10:32.473 SetSwitchValue                    INFO       Testing with steps that are 75% offset from integer SwitchStep values
06:10:33.145 SetSwitchValue Offset:  75%       INFO       Set/Read differ by 70-80% of SwitchStep. Set: 0.75, Read: 0
06:10:36.397 SetSwitchValue                    OK         Switch has been reset to its original state

06:10:39.552 GetSwitchName                     OK       Found switch 8
06:10:39.567 GetSwitchName                     OK         Name: Sensor #1
06:10:39.598 GetSwitchDescription              OK         Description: Sensor #1
06:10:39.598 MinSwitchValue                    OK         Minimum: 0
06:10:39.614 MaxSwitchValue                    OK         Maximum: 1024
06:10:39.630 SwitchStep                        OK         Step size: 1
06:10:39.630 SwitchStep                        OK         Step size is greater than zero
06:10:39.646 SwitchStep                        OK         Step size is less than the range of possible values
06:10:39.646 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:10:39.662 CanWrite                          OK         CanWrite: False
06:10:39.662 GetSwitch                         OK         False
06:10:39.676 GetSwitchValue                    OK         11
06:10:39.726 SetSwitch                         OK         CanWrite is False and MethodNotImplementedException was thrown
06:10:39.739 SetSwitchValue                    OK         CanWrite is False and MethodNotImplementedException was thrown

06:10:39.787 GetSwitchName                     OK       Found switch 9
06:10:39.787 GetSwitchName                     OK         Name: Sensor #2
06:10:39.801 GetSwitchDescription              OK         Description: Sensor #2
06:10:39.801 MinSwitchValue                    OK         Minimum: 0
06:10:39.817 MaxSwitchValue                    OK         Maximum: 1024
06:10:39.817 SwitchStep                        OK         Step size: 1
06:10:39.833 SwitchStep                        OK         Step size is greater than zero
06:10:39.833 SwitchStep                        OK         Step size is less than the range of possible values
06:10:39.849 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:10:39.849 CanWrite                          OK         CanWrite: False
06:10:39.865 GetSwitch                         OK         False
06:10:39.865 GetSwitchValue                    OK         12
06:10:39.927 SetSwitch                         OK         CanWrite is False and MethodNotImplementedException was thrown
06:10:39.927 SetSwitchValue                    OK         CanWrite is False and MethodNotImplementedException was thrown

06:10:39.990 GetSwitchName                     OK       Found switch 10
06:10:39.990 GetSwitchName                     OK         Name: Sensor #3
06:10:40.004 GetSwitchDescription              OK         Description: Sensor #3
06:10:40.020 MinSwitchValue                    OK         Minimum: 0
06:10:40.036 MaxSwitchValue                    OK         Maximum: 1024
06:10:40.052 SwitchStep                        OK         Step size: 1
06:10:40.052 SwitchStep                        OK         Step size is greater than zero
06:10:40.052 SwitchStep                        OK         Step size is less than the range of possible values
06:10:40.068 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:10:40.068 CanWrite                          OK         CanWrite: False
06:10:40.083 GetSwitch                         OK         False
06:10:40.083 GetSwitchValue                    OK         13
06:10:40.145 SetSwitch                         OK         CanWrite is False and MethodNotImplementedException was thrown
06:10:40.161 SetSwitchValue                    OK         CanWrite is False and MethodNotImplementedException was thrown

06:10:40.208 GetSwitchName                     OK       Found switch 11
06:10:40.240 GetSwitchName                     OK         Name: Sensor #4
06:10:40.254 GetSwitchDescription              OK         Description: Sensor #4
06:10:40.270 MinSwitchValue                    OK         Minimum: 0
06:10:40.286 MaxSwitchValue                    OK         Maximum: 1024
06:10:40.301 SwitchStep                        OK         Step size: 1
06:10:40.301 SwitchStep                        OK         Step size is greater than zero
06:10:40.301 SwitchStep                        OK         Step size is less than the range of possible values
06:10:40.318 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:10:40.318 CanWrite                          OK         CanWrite: False
06:10:40.334 GetSwitch                         OK         False
06:10:40.348 GetSwitchValue                    OK         14
06:10:40.395 SetSwitch                         OK         CanWrite is False and MethodNotImplementedException was thrown
06:10:40.411 SetSwitchValue                    OK         CanWrite is False and MethodNotImplementedException was thrown

06:10:40.459 GetSwitchName                     OK       Found switch 12
06:10:40.459 GetSwitchName                     OK         Name: Sensor #5
06:10:40.473 GetSwitchDescription              OK         Description: Sensor #5
06:10:40.473 MinSwitchValue                    OK         Minimum: 0
06:10:40.490 MaxSwitchValue                    OK         Maximum: 1024
06:10:40.505 SwitchStep                        OK         Step size: 1
06:10:40.505 SwitchStep                        OK         Step size is greater than zero
06:10:40.521 SwitchStep                        OK         Step size is less than the range of possible values
06:10:40.521 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:10:40.536 CanWrite                          OK         CanWrite: False
06:10:40.536 GetSwitch                         OK         False
06:10:40.552 GetSwitchValue                    OK         15
06:10:40.599 SetSwitch                         OK         CanWrite is False and MethodNotImplementedException was thrown
06:10:40.615 SetSwitchValue                    OK         CanWrite is False and MethodNotImplementedException was thrown

06:10:40.663 GetSwitchName                     OK       Found switch 13
06:10:40.663 GetSwitchName                     OK         Name: Sensor #6
06:10:40.677 GetSwitchDescription              OK         Description: Sensor #6
06:10:40.677 MinSwitchValue                    OK         Minimum: 0
06:10:40.693 MaxSwitchValue                    OK         Maximum: 1024
06:10:40.693 SwitchStep                        OK         Step size: 1
06:10:40.708 SwitchStep                        OK         Step size is greater than zero
06:10:40.708 SwitchStep                        OK         Step size is less than the range of possible values
06:10:40.708 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:10:40.724 CanWrite                          OK         CanWrite: False
06:10:40.739 GetSwitch                         OK         False
06:10:40.739 GetSwitchValue                    OK         16
06:10:40.786 SetSwitch                         OK         CanWrite is False and MethodNotImplementedException was thrown
06:10:40.801 SetSwitchValue                    OK         CanWrite is False and MethodNotImplementedException was thrown

06:10:40.866 GetSwitchName                     OK       Found switch 14
06:10:40.880 GetSwitchName                     OK         Name: Sensor #7
06:10:40.896 GetSwitchDescription              OK         Description: Sensor #7
06:10:40.896 MinSwitchValue                    OK         Minimum: 0
06:10:40.911 MaxSwitchValue                    OK         Maximum: 1024
06:10:40.926 SwitchStep                        OK         Step size: 1
06:10:40.926 SwitchStep                        OK         Step size is greater than zero
06:10:40.943 SwitchStep                        OK         Step size is less than the range of possible values
06:10:40.943 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:10:40.958 CanWrite                          OK         CanWrite: False
06:10:40.958 GetSwitch                         OK         False
06:10:40.974 GetSwitchValue                    OK         17
06:10:41.020 SetSwitch                         OK         CanWrite is False and MethodNotImplementedException was thrown
06:10:41.036 SetSwitchValue                    OK         CanWrite is False and MethodNotImplementedException was thrown

06:10:41.085 GetSwitchName                     OK       Found switch 15
06:10:41.085 GetSwitchName                     OK         Name: Sensor #8
06:10:41.099 GetSwitchDescription              OK         Description: Sensor #8
06:10:41.115 MinSwitchValue                    OK         Minimum: 0
06:10:41.115 MaxSwitchValue                    OK         Maximum: 1024
06:10:41.130 SwitchStep                        OK         Step size: 1
06:10:41.130 SwitchStep                        OK         Step size is greater than zero
06:10:41.146 SwitchStep                        OK         Step size is less than the range of possible values
06:10:41.146 SwitchStep                        OK         The switch range is an integer multiple of the step size.
06:10:41.161 CanWrite                          OK         CanWrite: False
06:10:41.161 GetSwitch                         OK         False
06:10:41.177 GetSwitchValue                    OK         18
06:10:41.223 SetSwitch                         OK         CanWrite is False and MethodNotImplementedException was thrown
06:10:41.239 SetSwitchValue                    OK         CanWrite is False and MethodNotImplementedException was thrown


Conformance test complete

No errors, warnings or issues found: your driver passes ASCOM validation!!
