// Brightstar Quantum wheel simulator for Arduino
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

char current_filter = '0';
char target_filter = '0';

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  String command = Serial.readStringUntil('\n');
	if (command.startsWith("SN")) {
		Serial.println("SN123456");
	} else if (command.startsWith("G")) {
		target_filter = command.charAt(1);
		while (current_filter != target_filter) {
			delay(1000);
			Serial.print("P");
			Serial.println(current_filter);
			if (current_filter < target_filter) {
				current_filter++;
			} else if (current_filter > target_filter) {
				current_filter--;
			}
		}
		delay(1000);
    Serial.print("P");
    Serial.println(current_filter);
  }
}
