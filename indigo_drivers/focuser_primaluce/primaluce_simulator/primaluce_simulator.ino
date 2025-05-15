// PrimaLuceLab SestoSenso2, Esatto, Arco and Giotto simulator for Arduino
//
// Copyright (c) 2023-2025 CloudMakers, s. r. o.
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

// This simulator needs https://arduinojson.org

#include <ArduinoJson.h>

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif


// Essato2
//#define STATE "{\"SN\":\"ESATTO20101\",\"MACADDR\":\"70:B3:D5:32:91:28\",\"MODNAME\":\"ESATTO2\",\"WIFI\":{\"CFG\":0,\"SSID\":\"ESATTO20101\",\"PWD\":\"primalucelab\"},\"SWVERS\":{\"SWAPP\":\"1.2\",\"SWWEB\":\"1.2\"},\"EXT_T\":\"-127.00\",\"VIN_12V\":\"14.04\",\"VIN_USB\":\"5.30\",\"PRESET_1\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_2\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_3\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_4\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_5\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_6\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_7\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_8\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_9\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_10\":{\"NAME\":\"\",\"M1POS\":0},\"DIMLED_1\":200,\"DIMLED_2\":200,\"DIMLED_3\":200,\"MOT1\":{\"ABS_POS\":191263,\"SPEED\":0,\"STATUS\":{\"HIZ\":1,\"UVLO\":0,\"TH_SD\":0,\"TH_WRN\":0,\"OCD\":0,\"WCMD\":0,\"NOPCMD\":0,\"BUSY\":0,\"DIR\":0,\"MST\":\"stop\"},\"CAL_MAXPOS\":439000,\"CAL_MINPOS\":0,\"CAL_STATUS\":\"stop\",\"HSENDET\":0,\"NTC_T\":\"16.92\",\"LOCKMOV\":1},\"DIMLEDS\":\"on\"}"

// SestoSenso 2
#define STATE "{\"SWID\":\"001\",\"MODNAME\":\"SESTOSENSO2\",\"SN\":\"SESTOSENSO20716\",\"HOSTNAME\":\"SESTOSENSO20716\",\"MACADDR\":\"70:B3:D5:32:95:2F\",\"SWVERS\":{\"SWAPP\":\"1.3\",\"SWWEB\":\"1.3\"},\"LANCFG\":\"ap\",\"LANSTATUS\":{\"LANCONN\":\"disconnected\",\"PINGGW\":0,\"PINGEXTHOST\":0},\"WIFIAP\":{\"SSID\":\"SESTOSENSO20716\",\"PWD\":\"primalucelab\",\"IP\":\"192.168.4.1\",\"NM\":\"255.255.255.0\",\"CHL\":2,\"MACADDR\":\"70:B3:D5:32:95:30\",\"STATUS\":{\"HIZ\":0,\"UVLO\":0,\"TH_SD\":0,\"TH_WRN\":0,\"OCD\":0,\"WCMD\":0,\"NOPCMD\":0,\"BUSY\":0,\"DIR\":\"In\",\"MST\":\"stop\"}},\"WIFISTA\":{\"SSID\":\"MySSID\",\"PWD\":\"MyPassword\",\"IP\":\"\",\"NM\":\"\",\"GW\":\"\",\"DNS\":\"\",\"MACADDR\":\"70:B3:D5:32:95:2F\"},\"EXT_T\":\"-127.00\",\"VIN_12V\":\"13.98\",\"ARCO\":1,\"LEDBLINKSTATUS\":\"PowerInLess11Volts\",\"PRESET_1\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_2\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_3\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_4\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_5\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_6\":{\"NAME\":\"\",\"M1POS\":0},\"PRESET_7\":{\"NAME\":\"\",\"M1POS\":0,\"M2POS\":0},\"PRESET_8\":{\"NAME\":\"\",\"M1POS\":0,\"M2POS\":0},\"PRESET_9\":{\"NAME\":\"\",\"M1POS\":0,\"M2POS\":0},\"PRESET_10\":{\"NAME\":\"\",\"M2POS\":0},\"PRESET_11\":{\"NAME\":\"\",\"M2POS\":0},\"PRESET_12\":{\"NAME\":\"\",\"M2POS\":0},\"DIMLED_1\":200,\"DIMLED_2\":200,\"DIMLED_3\":200,\"MOT1\":{\"ABS_POS\":18075,\"EL_POS\":0,\"MARK\":0,\"SPEED\":0,\"ACC\":5,\"DEC\":5,\"MAX_SPEED\":4,\"MIN_SPEED\":0,\"TVAL_HOLD\":2,\"TVAL_RUN\":20,\"TVAL_ACC\":20,\"TVAL_DEC\":20,\"T_FAST\":25,\"TON_MIN\":41,\"TOFF_MIN\":41,\"FS_SPD\":1023,\"OCD_TH\":2,\"STEP_MODE\":4,\"CONFIG\":11904,\"ALARM\":255,\"STATUS\":{\"HIZ\":1,\"UVLO\":0,\"TH_SD\":0,\"TH_WRN\":0,\"OCD\":0,\"WCMD\":0,\"NOPCMD\":0,\"BUSY\":0,\"DIR\":\"Out\",\"MST\":\"stop\"},\"NTC_T\":\"37.12\",\"CAL_MAXPOS\":17639,\"CAL_BKLASH\":0,\"CAL_MINPOS\":0,\"CAL_HOMEP\":1000,\"CAL_NUMOF\":0,\"CAL_DIR\":\"normal\",\"LASTDIR\":0,\"LOCKMOV\":1,\"HOLDCURR_STATUS\":1,\"FnRUN_ACC\":1,\"FnRUN_DEC\":1,\"FnRUN_SPD\":2,\"FnRUN_CURR_ACC\":7,\"FnRUN_CURR_DEC\":7,\"FnRUN_CURR_SPD\":7,\"FnRUN_CURR_HOLD\":3},\"RUNPRESET_L\":{\"RP_NAME\":\"light\",\"M1ACC\":10,\"M1DEC\":10,\"M1SPD\":10,\"M1CACC\":3,\"M1CDEC\":3,\"M1CSPD\":3,\"M1HOLD\":1},\"RUNPRESET_M\":{\"RP_NAME\":\"medium\",\"M1ACC\":6,\"M1DEC\":6,\"M1SPD\":6,\"M1CACC\":5,\"M1CDEC\":5,\"M1CSPD\":5,\"M1HOLD\":1},\"RUNPRESET_S\":{\"RP_NAME\":\"slow\",\"M1ACC\":1,\"M1DEC\":1,\"M1SPD\":2,\"M1CACC\":7,\"M1CDEC\":7,\"M1CSPD\":7,\"M1HOLD\":1},\"RUNPRESET_1\":{\"RP_NAME\":\"Esprit\",\"M1ACC\":1,\"M1DEC\":1,\"M1SPD\":2,\"M1CACC\":7,\"M1CDEC\":7,\"M1CSPD\":7,\"M1HOLD\":3},\"RUNPRESET_2\":{\"RP_NAME\":\"User 2\",\"M1ACC\":6,\"M1DEC\":6,\"M1SPD\":6,\"M1CACC\":5,\"M1CDEC\":5,\"M1CSPD\":5,\"M1HOLD\":1},\"RUNPRESET_3\":{\"RP_NAME\":\"User 3\",\"M1ACC\":1,\"M1DEC\":1,\"M1SPD\":2,\"M1CACC\":7,\"M1CDEC\":7,\"M1CSPD\":7,\"M1HOLD\":1},\"MOT2\":{\"ABS_POS\":0,\"EL_POS\":0,\"MARK\":0,\"SPEED\":0,\"ACC\":35,\"DEC\":35,\"MAX_SPEED\":30,\"MIN_SPEED\":0,\"TVAL_HOLD\":1,\"TVAL_RUN\":18,\"TVAL_ACC\":18,\"TVAL_DEC\":18,\"T_FAST\":25,\"TON_MIN\":41,\"TOFF_MIN\":41,\"FS_SPD\":1023,\"OCD_TH\":2,\"STEP_MODE\":4,\"CONFIG\":11904,\"ALARM\":255,\"STATUS\":{\"HIZ\":1,\"UVLO\":0,\"TH_SD\":0,\"TH_WRN\":0,\"OCD\":0,\"WCMD\":0,\"NOPCMD\":0,\"BUSY\":0,\"DIR\":\"In\",\"MST\":\"stop\"},\"CAL_MAXPOS\":0,\"HSENDET\":1,\"HEMISPHERE\":\"northern\",\"COMPENSATION_DEGS\":0.00000,\"CAL_STATUS\":\"stop\",\"CAL_MINPOS\":0,\"CAL_HOMEP\":1000,\"CAL_NUMOF\":0,\"CAL_DIR\":\"normal\",\"LASTDIR\":0,\"LOCKMOV\":1,\"HOLDCURR_STATUS\":0},\"DIMLEDS\":\"on\",\"LOGLEVEL\":\"no output\"}"

StaticJsonDocument<200> request;
StaticJsonDocument<5000> state;
StaticJsonDocument<5000> response;

char json[256];

void setup() {

  deserializeJson(state, STATE);
//
//  state["SN"] = "ESATTO20101";
//  state["MOT1"]["ABS_POS"] = 191263;
//  state["MOT1"]["SPEED"] = 0;
//  state["MOT1"]["STATUS"]["CAL_STATUS"] = "stop";
  
  Serial.begin(115200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  if (Serial.available()) {
    Serial.readBytesUntil('\n', json, sizeof(json));
    DeserializationError error = deserializeJson(request, json);
    if (error) {
      Serial.println("\"Error: syntax error\"");
      return;
    }
    JsonObject req = request["req"];
    if (!req) {
      Serial.println("\"Error: no req\"");
      return;
    }
    response.clear();
    JsonObject res = response.createNestedObject("res");
    for (JsonPair reqPair : req) {
      JsonString reqKey = reqPair.key();
      JsonObject reqValue = reqPair.value().as<JsonObject>();
      if (!strcmp(reqKey.c_str(), "get")) {
        if (reqValue.isNull()) {
          res["get"] = state;
        } else {
          for (JsonPair getPair : reqValue) {
            const char *key1 = getPair.key().c_str();
            JsonObject value1 = getPair.value().as<JsonObject>();
            if (value1.isNull()) {
              JsonVariant value = state[key1];
              if (!value.isNull()) {
                res["get"][key1] = state[key1];
              } else {
                Serial.print("\"Error: no ");
                Serial.print(key1);
                Serial.println("\"");
                return;
              }
            } else {
              for (JsonPair pair1 : value1) {
                const char *key2 = pair1.key().c_str();
                JsonObject value2 = pair1.value().as<JsonObject>();
                if (value2.isNull()) {
                  JsonVariant value = state[key1][key2];
                  if (!value.isNull()) {
                    res["get"][key1][key2] = value;
                  } else {
                    Serial.print("\"Error: no ");
                    Serial.print(key1);
                    Serial.print("->");
                    Serial.print(key2);
                    Serial.println("\"");
                    return;
                  }                
                }
              }       
            }
          }       
        } 
      } else if (!strcmp(reqKey.c_str(), "set")) {
        for (JsonPair setPair : reqValue) {
          const char *key1 = setPair.key().c_str();
          JsonVariant value1 = setPair.value().as<JsonVariant>();
          if (value1.isNull()) {
            Serial.print("\"Error: no value for ");
            Serial.print(key1);
            Serial.println("\"");
            return;
          }
          if (value1.is<JsonObject>()) {
            for (JsonPair pair1 : value1.as<JsonObject>()) {
              const char *key2 = pair1.key().c_str();
              JsonVariant value2 = pair1.value().as<JsonVariant>();
              if (value2.isNull()) {
                Serial.print("\"Error: no value for ");
                Serial.print(key1);
                Serial.print("->");
                Serial.print(key2);
                Serial.println("\"");
                return;
              }
              state[key1][key2] = value2;
              res["set"][key1][key2] = "done";
            }       
          } else {
            state[key1] = value1;
            res["set"][key1] = "done";
          }
        }       
      } else if (!strcmp(reqKey.c_str(), "cmd")) {
				JsonVariant value = reqValue["MOT1"]["MOVE_ABS"]["STEP"];
				if (value.is<int>()) {
					state["MOT1"]["ABS_POS"] = value.as<int>();
					res["cmd"]["MOT1"]["MOVE_ABS"]["STEP"] = "done";
					serializeJson(response, Serial);
					Serial.println();
					return;
				}
				value = reqValue["MOT1"]["MOT_STOP"];
				if (value.is<String>()) {
					res["cmd"]["MOT1"]["MOT_STOP"] = "done";
					serializeJson(response, Serial);
					Serial.println();
					return;
				}
				Serial.println("\"Error: invalid cmd\"");
				return;
      } else {
        Serial.println("\"Error: no get, set or cmd\"");
        return;
      }
    }
    serializeJson(response, Serial);
    Serial.println();
  }
}
