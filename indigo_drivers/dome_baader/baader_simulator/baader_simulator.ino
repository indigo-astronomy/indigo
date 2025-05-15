// Baader Planetaruium Dome simulator for Arduino
//
// https://www.baader-planetarium.com/en
//
// Copyright (c) 2020-2025 Rumen G. Bogdanovski
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

//#define OLD_FIRMWARE

#include <EEPROM.h>

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

// Communication constants
#define BAUD_RATE                  9600
#define SERIAL_TIMEOUT             1000
#define COMMAND_LENGTH             9


#define STATE_BUFFER_CLEAR         0
#define STATE_COMMAND_STARTED      1
#define STATE_COMMAND_RECEIVED     2
#define STATE_COMMAND_ERROR        3

char recv_buffer[COMMAND_LENGTH+1] = {0};
String response_message;
char current_symbol;
int bytes_received = 0;
int ReceivedCommandLength = 0;
int command_receiver_state = STATE_BUFFER_CLEAR;

#define BASE_DELAY_TIME             20  // 0.02sec
#define R_DELAY_TIME                (BASE_DELAY_TIME)
#define S_DELAY_TIME                (10 * BASE_DELAY_TIME)
#define F_DELAY_TIME                (10 * BASE_DELAY_TIME)

long int delay_start;
long int r_delay_start;
long int s_delay_start;
long int f_delay_start;


// Device states

#define R_STATE_STOPPED         0
#define R_STATE_MOVING_LEFT     1 //counter-clockwise
#define R_STATE_MOVING_RIGHT    2 //clockwise

#define S_STATE_CLOSED          0
#define S_STATE_OPEN            1
#define S_STATE_OPENING         2
#define S_STATE_CLOSING         3
#define S_STATE_ABORTED         4

#define F_STATE_CLOSED          0
#define F_STATE_OPEN            1
#define F_STATE_OPENING         2
#define F_STATE_CLOSING         3
#define F_STATE_ABORTED         4

#define ERROR_MESSAGE           "d#comerro"
#define OK_MESSAGE              "d#gotmess"

int r_state;
int r_requested_state;

int s_state;
int s_requested_state;

int f_state;
int f_requested_state;

// Configuration

#define R_MAX_STEPS            3600
#define S_MAX_STEPS            100
#define F_MAX_STEPS            100

/* Runtime parameters */
long int r_position = 0;
long int r_requested_position = 0;
long int s_position = 0;
long int s_requested_position = 0;
long int f_position = 0;
int eme[4] = {0};


String baader_process_command(char command_buffer[]);

void setup() {
  Serial.setTimeout(SERIAL_TIMEOUT);
  Serial.begin(BAUD_RATE);
  while (!Serial);

  delay_start = r_delay_start = s_delay_start = f_delay_start = millis();
  r_state = R_STATE_STOPPED;
  r_requested_state = R_STATE_STOPPED;
  s_state = S_STATE_CLOSED;
  s_requested_state = S_STATE_CLOSED;
  f_state = F_STATE_CLOSED;
  f_requested_state = F_STATE_CLOSED;
}

void loop() {
    switch (command_receiver_state) {
      case STATE_BUFFER_CLEAR:
      case STATE_COMMAND_STARTED:
        if (Serial.available()) {
          current_symbol = Serial.read();
          bytes_received++;
          command_receiver_state = STATE_COMMAND_STARTED;
          recv_buffer[bytes_received-1] = current_symbol;
          if (bytes_received == COMMAND_LENGTH){
             recv_buffer[bytes_received] = 0;
             command_receiver_state = STATE_COMMAND_RECEIVED;
          }
        }
        break;
      case STATE_COMMAND_RECEIVED:
        response_message = baader_process_command(recv_buffer);
        Serial.print(response_message);
        command_receiver_state = STATE_BUFFER_CLEAR;
        recv_buffer[0] = 0;
        bytes_received = 0;
        break;
    }

    // check if delay has timed out
    if (((millis() - delay_start) >= BASE_DELAY_TIME)) {
      delay_start += BASE_DELAY_TIME; // this prevents delay drifting
      // Change Shutter state
      if (s_requested_state != s_state) {
        switch (s_requested_state) {
          case S_STATE_OPENING:
            if (s_state == S_STATE_OPEN) {
              s_requested_state = s_state;
            } else {
              s_state = s_requested_state;
            }
            break;
          case S_STATE_CLOSING:
            if (s_state == S_STATE_CLOSED) {
              s_requested_state = s_state;
            } else {
              s_state = s_requested_state;
            }
            break;
          case S_STATE_ABORTED:
            if ((s_state == S_STATE_OPENING) || (s_state == S_STATE_CLOSING)) {
              s_state = S_STATE_ABORTED;
            }
            break;
        }
      }

      // Change Flap state
      if (f_requested_state != f_state) {
        switch (f_requested_state) {
          case F_STATE_OPENING:
            if (f_state == F_STATE_OPEN) {
              f_requested_state = f_state;
            } else {
              f_state = f_requested_state;
            }
            break;
          case F_STATE_CLOSING:
            if (f_state == F_STATE_CLOSED) {
              f_requested_state = f_state;
            } else {
              f_state = f_requested_state;
            }
            break;
          case F_STATE_ABORTED:
            if ((f_state == F_STATE_OPENING) || (f_state == F_STATE_CLOSING)) {
              f_state = F_STATE_ABORTED;
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
            } else {
              r_requested_state = r_state;
            }
            break;
          case R_STATE_MOVING_RIGHT:
            if (r_state != R_STATE_MOVING_RIGHT) {
              r_state = r_requested_state;
            } else {
              r_requested_state = r_state;
            }
            break;
          case R_STATE_STOPPED:
            if ((r_state == R_STATE_MOVING_LEFT) || (r_state == R_STATE_MOVING_RIGHT)) {
              r_state = R_STATE_STOPPED;
            }
            break;
        }
      }

      // Process Shutter move request
      if (((millis() - s_delay_start) >= S_DELAY_TIME)) {
        s_delay_start += S_DELAY_TIME; // this prevents delay drifting
        if (s_requested_position != s_position) {
          if (s_state == S_STATE_OPENING) {
            if (s_position < S_MAX_STEPS) s_position++;
          } else if (s_state == S_STATE_CLOSING) {
            if (s_position > 0) s_position--;
          }
        }
        if (s_position == S_MAX_STEPS) s_state = S_STATE_OPEN;
        else if (s_position == 0) s_state = S_STATE_CLOSED;
        else if (s_requested_position == s_position) s_state = S_STATE_ABORTED;
      }

      // Process Flap move request
      if (((millis() - f_delay_start) >= F_DELAY_TIME)) {
        f_delay_start += F_DELAY_TIME; // this prevents delay drifting
         if (f_state == F_STATE_OPENING) {
          if (f_position < F_MAX_STEPS) f_position++;
          if (f_position == F_MAX_STEPS) f_state = F_STATE_OPEN;
        } else if (f_state == F_STATE_CLOSING) {
          if (f_position > 0) f_position--;
          if (f_position == 0) {
            f_state = F_STATE_CLOSED;
            f_position = 0;
          }
        }
      }

      // Process Rotator request
      if (((millis() - r_delay_start) >= R_DELAY_TIME)) {
        r_delay_start += R_DELAY_TIME; // this prevents delay drifting
        if (r_requested_position != r_position) {
          if (r_state == R_STATE_MOVING_LEFT) {
            r_position--;
          } else if (r_state == R_STATE_MOVING_RIGHT) {
            r_position++;
          }
          if (r_position > R_MAX_STEPS) {
            r_position = 0;
          }
          if (0 > r_position ) {
            r_position = R_MAX_STEPS;
          }
        } else {
          r_requested_state = R_STATE_STOPPED;
        }
      }
    }
}

String baader_process_command(char command_buffer[]) {
  String response;
  String command;

  command = String(command_buffer);

  response = OK_MESSAGE;

    /* Firmware protocol commands */
    //======================================
    if (command.equals("d#ser_num")) {
      response = "d#3141592";
    }
    //======================================
#ifdef OLD_FIRMWARE
    else if (command.equals("d#getshut")) {
      if (s_state == S_STATE_OPEN) {
        response = "d#shutope";
      } else if (s_state == S_STATE_CLOSED) {
        response = "d#shutclo";
      } else {
        response = "d#shutrun";
      }
    }
#else
    else if (command.equals("d#getshut")) {
      if (s_state == S_STATE_OPEN) {
        response = "d#shutope";
      } else if (s_state == S_STATE_CLOSED) {
        response = "d#shutclo";
      } else {
        char buffer[4];
        snprintf(buffer, sizeof(buffer), "%02d", s_position);
        response = "d#shut_" + String(buffer);
      }
    }
#endif
    //=======================================
    else if (command.equals("d#opeshut")) {
      if (s_state == S_STATE_CLOSING) {
        s_requested_state = S_STATE_ABORTED;
        s_requested_position = s_position;
      } else {
        s_requested_state = S_STATE_OPENING;
        s_requested_position = S_MAX_STEPS;
      }
    }
    //=======================================
    else if (command.equals("d#closhut")) {
      if (s_state == S_STATE_OPENING) {
        s_requested_state = S_STATE_ABORTED;
        s_requested_position = s_position;
      } else {
        s_requested_state = S_STATE_CLOSING;
        s_requested_position = 0;
      }
    }
    //========================================
    else if (command.startsWith("d#azi")) {
      int azimuth = command.substring(5).toInt();
      r_requested_position = (R_MAX_STEPS / 3600) * azimuth;
      long int delta = r_position - r_requested_position;
        if (0 > delta){
          if (abs(delta) > R_MAX_STEPS/2){
            r_requested_state = R_STATE_MOVING_LEFT;
          } else {
            r_requested_state = R_STATE_MOVING_RIGHT;
          }
        } else {
          if (abs(delta) > R_MAX_STEPS/2){
            r_requested_state = R_STATE_MOVING_RIGHT;
          } else {
            r_requested_state = R_STATE_MOVING_LEFT;
          }
        }
    }
    //=======================================
#ifdef OLD_FIRMWARE
    else if (command.equals("d#getazim")) {
      char buffer[5];
      snprintf(buffer, sizeof(buffer), "%04d", r_position);
      if (r_state == R_STATE_STOPPED)
          response = "d#azr" + String(buffer);
      else
          response = "d#azi" + String(buffer);
    }
#else
    else if (command.equals("d#getazim")) {
      char buffer[5];
      snprintf(buffer, sizeof(buffer), "%04d", r_position);
      response = "d#azi" + String(buffer);
    }
#endif
    //========================================
    else if (command.equals("d#stopdom")) {
      r_requested_state = R_STATE_STOPPED;
      s_requested_state = S_STATE_ABORTED;
      f_requested_state = F_STATE_ABORTED;
    }
    //======================================
    else if (command.startsWith("d#mvsht")) {
      int pos = command.substring(7).toInt();
      s_requested_position = pos;
      long int delta = s_position - s_requested_position;
      if (0 > delta){
        s_requested_state = S_STATE_OPENING;
      } else {
        s_requested_state = S_STATE_CLOSING;
      }
    }
    //======================================
#ifdef OLD_FIRMWARE
    else if (command.equals("d#getflap")) {
      if (f_state == F_STATE_OPEN) {
        response = "d#flapope";
      } else if (f_state == F_STATE_CLOSED) {
        response = "d#flapclo";
      } else {
        response = "d#flaprun";
      }
    }
#else
    else if (command.equals("d#getflap")) {
      if (f_state == F_STATE_OPEN) {
        response = "d#flapope";
      } else if (f_state == F_STATE_CLOSED) {
        response = "d#flapclo";
      } else if (f_state == F_STATE_CLOSING) {
        response = "d#flaprcl";
      } else if (f_state == F_STATE_OPENING) {
        response = "d#flaprop";
      } else {
        response = "d#flaprim";
      }
    }
#endif
    //=======================================
    else if (command.equals("d#opeflap")) {
      if (s_position < 5) response = "d#err_sht";
      else {
        if (f_state == F_STATE_CLOSING) {
          f_requested_state = F_STATE_ABORTED;
        } else {
          f_requested_state = F_STATE_OPENING;
        }
      }
    }
    //=======================================
    else if (command.equals("d#cloflap")) {
       if (s_position < 5) response = "d#err_sht";
      else {
        if (f_state == F_STATE_OPENING) {
          f_requested_state = F_STATE_ABORTED;
        } else {
          f_requested_state = F_STATE_CLOSING;
        }
      }
    }
    //=======================================
    else if (command.equals("d#opefull")) {
      // Open shutter
      if (s_state == S_STATE_CLOSING) {
        s_requested_state = S_STATE_ABORTED;
        s_requested_position = s_position;
      } else {
        s_requested_state = S_STATE_OPENING;
        s_requested_position = S_MAX_STEPS;
      }
      // Open flap
      if (f_state == F_STATE_CLOSING) {
        f_requested_state = F_STATE_ABORTED;
      } else {
        f_requested_state = F_STATE_OPENING;
      }
    }
    //======================================
    else if (command.equals("d#get_eme")) {
      char buffer[5];
      snprintf(buffer, sizeof(buffer), "%1d%1d%1d%1d", eme[0], eme[1], eme[2], eme[3]);
      response = "d#eme" + String(buffer);
    }
    //======================================
    else if (command.equals("d#chk_aon")) {
      response = "d#automod";
    }
    //======== Simulator Commands == =========
    else if (command.startsWith("d#EME")) {
      eme[0] = (command.substring(5,6).toInt() > 0) ? 1 : 0;
      eme[1] = (command.substring(6,7).toInt() > 0) ? 1 : 0;
      eme[2] = (command.substring(7,8).toInt() > 0) ? 1 : 0;
      eme[3] = (command.substring(8,9).toInt() > 0) ? 1 : 0;
    }
    //========================================
    else {
      response = ERROR_MESSAGE;
    }
  return response;
}
