#ifndef LOCK_STATE_H
#define LOCK_STATE_H

#include <Arduino.h>
#include <Preferences.h>
#include "display.h"

enum class LockPosition {
    OPEN,
    CLOSED,
    UNKNOWN
};

class LockState {
public:
    static LockState& getInstance();
    
    void setPosition(LockPosition position);
    void updatePhysicalState(bool isPhysicallyOpen);
    
    LockPosition getPosition() const;
    bool isOpen() const;
    bool isPhysicallyOpen() const;
    String getStatusString() const;
    
    void checkAndHandleDeepSleep();
    void handleWakeUp();
    
private:
    LockState();
    ~LockState();
    
    void loadState();
    void saveState();
    void enterDeepSleep();
    
    LockPosition currentPosition;
    bool physicallyOpen;
    Preferences preferences;
    static const char* STORAGE_NAMESPACE;
    static const char* POSITION_KEY;
};

#endif 