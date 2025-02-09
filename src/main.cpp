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
  
  #ifdef ANCHOR
  initializeMotor();
  LockState::getInstance().handleWakeUp();
  #endif
  
  setupUWBHardware();
  configureUWB();
  setUWBToMode("TAG");
  stopUWBRanging();
  putToSleep();    // Put UWB module to sleep initially

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
  static bool uwbConfigured = false;
  auto& lockState = LockState::getInstance();
  lockState.updatePhysicalState(!digitalRead(LOCK_OPEN));
  lockState.checkAndHandleDeepSleep();
  
  if (deviceConnected) {
    if (!uwbConfigured) {
      // Configure UWB module as Anchor
      configureUWB();
      startUWBRanging();
      uwbConfigured = true;
      updateDisplay("UWB Anchor configured");
    }
    float distance = getUWBDistance();
    String statusMessage = lockState.getStatusString();

    // Only command opening if within distance, lock not open, and REQUEST_OPENING is pressed
    if (distance >= 0) {
      if (distance < UNLOCK_DISTANCE && accessGranted && digitalRead(REQUEST_OPENING) == LOW) {
        motor("OPEN");
        motor("CLOSE");
        updateDisplay("Unlocked\n" + statusMessage + "\nDist: " + String(distance, 2) + "m");
      }
      // When lock is commanded to close, the logic no longer takes request_opening into account, but shows its state
      else if (distance >= UNLOCK_DISTANCE) {
        if (digitalRead(REQUEST_OPENING) == LOW) {
          showFlashEffect(true);
        }
        updateDisplay("Locked\n" + statusMessage + "\nDist: " + String(distance, 2));
      } 
      else {
        updateDisplay(statusMessage + "\nDist: " + String(distance, 2));
      }
    } else {
      updateDisplay(statusMessage + "\nUWB ranging error");
    }
  } else {
    if(uwbConfigured) {
        setUWBToMode("TAG");
        putToSleep();
        uwbConfigured = false;
        updateDisplay("UWB Anchor sleeping");
    }
    updateDisplay("Waiting for tag...");
    if (!isAdvertising) {
      pAdvertising->start();
      isAdvertising = true;
    }
  }
  #endif

  #ifdef TAG
  static bool uwb_ranging = false;
  static bool ble_was_connected = false; // Track if BLE was ever connected

  if (connected) {
    if (!uwb_ranging) {
      configureUWB(); // Initialise only once the BLE is connected
      startUWBRanging();
      uwb_ranging = true;
    }
    ble_was_connected = true; // Record successful BLE connection

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
          stopUWBRanging();
          uwb_ranging = false;
          updateDisplay("Disconnected\nWill try to reconnect");
          doScan = true;
        }
      }
  } else {
      // Only put to sleep if BLE was previously connected and now isn't 
      if (ble_was_connected) {
          if(uwb_ranging) {
              stopUWBRanging();
              uwb_ranging = false;
          }
          putToSleep();  // Put UWB to sleep only after we have lost the BLE connection
          ble_was_connected = false; // Reset this flag since we're now disconnected
      }

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
        // putToSleep();  //Removed this, the UART will wake it up
          BLEScan* pBLEScan = BLEDevice::getScan();
          pBLEScan->start(5, false);  // Scan for 5 seconds
          doScan = false;
          lastConnectAttempt = currentMillis;
        }
      } else {
        // Check if still connected
        if (!pClient->isConnected()) {
          connected = false;
          stopUWBRanging();
          uwb_ranging = false;
          updateDisplay("Disconnected\nWill try to reconnect");
          doScan = true;
        }
      }
    }
  #endif

  delay(100);
}