// Trutek wheel simulator for Arduino
//
// Copyright (c) 2018 CloudMakers, s. r. o.
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

int current_filter = 1;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  char buffer[4];
  if (Serial.available() > 3) {
    Serial.readBytes(buffer, 4);
    switch (buffer[1]) {
      case 1:
        current_filter = buffer[2];
        buffer[0] = 0xA5;
        buffer[1] = 0x81;
        buffer[2] = current_filter;
        buffer[3] = buffer[0] + buffer[1] + buffer[2];
        delay(3000);
        Serial.write(buffer, 4);
        break;
      case 2:
        buffer[0] = 0xA5;
        buffer[1] = 0x82;
        buffer[2] = 0x30 + current_filter;
        buffer[3] = buffer[0] + buffer[1] + buffer[2];
        Serial.write(buffer, 4);
        break;
      case 3:
        current_filter = 1;
        buffer[0] = 0xA5;
        buffer[1] = 0x83;
        buffer[2] = 0x35;
        buffer[3] = buffer[0] + buffer[1] + buffer[2];
        Serial.write(buffer, 4);
        break;
    }
  }
}
