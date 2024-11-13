#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

// Motor control pins
#define IN1 1    // Motor control pin 1
#define IN2 2    // Motor control pin 2
#define EEP 42   // Motor enable pin

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 39
#define I2C_SCL 38

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void updateDisplay(String message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(message);
  display.display();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Minimal Motor Control with Display");

  // Initialize I2C and OLED display
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Halt if display initialization fails
  }

  // Set motor control pins as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EEP, OUTPUT);

  // Enable motor by setting EEP HIGH
  digitalWrite(EEP, HIGH);
  
  // Initial display message
  updateDisplay("Motor Control Ready");
}

void loop() {
  // Rotate motor in one direction
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  Serial.println("Motor rotating in one direction");
  updateDisplay("Motor rotating\none direction");
  delay(2000); // Rotate for 2 seconds

  // Stop motor
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  Serial.println("Motor stopped");
  updateDisplay("Motor stopped");
  delay(1000); // Stop for 1 second

  // Rotate motor in the opposite direction
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  Serial.println("Motor rotating in opposite direction");
  updateDisplay("Motor rotating\nopposite direction");
  delay(2000); // Rotate for 2 seconds

  // Stop motor
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  Serial.println("Motor stopped");
  updateDisplay("Motor stopped");
  delay(1000); // Stop for 1 second
}
