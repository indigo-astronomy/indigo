// SQM simulator for Arduino
//
// Copyright (c) 2019-2025 CloudMakers, s. r. o.
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

char led[6] = "A5,00";

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
   String command = Serial.readStringUntil('x');
  if (command.equals("i")) {
    Serial.println("i,00000002,00000003,00000001,00000413");
  } else if (command.equals("r")) {
    Serial.println("r, 20.70m,0000022921Hz,0000000020c,0000000.000s, 039.4C");
  } else if (command.equals("u")) {
    Serial.println("u, 20.70m,0000022921Hz,0000000020c,0000000.000s, 039.4C");
  } else if (command.equals("L5")) {
    Serial.println("L5,238");
  } else if (command.equals("L5")) {
    Serial.println("L5,238");
  } else if (command.equals("A50")) {
    led[3] = '0';
  } else if (command.equals("A51")) {
    led[3] = '1';
  } else if (command.equals("A5d")) {
    led[4] = '0';
  } else if (command.equals("A5e")) {
    led[4] = '1';
  } else if (command.equals("A5")) {
    Serial.println(led);
  }
}
