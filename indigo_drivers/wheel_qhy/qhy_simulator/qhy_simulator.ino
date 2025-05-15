// QHY CFW wheel simulator for Arduino
//
// Copyright (c) 2020-2025 CloudMakers, s. r. o.
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
//
// https://www.qhyccd.com/index.php?m=content&c=index&a=show&catid=68&id=234

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#define CFW1
//#define CFW2
//#define CFW3


char current_filter = '0';

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  char command = Serial.read();
  if ((command >= '0' && command <= '9') || (command >= 'A' && command <= 'F')) {
#ifdef CFW1
    if (current_filter != command) {
      delay(3000);
      Serial.write('-');
    }
#endif    
#ifdef CFW2
    if (current_filter != command) {
      delay(3000);
      Serial.write(command);
    }
#endif    
#ifdef CFW3
    delay(3000);
    Serial.write(command);
#endif    
    current_filter = command;
#ifdef CFW3
  } else if ((command == 'N')) {
    Serial.read(); // O
    Serial.read(); // W
    Serial.write(current_filter);
  } else if ((command == 'V')) {
    Serial.read(); // R
    Serial.read(); // S
    Serial.write("20200903");
  } else if ((command == 'M')) {
    Serial.read(); // X
    Serial.read(); // P
    Serial.write('7');
#endif    
  }
}
