// Pegasus pocket powerbox simulator for Arduino
//
// Copyright (c) 2019-2026 CloudMakers, s. r. o.
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

// Select the simulated model, the default is the original Pocket Powerbox (PPB).
//#define PPBA
//#define PPBM
//#define SPB

#if defined(PPBA) || defined(PPBM)
#define ADVANCE
#endif

bool power_1234 = true;
byte power_5 = 0;
byte power_6 = 0;
bool power_dslr = true;
bool autodev = true;
// The power-on outlet mask PE stores; it does not switch the outlets now, a reboot applies it.
String boot_mask = "1111";
#ifdef ADVANCE
bool power_alert = 0;
int power_adj = 5;
int dew_aggr = 210;
#endif

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  String command = Serial.readStringUntil('\n');
  command.trim();
  if (command.length() == 0) {
    return;
  }
  if (command.equals("P#")) {
#if defined(PPBA)
    Serial.println("PPBA_OK");
#elif defined(PPBM)
    Serial.println("PPBM_OK");
#elif defined(SPB)
    Serial.println("SPB");
#else
    Serial.println("PPB_OK");
#endif
  } else if (command.startsWith("PE:")) {
    // Only the power-on state is stored, the outlets keep their current state.
    boot_mask = command.substring(3);
    Serial.println("PE:1");
  } else if (command.startsWith("P1:")) {
    power_1234 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("P2:")) {
    // 0 and 1 switch the output; on the Advance and Micro 3, 5, 8, 9 and 12 only select the
    // voltage and leave the output as it is.
    int value = command.substring(3).toInt();
    if (value == 0) {
      power_dslr = false;
    } else if (value == 1) {
      power_dslr = true;
#ifdef ADVANCE
    } else {
      power_adj = value;
#endif
    }
    Serial.println(command);
  } else if (command.startsWith("P3:") || command.startsWith("P4:")) {
    // The PPB names the dew outputs by their power port in the reply: P3 answers P5:nnn and
    // P4 answers P6:nnn. The Advance and Micro echo the command number.
    int channel = command.charAt(1) - '3';
    int value = command.substring(3).toInt();
    if (channel == 0) {
      power_5 = value;
    } else {
      power_6 = value;
    }
#ifdef ADVANCE
    Serial.print(channel == 0 ? "P3:" : "P4:");
#else
    Serial.print(channel == 0 ? "P5:" : "P6:");
#endif
    Serial.println(value);
  } else if (command.equals("PF")) {
    // Reboot, no answer. The outlets come back in the power-on state PE stored, of which the
    // firmware uses only the last two digits: the 12 V outputs, then the DSLR output.
    if (boot_mask.length() == 4) {
      power_1234 = boot_mask.charAt(2) == '1';
      power_dslr = boot_mask.charAt(3) == '1';
    }
  } else if (command.equals("PA")) {
#if defined(PPBA)
    Serial.print("PPBA:12.2:");
#elif defined(PPBM)
    Serial.print("PPBM:12.2:");
#else
    Serial.print("PPB:12.2:");
#endif
    // The current is in 1/65 A, 1 A with any load switched on.
    Serial.print((power_1234 || power_dslr || power_5 || power_6) ? 65 : 0);
    Serial.print(".0:23.2:59:14.7:");
    Serial.print(power_1234 ? '1' : '0');
    Serial.print(':');
#ifdef SPB
    // The Saddle box has no DSLR output.
    Serial.print('0');
#else
    Serial.print(power_dslr ? '1' : '0');
#endif
    Serial.print(':');
    Serial.print(power_5);
    Serial.print(':');
    Serial.print(power_6);
    Serial.print(':');
#ifdef ADVANCE
    Serial.print(autodev ? '1' : '0');
    Serial.print(':');
    Serial.print(power_alert ? '1' : '0');
    Serial.print(':');
    Serial.println(power_adj);
#else
    Serial.println(autodev ? '1' : '0');
#endif
  } else if (command.startsWith("PL:")) {
    Serial.println(command);
  } else if (command.equals("PV")) {
    Serial.println("1.5");
#ifdef ADVANCE
  } else if (command.startsWith("DA:")) {
    dew_aggr = command.substring(3).toInt();
    Serial.println(command);
  } else if (command.equals("PD:99")) {
    Serial.print("PD:");
    Serial.println(dew_aggr);
  } else if (command.startsWith("PD:")) {
    // The Advance answers with the dew aggressiveness instead of an echo.
    autodev = command.charAt(3) == '1';
    Serial.print("PD:");
    Serial.println(dew_aggr);
#else
  } else if (command.startsWith("PD:")) {
    autodev = command.charAt(3) == '1';
    Serial.println(command);
#endif
  }
}
