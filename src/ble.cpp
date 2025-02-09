#include "ble.h"

#ifdef ANCHOR
BLEAdvertising *pAdvertising = nullptr;
bool isAdvertising = false;
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool accessGranted = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      isAdvertising = false;
      updateDisplay("Device connected\nStarting UWB ranging");
      startUWBRanging();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      accessGranted = false;
      updateDisplay("Device disconnected");
      stopUWBRanging();
      pAdvertising->start();
      isAdvertising = true;
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue();
      std::string stdValue = value.c_str();
      if (value == SECRET_KEY) {
        accessGranted = true;
        updateDisplay("Access granted!");
      } else {
        accessGranted = false;
        updateDisplay("Access denied!");
      }
    }
};

void setupBLEServer() {
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pService->start();

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinInterval(0x20); // Minimum advertising interval for iOS (100ms)
  pAdvertising->setMaxInterval(0x40); // Maximum advertising interval for iOS (200ms)
  pAdvertising->start();
  isAdvertising = true;
}
#endif

#ifdef TAG
bool doConnect = false;
bool connected = false;
BLEAddress *pServerAddress;
BLERemoteCharacteristic* pRemoteCharacteristic;
bool doScan = false;
unsigned long lastConnectAttempt = 0;
BLEClient* pClient = nullptr;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {
      updateDisplay("Anchor found:\n" + String(advertisedDevice.getAddress().toString().c_str()));
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
    }
  }
};

void setupBLEClient() {
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

bool connectToServer() {
  updateDisplay("Connecting to:\n" + String(pServerAddress->toString().c_str()));
  
  pClient = BLEDevice::createClient();
  if (!pClient->connect(*pServerAddress)) {
    updateDisplay("Connection failed");
    return false;
  }

  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) {
    updateDisplay("Failed to find\nour service");
    pClient->disconnect();
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHARACTERISTIC_UUID));
  if (pRemoteCharacteristic == nullptr) {
    updateDisplay("Failed to find\nour characteristic");
    pClient->disconnect();
    return false;
  }

  connected = true;
  startUWBRanging();
  return true;
}
#endif