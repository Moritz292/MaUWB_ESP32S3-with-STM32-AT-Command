#ifndef CONFIG_H
#define CONFIG_H

// User config
#define TAG
//#define ANCHOR

// BLE definitions
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SECRET_KEY          "MySecretKey123"

// Motor control pins
#define IN1 1    // Motor control pin 1
#define IN2 2    // Motor control pin 2
#define LOCK_OPEN 5 // switch when pressed lock open
#define REQUEST_OPENING 6 // switch when pressed request opening
#define EEP 42   // Motor enable pin
#define BAT_PIN 4 // Adjust the pin number as per your hardware setup

// PWMC configuration
#define PWMC_FREQUENCY       1000
#define PWMC_RESOLUTION      8

// UWB definitions
#define UWB_INDEX 0
#define FREQ_850K
#define UWB_TAG_COUNT 5

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 39
#define I2C_SCL 38

// UWB Serial configuration
#define SERIAL_LOG Serial
#define SERIAL_AT Serial2
#define IO_RXD2 18
#define IO_TXD2 17
#define RESET 16

// Unlock distance threshold (in meters)
#define UNLOCK_DISTANCE 2.0

// BLE Time in milliseconds before attempting to reconnect
#define RECONNECT_INTERVAL 5000  


#endif