#define BAUD_RATE                  9600
#define SERIAL_TIMEOUT             1000
#define COMMAND_LENGTH             30
#define START_COMMAND              '@'
#define SEPARATOR                  ','
#define COMMAND_TERMINATION_CR     '\r'
#define COMMAND_TERMINATION_LF     '\n'
#define START_RESPONSE             ':'
#define RESPONSE_TERMINATION       '#'
#define ERROR_MESSAGE              ":Err#"
#define RAIN_DETECTED              ":Rain#"
#define RAIN_STOPPED               ":RainStopped#"
#define BATTERY_ENABLED            ":BateryStatusEnabled#"
#define BATTERY_DISABLED           ":BateryStatusDisabled#"

#define STATE_BUFFER_CLEAR         0
#define STATE_COMMAND_STARTED      1
#define STATE_COMMAND_RECEIVED     2
#define STATE_COMMAND_ERROR        3


#define COMPOSE_VALUE_RESPONSE(command, value) (":" + command.substring(0,3) + String(value, DEC) + "#")
#define COMPOSE_RESPONSE(command)              (":" + command.substring(0,3) + "#")
#define NO_PARAMS(command)                     ((command.length() <= 4) || (command.charAt(3) != ','))
#define GET_INT_PARAM(command)                 (command.substring(4).toInt())


char ReceiveBuffer[COMMAND_LENGTH];
String ResponseMessage;
char CurrentSymbol;
int ReceiveCounter = 0;
int ReceivedCommandLength = 0;
int CommandReceiverState = STATE_BUFFER_CLEAR;

#define DELAY_TIME 1000  // 1sec
long int delay_start;

long int r_delay_start;
long int s_delay_start;


String r_version = "3.0.0";
String s_version = "3.0.1";

#define R_STATE_STOPPED         0
#define R_STATE_MOVNG_LEFT      1
#define R_STATE_MOVING_RIGHT    2

#define S_STATE_CLOSED          0
#define S_STATE_OPEN            1
#define S_STATE_OPENING         2
#define S_STATE_CLOSING         3
#define S_STATE_ABORTED         4

int r_state;
int r_requested_state;

int s_state;
int s_requested_state;


/* Default configuration */
long int r_accelleration_ramp = 1500;
long int s_accelleration_ramp = 1500;
long int r_dead_zone = 300;
long int r_home_position = 0;
long int r_max_steps = 55080;
long int s_max_steps = 12000;
long int r_velosity = 600; // ~4deg/sec
long int s_velosity = 800; // ~5deg/sec

/* Runtime parameters */
long int r_position = 1600;
long int s_position = 0;
int s_battery_voltage = 1000;
int s_battery_status_report = 1;


String SES_response() {
  bool open_sensor = false;
  bool closed_sensor = false;

  if (s_position == s_max_steps) open_sensor = true;
  if (s_position == 0) closed_sensor = true;

  return ":SES," +
          String(s_position, DEC) + "," +
          String(s_max_steps, DEC) + "," +
          String(open_sensor, DEC) + "," +
          String(closed_sensor, DEC) +
          "#";
}


String SER_response() {
  bool home_sensor = false;

  if (r_position == r_home_position) home_sensor = true;

  return ":SER," +
          String(r_position, DEC) + "," +
          String(home_sensor, DEC) + "," +
          String(r_max_steps, DEC) + "," +
          String(r_home_position, DEC) + "," +
          String(r_dead_zone, DEC) +
          "#";
}


String NexDomeProcessCommand(char ReceivedBuffer[], int BufferLength);

void setup() {
  Serial.setTimeout(SERIAL_TIMEOUT);
  Serial.begin(BAUD_RATE);
  while (!Serial);

  delay_start = r_delay_start = s_delay_start = millis();
  r_state = R_STATE_STOPPED;
  r_requested_state = R_STATE_STOPPED;
  s_state = S_STATE_CLOSED;
  s_requested_state = S_STATE_CLOSED;
}

void loop() {
  switch (CommandReceiverState) {
    case STATE_BUFFER_CLEAR:
      if (Serial.available()) {
       CurrentSymbol = Serial.read();
       if (START_COMMAND == CurrentSymbol){
         CommandReceiverState = STATE_COMMAND_STARTED;  
       }
      }
      break;
        
    case STATE_COMMAND_STARTED:
      if (Serial.available()) {
        CurrentSymbol = Serial.read();
        if (START_COMMAND == CurrentSymbol){
          ReceiveCounter = 0; 
        }
        else{
          if ((COMMAND_TERMINATION_CR == CurrentSymbol) || (COMMAND_TERMINATION_LF == CurrentSymbol)){
            if (0 == ReceiveCounter){
              CommandReceiverState = STATE_COMMAND_ERROR;
            }
            else{
              ReceiveBuffer[ReceiveCounter] = 0;
              CommandReceiverState = STATE_COMMAND_RECEIVED;
            }
          }
          else{
            if (ReceiveCounter < COMMAND_LENGTH){
              ReceiveBuffer[ReceiveCounter] = CurrentSymbol;
              ReceiveCounter++;
            }
            else{
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

    if (s_battery_status_report){
      // Report Battery Voltage
      ResponseMessage = ":" + String("BV") + String(s_battery_voltage, DEC) + "#";
      Serial.print(ResponseMessage);
      if ((s_state == S_STATE_OPENING) || (s_state == S_STATE_CLOSING)) {
        Serial.print(String("S") + String(s_position, DEC) + "\n");
      }
    }

  }

  if (s_requested_state != s_state) {
    switch (s_requested_state) {
      case S_STATE_OPENING:
        if (s_state == S_STATE_OPEN) {
          s_requested_state = s_state;
        } else {
          s_state = s_requested_state;
          Serial.print(":open#");
        }
        break;
      case S_STATE_CLOSING:
        if (s_state == S_STATE_CLOSED) {
          s_requested_state = s_state;
        } else {
          s_state = s_requested_state;
          Serial.print(":close#");
        }
        break;
    }
  }

  if (((millis() - s_delay_start) >= 1)) {
    s_delay_start += 1; // this prevents delay drifting
    if (s_state == S_STATE_OPENING) {
      if (s_position < s_max_steps) s_position++;
      else {
        s_state = S_STATE_OPEN;
        s_position = s_max_steps;
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

    response = COMPOSE_RESPONSE(command);
    
    /* Firmware protocol commands */
    if (command.equals("ARR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r_accelleration_ramp);
    }
    else if (command.equals("ARS")) {
      response = COMPOSE_VALUE_RESPONSE(command, s_accelleration_ramp);
    }
    else if (command.startsWith("AWR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      r_accelleration_ramp = GET_INT_PARAM(command);
    }
    else if (command.startsWith("AWS")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      s_accelleration_ramp = GET_INT_PARAM(command);
    }
    else if (command.equals("CLS")) {
      s_requested_state = S_STATE_CLOSING;
    }
    else if (command.equals("DRR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r_dead_zone);
    }
    else if (command.startsWith("DWR")) {
      if NO_PARAMS(command) return ERROR_MESSAGE;
      r_dead_zone = GET_INT_PARAM(command);
    }
    else if (command.equals("FRR")) {
      response = ":" + command.substring(0,2) + r_version + "#";
    }
    else if (command.equals("FRS")) {
      response = ":" + command.substring(0,2) + s_version + "#";
    }
    else if (command.startsWith("GAR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      int azimuth = GET_INT_PARAM(command);
      // Check if deff is more than dead zone and then process goto...
    }
    else if (command.equals("GHR")) {
      // Goto Home position
    }
    else if (command.equals("HRR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r_home_position);
    }
    else if (command.startsWith("HWR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      r_home_position = GET_INT_PARAM(command);
    }
    else if (command.equals("OPS")) {
      s_requested_state = S_STATE_OPENING;
    }
    else if (command.equals("PRR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r_position);
    }
    else if (command.equals("PRS")) {
      response = COMPOSE_VALUE_RESPONSE(command, s_position);
    }
    else if (command.startsWith("PWR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      r_position = GET_INT_PARAM(command);
    }
    else if (command.startsWith("PWS")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      s_position = GET_INT_PARAM(command);
    }
    else if (command.equals("RRR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r_max_steps);
    }
    else if (command.equals("RRS")) {
      response = COMPOSE_VALUE_RESPONSE(command, s_max_steps);
    }
    else if (command.startsWith("RWR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      r_max_steps = GET_INT_PARAM(command);
    }
    else if (command.startsWith("RWS")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      s_max_steps = GET_INT_PARAM(command);
    }
    else if (command.equals("SRR")) {
      response = SER_response();
    }
    else if (command.equals("SRS")) {
      response = SES_response();
    }
    else if (command.equals("SWR")) {
      // abort motion
    }
    else if (command.equals("SWS")) {
      // abort motion
    }
    else if (command.equals("VRR")) {
      response = COMPOSE_VALUE_RESPONSE(command, r_velosity);
    }
    else if (command.equals("VRS")) {
      response = COMPOSE_VALUE_RESPONSE(command, s_velosity);
    }
    else if (command.startsWith("VWR")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      r_velosity = GET_INT_PARAM(command);
    }
    else if (command.startsWith("VWS")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      s_velosity = GET_INT_PARAM(command);
    }
    else if (command.equals("ZDR")) {
    }
    else if (command.equals("ZDS")) {
    }
    else if (command.equals("ZRR")) {
    }
    else if (command.equals("ZRS")) {
    }
    else if (command.equals("ZWR")) {
    }
    else if (command.equals("ZWS")) {
    }
    /* Additional commands to simulate events */
    /* Set battery voltage in range [0..1023], @WBV,dddd -> :WBV# */
    else if (command.startsWith("wbv")) {
      if (NO_PARAMS(command)) return ERROR_MESSAGE;
      s_battery_voltage = GET_INT_PARAM(command);
    }
    /* Simulate rain */
    else if (command.equals("StartRain")) {
      response = RAIN_DETECTED;
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
    else if (command.equals("DisBatStatus")) {
      response = BATTERY_DISABLED;
      s_battery_status_report = 0;
    }  
    else{
      response = ERROR_MESSAGE;
    }
  }
  
  return response;
}
