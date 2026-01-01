#pragma once

#include "ui/UIManager.h"
#include "system/EventSystem.h"

class IRemoteController {
public:
    virtual ~IRemoteController() {}

    virtual const char* getMenuText() const = 0;
    virtual int getMenuItemId() const = 0;

    virtual void ensureCreated(UIManager* uiManager) = 0;
    virtual UIWindow* getWindow() const = 0;

    virtual void handleKeyEvent(const KeyEvent& event, void (*sendCommand)(const char* cmd, void* ctx), void* ctx) = 0;
};

