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
