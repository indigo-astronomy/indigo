// iOptron simulator for Arduino
//
// Copyright (c) 2018-2021 CloudMakers, s. r. o.
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

#define VERSION  "8406"

#include "common.h"

int tz =      1;
char lon[] =  "+048*15:55";
char lat[] =  "+17*33:35";
char date[] = "01/01/21";

char tracking_mode = '0';
char guide_rate = '2';

void parseCommand() {
  char command[32], buffer[32];
  if (Serial.read() == ':') {
    int length = Serial.readBytesUntil('#', command, sizeof(command));
    command[length] = 0;
    if (strcmp(command, "V") == 0) {
        Serial.write("V1.00#");
    } else if (strncmp(command, "SG", 2) == 0) {
      tz = atoi(command + 2);
      Serial.write('1');
    } else if (strncmp(command, "Sg", 2) == 0) {
      strncpy(lon, command + 2, sizeof(lon) - 1);
      Serial.write('1');
    } else if (strncmp(command, "St", 2) == 0) {
      strncpy(lat, command + 2, sizeof(lat) - 1);
      Serial.write('1');
    } else if (strncmp(command, "SL", 2) == 0) {
      base_time = parse60(command + 2, "%d:%d:%d") - millis() / 1000;
      Serial.write('1');
    } else if (strncmp(command, "SC", 2) == 0) {
      strncpy(date, command + 2, sizeof(date) - 1);
      Serial.write("                                #                                #");
    } else if (strncmp(command, "GG", 2) == 0) {
      sprintf(buffer, "%c%02d:00#", tz >=0 ? 'E' : 'W', abs(tz));
      Serial.write(buffer);
    } else if (strncmp(command, "Gg", 2) == 0) {
      Serial.write(lon);
      Serial.write('#');
    } else if (strncmp(command, "Gt", 2) == 0) {
      Serial.write(lat);
      Serial.write('#');
    } else if (strncmp(command, "GL", 2) == 0) {
      Serial.write(format60(base_time +  millis() / 1000, "%02d:%02d:%02d.0#"));
    } else if (strncmp(command, "GR", 2) == 0) {
      Serial.write(format60(current_ra / 15 / 1000, "%02d:%02d:%02d.0#"));
    } else if (strncmp(command, "GD", 2) == 0) {
      Serial.write(format60(current_dec / 1000, "%+03d*%02d:%02d"));
      Serial.write('#');
    } else if (strncmp(command, "GC", 2) == 0) {
      Serial.write(date);
      Serial.write('#');
    } else if (strncmp(command, "Sr", 2) == 0) {
      target_ra = parse60(command + 2, "%d:%d:%d") * 15 * 1000;
      Serial.write('1');
    } else if (strncmp(command, "Sd", 2) == 0) {
      target_dec = parse60(command + 2, "%d*%d:%d") * 1000;
      Serial.write('1');
    } else if (strncmp(command, "MS", 2) == 0) {
      slewing = true;
      Serial.write('0');
    } else if (strncmp(command, "Q", 1) == 0) {
      slewing = false;
    } else if (strncmp(command, "CMR", 3) == 0) {
      current_ra = target_ra;
      current_dec = target_dec;
      Serial.write("Coordinates     matched.        #");
    } else if (strncmp(command, "STR", 3) == 0) {
      tracking_mode = command[3];
      Serial.write('1');
    } else if (strncmp(command, "GTR", 3) == 0) {
      Serial.write(tracking_mode);
    } else if (strncmp(command, "SGS", 3) == 0) {
      guide_rate = command[3];
      Serial.write('1');
    } else if (strncmp(command, "GGS", 3) == 0) {
      Serial.write(guide_rate);
    } else if (strncmp(command, "SE?", 3) == 0) {
      Serial.write(slewing ? '1' : '0');
    } else if (strncmp(command, "GAM", 3) == 0) {
      Serial.write('2');
    }
  }
}
