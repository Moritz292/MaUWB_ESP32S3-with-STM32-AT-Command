#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// User config
//#define TAG
#define ANCHOR

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

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
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      updateDisplay("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      updateDisplay("Device disconnected");
    }
};
#endif

#ifdef TAG
static boolean doConnect = false;
static boolean connected = false;
static BLEAddress *pServerAddress;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {
      updateDisplay("Device found:\n" + String(advertisedDevice.getAddress().toString().c_str()));
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
    }
  }
};
#endif


void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  updateDisplay("Initializing...");

  BLEDevice::init("ESP32_UWB_DEVICE");

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
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  updateDisplay("Advertising started");
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
    updateDisplay("Connected\nReady for UWB");
    // Perform actions when a device is connected
    // For example, you could start the UWB distance measurement here
  } else {
    updateDisplay("Waiting for\n BLE connection...");
  }
  #endif

  #ifdef TAG
  if (doConnect) {
    if (connectToServer()) {
      updateDisplay("Connected to\nBLE Server");
    } else {
      updateDisplay("Failed to connect");
    }
    doConnect = false;
  }

  if (!connected) {
    updateDisplay("Scanning for\nBLE devices...");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->start(5, false);  // Scan for 5 seconds
  }
  #endif

  delay(1000);
}

#ifdef TAG
bool connectToServer() {
  updateDisplay("Connecting to:\n" + String(pServerAddress->toString().c_str()));
  
  BLEClient*  pClient  = BLEDevice::createClient();

  // Connect to the remote BLE Server.
  pClient->connect(*pServerAddress);

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) {
    updateDisplay("Failed to find\nour service");
    pClient->disconnect();
    return false;
  }

  connected = true;
  return true;
}
#endif