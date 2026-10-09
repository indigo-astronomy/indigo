// Interactive Astronomy SkyAlert simulator for Arduino
//
// Copyright (c) 2021-2026 CloudMakers, s. r. o.
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

// The driver talks to the SkyAlert at 115200 baud.
#define BAUD_RATE 115200

// The temperature and the sky brightness advance per record, as in the C simulator, so a client can
// tell a new reading from a repeated one.
int samples = 0;

void setup() {
  Serial.begin(BAUD_RATE);
  Serial.setTimeout(500);
  while (!Serial)
    ;
}

// The record answering "send": "Data", temperature [C], sky temperature [C], dampness [raw], sky
// brightness [raw], humidity [%], wind speed [raw], power [1 = ok, 0 = failure], firmware version and
// pressure [Pa], each line terminated by CR.
void loop() {
  String command = Serial.readStringUntil('\r');
  command.trim();
  if (command.equals("send")) {
    Serial.print("Data\r");
    Serial.print(20.3 + 0.1 * samples, 1);
    Serial.print("\r");
    Serial.print("1\r");
    Serial.print("1008\r");
    Serial.print(751 + samples);
    Serial.print("\r");
    Serial.print("66.3\r");
    Serial.print("415\r");
    Serial.print("1\r");
    Serial.print("1.8m\r");
    Serial.print("101791.83\r");
    samples++;
  }
}
