#include "config.h"
#include "display.h"
#include "motor.h"
#include "uwb.h"
#include "ble.h"
#include "lock_state.h"

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
    auto& lockState = LockState::getInstance();
    
    // Update physical state
    lockState.updatePhysicalState(!digitalRead(LOCK_OPEN));
    String statusMessage = lockState.getStatusString();

    if (distance >= 0) {
      if (distance < UNLOCK_DISTANCE && !accessGranted) {
        motor("OPEN");
        accessGranted = true;
        updateDisplay("Unlocked\n" + statusMessage + "\nDist: " + String(distance, 2) + "m");
      } else if (distance >= UNLOCK_DISTANCE && accessGranted) {
        motor("CLOSE");
        accessGranted = false;
        updateDisplay("Locked\n" + statusMessage + "\nDist: " + String(distance, 2) + "m");
      } else {
        updateDisplay(statusMessage + "\nDist: " + String(distance, 2) + "m");
      }
    } else {
      updateDisplay(statusMessage + "\nUWB ranging error");
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
    static float lastDistance = -1;
    if (distance >= 0) {
      // Only show effect when crossing the threshold
      if (distance < UNLOCK_DISTANCE && lastDistance >= UNLOCK_DISTANCE) {
        showFlashEffect(false); // Show unlocking animation
      } else if (distance >= UNLOCK_DISTANCE && lastDistance < UNLOCK_DISTANCE && lastDistance >= 0) {
        showFlashEffect(true); // Show locking animation
      }
      updateDisplay("Distance: " + String(distance, 2) + "m");
      lastDistance = distance;
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