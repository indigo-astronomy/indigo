// Interactive Astronomy SkyRoof simulator for Arduino
//
// Copyright (c) 2021-2025 CloudMakers, s. r. o.
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

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#define LCD
#endif

#define CLOSED  0
#define OPENED 30

#define STOPPED 0
#define OPEN    1
#define CLOSE  -1

int position = CLOSED;
int direction = STOPPED;
bool heater = false;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(500);
  while (!Serial)
    ;
}

void loop() {
  if (position > CLOSED && direction == CLOSE)
    position--;
  else if (position < OPENED && direction == OPEN)
    position++;
  else
    direction = STOPPED;    
  String command = Serial.readStringUntil('\r');
  if (command.equals("Open#")) {
    direction = OPEN;
    Serial.print("0#\r");
  } else if (command.equals("Close#")) {
    direction = CLOSE;
    Serial.print("0#\r");
  } else if (command.equals("Stop#")) {
    direction = STOPPED;
    Serial.print("0#\r");
  } else if (command.equals("Status#")) {
    if (position == CLOSED)    
      Serial.print("RoofClosed#\r");
    else if (position == OPENED)
      Serial.print("RoofOpen#\r");
    else
      Serial.print("Safety#\r");
  } else if (command.equals("Parkstatus#")) {
    Serial.print("0#\r");
  } else if (command.equals("HeaterOn#")) {
    heater = true;
  } else if (command.equals("HeaterOff#")) {
    heater = false;
  }
}
