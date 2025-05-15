// Optec wheel simulator for Arduino
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

bool is_ready = false;
bool goto_err = false;

char current_filter = '1';
char last_filter = '5';

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  char command[7] = { 0 };
  Serial.readBytes(command, 6);
  if (!strcmp(command, "WSMODE")) {
    is_ready = true;
    Serial.println("!");
  } else if (is_ready && !strcmp(command, "WEXITS")) {
    is_ready = false;
    Serial.println("END");
  } else if (is_ready && !strcmp(command, "WFILTR")) {
    Serial.println(current_filter);
  } else if (is_ready && !strncmp(command, "WGOTO", 5)) {
    char new_filter = command[5];
    if (new_filter >= '1' && new_filter <= last_filter) {
      if (goto_err) {
        Serial.println("ERR=4");
      } else {
        current_filter = new_filter;
        delay(3000);
        Serial.println("*");
      }
    } else {
      Serial.println("ERR=5");
    }
  }
}
