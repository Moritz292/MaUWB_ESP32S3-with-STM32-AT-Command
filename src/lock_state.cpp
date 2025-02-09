#include "lock_state.h"
#include "uwb.h"
#include "ble.h"
#include "motor.h"
#include "config.h"

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
    // Double check the pin state directly to confirm
    if (isPhysicallyOpen()) {
        enterDeepSleep();
    }
    #endif
}

void LockState::enterDeepSleep() {
    stopUWBRanging();
    sleep(1);
    
    // Check if the pin is in the non-trigger state before sleeping
    if (digitalRead(LOCK_OPEN) == 0) {  // Adjust this condition based on your sensor logic
        gpio_pullup_en(static_cast<gpio_num_t>(LOCK_OPEN));    // Add pull-up
        gpio_hold_en(static_cast<gpio_num_t>(LOCK_OPEN));      // Opti
        esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(LOCK_OPEN), 1);
        updateDisplay("Entering deep sleep\nWill wake when closed");
        setUWBToMode("TAG");  // Ensure device is in TAG mode
        putToSleep();    // Put UWB module to sleep initially
        display.clearDisplay();
        display.display();
        display.ssd1306_command(SSD1306_DISPLAYOFF);
        esp_deep_sleep_start();
    } else {
        updateDisplay("Cannot enter sleep\nLock state incorrect");
        delay(2000);
    }
}

void LockState::handleWakeUp() {
    #ifdef ANCHOR
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        // Re-initialize display after waking from deep sleep
        initializeDisplay();
        updateDisplay("Waking up from\ndeep sleep");
        sleep(1);
    }
    #endif
} 