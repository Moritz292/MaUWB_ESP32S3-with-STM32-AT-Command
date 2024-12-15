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

void showFlashEffect(bool isLocking) {
  // First flash the screen
  display.clearDisplay();
  display.fillScreen(WHITE);
  display.display();
  delay(100);
  
  // Show the text
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  #ifdef TAG
  const char* text = isLocking ? "<< >>" : "IN RANGE";
  #else
  const char* text = isLocking ? "LOCKED" : "UNLOCKED";
  #endif
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, (display.height() - h) / 2);
  display.print(text);
  display.display();
  delay(1000);

  // CRT power-off effect
  const int steps = 5;
  const int middle = display.height() / 2;
  
  for (int i = 0; i < steps; i++) {
    display.clearDisplay();
    // Calculate the height of the shrinking effect
    int height = display.height() * (1.0 - (float)i / steps);
    int start_y = middle - (height / 2);
    
    // Draw a white rectangle that gets smaller
    int rectHeight = max(1, height);  // Ensure minimum height of 1 pixel
    display.fillRect(0, start_y, display.width(), rectHeight, WHITE);
    
    // Add slight curve to the edges
    int curve = (i * 8) / steps;
    for (int x = 0; x < curve; x++) {
      int y_offset = (x * x) / (curve * 2);
      // Draw black pixels to create curved edges
      if (start_y + y_offset < display.height())
        display.drawPixel(x, start_y + y_offset, BLACK);
      if (start_y + rectHeight - y_offset >= 0)
        display.drawPixel(x, start_y + rectHeight - y_offset, BLACK);
      if (start_y + y_offset < display.height())
        display.drawPixel(display.width() - 1 - x, start_y + y_offset, BLACK);
      if (start_y + rectHeight - y_offset >= 0)
        display.drawPixel(display.width() - 1 - x, start_y + rectHeight - y_offset, BLACK);
    }
    
    display.display();
    delay(10); // Changed from 20 to 10 to make effect 2x faster
  }
  
  display.clearDisplay();
  display.display();
  delay(100);
}
