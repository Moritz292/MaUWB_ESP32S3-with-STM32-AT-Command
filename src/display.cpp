#include "display.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void initializeDisplay() {
  Wire.begin(I2C_SDA, I2C_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
}

void updateDisplay(String message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  #ifdef TAG
  display.println("TAG MODE");
  #endif
  #ifdef ANCHOR
  display.println("ANCHOR MODE");
  #endif
  display.println(message);
  display.display();
}
