// USB_Dewpoint v1 and v2 simulator for Arduino
//
// Copyright (c) 2019 Rumen G. Bogdanovski
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

#define V2

#define READINGS_UPDATE 10
int epoch = 0;

float temp_ch1 = 27.68;
float temp_ch2 = 27.11;
float temp_amb = 28.05;
float rh = 61.73;
byte  output_ch1 = 0;
byte  output_ch2 = 0;
byte  output_ch3 = 0;
byte  cal_ch1 = 0;
byte  cal_ch2 = 0;
byte  cal_amb = 0;
byte  threshold_ch1 = 2;
byte  threshold_ch2 = 2;
bool  auto_mode = 0;
bool  ch2_3_linked = 0;
byte  aggressivity = 4;

float dewpoint(float Tamb, float RH) {
  return Tamb - (100 - RH) / 5.0;
}

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(2000);
  while (!Serial)
    ;
  randomSeed(analogRead(0));
}

void loop() {
  // update readings if needed
  if (0 == (epoch++ % READINGS_UPDATE)) {
    temp_ch1 += random(-50,50)/100.0;
    if (temp_ch1 < -40) temp_ch1 = -40;
    if (temp_ch1 > 50) temp_ch1 = 50;

    temp_ch2 += random(-50,50)/100.0;
    if (temp_ch2 < -40) temp_ch2 = -40;
    if (temp_ch2 > 50) temp_ch2 = 50;

    temp_amb += random(-50,50)/100.0;
    if (temp_amb < -40) temp_amb = -40;
    if (temp_amb > 50) temp_amb = 50;

    rh += random(-50,50)/100.0;
    if (rh < 0) rh = 0;
    if (rh > 100) rh = 100;
  }

  float dew_point = dewpoint(temp_amb, rh);
  /* In automatic mode the target of USB_Dewpoint is to keep over the night :
      - "Temp1 + Calibration1 > Dewpoint + Threshold1"
      - "Temp2 + Calibration2 > Dewpoint + Threshold2"
     To do so, “Temp1” and “Output1” are linked, “Temp2” and “Output2” are linked.
  */
  if (auto_mode) {
    if ((temp_ch1 + cal_ch1) < (dew_point + threshold_ch1)) {
      output_ch1 = 88;
    } else {
      output_ch1 = 0;
    }
    if ((temp_ch2 + cal_ch2) < (dew_point + threshold_ch2)) {
      output_ch2 = 88;
    } else {
      output_ch2 = 0;
    }
    if (ch2_3_linked) {
      output_ch3 = output_ch2;
    }
  }

  char cmd[7];
  Serial.readBytes(cmd, 6);
  cmd[6] = 0;
  char response[80];
  String command = String(cmd);
  //========================================================== SWHOIS
  if (command.equals("SWHOIS")) {
    #ifdef V1
    Serial.println("UDP");
    #else
    Serial.println("UDP2(1446)");
    #endif
  //========================================================== SEERAZ
  } else if (command.equals("SEERAZ")) {
    output_ch1 = 0;
    output_ch2 = 0;
    output_ch3 = 0;
    cal_ch1 = 0;
    cal_ch2 = 0;
    cal_amb = 0;
    threshold_ch1 = 2;
    threshold_ch2 = 2;
    auto_mode = 0;
    ch2_3_linked = 0;
    aggressivity = 4;
    Serial.println("EEPROM RESET");
  //========================================================== SGETAL
  } else if (command.equals("SGETAL")) {
  #ifdef V1
    // Tloc=26.37-Tamb=26.65-RH=35.73-DP=-19.55-TH=00-C=1201
    // Tloc=27.68-Tamb=28.05-RH=34.04-DP=-19.37-TH=02-C=1201
    Serial.print("Tloc=");
    Serial.print(temp_ch1, 2);
    Serial.print("-Tamb=");
    Serial.print(temp_amb, 2);
    Serial.print("-RH=");
    Serial.print(rh, 2);
    Serial.print("-DP=");
    Serial.print(dewpoint(temp_amb, rh), 2);
    Serial.println("-TH=00-C=1201");
  #else
    // ##22.37/22.62/23.35/50.77/12.55/0/0/0/0/0/0/2/2/0/0/4**
    // Fields are in order:
    // temperature ch 1
    // temperature ch 2
    // temperature ambient
    // relative humidity
    // dew point
    // output ch 1
    // output ch 2
    // output ch 3
    // calibration ch 1
    // calibration ch 2
    // calibration ambient
    // threshold ch 1
    // threshold ch 2
    // auto mode
    // outputs ch 2 & 3 linked
    // aggressivity
    Serial.print("##");
    Serial.print(temp_ch1, 2);
    Serial.print("/");
    Serial.print(temp_ch2, 2);
    Serial.print("/");
    Serial.print(temp_amb, 2);
    Serial.print("/");
    Serial.print(rh, 2);
    Serial.print("/");
    Serial.print(dewpoint(temp_amb, rh), 2);
    Serial.print("/");
    Serial.print(output_ch1);
    Serial.print("/");
    Serial.print(output_ch2);
    Serial.print("/");
    Serial.print(output_ch3);
    Serial.print("/");
    Serial.print(cal_ch1);
    Serial.print("/");
    Serial.print(cal_ch2);
    Serial.print("/");
    Serial.print(cal_amb);
    Serial.print("/");
    Serial.print(threshold_ch1);
    Serial.print("/");
    Serial.print(threshold_ch2);
    Serial.print("/");
    Serial.print(auto_mode);
    Serial.print("/");
    Serial.print(ch2_3_linked);
    Serial.print("/");
    Serial.print(aggressivity);
    Serial.println("**");
  #endif

  #ifdef V2
  //========================================================== SCA
	} else if (command.startsWith("SCA")) {
    cal_ch1 = command.substring(3, 4).toInt();
		cal_ch2 = command.substring(4, 5).toInt();
    cal_amb = command.substring(5, 6).toInt();
    Serial.println("DONE");
  //========================================================== STHR
  } else if (command.startsWith("STHR")) {
    threshold_ch1 = command.substring(4, 5).toInt();
    threshold_ch2 = command.substring(5, 6).toInt();
    Serial.println("DONE");
  //========================================================== SLINK
  } else if (command.startsWith("SLINK")) {
    ch2_3_linked = command.charAt(5) == '1';
    Serial.println("DONE");
  //========================================================== SAUTO
  } else if (command.startsWith("SAUTO")) {
    auto_mode = command.charAt(5) == '1';
    Serial.println("DONE");
  //========================================================== SAGGR
  } else if (command.startsWith("SAGGR")) {
    aggressivity = command.substring(5, 6).toInt();
    Serial.println("DONE");
  //========================================================== S1O
  } else if (command.startsWith("S1O")) {
    output_ch1 = command.substring(3, 6).toInt();
    Serial.println("DONE");
  //========================================================== S2O
  } else if (command.startsWith("S2O")) {
    output_ch2 = command.substring(3, 6).toInt();
    if (ch2_3_linked) output_ch3 = output_ch2;
    Serial.println("DONE");
  //========================================================== S3O
  } else if (command.startsWith("S3O")) {
    output_ch3 = command.substring(3, 6).toInt();
    if (ch2_3_linked) output_ch2 = output_ch3;
    Serial.println("DONE");
  #endif
  } else if (command.startsWith("S")){
    // Do nothing... no repsonse (mimics the real device);
  } else {
    Serial.println("Unknown command");
  }
}
