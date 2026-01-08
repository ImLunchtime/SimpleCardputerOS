#pragma once

#include "ITool.h"
#include "assets/panel_intercom.h"

class IntercomTool : public ITool {
public:
    IntercomTool()
        : window(nullptr),
          image(nullptr),
          uiManager(nullptr) {
    }

    const char* getMenuText() const override {
        return "Intercom";
    }

    int getMenuItemId() const override {
        return 104;
    }

    void ensureCreated(UIManager* uiManager) override {
        if (window) {
            return;
        }
        window = uiManager->createWindow(WINDOW_ID, 30, 20, 154, 108, "Intercom", "IntercomWindow");
        image = uiManager->createImage(IMAGE_ID, 11, 11, 144, 98, panel_intercom, panel_intercom_size, "IntercomImage", window);
        uiManager->setWidgetTreeVisible(window, false);
    }

    UIWindow* getWindow() const override {
        return window;
    }

    void handleKeyEvent(const KeyEvent& event, void (*sendCommand)(const char* cmd, void* ctx), void* ctx) override {
        (void)event;
        (void)sendCommand;
        (void)ctx;
    }

private:
    enum ControlIds {
        WINDOW_ID = 300,
        IMAGE_ID = 301
    };

    UIWindow* window;
    UIImage* image;
    UIManager* uiManager;
};
