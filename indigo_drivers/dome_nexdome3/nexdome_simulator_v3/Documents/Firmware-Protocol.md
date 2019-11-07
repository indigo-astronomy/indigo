# Firmware Command Protocol

In normal use, the Rotator unit handles all communications and will forward commands and responses to and from the shutter unit.
For diagnostic purposes, it is also possible to issue shutter commands directly to the shutter unit by connecting a USB cable.
The shutter will not forward commands to the rotator.

## Command Grammar

Commands have the form: 

<kbd>@</kbd> `Verb` `target` <kbd>,</kbd> `Parameter` <kbd>`<CR>`</kbd><kbd>`<LF>`</kbd>

The parts of this command syntax are:

- <kbd>@</kbd> is a literal character that marks the start of a new command and clears the receive buffer.
- `Verb` is the command verb, which normally consists of two characters. Single character verbs are also possible but in this case the entire command is a single character.
- `Device` is the target device for the command, generally `R` (rotator) or `S` (shutter).
- <kbd>,</kbd> is a literal character that separates the device ID from the parameter. The comma is required if a parameter is present and should be absent if there is no parameter.
- `Parameter` is usually a signed or unsigned decimal integer and is the data payload for commands that need data. Refer to the notes for each command for specific requirements.
- <kbd>`<CR>`</kbd><kbd>`<LF>`</kbd> is the command terminator and submits the command to the command dispatcher. Only one is required and if both are present then they can be in any order.

Example: `@AWS,1000` {set shutter acceleration ramp time to 1000 ms}.

## Errors

Any unrecognised or invalid command responds with the text `:Err#`.

## Responses

In general, responses always begin with `:` and end with `#`.

Unless otherwise stated in the table below, all commands respond by echoing their command code and target device, in the format:

<kbd>:</kbd> cct <kbd>#</kbd>

where `cc` is the original command verb and `t` is the target device ('R' for rotator, 'S' for shutter).

The parameter value (if any) is not echoed. For example, the response to `@GAR,1000` is `:GAR#`. Receipt of this echo response indicates that the command is valid and was successfully received.

Commands that return a value include the value immediately after the target and before the terminating `#`. Example: `:VRR10000#`

Any command that cannot be processed for any reason will respond with `:Err#`

**Other status and debug output may be generated at any time, not necessarily in response to any command.** See [Event Notifications](#event-notifications) below.

## Command Details

In the following table, upper case letters are literal characters. Lower case letters are placeholders.
Unless otherwise specified, the range of step values is 0 to 4,294,967,295 (2^32 - 1).
Due to firmware resource constraints, range checking for "sensible settings" is left to driver and application developers.
If in doubt, the ASCOM driver (open source) serves as a reference implementation.

Cmd | Targets | Parameter | Default | Min | Max | Response   | Example    | Description
--- | ------- | --------- | ------- | --- | --- | ---------- | ---------- | -----------------------------------------------------------------------------
AR  | RS      | none      |         |     |     | :ARddddd#  | @ARR       | Read acceleration ramp time in milliseconds
AW  | RS      | ddddd     | 1500    | 100 |     | :AWt#      | @AWS,1000  | Write acceleration ramp time
CL  | S       |           |         |     |     | :CLS#      | @CLS       | Close the shutter (see [event notifications][Events])
DR  | R       | none      |         |     |     | :DRRddddd# | @DRR       | Read Dead-zone in steps (153 steps = 1 degree)
DW  | R       | ddddd     | 300     | 0   |     | :DWR#      | @DWR,300   | Write Dead-zone in steps [0..10000] default 300
FR  | RS      | none      |         |     |     | :FRstring# | @FRR       | Reads the semantic version (SemVer) string of the firmware.
GA  | R       | ddd       |         | 0   | 359 | :GAR#      | @GAR,180   | Goto Azimuth (param: integer degrees)
GH  | R       | none      |         |     |     | :GHR#      | @GHR       | Rotates clockwise until the home sensor is detected and synchronizes the azimuth to the home position.
HR  | R       | none      |         |     |     | :HRRddddd# | @HRR       | Home position Read (steps clockwise from true north)
HW  | R       | ddddd     | 0       | 0   | @RR | :HWR#      | @HWR,1000  | Home position Write (seps clockwise from true north)
OP  | S       |           |         |     |     | :OPS#      | @OPS       | Open the shutter (see [event notifications][Events])
PR  | RS      | none      |         |     |     | :PRt-dddd# | @PRR       | Position Read - get current step position in whole steps (signed integer)
PW  | RS      | ddddd     |         | 0   | @RR | :PWt#      | @PWR,-1000 | Position Write (sync) - set step position
RR  | RS      | none      |         |     |     | :RRtdddd   | @RRS       | Range Read - read the range of travel in whole steps.
RW  | R       | ddddd     | 55080   | 0   |     | :RWR#      | @RWR,64000 | Range Write (Rotator). Sets the dome circumference, in whole steps.
RW  | S       | ddddd     | 46000   | 0   |     | :RWS#      | @RWS,64000 | Range Write (Shutter). Sets the maximum shutter travel in whole steps.
SR  | RS      | none      |         |     |     | :SEt,...#  | @SRR       | Requests an immediate status report. Status returned as comma separated values.
SW  | RS      | none      |         |     |     | :SWt#      | @SWS       | Performs an immediate "hard stop" without decelerating.
VR  | RS      | none      |         |     |     | :VRtddddd# | @VRR       | Velocity read - motor speed in whole steps per second
VW  | R       | ddddd     | 600     | 32  |     | :VWR#      | @VWR,10000 | Velocity Write (Rotator) - motor speed in whole steps per second
VW  | S       | ddddd     | 800     | 32  |     | :VWS#      | @VWS,10000 | Velocity Write (Shutter) - motor speed in whole steps per second
ZD  | RS      | none      |         |     |     | :ZDt#      | @ZDR       | Load factory defaults into working settings (does not save)
ZR  | RS      | none      |         |     |     | :ZRt#      | @ZRR       | Load saved settings from EEPROM into memory
ZW  | RS      | none      |         |     |     | :ZWt#      | @ZWR       | Write working settings to EEPROM

## Event Notifications

The firmware will produce notifications of various events that can happen whether or not the application is requesting them.
These may arrive at any time, for example in between a command and a response, but they will never divide a response.
**Client software must be prepared to handle these event notifications at any time**.

Format          | Description
--------------  | -----------------------------------------------------------------------------------
XB->{state}     | The current state of the XBee communications link. The states are enumerated below.
XB->Start       | Waiting for XBee to boot up or reboot
XB->WaitAT      | Waiting for XBee to enter command mode
XB->Config      | Sending configuration
XB->Detect      | Attempting to detect the shutter
XB->Online      | Communications link established
Pddddd          | Rotator position (ddddd = **signed** decimal integer)
Sddddd          | Shutter position (ddddd = **signed** decimal integer)
:SER,p,a,c,h,d# | Rotator status report. Sent when the rotator motor stops or when requested by @SRR
                |  p = current azimuth position in whole steps,  
                |  a = AtHome (1 = home sensor active, 0 = home sensor inactive)
                |  c = dome circumference in whole steps
                |  h = home position sensor location, in whole steps clockwise from true north
                |  d = dead zone (smallest allowed movement in steps)
:SES,p,l,o,c#   | Shutter status report. Sent when shutter motor stops or when requested by @SRS
                |  p = position in steps
                |  l = limit of travel (fully open position) in steps
                |  o = state of open limit switch, 1=active, 0=inactive
                |  c = state of closed limit switch, 1=active, 0=inactive
:left#          | The rotator is about to move to the left (counter-clockwise)
:right#         | The rotator is about to move to the right (clockwise)
:open#          | The shutter is about to move towards open
:close#         | The shutter is about to move towards closed
:BVddddd#       | Shutter battery volts in raw analogue-to-digital units (ADUs), range [0..1023].
:Rain#          | Rain detected (shutter will auto-close)
:RainStopped#   | Rain detector was previously indicating rain but has now stopped doing so.

### Notes on Events

Position updates occur about every 250 milliseconds while motion is occurring.
When motion ceases, an SER or SES status event is emitted and this indicates that motion of the corresponding motor has ceased.

Important: Don't assume that the motors will only move when requested bu the application.
Motors can move for a variety of reasons, including:

- because the user is pressing the toggle switch
- a weather shutdown event

If that happens, all the normal events will be sent, including direction notification, position updates and finally a status report when the movement stops.
Therefore, applications must be prepared to accept event notifications even if they are not currently requesting anything.

## Other Output

It is possible that other undocumented output may be produced and the host application should be constructed in such a way that this output is ignored.
Undocumented/Diagnostic output should not be relied upon by a host application for any purpose as it may change without notice.

[Events]: #EventNotifications "Notifications that can occur at any time"
