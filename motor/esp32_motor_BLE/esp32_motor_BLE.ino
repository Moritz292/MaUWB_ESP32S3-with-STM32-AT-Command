#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <Arduino.h>
#include <driver/ledc.h>

// User config
//#define TAG
#define ANCHOR

// BLE definitions
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SECRET_KEY          "MySecretKey123"

// Motor control pins
#define IN1 1    // Motor control pin 1
#define IN2 2    // Motor control pin 2
#define LOCK_OPEN 41 // switch when pressed lock open
#define EEP 42   // Motor enable pin

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
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
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
#define UNLOCK_DISTANCE 3.0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void motor(String direction) {
  // Ensure the lock is closed before unlocking
  if (isButtonPressed(LOCK_OPEN)) {
    Serial.println("Lock is already open. Cannot unlock.");
    updateDisplay("Lock already open");
    return;
  }
  // Fade in from 0 to 100% over 0.5 seconds
  int fadeTime = 500;                // total fade time in milliseconds
  int fadeSteps = 50;                // number of steps for fading
  int delayTime = fadeTime / fadeSteps;  // delay time between steps

  // Set the motor direction
  if (direction == "OPEN") {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (direction == "CLOSE") {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    Serial.println("Invalid direction");
    return;
  }

  // Ramp up the PWM duty cycle from 0 to 255
  for (int duty = 0; duty <= 255; duty += (255 / fadeSteps)) {
    analogWrite(EEP, duty);          // Set PWM duty cycle
    delay(delayTime);                // Short delay between increments
  }

  // Maintain full speed for 1 second
  analogWrite(EEP, 255);             // 100% duty cycle
  delay(200);

  // Stop motor after fade
  analogWrite(EEP, 0);               // Stop PWM signal
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  Serial.println("Motor stopped after fade");
  updateDisplay("Motor stopped");
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

bool isButtonPressed(int buttonPin) {
  static int lastState = HIGH;              // Last stable state of the button
  static unsigned long lastDebounceTime = 0; // Last time the state changed
  const int DEBOUNCE_DELAY = 50;            // 50 milliseconds debounce time

  int currentState = digitalRead(buttonPin);

  // Check if the button state has changed
  if (currentState != lastState) {
    lastDebounceTime = millis(); // Reset the debounce timer
  }

  // If the state has been stable for longer than the debounce delay
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentState == LOW) { // Button is pressed
      lastState = currentState; // Update the last state
      return true;
    }
  }

  lastState = currentState; // Update the last state
  return false; // Button is not pressed
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
      updateDisplay("Device connected\nStarting UWB ranging");
      // Start UWB ranging
      startUWBRanging();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      accessGranted = false;
      updateDisplay("Device disconnected");
      
      // Stop UWB ranging
      stopUWBRanging();
      
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
  Serial.println("Starting BLE+UWB Lock!");
  // Configure LEDC for PWM using the new ledcAttachChannel function
  if (!ledcAttach(EEP, PWMC_FREQUENCY, PWMC_RESOLUTION)) {
    Serial.println(F("LEDC configuration failed"));
    for (;;);
  }

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EEP, OUTPUT);
  pinMode(LOCK_OPEN, INPUT_PULLUP);  // Configure the pin as input with pull-up

  //digitalWrite(EEP, LOW);  // Disable motor


  // Initialize I2C and OLED display
  Wire.begin(I2C_SDA, I2C_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  updateDisplay("Initializing...");

  // Initialize BLE
  BLEDevice::init("ESP32_BLE_UWB_LOCK");

  // Initialize UWB
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);
  SERIAL_AT.begin(115200, SERIAL_8N1, IO_RXD2, IO_TXD2);
  
  // UWB setup commands
  sendData("AT?", 2000, 1);
  sendData("AT+RESTORE", 5000, 1);
  sendData(config_cmd(), 2000, 1);
  sendData(cap_cmd(), 2000, 1);
  sendData("AT+SETRPT=1", 2000, 1);
  sendData("AT+SAVE", 2000, 1);
  sendData("AT+RESTART", 2000, 1);

  // Setup BLE server or client based on ANCHOR or TAG mode
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
    // Print the button state for debugging
    String buttonStateMessage = "Button: " + String(digitalRead(LOCK_OPEN) == LOW ? "Open" : "Closed");

    if (distance >= 0) {
      if (distance < UNLOCK_DISTANCE && !accessGranted) { // Lock must be closed to unlock
        updateDisplay("Unlocked!\nDistance: " + buttonStateMessage + String(distance, 2) + "m");
        motor("OPEN");
        accessGranted = true;
      } else if (distance >= UNLOCK_DISTANCE && accessGranted) {
        updateDisplay("Locked!\nDistance: " + buttonStateMessage + String(distance, 2) + "m");
        motor("CLOSE");
        accessGranted = false;
      } else {
        updateDisplay("Distance: " + buttonStateMessage + String(distance, 2) + "m");
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

#ifdef ANCHOR
void setupBLEServer() {
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
}
#endif

#ifdef TAG
void setupBLEClient() {
  // Create the BLE Scan
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

bool connectToServer() {
  updateDisplay("Connecting to:\n" + String(pServerAddress->toString().c_str()));
  
  pClient = BLEDevice::createClient();

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
  
  // Start UWB ranging
  startUWBRanging();
  
  return true;
}
#endif

void startUWBRanging() {
  // Send UWB command to start ranging
  sendData("AT+RANGE=1", 2000, 1);
}

void stopUWBRanging() {
  // Send UWB command to stop ranging
  sendData("AT+RANGE=0", 2000, 1);
}

float getUWBDistance() {
  String response = "";
  // Read the available data setup setup: sendData("AT+SETRPT=1", 2000, 1);
  // Check if there's data available in the serial buffer
  if (SERIAL_AT.available()) {
    // Read the available data
    while (SERIAL_AT.available()) {
      char c = SERIAL_AT.read();
      if (c != '\r') {
        response += c;
      }
    }
  }
  // Parse the response to get the distance
  // This is a simplified example, you may need to adjust based on the actual response format
  int startIndex = response.indexOf("(") + 1;
  int endIndex = response.indexOf(")");
  if (startIndex > 0 && endIndex > startIndex) {
    String distanceStr = response.substring(startIndex, endIndex);
    return distanceStr.toFloat() / 100.0; // Convert cm to meters
  }
  return -1; // No valid distance data available
}

String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    // command = command + "\r\n";

    SERIAL_LOG.println(command);
    SERIAL_AT.println(command); // send the read character to the SERIAL_LOG

    long int time = millis();

    while ((time + timeout) > millis())
    {
        while (SERIAL_AT.available())
        {

            // The esp has data so display its output to the serial window
            char c = SERIAL_AT.read(); // read the next character.
            response += c;
        }

    }

    if (debug)
    {
        SERIAL_LOG.println(response);
    }

    return response;
}

String config_cmd()
{
    String temp = "AT+SETCFG=";

    // Set device id
    temp = temp + UWB_INDEX;

    // Set device role
#ifdef TAG
    temp = temp + ",0";
#endif
#ifdef ANCHOR
    temp = temp + ",1";
#endif

    // Set frequence 850k or 6.8M
#ifdef FREQ_850K
    temp = temp + ",0";
#endif
#ifdef FREQ_6800K
    temp = temp + ",1";
#endif

    // Set range filter
    temp = temp + ",1";

    return temp;
}

String cap_cmd()
{
    String temp = "AT+SETCAP=";

    // Set Tag capacity
    temp = temp + UWB_TAG_COUNT;

    //  Time of a single time slot
#ifdef FREQ_850K
    temp = temp + ",15";
#endif
#ifdef FREQ_6800K
    temp = temp + ",10";
#endif

    return temp;
}