#pragma once

#include "ITool.h"
#include "assets/remote_rover_keyboard_tips.h"

class RoverRemoteTool : public ITool {
public:
    RoverRemoteTool()
        : window(nullptr),
          tipsImage(nullptr) {
    }

    const char* getMenuText() const override {
        return "ESP-NOW Rover Remote";
    }

    int getMenuItemId() const override {
        return 101;
    }

    void ensureCreated(UIManager* manager) override {
        if (!manager) return;
        UIWidget* existingWindow = manager->getWidget(REMOTE_WINDOW_ID);
        if (existingWindow && existingWindow->getType() == WIDGET_WINDOW) {
            window = static_cast<UIWindow*>(existingWindow);
            UIWidget* existingImage = manager->getWidget(REMOTE_IMAGE_ID);
            tipsImage = existingImage ? static_cast<UIImage*>(existingImage) : nullptr;
            return;
        }
        window = nullptr;
        tipsImage = nullptr;
        window = manager->createWindow(REMOTE_WINDOW_ID, 30, 20, 144, 100, "ESP-NOW RoverRemote Remote", "RoverRemoteWindow");
        tipsImage = manager->createImage(REMOTE_IMAGE_ID, 15, 42, 128, 60, remote_rover_keyboard_tips, remote_rover_keyboard_tips_size, "TipsImage", window);
        manager->setWidgetTreeVisible(window, false);
    }

    UIWindow* getWindow() const override {
        return window;
    }

    void handleKeyEvent(const KeyEvent& event, void (*sendCommand)(const char* cmd, void* ctx), void* ctx) override {
        if (!sendCommand) {
            return;
        }
        if (!event.text.isEmpty()) {
            char key = event.text.charAt(0);
            if (key >= 'a' && key <= 'z') {
                key = static_cast<char>(key - 'a' + 'A');
            }
            if (key == 'E') {
                sendCommand("C_FD", ctx);
                return;
            }
            if (key == 'A') {
                sendCommand("C_LS", ctx);
                return;
            }
            if (key == 'S') {
                sendCommand("C_BK", ctx);
                return;
            }
            if (key == 'D') {
                sendCommand("C_RS", ctx);
                return;
            }
            if (key == 'K') {
                sendCommand("C_TL", ctx);
                return;
            }
            if (key == 'L') {
                sendCommand("C_TR", ctx);
                return;
            }
        }
        if (event.text.isEmpty() &&
            !event.enter &&
            !event.del &&
            !event.tab &&
            !event.up &&
            !event.down &&
            !event.left &&
            !event.right &&
            !event.esc) {
            sendCommand("C_ST", ctx);
        }
    }

private:
    enum ControlIds {
        REMOTE_WINDOW_ID = 10,
        REMOTE_IMAGE_ID = 12
    };

    UIWindow* window;
    UIImage* tipsImage;
};
