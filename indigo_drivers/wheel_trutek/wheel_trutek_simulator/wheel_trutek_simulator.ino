// Trutek wheel simulator for Arduino
//
// Copyright (c) 2018-2025 CloudMakers, s. r. o.
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

// The wheel turns one slot every STEP_TIME ms; the real rotation time is not known.
#define STEP_TIME 500

int current_filter = 1;
int target_filter = 1;
unsigned long last_step = 0;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  char buffer[4];
  if (current_filter != target_filter && millis() - last_step >= STEP_TIME) {
    if (current_filter < target_filter) {
      current_filter++;
    } else {
      current_filter--;
    }
    last_step = millis();
  }
  if (Serial.available() > 3) {
    Serial.readBytes(buffer, 4);
    switch (buffer[1]) {
      case 1:
        if (current_filter == target_filter) {
          last_step = millis();
        }
        target_filter = buffer[2];
        buffer[0] = 0xA5;
        buffer[1] = 0x81;
				if (current_filter == target_filter) {
					buffer[2] = target_filter;
				} else {
					buffer[2] = 0;
				}
        buffer[3] = buffer[0] + buffer[1] + buffer[2];
        Serial.write(buffer, 4);
        break;
      case 2:
        buffer[0] = 0xA5;
        buffer[1] = 0x82;
				if (current_filter == target_filter) {
					buffer[2] = 0x30 + current_filter;
				} else {
					buffer[2] = 0x30;
				}
        buffer[3] = buffer[0] + buffer[1] + buffer[2];
        Serial.write(buffer, 4);
        break;
      case 3:
        // The slot count; the wheel stays where it is.
        buffer[0] = 0xA5;
        buffer[1] = 0x83;
        buffer[2] = 0x35;
        buffer[3] = buffer[0] + buffer[1] + buffer[2];
        Serial.write(buffer, 4);
        break;
    }
  }
}
