#include "motor.h"
#include "lock_state.h"

void initializeMotor() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EEP, OUTPUT);
  pinMode(LOCK_OPEN, INPUT_PULLUP);
  
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
    
    // If lock is already in desired position, do nothing
    if ((direction == "OPEN" && lockState.isOpen()) ||
        (direction == "CLOSE" && !lockState.isOpen())) {
        return;
    }

    // Check physical switch
    if (isPhysicallyOpen) {
        Serial.println("Lock is physically open");
        lockState.setPosition(LockPosition::OPEN);
        delay(500);
        return;
    }

    int fadeTime = 500;
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

    analogWrite(EEP, 255);
    delay(200); // time motor runs on full power

    analogWrite(EEP, 0);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    Serial.println("Motor stopped after fade");
    updateDisplay("Motor stopped");

    // Update state after motor operation
    if (direction == "OPEN") {
        lockState.setPosition(LockPosition::OPEN);
    } else if (direction == "CLOSE") {
        lockState.setPosition(LockPosition::CLOSED);
    }
}

bool isButtonPressed(int buttonPin) {
  static int lastState = HIGH;
  static unsigned long lastDebounceTime = 0;
  const int DEBOUNCE_DELAY = 50; // Adjust as needed

  int currentState = digitalRead(buttonPin);

  if (currentState != lastState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    lastState = currentState;
    return currentState == LOW;
  }

  return false;
}