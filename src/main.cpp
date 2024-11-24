#include "config.h"
#include "display.h"
#include "motor.h"
#include "uwb.h"
#include "ble.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE+UWB Lock!");

  initializeDisplay();
  updateDisplay("Initializing...");
  
  initializeMotor();
  initializeUWB();
  
  BLEDevice::init("ESP32_BLE_UWB_LOCK");
  
  #ifdef ANCHOR
  setupBLEServer();
  #endif

  #ifdef TAG
  setupBLEClient();
  #endif

  updateDisplay("Ready");
}

void loop() {
  #ifdef ANCHOR
  if (deviceConnected) {
    float distance = getUWBDistance();
    String buttonStateMessage = "Lock: " + String(digitalRead(LOCK_OPEN) == LOW ? "Open" : "Closed");

    if (distance >= 0) {
      if (distance < UNLOCK_DISTANCE && !accessGranted) {
        updateDisplay("Unlocked!\nDistance: " + buttonStateMessage +" "+ String(distance, 2) + "m");
        motor("OPEN");
        accessGranted = true;
      } else if (distance >= UNLOCK_DISTANCE && accessGranted) {
        updateDisplay("Locked!\nDistance: " + buttonStateMessage +" "+ String(distance, 2) + "m");
        motor("CLOSE");
        accessGranted = false;
      } else {
        updateDisplay("Distance: " + buttonStateMessage +" "+ String(distance, 2) + "m");
              }
    } else {
      updateDisplay("UWB ranging error, Bluetooth still connected");
    }
  } else {
    updateDisplay("Waiting for tag...");
    if (!isAdvertising) {
      pAdvertising->start();
      isAdvertising = true;
    }
  }
  #endif

  #ifdef TAG
  if (connected) {
    float distance = getUWBDistance();
    if (distance >= 0) {
      updateDisplay("Distance: " + String(distance, 2) + "m");
    } else {
      updateDisplay("UWB ranging error, Bluetooth still connected");
    }
    unsigned long currentMillis = millis();
    if (currentMillis - RECONNECT_INTERVAL >= 5000) {
      if (!pClient->isConnected()) {
        connected = false;
        updateDisplay("Disconnected\nWill try to reconnect");
        doScan = true;
      }
    }
  } else {
    unsigned long currentMillis = millis();

    if (doConnect) {
      if (connectToServer()) {
        updateDisplay("Connected to lock\nSending secret...");
        pRemoteCharacteristic->writeValue(SECRET_KEY);
      } else {
        updateDisplay("Failed to connect");
        doScan = true; // Set to scan again if connection fails
      }
      doConnect = false;
    }

    if (!connected) {
      if (doScan || (currentMillis - lastConnectAttempt > RECONNECT_INTERVAL)) {
        updateDisplay("Scanning for lock...");
        BLEScan* pBLEScan = BLEDevice::getScan();
        pBLEScan->start(5, false);  // Scan for 5 seconds
        doScan = false;
        lastConnectAttempt = currentMillis;
      }
    } else {
      // Check if still connected
      if (!pClient->isConnected()) {
        connected = false;
        updateDisplay("Disconnected\nWill try to reconnect");
        doScan = true;
      }
    }
  }
  #endif

  delay(100);
}