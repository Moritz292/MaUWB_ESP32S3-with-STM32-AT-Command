#include "uwb.h"

void setupUWBHardware() {
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);  SERIAL_AT.begin(115200, SERIAL_8N1, IO_RXD2, IO_TXD2);
}

void configureUWB() {
  sendData("AT?", 2000, 1);
  sendData("AT+RESTORE", 5000, 1);
  sendData(config_cmd(), 2000, 1);
  sendData(cap_cmd(), 2000, 1);
  sendData("AT+SETRPT=1", 2000, 1);
  sendData("AT+SAVE", 2000, 1);
  sendData("AT+RESTART", 2000, 1);
}

void startUWBRanging() {
  sendData("AT+RANGE=1", 2000, 1);
}

void stopUWBRanging() {
  sendData("AT+RANGE=0", 2000, 1);
}

float getUWBDistance() {
  String response = "";
  if (SERIAL_AT.available()) {
    while (SERIAL_AT.available()) {
      char c = SERIAL_AT.read();
      if (c != '\r') {
        response += c;
      }
    }
  }
  
  int startIndex = response.indexOf("(") + 1;
  int endIndex = response.indexOf(")");
  if (startIndex > 0 && endIndex > startIndex) {
    String distanceStr = response.substring(startIndex, endIndex);
    return distanceStr.toFloat() / 100.0;
  }
  return -1;
}

String sendData(String command, const int timeout, boolean debug) {
    String response = "";
    SERIAL_LOG.println(command);
    SERIAL_AT.println(command);

    long int time = millis();
    while ((time + timeout) > millis()) {
        while (SERIAL_AT.available()) {
            char c = SERIAL_AT.read();
            response += c;
        }
    }

    if (debug) {
        SERIAL_LOG.println(response);
    }
    return response;
}

void setUWBToMode(String mode) {
    String setCfgCommand = "AT+SETCFG=" + String(UWB_INDEX) + ",0,";

    if (mode == "ANCHOR") {
        setCfgCommand += "1,"; // Set device to ANCHOR mode
    } else if (mode == "TAG") {
       setCfgCommand += "0,"; // Set device to TAG mode
    } else {
        // Handle invalid modes (e.g., log an error, use a default)
        // I'm choosing to return here to abort if invalid
        return;
    }

    #ifdef FREQ_850K
      setCfgCommand += "0,";
    #endif
    #ifdef FREQ_6800K
      setCfgCommand += "1,";
    #endif

    setCfgCommand += "1"; // last parameter

    sendData(setCfgCommand, 2000, 1);
    sendData("AT+SAVE", 2000, 1);
    sendData("AT+RESTART", 2000, 1);
}

void putToSleep() {
    sendData("AT+SLEEP=65535", 2000, 1); // Set device to deep sleep
}

String config_cmd() {
    String temp = "AT+SETCFG=";
    temp = temp + UWB_INDEX;
    
    #ifdef TAG
    temp = temp + ",0";
    #endif
    #ifdef ANCHOR
    temp = temp + ",1";
    #endif

    #ifdef FREQ_850K
    temp = temp + ",0";
    #endif
    #ifdef FREQ_6800K
    temp = temp + ",1";
    #endif

    temp = temp + ",1";
    return temp;
}

String cap_cmd() {
    String temp = "AT+SETCAP=";
    temp = temp + UWB_TAG_COUNT;

    #ifdef FREQ_850K
    temp = temp + ",15";
    #endif
    #ifdef FREQ_6800K
    temp = temp + ",10";
    #endif

    return temp;
}