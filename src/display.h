#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SSD1306.h>
#include "config.h"

extern Adafruit_SSD1306 display;

void updateDisplay(String message);
void initializeDisplay();
void showFlashEffect(bool isLocking);
float readBatteryVoltage();

#endif