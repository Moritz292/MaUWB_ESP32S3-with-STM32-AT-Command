#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <driver/ledc.h>

// Motor control pins
#define IN1 1    // Motor control pin 1
#define IN2 2    // Motor control pin 2
#define EEP 42   // Motor enable pin (PWM)

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 39
#define I2C_SCL 38

// LEDC configuration
#define LEDC_CHANNEL         0
#define LEDC_FREQUENCY       1000
#define LEDC_RESOLUTION      8

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void updateDisplay(String message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(message);
  display.display();
}

void fadeMotor(bool direction) {
  // Fade in from 0 to 100% over 0.5 seconds
  int fadeTime = 500;                // total fade time in milliseconds
  int fadeSteps = 50;                // number of steps for fading
  int delayTime = fadeTime / fadeSteps;  // delay time between steps

  // Set the motor direction
  if (direction == HIGH) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }

  // Ramp up the PWM duty cycle from 0 to 255
  for (int duty = 0; duty <= 255; duty += (255 / fadeSteps)) {
    analogWrite(EEP, duty);          // Set PWM duty cycle
    delay(delayTime);                // Short delay between increments
  }

  // Maintain full speed for 1 second
  analogWrite(EEP, 255);             // 100% duty cycle
  delay(1000);

  // Stop motor after fade
  analogWrite(EEP, 0);               // Stop PWM signal
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  Serial.println("Motor stopped after fade");
  updateDisplay("Motor stopped");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Motor Control with PWM and Display");

  // Initialize I2C and OLED display
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // Set motor control pins as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // Configure LEDC for PWM using the new ledcAttachChannel function
  if (!ledcAttach(EEP, LEDC_FREQUENCY, LEDC_RESOLUTION)) {
    Serial.println(F("LEDC configuration failed"));
    for (;;);
  }

  // Initial display message
  updateDisplay("Motor Control Ready");
}

void loop() {
  // Rotate motor at 50% speed in one direction
  Serial.println("Fading motor speed up in one direction");
  updateDisplay("Fading motor speed up in one direction");
  fadeMotor(HIGH);   // Call fade function with direction HIGH
  delay(2000);       // Delay before next fade

  Serial.println("Fading motor speed up in opposite direction");
  updateDisplay("Fading motor speed up in opposite direction");
  fadeMotor(LOW);    // Call fade function with direction LOW
  delay(2000);       // Delay before repeating fade
}
