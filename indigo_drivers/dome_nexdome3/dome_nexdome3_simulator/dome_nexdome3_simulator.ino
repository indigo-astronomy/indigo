// NexDome Firmware v3 simulator for Arduino
// https://github.com/nexdome/Firmware
//
// Copyright (c) 2019-2025 Ivan Gorchev and Rumen G. Bogdanovski
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

#include <EEPROM.h>

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#define VERSION_3_2

#ifndef VERSION_3_2
#define R_VERSION                  "3.1.0"
#define S_VERSION                  "3.1.1"
#else
#define R_VERSION                  "3.2.0"
#define S_VERSION                  "3.2.1"
#endif

#define R_DATA_ID                  0xbab1
#define S_DATA_ID                  0xbab2

// Communication constants
#define BAUD_RATE                  9600
#define SERIAL_TIMEOUT             1000
#define COMMAND_LENGTH             30
#define START_COMMAND              '@'
#define SEPARATOR                  ','
#define COMMAND_TERMINATION_CR     '\r'
#define COMMAND_TERMINATION_LF     '\n'

#define RESPONSE_START             ":"
#define RESPONSE_END               "#\n"
#define ERROR_MESSAGE              (compose_message("Err"))
#define RAIN_DETECTED              (compose_message("Rain"))
#define RAIN_STOPPED               (compose_message("RainStopped"))

// Non standard reponces
#define BATTERY_ENABLED            "BateryStatusEnabled\n"
#define BATTERY_DISABLED           "BateryStatusDisabled\n"
#define EEPROM_CLEAR               "EEPROMisClear\n"

#define COMPOSE_VALUE_RESPONSE(command, value) (compose_message(command.substring(0,3) + String(value, DEC)))
#define COMPOSE_COMMAND_RESPONSE(command)      (compose_message(command.substring(0,3)))

#define XB_START_MSG               "XB->Start\n"
#define XB_WAITAT_MSG              "XB->WaitAT\n"
#define XB_CONFIG_MSG              "XB->Config\n"
#define XB_DETECT_MSG              "XB->Detect\n"
#define XB_ONLINE_MSG              "XB->Online\n"

#define STATE_BUFFER_CLEAR         0
#define STATE_COMMAND_STARTED      1
#define STATE_COMMAND_RECEIVED     2
#define STATE_COMMAND_ERROR        3

#define NO_PARAMS(command)                     ((command.length() <= 4) || (command.charAt(3) != ','))
#define GET_INT_PARAM(command)                 (command.substring(4).toInt())
#define DEGREES_TO_STEPS(deg)                  (deg * steps_per_degree)


char ReceiveBuffer[COMMAND_LENGTH];
String ResponseMessage;
char CurrentSymbol;
int ReceiveCounter = 0;
int ReceivedCommandLength = 0;
int CommandReceiverState = STATE_BUFFER_CLEAR;

#define DELAY_TIME             1000  // 1sec
#define XB_DELAY_TIME          10000 // 10sec
#define XB_TRANSITION_TIME     1000  // 1sek

long int delay_start;
long int r_delay_start;
long int s_delay_start;
long int xb_delay_start;

// Device states

#define R_STATE_STOPPED         0
#define R_STATE_MOVING_LEFT     1 //counter-clockwise
#define R_STATE_MOVING_RIGHT    2 //clockwise

#define S_STATE_CLOSED          0
#define S_STATE_OPEN            1
#define S_STATE_OPENING         2
#define S_STATE_CLOSING         3
#define S_STATE_ABORTED         4

#define XB_STATE_START          0
#define XB_STATE_WAITAT         1
#define XB_STATE_CONFIG         2
#define XB_STATE_DETECT         3
#define XB_STATE_ONLINE         4

int r_state;
int r_requested_state;

int s_state;
int s_requested_state;

int xb_state;

// Configuration

#define EEPROM_R_ADDRESS       0
#define EEPROM_S_ADDRESS       (EEPROM_R_ADDRESS + sizeof(rotator_config_t))

#define R_MAX_STEPS            55080
#define S_MAX_STEPS            26000

typedef struct {
  int data_id;
  long int accelleration_ramp;
  long int dead_zone;
  long int home_position;
  long int max_steps;
  long int velocity;
} rotator_config_t;

typedef struct {
  int data_id;
  long int accelleration_ramp;
  long int max_steps;
  long int velocity;
} shutter_config_t;

rotator_config_t r;
shutter_config_t s;

/* Runtime parameters */
long int r_position = 0;
long int r_requested_position = 0;
long int s_position = 0;
int s_battery_voltage = 1000;
int s_battery_status_report = 1;

void rotator_defaults() {
  r.data_id = R_DATA_ID;
  r.accelleration_ramp = 1500;
  r.dead_zone = 300;
  r.home_position = 0;
  r.max_steps = R_MAX_STEPS;
  r.velocity = 600; // ~4deg/sec
}

void shutter_defaults() {
  s.data_id = S_DATA_ID;
  s.accelleration_ramp = 1500;
  s.max_steps = S_MAX_STEPS;
  s.velocity = 800; // ~5deg/sec
}

String compose_message(String message) {
  return RESPONSE_START + message + RESPONSE_END;
}

String SES_response() {
  bool open_sensor = false;
  bool closed_sensor = false;

  if (s_position == S_MAX_STEPS) open_sensor = true;
  if (s_position == 0) closed_sensor = true;

  return compose_message("SES," +
         String(s_position, DEC) + "," +
         String(s.max_steps, DEC) + "," +
         String(open_sensor, DEC) + "," +
         String(closed_sensor, DEC)
         );
}

String SER_response() {
  bool home_sensor = false;

  if (r_position == r.home_position) home_sensor = true;

  return compose_message("SER," +
         String(r_position, DEC) + "," +
         String(home_sensor, DEC) + "," +
         String(r.max_steps, DEC) + "," +
         String(r.home_position, DEC) + "," +
         String(r.dead_zone, DEC)
         );
}

void load_rotator_settings() {
  int eerpom_r_id;
  EEPROM.get(EEPROM_R_ADDRESS, eerpom_r_id);
  if (R_DATA_ID != eerpom_r_id){
    rotator_defaults();
  } else {
    EEPROM.get(EEPROM_R_ADDRESS, r);
  }
}


void load_shutter_settings() {
  int eerpom_s_id;
  EEPROM.get(EEPROM_S_ADDRESS, eerpom_s_id);
  if (S_DATA_ID != eerpom_s_id){
    shutter_defaults();
  } else {
    EEPROM.get(EEPROM_S_ADDRESS, s);
  }
}


String NexDomeProcessCommand(char ReceivedBuffer[], int BufferLength);

void setup() {
  Serial.setTimeout(SERIAL_TIMEOUT);
  Serial.begin(BAUD_RATE);
  while (!Serial);


  /* Default configuration */
  load_rotator_settings();
  load_shutter_settings();

  delay_start = r_delay_start = s_delay_start = xb_delay_start = millis();
  r_state = R_STATE_STOPPED;
  r_requested_state = R_STATE_STOPPED;
  s_state = S_STATE_CLOSED;
  s_requested_state = S_STATE_CLOSED;
  xb_state = XB_STATE_START;
  Serial.print(XB_START_MSG);
}

void loop() {
  switch (CommandReceiverState) {
    case STATE_BUFFER_CLEAR:
      if (Serial.available()) {
        CurrentSymbol = Serial.read();
        if (START_COMMAND == CurrentSymbol) {
          CommandReceiverState = STATE_COMMAND_STARTED;
        }
      }
      break;

    case STATE_COMMAND_STARTED:
      if (Serial.available()) {
        CurrentSymbol = Serial.read();
        if (START_COMMAND == CurrentSymbol) {
          ReceiveCounter = 0;
        }
        else {
          if ((COMMAND_TERMINATION_CR == CurrentSymbol) || (COMMAND_TERMINATION_LF == CurrentSymbol)) {
            if (0 == ReceiveCounter) {
              CommandReceiverState = STATE_COMMAND_ERROR;
            }
            else {
              ReceiveBuffer[ReceiveCounter] = 0;
              CommandReceiverState = STATE_COMMAND_RECEIVED;
            }
          }
          else {
            if (ReceiveCounter < COMMAND_LENGTH) {
              ReceiveBuffer[ReceiveCounter] = CurrentSymbol;
              ReceiveCounter++;
            }
            else {
              CommandReceiverState = STATE_COMMAND_ERROR;
              ReceiveCounter = 0;
            }
          }
        }
      }
      break;

    case STATE_COMMAND_RECEIVED:
      ResponseMessage = NexDomeProcessCommand(ReceiveBuffer, ReceiveCounter);
      Serial.print(ResponseMessage);
      CommandReceiverState = STATE_BUFFER_CLEAR;
      ReceiveBuffer[0] = 0;
      ReceiveCounter = 0;
      break;

    case STATE_COMMAND_ERROR:
      Serial.print(ERROR_MESSAGE);
      CommandReceiverState = STATE_BUFFER_CLEAR;
      break;
  }

  // check if delay has timed out
  if (((millis() - delay_start) >= DELAY_TIME)) {
    delay_start += DELAY_TIME; // this prevents delay drifting

    if (s_battery_status_report) {
      // Report Battery Voltage
      ResponseMessage = compose_message(String("BV") + String(s_battery_voltage, DEC));
      Serial.print(ResponseMessage);
    }

    // Report Shutter position if needed
    if ((s_state == S_STATE_OPENING) || (s_state == S_STATE_CLOSING)) {
      Serial.print("S" + String(s_position, DEC) + "\n");
    }

    // Report Rotator position if needed
    if ((r_state == R_STATE_MOVING_LEFT) || (r_state == R_STATE_MOVING_RIGHT)) {
      Serial.print("P" + String(r_position, DEC) + "\n");
    }
  }

  // check if XB report is needed
  if (((millis() - xb_delay_start) >= XB_DELAY_TIME) && (xb_state == XB_STATE_ONLINE)) {
    xb_delay_start += XB_DELAY_TIME; // this prevents delay drifting
    Serial.print(XB_ONLINE_MSG);
  } else if (((millis() - xb_delay_start) >= XB_TRANSITION_TIME) && (xb_state != XB_STATE_ONLINE)) {
    xb_delay_start += XB_TRANSITION_TIME; // this prevents delay drifting
    xb_state++;
    switch(xb_state) {
      case XB_STATE_START:
        Serial.print(XB_START_MSG);
        break;
      case XB_STATE_WAITAT:
        Serial.print(XB_WAITAT_MSG);
        break;
      case XB_STATE_CONFIG:
        Serial.print(XB_CONFIG_MSG);
        break;
      case XB_STATE_DETECT:
        Serial.print(XB_DETECT_MSG);
        break;
      case XB_STATE_ONLINE:
        Serial.print(XB_ONLINE_MSG);
        break;
    }
  }

  // Change Shutter state
  if (s_requested_state != s_state) {
    switch (s_requested_state) {
      case S_STATE_OPENING:
        if (s_state == S_STATE_OPEN) {
          s_requested_state = s_state;
        } else {
          s_state = s_requested_state;
          Serial.print(compose_message("open"));
        }
        break;
      case S_STATE_CLOSING:
        if (s_state == S_STATE_CLOSED) {
          s_requested_state = s_state;
        } else {
          s_state = s_requested_state;
          Serial.print(compose_message("close"));
        }
        break;
      case S_STATE_ABORTED:
        if ((s_state == S_STATE_OPENING) || (s_state == S_STATE_CLOSING)) {
          s_state = S_STATE_ABORTED;
          Serial.print(SES_response());
        }
        break;
    }
  }

  // Change Rotator state
  if (r_requested_state != r_state) {
    switch (r_requested_state) {
      case R_STATE_MOVING_LEFT:
        if (r_state != R_STATE_MOVING_LEFT) {
          r_state = r_requested_state;
          Serial.print(compose_message("left"));
        } else {
          r_requested_state = r_state;
        }
        break;
      case R_STATE_MOVING_RIGHT:
        if (r_state != R_STATE_MOVING_RIGHT) {
          r_state = r_requested_state;
          Serial.print(compose_message("right"));
        } else {
          r_requested_state = r_state;
        }
        break;
      case R_STATE_STOPPED:
        if ((r_state == R_STATE_MOVING_LEFT) || (r_state == R_STATE_MOVING_RIGHT)) {
          r_state = R_STATE_STOPPED;
          Serial.print(SER_response());
        }
        break;
    }
  }

  // Process Shutter move request
  if (((millis() - s_delay_start) >= 1)) {
    s_delay_start += 1; // this prevents delay drifting
    if (s_state == S_STATE_OPENING) {
      if ((s_position < s.max_steps) && (s_position < S_MAX_STEPS)) s_position++;
      else {
        s_state = S_STATE_OPEN;
        Serial.print(SES_response());
      }
    } else if (s_state == S_STATE_CLOSING) {
      if (s_position > 0) s_position--;
      else {
        s_state = S_STATE_CLOSED;
        s_position = 0;
        Serial.print(SES_response());
      }
    }
  }

  // Process Rotator request
  if (((millis() - r_delay_start) >= 1)) {
    r_delay_start += 1; // this prevents delay drifting
    if (r_requested_position != r_position) {
      if (r_state == R_STATE_MOVING_LEFT) {
        r_position--;
      }
      else if (r_state == R_STATE_MOVING_RIGHT) {
        r_position++;
      }
      if (r_position > r.max_steps) {
        r_position = 0;
      }
      if (0 > r_position ) {
        r_position = r.max_steps;
      }
    }
    else {
      r_requested_state = R_STATE_STOPPED;
    }
  }
}

String NexDomeProcessCommand(char ReceivedBuffer[], int BufferLength)
{
  String response;
  String command;

  if (BufferLength < 3) {
    response = ERROR_MESSAGE;
  }
  else {
    command = String(ReceivedBuffer);

    response = COMPOSE_COMMAND_RESPONSE(command);

    /* Firmware protocol commands */
    //======================================
    if (command.equals("ARR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r.accelleration_ramp);
    }
    //======================================
    else if (command.equals("ARS")) {
      response = COMPOSE_VALUE_RESPONSE(command, s.accelleration_ramp);
    }
    //======================================
    else if (command.startsWith("AWR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      int accelleration_ramp = GET_INT_PARAM(command);
      if (accelleration_ramp < 100)
        response = ERROR_MESSAGE;
      else
        r.accelleration_ramp = accelleration_ramp;
    }
    //======================================
    else if (command.startsWith("AWS")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      int accelleration_ramp = GET_INT_PARAM(command);
      if (accelleration_ramp < 100)
        response = ERROR_MESSAGE;
      else
        s.accelleration_ramp = accelleration_ramp;
    }
    //=======================================
    else if (command.equals("CLS")) {
      s_requested_state = S_STATE_CLOSING;
    }
    //=======================================
    else if (command.equals("DRR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r.dead_zone);
    }
    //=======================================
    else if (command.startsWith("DWR")) {
      if NO_PARAMS(command) return ERROR_MESSAGE;
      int dead_zone = GET_INT_PARAM(command);
      if ((dead_zone < 0) || (dead_zone > 10000))
        response = ERROR_MESSAGE;
      else
        r.dead_zone = dead_zone;
    }
    //========================================
    else if (command.equals("FRR")) {
      response = compose_message(command.substring(0, 2) + R_VERSION);
    }
    //========================================
    else if (command.equals("FRS")) {
      response = compose_message(command.substring(0, 2) + S_VERSION);
    }
    //========================================
    else if (command.startsWith("GAR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      int azimuth = GET_INT_PARAM(command);
      r_requested_position = (r.max_steps / 360) * azimuth;
      long int delta = r_position - r_requested_position;
      if (abs(delta) > r.dead_zone) {
        if (0 > delta){
          if (abs(delta) > r.max_steps/2){
            r_requested_state = R_STATE_MOVING_LEFT;
          } else {
            r_requested_state = R_STATE_MOVING_RIGHT;
          }
        } else {
          if (abs(delta) > r.max_steps/2){
            r_requested_state = R_STATE_MOVING_RIGHT;
          } else {
            r_requested_state = R_STATE_MOVING_LEFT;
          }
        }
      }
    }
	//========================================
#ifdef VERSION_3_2
    else if (command.startsWith("GSR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      r_requested_position = GET_INT_PARAM(command);
      long int delta = r_position - r_requested_position;
      if (abs(delta) > r.dead_zone) {
        if (0 > delta){
          if (abs(delta) > r.max_steps/2){
            r_requested_state = R_STATE_MOVING_LEFT;
          } else {
            r_requested_state = R_STATE_MOVING_RIGHT;
          }
        } else {
          if (abs(delta) > r.max_steps/2){
            r_requested_state = R_STATE_MOVING_RIGHT;
          } else {
            r_requested_state = R_STATE_MOVING_LEFT;
          }
        }
      }
    }
#endif /* VERSION_3_2 */
    //=======================================
    else if (command.equals("GHR")) {
      r_requested_position = r.home_position;
      r_requested_state = R_STATE_MOVING_RIGHT;
    }
    //=======================================
    else if (command.equals("HRR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r.home_position);
    }
    //=======================================
    else if (command.startsWith("HWR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      int home_position = GET_INT_PARAM(command);
      if ((home_position < 0) || (home_position > r.max_steps))
        response = ERROR_MESSAGE;
      else
        r.home_position = home_position;
    }
    //=======================================
    else if (command.equals("OPS")) {
      s_requested_state = S_STATE_OPENING;
    }
    //=======================================
    else if (command.equals("PRR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r_position);
    }
    //=======================================
    else if (command.equals("PRS")) {
      response = COMPOSE_VALUE_RESPONSE(command, s_position);
    }
    //=======================================
    else if (command.startsWith("PWR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      r_position = GET_INT_PARAM(command);
    }
    //=======================================
    else if (command.startsWith("PWS")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      s_position = GET_INT_PARAM(command);
    }
    //=======================================
    else if (command.equals("RRR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r.max_steps);
    }
    //=======================================
    else if (command.equals("RRS")) {
      response = COMPOSE_VALUE_RESPONSE(command, s.max_steps);
    }
    //=======================================
    else if (command.startsWith("RWR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      r.max_steps = GET_INT_PARAM(command);
    }
    //=======================================
    else if (command.startsWith("RWS")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      s.max_steps = GET_INT_PARAM(command);
    }
    //=======================================
    else if (command.equals("SRR")) {
      response = SER_response();
    }
    //=======================================
    else if (command.equals("SRS")) {
      response = SES_response();
    }
    //=======================================
    else if (command.equals("SWR")) {
      r_requested_state = R_STATE_STOPPED;
    }
    //=======================================
    else if (command.equals("SWS")) {
      s_requested_state = S_STATE_ABORTED;
    }
    //=======================================
    else if (command.equals("VRR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r.velocity);
    }
    //=======================================
    else if (command.equals("VRS")) {
      response = COMPOSE_VALUE_RESPONSE(command, s.velocity);
    }
    //=======================================
    else if (command.startsWith("VWR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      int velocity = GET_INT_PARAM(command);
      if (velocity < 32)
        response = ERROR_MESSAGE;
      else
        r.velocity = velocity;
    }
    //=======================================
    else if (command.startsWith("VWS")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      int velocity = GET_INT_PARAM(command);
      if (velocity < 32)
        response = ERROR_MESSAGE;
      else
        s.velocity = velocity;
    }
    //======================================
    else if (command.equals("ZDR")) {
      rotator_defaults();
    }
    //======================================
    else if (command.equals("ZDS")) {
      shutter_defaults();
    }
    //======================================
    else if (command.equals("ZRR")) {
      load_rotator_settings();
    }
    //======================================
    else if (command.equals("ZRS")) {
      load_shutter_settings();
    }
    //======================================
    else if (command.equals("ZWR")) {
      EEPROM.put(EEPROM_R_ADDRESS, r);
    }
    //======================================
    else if (command.equals("ZWS")) {
      EEPROM.put(EEPROM_S_ADDRESS, s);
    }
    //======================================
    /* Additional commands to simulate events */
    /* Set battery voltage in range [0..1023], @wbv,dddd -> :wbv# */
    else if (command.startsWith("wbv")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      s_battery_voltage = GET_INT_PARAM(command);
    }
    /* Simulate rain */
    else if (command.equals("StartRain")) {
      response = RAIN_DETECTED;
      s_requested_state = S_STATE_CLOSING;
    }
    /* Stop rain */
    else if (command.equals("StopRain")) {
      response = RAIN_STOPPED;
    }
    /* Enable battery status report */
    else if (command.equals("EnaBatStatus")) {
      response = BATTERY_ENABLED;
      s_battery_status_report = 1;
    }
    /* Disable battery status report */
    else if (command.equals("DisBatStatus")) {
      response = BATTERY_DISABLED;
      s_battery_status_report = 0;
    }
    /* Simulate Xbee reset */
    else if (command.equals("XBReset")) {
      xb_state = XB_STATE_START;
      response = XB_START_MSG;
      xb_delay_start = millis();
    }
    /* Delete EEPROM */
    else if (command.equals("DeleteEEPROM")){
      for (int i = 0 ; i < EEPROM.length() ; i++) {
        EEPROM.write(i, 0);
        response = EEPROM_CLEAR;
      }
    }
    //========================================
    else {
      response = ERROR_MESSAGE;
    }
  }

  return response;
}
