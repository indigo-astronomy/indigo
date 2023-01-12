// Pegasus Indigo wheel simulator for Arduino
//
// Copyright (c) 2023 CloudMakers, s. r. o.
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
#endif

int current_filter = 1;
int target_filter = 1;
int moving = 0;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void update_state() {
  static long unsigned last_millis = 0;
  long unsigned current_millis = millis();
  long unsigned lapse = current_millis - last_millis;
  if (moving == 1) {
    if (lapse < 1000)
      return;
    if (target_filter < current_filter) {
      current_filter--;
    } else if (target_filter > current_filter) {
      current_filter++;
    } else {
      moving = 0;
    }
  }
  last_millis = current_millis;
}


void loop() {
  update_state();
  String command = Serial.readStringUntil('\n');
  if (command.equals("W#")) {
    Serial.println("FW_OK");
  } else if (command.equals("WA")) {
    Serial.print("FW_OK:");
    Serial.print(current_filter);
    Serial.print(":");
    Serial.println(moving);
  } else if (command.equals("WF")) {
    Serial.print("WF:");
    Serial.println(moving == 0 ? current_filter : -1);
  } else if (command.equals("WV")) {
    Serial.println("WV:1.1");
  } else if (command.startsWith("WM:")) {
    target_filter = command.substring(3).toInt();
    moving = 1;
    Serial.println(command);
  } else if (command.equals("WR")) {
    Serial.print("WR:");
    Serial.println(moving);
  } else if (command.equals("WI")) {
    moving = 0;
    current_filter = 1;
    Serial.println("WI:1");
  } else if (command.equals("WQ")) {
  }
}
