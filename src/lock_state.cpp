#include "lock_state.h"
#include "uwb.h"
#include "ble.h"

const char* LockState::STORAGE_NAMESPACE = "lock";
const char* LockState::POSITION_KEY = "position";

LockState::LockState() : currentPosition(LockPosition::UNKNOWN), physicallyOpen(false) {
    preferences.begin(STORAGE_NAMESPACE, false);  // false = RW mode
    loadState();
}

LockState::~LockState() {
    preferences.end();
}

LockState& LockState::getInstance() {
    static LockState instance;
    return instance;
}

void LockState::loadState() {
    // Default to UNKNOWN if no value is stored
    int storedPosition = preferences.getInt(POSITION_KEY, static_cast<int>(LockPosition::UNKNOWN));
    currentPosition = static_cast<LockPosition>(storedPosition);
}

void LockState::saveState() {
    preferences.putInt(POSITION_KEY, static_cast<int>(currentPosition));
}

void LockState::setPosition(LockPosition position) {
    currentPosition = position;
    saveState();  // Persist the new state
}

LockPosition LockState::getPosition() const {
    return currentPosition;
}

bool LockState::isOpen() const {
    return currentPosition == LockPosition::OPEN;
}

void LockState::updatePhysicalState(bool isPhysicallyOpen) {
    physicallyOpen = isPhysicallyOpen;
}

bool LockState::isPhysicallyOpen() const {
    return physicallyOpen;
}

String LockState::getStatusString() const {
    String motorState = (currentPosition == LockPosition::OPEN) ? "Unlocked" : "Locked";
    String physicalState = physicallyOpen ? "Open" : "Closed";
    return "Lock: " + motorState + "/" + physicalState;
}

void LockState::checkAndHandleDeepSleep() {
    #ifdef ANCHOR
    if (isPhysicallyOpen() || currentPosition == LockPosition::OPEN) {
        enterDeepSleep();
    }
    #endif
}

void LockState::enterDeepSleep() {
    stopUWBRanging();  // Stop UWB ranging before sleep
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_41, 0); // Use LOCK_OPEN pin for wakeup
    updateDisplay("Entering deep sleep\nWill wake when closed");
    delay(2000); // Give time to read the message
    esp_deep_sleep_start();
}

void LockState::handleWakeUp() {
    #ifdef ANCHOR
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        updateDisplay("Waking up from\ndeep sleep");
        delay(1000);
    }
    #endif
} 