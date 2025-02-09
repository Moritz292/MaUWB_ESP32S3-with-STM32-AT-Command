#ifndef BLE_H
#define BLE_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include "config.h"
#include "display.h"
#include "uwb.h"

#ifdef ANCHOR
void setupBLEServer();
extern BLEAdvertising *pAdvertising;
extern bool isAdvertising;
extern BLEServer* pServer;
extern BLECharacteristic* pCharacteristic;
extern bool deviceConnected;
extern bool accessGranted;
#endif

#ifdef TAG
void setupBLEClient();
bool connectToServer();
extern bool doConnect;
extern bool connected;
extern BLEAddress *pServerAddress;
extern BLERemoteCharacteristic* pRemoteCharacteristic;
extern bool doScan;
extern unsigned long lastConnectAttempt;
extern BLEClient* pClient;
#endif

#endif