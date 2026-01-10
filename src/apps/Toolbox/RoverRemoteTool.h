#pragma once

#include "ITool.h"
#include "assets/remote_rover_keyboard_tips.h"

class RoverRemoteTool : public ImagePanelTool {
public:
    const char* getMenuText() const override {
        return "ESP-NOW Rover Remote";
    }

    int getMenuItemId() const override {
        return 101;
    }

    bool usesEspNow() const override {
        return true;
    }

    const char* getEnterCommand() const override {
        return "C_ST";
    }

    const char* getEnterTipLine1() const override {
        return "Starting ESP-NOW";
    }

    const char* getEnterTipLine2() const override {
        return "Please wait...";
    }

    void handleKeyEvent(const KeyEvent& event, const ToolServices& services) override {
        if (!services.sendCommand) {
            return;
        }
        if (!event.text.isEmpty()) {
            char key = event.text.charAt(0);
            if (key >= 'a' && key <= 'z') {
                key = static_cast<char>(key - 'a' + 'A');
            }
            if (key == 'E') {
                services.send("C_FD");
                return;
            }
            if (key == 'A') {
                services.send("C_LS");
                return;
            }
            if (key == 'S') {
                services.send("C_BK");
                return;
            }
            if (key == 'D') {
                services.send("C_RS");
                return;
            }
            if (key == 'K') {
                services.send("C_TL");
                return;
            }
            if (key == 'L') {
                services.send("C_TR");
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
            services.send("C_ST");
        }
    }

private:
    enum ControlIds {
        REMOTE_WINDOW_ID = 10,
        REMOTE_IMAGE_ID = 12
    };

    int getWindowWidgetId() const override { return REMOTE_WINDOW_ID; }
    int getImageWidgetId() const override { return REMOTE_IMAGE_ID; }
    int getWindowW() const override { return 144; }
    int getWindowH() const override { return 100; }
    const char* getWindowTitle() const override { return "ESP-NOW Rover Remote"; }
    const char* getWindowName() const override { return "RoverRemoteWindow"; }
    int getImageX() const override { return 15; }
    int getImageY() const override { return 42; }
    int getImageW() const override { return 128; }
    int getImageH() const override { return 60; }
    const uint8_t* getImageData() const override { return remote_rover_keyboard_tips; }
    size_t getImageDataSize() const override { return remote_rover_keyboard_tips_size; }
    const char* getImageName() const override { return "TipsImage"; }
};
