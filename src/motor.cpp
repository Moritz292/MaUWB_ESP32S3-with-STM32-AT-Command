#include "motor.h"
#include "lock_state.h"

void initializeMotor() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EEP, OUTPUT);
  pinMode(LOCK_OPEN, INPUT_PULLUP);
  pinMode(REQUEST_OPENING, INPUT_PULLUP);
  
  if (!ledcAttach(EEP, PWMC_FREQUENCY, PWMC_RESOLUTION)) {
    Serial.println(F("LEDC configuration failed"));
    for (;;);
  }
}

void motor(String direction) {
    auto& lockState = LockState::getInstance();
    
    // Update physical state
    bool isPhysicallyOpen = !digitalRead(LOCK_OPEN);
    lockState.updatePhysicalState(isPhysicallyOpen);

    int fadeTime = (direction == "OPEN") ? 600 : 400;
    int fadeSteps = 50;
    int delayTime = fadeTime / fadeSteps;

    if (direction == "OPEN") {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
    } else if (direction == "CLOSE") {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
    } else {
        Serial.println("Invalid direction");
        return;
    }

    for (int duty = 0; duty <= 255; duty += (255 / fadeSteps)) {
        analogWrite(EEP, duty);
        delay(delayTime);
    }


    analogWrite(EEP, 0);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    Serial.println("Motor stopped after fade");
    updateDisplay("Motor stopped");

    // Update state after motor operation and show effect
    if (direction == "CLOSE") {
        showFlashEffect(false);  // Show unlocking effect
    } 
}
