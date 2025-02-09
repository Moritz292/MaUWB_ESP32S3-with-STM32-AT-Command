#ifndef UWB_H
#define UWB_H

#include <Arduino.h>
#include "config.h"

void setupUWBHardware();
void configureUWB();
void startUWBRanging();
void stopUWBRanging();
void setUWBToMode(String mode);
void putToSleep();
float getUWBDistance();
String sendData(String command, const int timeout, boolean debug);
String config_cmd();
String cap_cmd();

#endif