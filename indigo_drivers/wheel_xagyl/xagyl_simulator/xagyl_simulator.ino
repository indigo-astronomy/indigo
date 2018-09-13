// Xagyl wheel simulator for Arduino
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

char current_filter = '1';

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  char command[2];
  if (Serial.available() > 1) {
    Serial.readBytes(command, 2);
    switch (command[0]) {
      case 'I':
        switch (command[1]) {
          case '0':
            Serial.println("Xagyl FW5125VX");
            break;
          case '1':
            Serial.println("FW3.1.5");
            break;
          case '2':
            Serial.print("P");
            Serial.println(current_filter);
            break;
          case '3':
            Serial.println("S/N: 123456");
            break;
          case '8':
            Serial.println("FilterSlots 5");
            break;
        }
        break;
      case 'G':
        current_filter = command[1];  
        break;
    }
  }
}
