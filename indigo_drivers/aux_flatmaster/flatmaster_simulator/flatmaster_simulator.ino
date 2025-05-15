// Pegasus Astro FlatMaster for Arduino
//
// Copyright (c) 2019-2025 Rumen G. Bogdanovski
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

bool power = false;
int intensity = 20;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  String command = Serial.readStringUntil('\n');
  if (command.equals("#")) {
    Serial.println("OK_FM");
  } else if (command.startsWith("E:")) {
    power = command.charAt(2) == '1';
    if (power)
      Serial.println("E:1");
    else
      Serial.println("E:0");
  } else if (command.startsWith("L:")) {
    intensity = command.substring(2).toInt();
    if (intensity > 220) intensity = 220;
    if (intensity < 20) intensity = 20;
    Serial.print("L:");
    Serial.println(intensity);
  } else if (command.equals("V")) {
    Serial.println("1.1");
  } else if (command.length() > 0) {
    Serial.println("ERR");
  }
}
