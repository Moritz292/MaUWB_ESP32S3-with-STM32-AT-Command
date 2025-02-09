#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>

// User config
//#define TAG
#define ANCHOR

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SECRET_KEY          "MySecretKey123"

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 39
#define I2C_SCL 38

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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
      updateDisplay("Device connected\nWaiting for auth...");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      accessGranted = false;
      updateDisplay("Device disconnected");
      
      // Restart advertising
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
#endif

#ifdef TAG
static boolean doConnect = false;
static boolean connected = false;
static BLEAddress *pServerAddress;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static boolean doScan = false;
unsigned long lastConnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds
static BLEClient* pClient = nullptr;

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
#endif

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Lock!");

  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  updateDisplay("Initializing...");

  BLEDevice::init("ESP32_BLE_LOCK");

  #ifdef ANCHOR
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
  isAdvertising = true;
  updateDisplay("Lock ready");
  #endif

  #ifdef TAG
  // Create the BLE Scan
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
  updateDisplay("Ready to scan");
  #endif
}

void loop() {
  #ifdef ANCHOR
  if (deviceConnected) {
    if (accessGranted) {
      updateDisplay("Access granted\nLock open");
    } else {
      updateDisplay("Waiting for\nauthentication...");
    }
  } else {
    updateDisplay("Lock ready\nWaiting for tag...");
    
    // Ensure advertising is running
    if (!isAdvertising) {
      pAdvertising->start();
      isAdvertising = true;
    }
  }
  #endif

  #ifdef TAG
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
  #endif

  delay(1000);
}

#ifdef TAG
bool connectToServer() {
  updateDisplay("Connecting to:\n" + String(pServerAddress->toString().c_str()));
  
  pClient = BLEDevice::createClient();

  // Connect to the remote BLE Server.
  if (!pClient->connect(*pServerAddress)) {
    updateDisplay("Connection failed");
    return false;
  }

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) {
    updateDisplay("Failed to find\nour service");
    pClient->disconnect();
    return false;
  }

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHARACTERISTIC_UUID));
  if (pRemoteCharacteristic == nullptr) {
    updateDisplay("Failed to find\nour characteristic");
    pClient->disconnect();
    return false;
  }

  connected = true;
  return true;
}
#endif