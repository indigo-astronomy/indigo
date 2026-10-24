// Copyright (c) 2026 Rumen G. Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 3.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>, shared library for all PrimaLuce Lab drivers;
//     the protocol code comes from indigo_focuser_primaluce by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** PrimaLuce Lab JSON protocol library
 \file primalucelib.h
 */

// The JSON protocol spoken by PrimaLuce Lab controllers: SESTO SENSO 2 and 3 and ESATTO focusers,
// the ARCO rotator (motor 2 of a focuser controller), the GIOTTO flat panel and the ALTO cover.
// See PROTOCOL.md in this directory for the protocol itself and for which parts are verified on
// hardware.
//
// The library is built as a static library (libprimalucelib.a, primalucelib.lib on Windows) and
// linked into every PrimaLuce driver. A driver includes this header from the include { } block of
// its .driver definition:
//
//   #include "../shared/primalucelib/primalucelib.h"
//
// The library knows nothing about devices or properties: a driver keeps one primalucelib_link per
// controller connection, in its private data, and serializes the requests on it (the device
// queue does this).
//
// Naming: primalucelib_<object>_<action>() sends one request. primalucelib_reply_<what>() reads
// the reply of the last request without sending anything. Pointers returned by either point into
// the link's response buffer and stay valid until the next request on the same link.

#ifndef primalucelib_h
#define primalucelib_h

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver.h>
#include <indigo/indigo_uni_io.h>

// Only the jsmn types are visible outside the library; its functions are compiled into the library
// with internal linkage. JSMN_PARENT_LINKS changes the token layout, so it is set here for everyone.
#define JSMN_STRICT
#define JSMN_PARENT_LINKS
#define JSMN_HEADER
#include "jsmn.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma mark - Transport

#define PRIMALUCE_SERIAL_BAUDRATE                 115200
// The controller's web server answers GET /ajax.php?jreq=<URL-encoded request>, one request per connection.
#define PRIMALUCE_HTTP_DEFAULT_PORT               80
#define PRIMALUCE_HTTP_REQUEST_PATH               "/ajax.php?jreq="
// Seconds allowed for the HTTP connect and for each chunk of the HTTP reply.
#define PRIMALUCE_HTTP_TIMEOUT                    5
// The full state of a SESTO SENSO 2 is about 3.9 kB over USB.
#define PRIMALUCE_MAX_RESPONSE_SIZE               8192
#define PRIMALUCE_MAX_TOKENS                      1024
// After REBOOT the controller prints a boot log on the serial line; it is polled this often, this far apart.
#define PRIMALUCE_RESTART_POLLS                   20
#define PRIMALUCE_RESTART_POLL_INTERVAL           0.5
// A restarting controller is asked whether it is back with this read timeout (seconds).
#define PRIMALUCE_RESTART_PROBE_TIMEOUT           0.3
// Replies that answer an earlier request are dropped, this many at most, before a request fails.
#define PRIMALUCE_MAX_STALE_REPLIES               4

#pragma mark - Envelope keys

#define PRIMALUCE_KEY_REQUEST                     "req"
#define PRIMALUCE_KEY_RESPONSE                    "res"
#define PRIMALUCE_VERB_GET                        "get"
#define PRIMALUCE_VERB_SET                        "set"
#define PRIMALUCE_VERB_CMD                        "cmd"
#define PRIMALUCE_VERB_SRV                        "srv"
// A refused request is answered with this key under the request verb (or under the motor node).
#define PRIMALUCE_KEY_ERROR                       "ERROR"

#pragma mark - Controller keys

#define PRIMALUCE_KEY_MODEL_NAME                  "MODNAME"
#define PRIMALUCE_KEY_SERIAL_NUMBER               "SN"
#define PRIMALUCE_KEY_FIRMWARE_VERSIONS           "SWVERS"
#define PRIMALUCE_KEY_APPLICATION_FIRMWARE        "SWAPP"
#define PRIMALUCE_KEY_WEB_FIRMWARE                "SWWEB"
#define PRIMALUCE_KEY_CALIBRATION_RESTART         "CALRESTART"
#define PRIMALUCE_KEY_EXTERNAL_TEMPERATURE        "EXT_T"
#define PRIMALUCE_KEY_SUPPLY_VOLTAGE              "VIN_12V"
#define PRIMALUCE_KEY_USB_VOLTAGE                 "VIN_USB"
#define PRIMALUCE_KEY_LEDS                        "DIMLEDS"
#define PRIMALUCE_KEY_LAN_CONFIG                  "LANCFG"
#define PRIMALUCE_KEY_ACCESS_POINT                "WIFIAP"
#define PRIMALUCE_KEY_STATION                     "WIFISTA"
#define PRIMALUCE_KEY_SSID                        "SSID"
#define PRIMALUCE_KEY_PASSWORD                    "PWD"
#define PRIMALUCE_KEY_ACCESS_POINT_STATUS         "STATUS"
#define PRIMALUCE_KEY_SET_ACCESS_POINT_STATUS     "AP_SET_STATUS"
#define PRIMALUCE_KEY_REBOOT                      "REBOOT"
#define PRIMALUCE_KEY_LOG_LEVEL                   "LOGLEVEL"
#define PRIMALUCE_KEY_RUN_PRESET                  "RUNPRESET"
#define PRIMALUCE_KEY_RUN_PRESET_LIGHT            "RUNPRESET_L"
#define PRIMALUCE_KEY_RUN_PRESET_MEDIUM           "RUNPRESET_M"
#define PRIMALUCE_KEY_RUN_PRESET_SLOW             "RUNPRESET_S"
#define PRIMALUCE_KEY_RUN_PRESET_USER_PREFIX      "RUNPRESET_"
#define PRIMALUCE_KEY_STORED_POSITION_PREFIX      "PRESET_"
#define PRIMALUCE_KEY_STORED_POSITION_NAME        "NAME"
#define PRIMALUCE_KEY_STORED_POSITION_STEPS       "M1POS"
#define PRIMALUCE_KEY_RECOVERY_DELAY              "RECOVER_DELAY"
#define PRIMALUCE_KEY_MODEL_SUBMODEL              "GET_MODEL_SUBMODEL"

#pragma mark - Motor keys

#define PRIMALUCE_KEY_FOCUSER_MOTOR               "MOT1"
#define PRIMALUCE_KEY_ROTATOR_MOTOR               "MOT2"
// The ALTO cover drives its arm with motor 1, like a focuser.
#define PRIMALUCE_KEY_COVER_MOTOR                 PRIMALUCE_KEY_FOCUSER_MOTOR
// Documented mechanical position (protocol 4.0).
#define PRIMALUCE_KEY_ABS_POS_STEP                "ABS_POS_STEP"
// Older firmware: undocumented signed driver register, queried with the unit as its value.
#define PRIMALUCE_KEY_ABS_POS                     "ABS_POS"
// ARCO: mechanical angle.
#define PRIMALUCE_KEY_ABS_POS_DEG                 "ABS_POS_DEG"
// ARCO: angle after a sync, the reference of MOVE_ABS.
#define PRIMALUCE_KEY_POSITION_DEG                "POSITION_DEG"
#define PRIMALUCE_KEY_POSITION_STEP               "POSITION_STEP"
#define PRIMALUCE_KEY_POSITION_ARCSEC             "POSITION_ARCSEC"
#define PRIMALUCE_KEY_STATUS                      "STATUS"
#define PRIMALUCE_KEY_MOTOR_STATE                 "MST"
#define PRIMALUCE_KEY_BUSY                        "BUSY"
#define PRIMALUCE_KEY_MOTOR_TEMPERATURE           "NTC_T"
#define PRIMALUCE_KEY_BACKLASH                    "BKLASH"
#define PRIMALUCE_KEY_SPEED                       "SPEED"
#define PRIMALUCE_KEY_CALIBRATION_MIN             "CAL_MINPOS"
#define PRIMALUCE_KEY_CALIBRATION_MAX             "CAL_MAXPOS"
#define PRIMALUCE_KEY_CALIBRATION_DIRECTION       "CAL_DIR"
#define PRIMALUCE_KEY_FOCUSER_CALIBRATION         "CAL_FOCUSER"
#define PRIMALUCE_KEY_ROTATOR_CALIBRATION         "CAL_STATUS"
#define PRIMALUCE_KEY_COVER_CALIBRATION           "CAL_ALTO"
#define PRIMALUCE_KEY_HOLD_CURRENT_STATUS         "HOLDCURR_STATUS"
#define PRIMALUCE_KEY_ACCELERATION_SPEED          "FnRUN_ACC"
#define PRIMALUCE_KEY_RUN_SPEED                   "FnRUN_SPD"
#define PRIMALUCE_KEY_DECELERATION_SPEED          "FnRUN_DEC"
#define PRIMALUCE_KEY_ACCELERATION_CURRENT        "FnRUN_CURR_ACC"
#define PRIMALUCE_KEY_RUN_CURRENT                 "FnRUN_CURR_SPD"
#define PRIMALUCE_KEY_DECELERATION_CURRENT        "FnRUN_CURR_DEC"
#define PRIMALUCE_KEY_HOLD_CURRENT                "FnRUN_CURR_HOLD"
// The same settings inside a run preset.
#define PRIMALUCE_KEY_PRESET_ACCELERATION_SPEED   "M1ACC"
#define PRIMALUCE_KEY_PRESET_RUN_SPEED            "M1SPD"
#define PRIMALUCE_KEY_PRESET_DECELERATION_SPEED   "M1DEC"
#define PRIMALUCE_KEY_PRESET_ACCELERATION_CURRENT "M1CACC"
#define PRIMALUCE_KEY_PRESET_RUN_CURRENT          "M1CSPD"
#define PRIMALUCE_KEY_PRESET_DECELERATION_CURRENT "M1CDEC"
#define PRIMALUCE_KEY_PRESET_HOLD_CURRENT         "M1HOLD"
// Motor commands and the keys their acknowledgements come back under.
#define PRIMALUCE_KEY_MOVE_ABSOLUTE               "MOVE_ABS"
#define PRIMALUCE_KEY_GOTO                        "GOTO"
#define PRIMALUCE_KEY_MOTOR_STOP                  "MOT_STOP"
#define PRIMALUCE_KEY_MOTOR_ABORT                 "MOT_ABORT"
#define PRIMALUCE_KEY_SYNC_POSITION               "SYNC_POS"
#define PRIMALUCE_KEY_FAST_INWARD                 "F_INW"
#define PRIMALUCE_KEY_FAST_OUTWARD                "F_OUTW"
#define PRIMALUCE_KEY_STEP                        "STEP"
#define PRIMALUCE_KEY_DEGREES                     "DEG"
#define PRIMALUCE_KEY_ARCSECONDS                  "ARCSEC"
#define PRIMALUCE_KEY_ROTATOR_ENABLE              "ARCO"
#define PRIMALUCE_KEY_ROTATOR_SERIAL_NUMBER       "ARCO_SN"

#pragma mark - Flat panel and cover keys

#define PRIMALUCE_KEY_LIGHT                       "LIGHT"
#define PRIMALUCE_KEY_BRIGHTNESS                  "BRIGHTNESS"
#define PRIMALUCE_KEY_MAX_BRIGHTNESS              "MAX_BRIGHTNESS"
#define PRIMALUCE_KEY_LIGHT_FADING                "LIGHT_FADING"
#define PRIMALUCE_KEY_LIGHT_AT_POWER_ON           "LIGHT_ATPWON"
#define PRIMALUCE_KEY_PANEL_TEMPERATURE           "NTC_T"
#define PRIMALUCE_KEY_PANEL_RECALL_PRESET         "PRESET"
#define PRIMALUCE_KEY_COVER_POSITION              "POSITION"

#pragma mark - Values

// The acknowledgement of an accepted set or cmd.
#define PRIMALUCE_DONE                            "done"
// The acknowledgement of a move or stop when the motor has no 12 V supply (observed on a SESTO SENSO 2).
#define PRIMALUCE_POWER_SUPPLY_ERROR              "12V_PowerSupply_Error"
// MST of a motor at rest; "move", "CstSpeed", "acc", "dec" and similar while it runs.
#define PRIMALUCE_MOTOR_STOPPED                   "stop"
#define PRIMALUCE_ROTATOR_CALIBRATION_RUNNING     "exec"
#define PRIMALUCE_ROTATOR_CALIBRATION_STOPPED     "stop"
#define PRIMALUCE_LEDS_OFF                        "off"
#define PRIMALUCE_LEDS_LOW                        "low"
#define PRIMALUCE_LEDS_MIDDLE                     "middle"
#define PRIMALUCE_LEDS_ON                         "on"
#define PRIMALUCE_ACCESS_POINT_ON                 "on"
#define PRIMALUCE_ACCESS_POINT_OFF                "off"
// LANCFG: own access point or station on an existing network; applied after REBOOT.
#define PRIMALUCE_LAN_ACCESS_POINT                "ap"
#define PRIMALUCE_LAN_STATION                     "sta"
#define PRIMALUCE_LOG_LEVEL_NO_OUTPUT             "no output"
#define PRIMALUCE_RUN_PRESET_LIGHT                "light"
#define PRIMALUCE_RUN_PRESET_MEDIUM               "medium"
#define PRIMALUCE_RUN_PRESET_SLOW                 "slow"
#define PRIMALUCE_CALIBRATION_DIRECTION_NORMAL    "normal"
#define PRIMALUCE_CALIBRATION_DIRECTION_INVERTED  "invert"
// CAL_FOCUSER steps. SESTO SENSO 2 and ESATTO: Init, StoreAsMinPos, GoOutToFindMaxPos, StoreAsMaxPos.
#define PRIMALUCE_FOCUSER_CALIBRATION_INIT        "Init"
#define PRIMALUCE_FOCUSER_CALIBRATION_STORE_MIN   "StoreAsMinPos"
#define PRIMALUCE_FOCUSER_CALIBRATION_FIND_MAX    "GoOutToFindMaxPos"
#define PRIMALUCE_FOCUSER_CALIBRATION_STORE_MAX   "StoreAsMaxPos"
// Further SESTO SENSO 3 steps (from the INDI driver, not verified on hardware).
#define PRIMALUCE_FOCUSER_CALIBRATION_FIND_MIN    "GoInToFindMinPos"
#define PRIMALUCE_FOCUSER_CALIBRATION_STOP_MOTOR  "StopMotor"
#define PRIMALUCE_FOCUSER_CALIBRATION_AUTO_START  "start_auto_cal"
#define PRIMALUCE_FOCUSER_CALIBRATION_AUTO_STOP   "stop_calib"
// CAL_ALTO steps (from the INDI driver, not verified on hardware).
#define PRIMALUCE_COVER_CALIBRATION_INIT          "Init"
#define PRIMALUCE_COVER_CALIBRATION_OPEN_FAST     "Open_Fast"
#define PRIMALUCE_COVER_CALIBRATION_OPEN_SLOW     "Open_Slow"
#define PRIMALUCE_COVER_CALIBRATION_CLOSE_FAST    "Close_Fast"
#define PRIMALUCE_COVER_CALIBRATION_CLOSE_SLOW    "Close_Slow"
#define PRIMALUCE_COVER_CALIBRATION_STORE_CLOSED  "StoreAsClosedPos"
#define PRIMALUCE_COVER_CALIBRATION_STORE_OPEN    "StoreAsMaxOpenPos"
// MODNAME prefixes.
#define PRIMALUCE_MODEL_NAME_SESTOSENSO           "SESTOSENSO"
#define PRIMALUCE_MODEL_NAME_SESTOSENSO3          "SESTOSENSO3"
#define PRIMALUCE_MODEL_NAME_ESATTO               "ESATTO"
#define PRIMALUCE_MODEL_NAME_GIOTTO               "GIOTTO"
#define PRIMALUCE_MODEL_NAME_ALTO                 "ALTO"

#pragma mark - Limits and sentinels

// EXT_T of a controller without an external probe.
#define PRIMALUCE_NO_PROBE_TEMPERATURE            (-127)
// SESTO SENSO 2 and ESATTO firmware older than this is not supported (SESTO SENSO 3 has its own numbering).
#define PRIMALUCE_MINIMUM_FIRMWARE_VERSION        3.05
// Named focuser positions PRESET_1 to PRESET_9; the controller refuses position 0.
#define PRIMALUCE_STORED_POSITION_COUNT           9
// GIOTTO brightness presets PRESET_1 to PRESET_9.
#define PRIMALUCE_PANEL_PRESET_COUNT              9
// ALTO cover positions PRESET_1 to PRESET_3.
#define PRIMALUCE_COVER_PRESET_COUNT              3
// User run presets RUNPRESET_1 to RUNPRESET_3.
#define PRIMALUCE_USER_RUN_PRESET_COUNT           3
// SESTO SENSO 3 RECOVER_DELAY range in seconds.
#define PRIMALUCE_RECOVERY_DELAY_MIN              (-1)
#define PRIMALUCE_RECOVERY_DELAY_MAX              120
// ALTO POSITION: 0 is closed (parked), 100 fully open.
#define PRIMALUCE_COVER_CLOSED                    0
#define PRIMALUCE_COVER_OPEN                      100

#pragma mark - Requests

// The exact bytes every request is sent as. The spacing inside some of them is historical and kept
// on purpose: the hardware accepts both, and changing it would change the recorded wire traffic.
#define PRIMALUCE_REQUEST_GET_STATE                       "{\"req\":{\"get\": \"\"}}"
#define PRIMALUCE_REQUEST_GET_MODEL_NAME                  "{\"req\":{\"get\":{\"MODNAME\":\"\"}}}"
#define PRIMALUCE_REQUEST_GET_FIRMWARE_VERSION            "{\"req\":{\"get\":{\"SWVERS\":{\"SWAPP\":\"\"}}}}"
#define PRIMALUCE_REQUEST_DISABLE_LOG_OUTPUT              "{\"req\":{\"cmd\":{\"LOGLEVEL\":\"no output\"}}}"
#define PRIMALUCE_REQUEST_GET_ENVIRONMENT                 "{\"req\":{\"get\":{\"EXT_T\":\"\", \"VIN_12V\": \"\", \"MOT1\":{\"NTC_T\":\"\"}}}}"
#define PRIMALUCE_REQUEST_GET_LAN_CONFIG                  "{\"req\":{\"get\":{\"LANCFG\":\"\"}}}"
// LANCFG mode
#define PRIMALUCE_REQUEST_SET_LAN_CONFIG                  "{\"req\":{\"set\":{\"LANCFG\":\"%s\"}}}"
#define PRIMALUCE_REQUEST_REBOOT                          "{\"req\":{\"cmd\":{\"REBOOT\":\"\"}}}"
// PRIMALUCE_ACCESS_POINT_ON or PRIMALUCE_ACCESS_POINT_OFF
#define PRIMALUCE_REQUEST_SET_ACCESS_POINT                "{\"req\":{\"cmd\":{\"AP_SET_STATUS\":\"%s\"}}}"
// SSID, password
#define PRIMALUCE_REQUEST_SET_ACCESS_POINT_CREDENTIALS    "{\"req\":{\"set\":{\"WIFIAP\":{\"SSID\":\"%s\", \"PWD\":\"%s\"}}}}"
// SSID, password
#define PRIMALUCE_REQUEST_SET_STATION_CREDENTIALS         "{\"req\":{\"set\":{\"WIFISTA\":{\"SSID\":\"%s\", \"PWD\":\"%s\"}}}}"
// PRIMALUCE_LEDS_*
#define PRIMALUCE_REQUEST_SET_LEDS                        "{\"req\":{\"cmd\":{\"DIMLEDS\":\"%s\"}}}"
#define PRIMALUCE_REQUEST_GET_MODEL_SUBMODEL              "{\"req\":{\"srv\":{\"GET_MODEL_SUBMODEL\":\"\"}}}"
#define PRIMALUCE_REQUEST_GET_RECOVERY_DELAY              "{\"req\":{\"get\":{\"RECOVER_DELAY\":\"\"}}}"
// seconds
#define PRIMALUCE_REQUEST_SET_RECOVERY_DELAY              "{\"req\":{\"set\":{\"RECOVER_DELAY\":%d}}}"
#define PRIMALUCE_REQUEST_FOCUSER_GET_POSITION_STEP       "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS_STEP\":\"\",\"STATUS\":\"\"}}}}"
#define PRIMALUCE_REQUEST_FOCUSER_GET_POSITION_REGISTER   "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS\":\"STEP\",\"STATUS\":\"\"}}}}"
#define PRIMALUCE_REQUEST_FOCUSER_GET_MOTOR_STATE         "{\"req\":{\"get\":{\"MOT1\":{\"STATUS\":{\"MST\":\"\"}}}}}"
#define PRIMALUCE_REQUEST_FOCUSER_GET_CALIBRATION         "{\"req\":{\"get\":{\"MOT1\":{\"CAL_MINPOS\":\"\",\"CAL_MAXPOS\":\"\"}}}}"
#define PRIMALUCE_REQUEST_FOCUSER_GET_CALIBRATION_DIR     "{\"req\":{\"get\":{\"MOT1\":{\"CAL_DIR\":\"\"}}}}"
// steps; SESTO SENSO 2 and ESATTO
#define PRIMALUCE_REQUEST_FOCUSER_MOVE_ABSOLUTE           "{\"req\":{\"cmd\":{\"MOT1\":{\"MOVE_ABS\":{\"STEP\":%d}}}}}"
// steps; SESTO SENSO 3
#define PRIMALUCE_REQUEST_FOCUSER_GOTO                    "{\"req\":{\"cmd\":{\"MOT1\":{\"GOTO\":%d}}}}"
#define PRIMALUCE_REQUEST_FOCUSER_ABORT                   "{\"req\":{\"cmd\":{\"MOT1\":{\"MOT_ABORT\":\"\"}}}}"
#define PRIMALUCE_REQUEST_FOCUSER_STOP                    "{\"req\":{\"cmd\":{\"MOT1\":{\"MOT_STOP\":\"\"}}}}"
#define PRIMALUCE_REQUEST_FOCUSER_FAST_INWARD             "{\"req\":{\"cmd\":{\"MOT1\":{\"F_INW\":\"\"}}}}"
#define PRIMALUCE_REQUEST_FOCUSER_FAST_OUTWARD            "{\"req\":{\"cmd\":{\"MOT1\":{\"F_OUTW\":\"\"}}}}"
// steps
#define PRIMALUCE_REQUEST_FOCUSER_SET_BACKLASH            "{\"req\":{\"set\":{\"MOT1\":{\"BKLASH\":%d}}}}"
// speed
#define PRIMALUCE_REQUEST_FOCUSER_SET_SPEED               "{\"req\":{\"set\":{\"MOT1\":{\"SPEED\":%d}}}}"
// 0 or 1
#define PRIMALUCE_REQUEST_FOCUSER_SET_HOLD_CURRENT        "{\"req\":{\"set\":{\"MOT1\":{\"HOLDCURR_STATUS\":%d}}}}"
// acceleration, deceleration and run speed
#define PRIMALUCE_REQUEST_FOCUSER_SET_MOTOR_RATES         "{\"req\":{\"set\":{\"MOT1\":{\"FnRUN_ACC\":%d,\"FnRUN_DEC\":%d,\"FnRUN_SPD\":%d}}}}"
// acceleration, deceleration, run and hold current
#define PRIMALUCE_REQUEST_FOCUSER_SET_MOTOR_CURRENTS      "{\"req\":{\"set\":{\"MOT1\":{\"FnRUN_CURR_ACC\":%d,\"FnRUN_CURR_DEC\":%d,\"FnRUN_CURR_SPD\":%d,\"FnRUN_CURR_HOLD\":%d}}}}"
#define PRIMALUCE_REQUEST_FOCUSER_GET_MOTOR_CONFIG        "{\"req\":{\"get\":{\"MOT1\":{\"HOLDCURR_STATUS\":\"\",\"FnRUN_SPD\":\"\",\"FnRUN_DEC\":\"\",\"FnRUN_ACC\":\"\",\"FnRUN_CURR_SPD\":\"\",\"FnRUN_CURR_DEC\":\"\",\"FnRUN_CURR_ACC\":\"\",\"FnRUN_CURR_HOLD\":\"\"}}}}"
// PRIMALUCE_RUN_PRESET_LIGHT, _MEDIUM or _SLOW
#define PRIMALUCE_REQUEST_FOCUSER_APPLY_RUN_PRESET        "{\"req\":{\"cmd\":{\"RUNPRESET\":\"%s\"}}}"
// 1 to PRIMALUCE_USER_RUN_PRESET_COUNT
#define PRIMALUCE_REQUEST_FOCUSER_APPLY_USER_RUN_PRESET   "{\"req\":{\"cmd\":{\"RUNPRESET\":%d}}}"
// index, acceleration speed, deceleration speed, run speed, acceleration, deceleration, run and hold current
#define PRIMALUCE_REQUEST_FOCUSER_SET_USER_RUN_PRESET     "{\"req\":{\"set\":{\"RUNPRESET_%d\":{\"M1ACC\":%d,\"M1DEC\":%d,\"M1SPD\":%d,\"M1CACC\":%d,\"M1CDEC\":%d,\"M1CSPD\":%d,\"M1HOLD\":%d}}}}"
// index 1 to PRIMALUCE_STORED_POSITION_COUNT, steps
#define PRIMALUCE_REQUEST_FOCUSER_SET_STORED_POSITION     "{\"req\":{\"set\":{\"PRESET_%d\":{\"M1POS\":%d}}}}"
// index 1 to PRIMALUCE_STORED_POSITION_COUNT, name
#define PRIMALUCE_REQUEST_FOCUSER_SET_STORED_NAME         "{\"req\":{\"set\":{\"PRESET_%d\":{\"NAME\":\"%s\"}}}}"
// PRIMALUCE_FOCUSER_CALIBRATION_* step, or "MoveIn-<steps>" / "MoveOut-<steps>" on a SESTO SENSO 3
#define PRIMALUCE_REQUEST_FOCUSER_CALIBRATION_STEP        "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"%s\"}}}}"
// PRIMALUCE_CALIBRATION_DIRECTION_*
#define PRIMALUCE_REQUEST_FOCUSER_SET_CALIBRATION_DIR     "{\"req\":{\"set\": {\"MOT1\": {\"CAL_DIR\":\"%s\"}}}}"
// 0 or 1
#define PRIMALUCE_REQUEST_ROTATOR_ENABLE                  "{\"req\":{\"set\": {\"ARCO\":%d}}}"
#define PRIMALUCE_REQUEST_ROTATOR_GET_SERIAL_NUMBER       "{\"req\":{\"get\":{\"ARCO_SN\":\"\"}}}"
#define PRIMALUCE_REQUEST_ROTATOR_GET_POSITION_DEG        "{\"req\":{\"get\":{\"MOT2\":{\"POSITION_DEG\":\"\",\"STATUS\":\"\"}}}}"
#define PRIMALUCE_REQUEST_ROTATOR_GET_ABS_POS_DEG         "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS_DEG\":\"\",\"STATUS\":\"\"}}}}"
#define PRIMALUCE_REQUEST_ROTATOR_GET_ABS_POS_REGISTER    "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS\":\"DEG\",\"STATUS\":\"\"}}}}"
// degrees
#define PRIMALUCE_REQUEST_ROTATOR_MOVE_ABSOLUTE           "{\"req\":{\"cmd\":{\"MOT2\":{\"MOVE_ABS\":{\"DEG\":%g}}}}}"
// degrees
#define PRIMALUCE_REQUEST_ROTATOR_SYNC                    "{\"req\":{\"cmd\":{\"MOT2\":{\"SYNC_POS\":{\"DEG\":%g}}}}}"
#define PRIMALUCE_REQUEST_ROTATOR_STOP                    "{\"req\":{\"cmd\":{\"MOT2\":{\"MOT_STOP\":\"\"}}}}"
#define PRIMALUCE_REQUEST_ROTATOR_GET_CALIBRATION         "{\"req\":{\"get\":{\"MOT2\":{\"CAL_STATUS\":\"\"}}}}"
// PRIMALUCE_ROTATOR_CALIBRATION_RUNNING starts a calibration, _STOPPED ends it
#define PRIMALUCE_REQUEST_ROTATOR_SET_CALIBRATION         "{\"req\":{\"set\":{\"MOT2\":{\"CAL_STATUS\":\"%s\"}}}}"
#define PRIMALUCE_REQUEST_PANEL_GET_MAX_BRIGHTNESS        "{\"req\":{\"get\":{\"MAX_BRIGHTNESS\":\"\"}}}"
#define PRIMALUCE_REQUEST_PANEL_GET_BRIGHTNESS            "{\"req\":{\"get\":{\"BRIGHTNESS\":\"\"}}}"
// brightness
#define PRIMALUCE_REQUEST_PANEL_SET_BRIGHTNESS            "{\"req\":{\"set\":{\"BRIGHTNESS\":%d}}}"
#define PRIMALUCE_REQUEST_PANEL_GET_LIGHT                 "{\"req\":{\"get\":{\"LIGHT\":\"\"}}}"
// 0 or 1
#define PRIMALUCE_REQUEST_PANEL_SET_LIGHT                 "{\"req\":{\"set\":{\"LIGHT\":%d}}}"
#define PRIMALUCE_REQUEST_PANEL_GET_STATE                 "{\"req\":{\"get\":{\"LIGHT\":\"\",\"BRIGHTNESS\":\"\"}}}"
#define PRIMALUCE_REQUEST_PANEL_GET_ENVIRONMENT           "{\"req\":{\"get\":{\"NTC_T\":\"\",\"VIN_12V\":\"\",\"VIN_USB\":\"\"}}}"
// index 1 to PRIMALUCE_PANEL_PRESET_COUNT, brightness
#define PRIMALUCE_REQUEST_PANEL_SET_PRESET_BRIGHTNESS     "{\"req\":{\"set\":{\"PRESET_%d\":{\"BRIGHTNESS\":%d}}}}"
// index 1 to PRIMALUCE_PANEL_PRESET_COUNT
#define PRIMALUCE_REQUEST_PANEL_RECALL_PRESET             "{\"req\":{\"cmd\":{\"PRESET\":%d}}}"
// 0 or 1
#define PRIMALUCE_REQUEST_PANEL_SET_FADING                "{\"req\":{\"set\":{\"LIGHT_FADING\":%d}}}"
// 0 or 1
#define PRIMALUCE_REQUEST_PANEL_SET_LIGHT_AT_POWER_ON     "{\"req\":{\"set\":{\"LIGHT_ATPWON\":%d}}}"
#define PRIMALUCE_REQUEST_COVER_GET_POSITION              "{\"req\":{\"get\":{\"MOT1\":{\"POSITION\":\"\"}}}}"
// PRIMALUCE_COVER_CLOSED to PRIMALUCE_COVER_OPEN
#define PRIMALUCE_REQUEST_COVER_SET_POSITION              "{\"req\":{\"set\":{\"MOT1\":{\"POSITION\":%d}}}}"
#define PRIMALUCE_REQUEST_COVER_GET_MOTOR_STATE           "{\"req\":{\"get\":{\"MOT1\":{\"STATUS\":\"\"}}}}"
#define PRIMALUCE_REQUEST_COVER_GET_MOTION                "{\"req\":{\"get\":{\"MOT1\":{\"POSITION\":\"\",\"STATUS\":\"\"}}}}"
#define PRIMALUCE_REQUEST_COVER_GET_ENVIRONMENT           "{\"req\":{\"get\":{\"VIN_12V\":\"\",\"VIN_USB\":\"\",\"MOT1\":{\"NTC_T\":\"\"}}}}"
// index 1 to PRIMALUCE_COVER_PRESET_COUNT, steps
#define PRIMALUCE_REQUEST_COVER_SET_PRESET_POSITION       "{\"req\":{\"set\":{\"PRESET_%d\":{\"M1POS\":%d}}}}"
#define PRIMALUCE_REQUEST_COVER_STOP                      "{\"req\":{\"cmd\":{\"MOT1\":{\"MOT_STOP\":\"\"}}}}"
// PRIMALUCE_COVER_CALIBRATION_*
#define PRIMALUCE_REQUEST_COVER_CALIBRATION_STEP          "{\"req\":{\"cmd\":{\"MOT1\":{\"CAL_ALTO\":\"%s\"}}}}"

#pragma mark - Reply paths

// NULL-terminated key paths into a reply, for primalucelib_reply_string(), _number() and _has().
// The tables are defined in primalucelib.c.
extern char *PRIMALUCE_REPLY_MODEL_NAME[];
extern char *PRIMALUCE_REPLY_SERIAL_NUMBER[];
extern char *PRIMALUCE_REPLY_APPLICATION_FIRMWARE[];
extern char *PRIMALUCE_REPLY_WEB_FIRMWARE[];
extern char *PRIMALUCE_REPLY_GET_ERROR[];
extern char *PRIMALUCE_REPLY_SET_ERROR[];
extern char *PRIMALUCE_REPLY_CMD_ERROR[];
extern char *PRIMALUCE_REPLY_EXTERNAL_TEMPERATURE[];
extern char *PRIMALUCE_REPLY_SUPPLY_VOLTAGE[];
extern char *PRIMALUCE_REPLY_USB_VOLTAGE[];
extern char *PRIMALUCE_REPLY_LEDS[];
extern char *PRIMALUCE_REPLY_LAN_CONFIG[];
extern char *PRIMALUCE_REPLY_ACCESS_POINT_STATUS[];
extern char *PRIMALUCE_REPLY_ACCESS_POINT_SSID[];
extern char *PRIMALUCE_REPLY_ACCESS_POINT_PASSWORD[];
extern char *PRIMALUCE_REPLY_STATION_SSID[];
extern char *PRIMALUCE_REPLY_STATION_PASSWORD[];
extern char *PRIMALUCE_REPLY_RECOVERY_DELAY[];
extern char *PRIMALUCE_REPLY_MODEL_SUBMODEL[];
extern char *PRIMALUCE_REPLY_FIRST_STORED_POSITION[];
extern char *PRIMALUCE_REPLY_FOCUSER_CALIBRATION_RESTART[];
extern char *PRIMALUCE_REPLY_FOCUSER_ERROR[];
extern char *PRIMALUCE_REPLY_FOCUSER_POSITION_STEP[];
extern char *PRIMALUCE_REPLY_FOCUSER_POSITION_REGISTER[];
extern char *PRIMALUCE_REPLY_FOCUSER_MOTOR_STATE[];
extern char *PRIMALUCE_REPLY_FOCUSER_MOTOR_TEMPERATURE[];
extern char *PRIMALUCE_REPLY_FOCUSER_CALIBRATION_MIN[];
extern char *PRIMALUCE_REPLY_FOCUSER_CALIBRATION_MAX[];
extern char *PRIMALUCE_REPLY_FOCUSER_BACKLASH[];
extern char *PRIMALUCE_REPLY_FOCUSER_SPEED[];
extern char *PRIMALUCE_REPLY_FOCUSER_HOLD_CURRENT_STATUS[];
extern char *PRIMALUCE_REPLY_FOCUSER_MOVE_ACK[];
extern char *PRIMALUCE_REPLY_FOCUSER_GOTO_ACK[];
extern char *PRIMALUCE_REPLY_FOCUSER_ABORT_ACK[];
extern char *PRIMALUCE_REPLY_FOCUSER_STOP_ACK[];
extern char *PRIMALUCE_REPLY_FOCUSER_FAST_INWARD_ACK[];
extern char *PRIMALUCE_REPLY_FOCUSER_FAST_OUTWARD_ACK[];
extern char *PRIMALUCE_REPLY_FOCUSER_BACKLASH_ACK[];
extern char *PRIMALUCE_REPLY_FOCUSER_SPEED_ACK[];
extern char *PRIMALUCE_REPLY_FOCUSER_HOLD_CURRENT_ACK[];
extern char *PRIMALUCE_REPLY_FOCUSER_CALIBRATION_ACK[];
extern char *PRIMALUCE_REPLY_FOCUSER_CALIBRATION_DIR[];
extern char *PRIMALUCE_REPLY_FOCUSER_CALIBRATION_DIR_ACK[];
extern char *PRIMALUCE_REPLY_ROTATOR_CALIBRATION_RESTART[];
extern char *PRIMALUCE_REPLY_ROTATOR_ERROR[];
extern char *PRIMALUCE_REPLY_ROTATOR_SERIAL_NUMBER[];
// The ARCO port switch in the full state, 1 when enabled.
extern char *PRIMALUCE_REPLY_ROTATOR_ENABLED[];
extern char *PRIMALUCE_REPLY_ROTATOR_POSITION_DEG[];
extern char *PRIMALUCE_REPLY_ROTATOR_ABS_POS_DEG[];
extern char *PRIMALUCE_REPLY_ROTATOR_ABS_POS_REGISTER[];
extern char *PRIMALUCE_REPLY_ROTATOR_MOTOR_STATE[];
extern char *PRIMALUCE_REPLY_ROTATOR_CALIBRATION_MIN[];
extern char *PRIMALUCE_REPLY_ROTATOR_CALIBRATION_MAX[];
extern char *PRIMALUCE_REPLY_ROTATOR_CALIBRATION_STATUS[];
extern char *PRIMALUCE_REPLY_ROTATOR_CALIBRATION_ACK[];
// The ARCO acknowledges MOVE_ABS under STEP, whichever unit the move was given in.
extern char *PRIMALUCE_REPLY_ROTATOR_MOVE_ACK[];
extern char *PRIMALUCE_REPLY_ROTATOR_MOVE_DEG_ACK[];
extern char *PRIMALUCE_REPLY_ROTATOR_SYNC_ACK[];
extern char *PRIMALUCE_REPLY_ROTATOR_STOP_ACK[];
extern char *PRIMALUCE_REPLY_PANEL_MAX_BRIGHTNESS[];
extern char *PRIMALUCE_REPLY_PANEL_BRIGHTNESS[];
extern char *PRIMALUCE_REPLY_PANEL_BRIGHTNESS_ACK[];
extern char *PRIMALUCE_REPLY_PANEL_LIGHT[];
extern char *PRIMALUCE_REPLY_PANEL_LIGHT_ACK[];
extern char *PRIMALUCE_REPLY_PANEL_TEMPERATURE[];
extern char *PRIMALUCE_REPLY_PANEL_FADING[];
extern char *PRIMALUCE_REPLY_PANEL_FADING_ACK[];
extern char *PRIMALUCE_REPLY_PANEL_LIGHT_AT_POWER_ON[];
extern char *PRIMALUCE_REPLY_PANEL_LIGHT_AT_POWER_ON_ACK[];
extern char *PRIMALUCE_REPLY_PANEL_RECALL_PRESET_ACK[];
extern char *PRIMALUCE_REPLY_PANEL_CALIBRATION_RESTART_CH1[];
extern char *PRIMALUCE_REPLY_PANEL_CALIBRATION_RESTART_CH2[];
extern char *PRIMALUCE_REPLY_COVER_POSITION[];
extern char *PRIMALUCE_REPLY_COVER_POSITION_ACK[];
extern char *PRIMALUCE_REPLY_COVER_MOTOR_STATE[];
extern char *PRIMALUCE_REPLY_COVER_STOP_ACK[];
extern char *PRIMALUCE_REPLY_COVER_CALIBRATION_ACK[];

#pragma mark - Types

typedef enum {
	PRIMALUCELIB_MODEL_UNKNOWN = 0,
	PRIMALUCELIB_MODEL_SESTOSENSO,
	PRIMALUCELIB_MODEL_SESTOSENSO3,
	PRIMALUCELIB_MODEL_ESATTO,
	PRIMALUCELIB_MODEL_GIOTTO,
	PRIMALUCELIB_MODEL_ALTO
} primalucelib_model;

// Where the ARCO angle is read from. POSITION_DEG is the synced angle and preferred where the
// firmware reports it; ABS_POS_DEG is the mechanical angle; older firmware has the ABS_POS register.
typedef enum {
	PRIMALUCELIB_ANGLE_POSITION_DEG = 0,
	PRIMALUCELIB_ANGLE_ABS_POS_REGISTER,
	PRIMALUCELIB_ANGLE_ABS_POS_DEG
} primalucelib_angle_source;

// Motor run settings, as MOT1 FnRUN_* and as the M1* members of a run preset. Speeds and currents
// are 0 to 10.
typedef struct {
	double acceleration_speed;
	double run_speed;
	double deceleration_speed;
	double acceleration_current;
	double run_current;
	double deceleration_current;
	double hold_current;
} primalucelib_run_config;

// One connection to a controller, over USB serial or over HTTP.
typedef struct {
	// Log prefix, normally the DRIVER_NAME of the driver that owns the link.
	const char *driver_name;
	// Serial link, NULL over HTTP or while closed.
	indigo_uni_handle *handle;
	bool is_http;
	char http_host[INDIGO_NAME_SIZE];
	int http_port;
	// The last request got no answer at all (as opposed to an answer refusing it).
	bool link_failed;
	// The controller restarts: requests are refused at once instead of waiting for their read timeout, and only
	// primalucelib_probe_restart() asks it, with a short timeout.
	bool restarting, probing;
	// The ERROR text of the last refused request, empty otherwise.
	char last_error[INDIGO_VALUE_SIZE];
	// The reply of the last request; string tokens are NUL-terminated in place.
	char response[PRIMALUCE_MAX_RESPONSE_SIZE];
	jsmntok_t tokens[PRIMALUCE_MAX_TOKENS];
	// The tokens the parser filled; a reply may use all PRIMALUCE_MAX_TOKENS, leaving no empty token after the last one.
	int token_count;
	jsmn_parser parser;
} primalucelib_link;

#pragma mark - Link

// Splits http://host[:port] (or tcp://host[:port]) into host and port; the port defaults to 80.
extern bool primalucelib_parse_http_url(const char *url, char *host, size_t host_size, int *port);
// Opens the link: an http:// or tcp:// URL selects HTTP, anything else is a serial port. Over HTTP nothing
// is contacted yet. error_message receives the reason of a failed serial open.
extern bool primalucelib_open(primalucelib_link *link, const char *driver_name, const char *port, const char **error_message);
extern bool primalucelib_is_open(primalucelib_link *link);
extern bool primalucelib_is_http(primalucelib_link *link);
extern void primalucelib_close(primalucelib_link *link);
// Sends one request (a printf format) and reads and parses the reply. True when the controller
// answered with a JSON object that carries no ERROR under res/cmd, res/set or res/get.
extern bool primalucelib_request(primalucelib_link *link, const char *format, ...);
extern bool primalucelib_vrequest(primalucelib_link *link, const char *format, va_list args);
// Drops pending serial input, such as a boot log.
extern void primalucelib_discard_input(primalucelib_link *link);
extern bool primalucelib_link_failed(primalucelib_link *link);
extern const char *primalucelib_last_error(primalucelib_link *link);
extern void primalucelib_clear_last_error(primalucelib_link *link);

#pragma mark - Reply access

// Token index of the value at path in the last reply, -1 if it is missing.
extern int primalucelib_reply_token(primalucelib_link *link, char *path[]);
extern bool primalucelib_reply_has(primalucelib_link *link, char *path[]);
// The string at path, NULL if it is missing or not a string.
extern char *primalucelib_reply_string(primalucelib_link *link, char *path[]);
// The number at path (a number or a numeric string), 0 if it is missing.
extern double primalucelib_reply_number(primalucelib_link *link, char *path[]);
// The acknowledgement at path is "done".
extern bool primalucelib_reply_is_done(primalucelib_link *link, char *path[]);
// True for "done"; any other text at path is kept as primalucelib_last_error(), the reason of the refusal.
extern bool primalucelib_reply_is_acknowledged(primalucelib_link *link, char *path[]);

#pragma mark - Values

extern bool primalucelib_is_done(const char *acknowledgement);
extern bool primalucelib_is_power_supply_error(const char *acknowledgement);
extern bool primalucelib_is_motor_stopped(const char *motor_state);
extern bool primalucelib_is_rotator_calibration_finished(const char *calibration_status);
extern bool primalucelib_is_no_probe_temperature(double temperature);
extern primalucelib_model primalucelib_model_from_name(const char *model_name);
extern bool primalucelib_model_is_focuser(primalucelib_model model);
extern bool primalucelib_model_is_sestosenso(primalucelib_model model);

#pragma mark - Controller

// The full state of the controller: identity, firmware, WiFi, LEDs, both motors, presets.
extern bool primalucelib_get_state(primalucelib_link *link);
extern char *primalucelib_get_model_name(primalucelib_link *link);
extern char *primalucelib_get_firmware_version(primalucelib_link *link);
// Diagnostic output interleaves with replies on the serial line; this turns it off.
extern bool primalucelib_disable_log_output(primalucelib_link *link);
// EXT_T, VIN_12V and MOT1 NTC_T; true when the answer carries EXT_T.
extern bool primalucelib_get_environment(primalucelib_link *link);
extern double primalucelib_reply_external_temperature(primalucelib_link *link);
extern double primalucelib_reply_supply_voltage(primalucelib_link *link);
extern double primalucelib_reply_usb_voltage(primalucelib_link *link);
extern double primalucelib_reply_motor_temperature(primalucelib_link *link);
extern bool primalucelib_set_leds(primalucelib_link *link, const char *mode);
extern char *primalucelib_get_lan_config(primalucelib_link *link);
// The new mode takes effect after primalucelib_reboot().
extern bool primalucelib_set_lan_config(primalucelib_link *link, const char *mode);
extern bool primalucelib_reboot(primalucelib_link *link);
// Waits for a restarted controller to answer MODNAME again over the serial line, dropping its boot log.
extern bool primalucelib_wait_for_restart(primalucelib_link *link);
// Marks the controller restarting: every request fails at once with the reason "The controller is restarting"
// until primalucelib_probe_restart() gets an answer or primalucelib_end_restart() is called.
extern void primalucelib_begin_restart(primalucelib_link *link);
extern bool primalucelib_is_restarting(primalucelib_link *link);
// One MODNAME request with a PRIMALUCE_RESTART_PROBE_TIMEOUT read timeout; true, and the restart is over, once
// the controller answers.
extern bool primalucelib_probe_restart(primalucelib_link *link);
extern void primalucelib_end_restart(primalucelib_link *link);
extern bool primalucelib_set_access_point(primalucelib_link *link, bool on);
extern bool primalucelib_set_access_point_credentials(primalucelib_link *link, const char *ssid, const char *password);
extern bool primalucelib_set_station_credentials(primalucelib_link *link, const char *ssid, const char *password);
// SESTO SENSO 3: "SESTOSENSO3SC", "SESTOSENSO3LS", ... from "Model = .., SubModel = .., ARCO = ..".
extern bool primalucelib_get_submodel(primalucelib_link *link, char *submodel, size_t size);
extern bool primalucelib_get_recovery_delay(primalucelib_link *link, int *seconds);
extern bool primalucelib_set_recovery_delay(primalucelib_link *link, int seconds);

#pragma mark - Focuser (MOT1)

// Requests the position (ABS_POS_STEP, or the ABS_POS register of older firmware) and the motor status.
extern bool primalucelib_focuser_request_position(primalucelib_link *link, bool position_register);
extern bool primalucelib_reply_has_focuser_position(primalucelib_link *link, bool position_register);
extern double primalucelib_reply_focuser_position(primalucelib_link *link, bool position_register);
// Request, and true only when the answer carries the position.
extern bool primalucelib_focuser_get_position(primalucelib_link *link, bool position_register, double *position);
// A SESTO SENSO 3 reports MST only to a dedicated request; the others include it in the position reply.
extern bool primalucelib_focuser_request_motor_state(primalucelib_link *link);
extern char *primalucelib_reply_focuser_motor_state(primalucelib_link *link);
// GOTO on a SESTO SENSO 3, MOVE_ABS otherwise. True when answered; acknowledgement receives the answer.
extern bool primalucelib_focuser_move_to(primalucelib_link *link, bool use_goto, int position, char **acknowledgement);
// MOT_ABORT stops at once; MOT_STOP decelerates and is the fallback. reason receives a refusal.
extern bool primalucelib_focuser_stop(primalucelib_link *link, char **reason);
extern bool primalucelib_focuser_fast_move(primalucelib_link *link, bool outward);
extern bool primalucelib_focuser_set_backlash(primalucelib_link *link, int steps);
extern bool primalucelib_focuser_set_speed(primalucelib_link *link, int speed);
extern bool primalucelib_focuser_set_hold_current(primalucelib_link *link, bool on);
extern bool primalucelib_focuser_set_motor_rates(primalucelib_link *link, int acceleration, int deceleration, int run);
extern bool primalucelib_focuser_set_motor_currents(primalucelib_link *link, int acceleration, int deceleration, int run, int hold);
extern bool primalucelib_focuser_get_motor_config(primalucelib_link *link);
// MOT1 FnRUN_* of the last reply (state, motor configuration or run preset answer).
extern void primalucelib_reply_motor_config(primalucelib_link *link, primalucelib_run_config *config);
// The M1* members of preset (PRIMALUCE_KEY_RUN_PRESET_LIGHT, ... "RUNPRESET_1", ...) in the last reply.
extern void primalucelib_reply_run_preset(primalucelib_link *link, const char *preset, primalucelib_run_config *config);
extern bool primalucelib_focuser_apply_run_preset(primalucelib_link *link, const char *name);
extern bool primalucelib_focuser_apply_user_run_preset(primalucelib_link *link, int index);
extern bool primalucelib_focuser_set_user_run_preset(primalucelib_link *link, int index, const primalucelib_run_config *config);
// Stored positions are numbered 1 to PRIMALUCE_STORED_POSITION_COUNT. A position below 1 and a name
// containing '"' or '\' are refused without a request.
extern bool primalucelib_focuser_set_stored_position(primalucelib_link *link, int index, int position);
extern bool primalucelib_focuser_set_stored_position_name(primalucelib_link *link, int index, const char *name);
extern double primalucelib_reply_stored_position(primalucelib_link *link, int index);
extern char *primalucelib_reply_stored_position_name(primalucelib_link *link, int index);
extern bool primalucelib_focuser_calibration_step(primalucelib_link *link, const char *step);
// CAL_MINPOS and CAL_MAXPOS; an uncalibrated controller reports a maximum not above the minimum.
extern bool primalucelib_focuser_get_calibration(primalucelib_link *link, double *min_position, double *max_position);
// A SESTO SENSO 3 (firmware 1.0.9) acknowledges every CAL_DIR write and then restarts; see primalucelib_wait_for_restart().
extern bool primalucelib_focuser_set_calibration_direction(primalucelib_link *link, const char *direction);
extern char *primalucelib_focuser_get_calibration_direction(primalucelib_link *link);
// SESTO SENSO 3 semi-automatic calibration: MoveOut-<steps> for a positive, MoveIn-<steps> for a negative count.
extern bool primalucelib_focuser_calibration_jog(primalucelib_link *link, int steps);

#pragma mark - Rotator (ARCO, MOT2)

extern bool primalucelib_rotator_enable(primalucelib_link *link, bool on);
extern char *primalucelib_rotator_get_serial_number(primalucelib_link *link);
extern primalucelib_angle_source primalucelib_rotator_angle_source(bool has_position_deg, bool has_abs_pos_register);
// Requests the angle from source with the motor status; angle is set when the request succeeds.
extern bool primalucelib_rotator_get_angle(primalucelib_link *link, primalucelib_angle_source source, double *angle);
extern double primalucelib_reply_rotator_angle(primalucelib_link *link, primalucelib_angle_source source);
extern char *primalucelib_reply_rotator_motor_state(primalucelib_link *link);
extern bool primalucelib_rotator_move_to(primalucelib_link *link, double angle, char **acknowledgement);
extern bool primalucelib_rotator_sync(primalucelib_link *link, double angle);
extern bool primalucelib_rotator_stop(primalucelib_link *link);
extern bool primalucelib_rotator_start_calibration(primalucelib_link *link);
// Ends a running calibration; the answer is not checked.
extern bool primalucelib_rotator_stop_calibration(primalucelib_link *link);
extern char *primalucelib_rotator_get_calibration_status(primalucelib_link *link);

#pragma mark - Flat panel (GIOTTO, protocol v2.1, verified on a GIOTTO with application firmware 1.03.09)

// A refused set or cmd fails with the controller's reason in primalucelib_last_error().
extern bool primalucelib_panel_get_max_brightness(primalucelib_link *link, int *brightness);
extern bool primalucelib_panel_get_brightness(primalucelib_link *link, int *brightness);
extern bool primalucelib_panel_set_brightness(primalucelib_link *link, int brightness);
extern bool primalucelib_panel_get_light(primalucelib_link *link, bool *on);
extern bool primalucelib_panel_set_light(primalucelib_link *link, bool on);
// Light and brightness in one request; both fields are required.
extern bool primalucelib_panel_get_state(primalucelib_link *link, bool *on, int *brightness);
// Panel temperature (NTC_T), 12 V and USB supply in one request; the temperature field is required.
extern bool primalucelib_panel_get_environment(primalucelib_link *link, double *temperature, double *supply, double *usb);
// Presets are numbered 1 to PRIMALUCE_PANEL_PRESET_COUNT; the panel refuses the brightness of a preset without a name.
extern bool primalucelib_panel_set_preset_brightness(primalucelib_link *link, int index, int brightness);
extern bool primalucelib_panel_set_preset_name(primalucelib_link *link, int index, const char *name);
extern double primalucelib_reply_panel_preset_brightness(primalucelib_link *link, int index);
// Sets the brightness of the preset; the light is not switched on.
extern bool primalucelib_panel_recall_preset(primalucelib_link *link, int index);
extern bool primalucelib_panel_set_fading(primalucelib_link *link, bool enabled);
extern bool primalucelib_panel_set_light_at_power_on(primalucelib_link *link, bool enabled);

#pragma mark - Cover (ALTO, verified on an ALTO1 with application firmware 1.03.11; calibration from the INDI driver)

// A refused set or cmd fails with the controller's reason in primalucelib_last_error(). The controller acknowledges any
// POSITION, so only PRIMALUCE_COVER_CLOSED to PRIMALUCE_COVER_OPEN may be passed.
extern bool primalucelib_cover_get_position(primalucelib_link *link, int *position);
extern bool primalucelib_cover_set_position(primalucelib_link *link, int position);
extern char *primalucelib_cover_get_motor_state(primalucelib_link *link);
// Position (0 to 100) and motor state (MST) in one request; both are required. The state stays valid until the next request.
extern bool primalucelib_cover_get_motion(primalucelib_link *link, int *position, char **state);
// Motor temperature (MOT1.NTC_T), 12 V and USB supply in one request; the temperature field is required.
extern bool primalucelib_cover_get_environment(primalucelib_link *link, double *temperature, double *supply, double *usb);
// Decelerated stop.
extern bool primalucelib_cover_stop(primalucelib_link *link);
extern bool primalucelib_cover_calibration_step(primalucelib_link *link, const char *step);
// Presets are numbered 1 to PRIMALUCE_COVER_PRESET_COUNT, positions in steps; a preset without a name refuses a position.
extern bool primalucelib_cover_set_preset_position(primalucelib_link *link, int index, int steps);
extern bool primalucelib_cover_set_preset_name(primalucelib_link *link, int index, const char *name);
// Moves the cover to the preset.
extern bool primalucelib_cover_recall_preset(primalucelib_link *link, int index);

#ifdef __cplusplus
}
#endif

#endif /* primalucelib_h */
