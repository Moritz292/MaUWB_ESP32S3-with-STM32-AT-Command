#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include "config.h"
#include "display.h"

void initializeMotor();
void motor(String direction);
bool isButtonPressed(int buttonPin);

#endif