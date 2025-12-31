#pragma once

#include "system/App.h"
#include "ui/UIManager.h"
#include "system/EventSystem.h"
#include "system/AppManager.h"
#include "assets/remote_rover_keyboard_tips.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

class RemoteApp : public App {
private:
    EventSystem* eventSystem;

    enum ControlIds {
        MENU_WINDOW_ID = 1,
        MENU_LIST_ID = 2,
        MENU_STATUS_LABEL_ID = 3,
        REMOTE_WINDOW_ID = 10,
        REMOTE_BACK_BUTTON_ID = 11,
        REMOTE_IMAGE_ID = 12
    };

    UILabel* menuStatusLabel;
    UIMenuList* remoteMenu;
    UIWindow* menuWindow;

    UIWindow* remoteWindow;
    UIImage* tipsImage;

    bool espNowInitialized;
    uint8_t broadcastAddress[6];

    class RemoteMenuList : public UIMenuList {
    public:
        RemoteMenuList(int id, int x, int y, int width, int height, const String& name, RemoteApp* app)
            : UIMenuList(id, x, y, width, height, name), parentApp(app) {}

        void onItemSelected(MenuItem* item) override {
            parentApp->handleRemoteSelection(item);
        }

    private:
        RemoteApp* parentApp;
    };

public:
    RemoteApp(EventSystem* events)
        : eventSystem(events),
          menuStatusLabel(nullptr),
          remoteMenu(nullptr),
          menuWindow(nullptr),
          remoteWindow(nullptr),
          tipsImage(nullptr),
          espNowInitialized(false) {
        broadcastAddress[0] = 0xFF;
        broadcastAddress[1] = 0xFF;
        broadcastAddress[2] = 0xFF;
        broadcastAddress[3] = 0xFF;
        broadcastAddress[4] = 0xFF;
        broadcastAddress[5] = 0xFF;
    }

    void setup() override {
        menuWindow = uiManager->createWindow(MENU_WINDOW_ID, 30, 20, 180, 100, "Remote", "RemoteMenuWindow");

        menuStatusLabel = uiManager->createLabel(MENU_STATUS_LABEL_ID, 15, 25, "Select a remote:", "RemoteStatus", menuWindow);

        remoteMenu = new RemoteMenuList(MENU_LIST_ID, 15, 40, 140, 50, "RemoteList", this);
        remoteMenu->setParent(menuWindow);
        uiManager->addWidget(remoteMenu);

        remoteMenu->addItem("ESP-NOW Rover Remote", 101);
        remoteMenu->setColors(TFT_GREEN, TFT_YELLOW, TFT_WHITE, TFT_DARKGREY);

        remoteWindow = uiManager->createWindow(REMOTE_WINDOW_ID, 30, 20, 144, 100, "ESP-NOW Rover Remote", "RoverWindow");

        tipsImage = uiManager->createImage(REMOTE_IMAGE_ID, 15, 42, 128, 60, remote_rover_keyboard_tips, remote_rover_keyboard_tips_size, "TipsImage", remoteWindow);

        uiManager->setWidgetTreeVisible(remoteWindow, false);

        uiManager->nextFocus();
    }

    void loop() override {
    }

    void onKeyEvent(const KeyEvent& event) override {
        if (event.esc) {
            if (remoteWindow && remoteWindow->isVisible()) {
                showMenuView();
                if (appManager) {
                    appManager->setGlobalEscEnabled(true);
                }
                return;
            }
        }
        if (remoteWindow && remoteWindow->isVisible()) {
            handleRemoteKeyEvent(event);
            return;
        }
        if (uiManager->handleKeyEvent(event)) {
            uiManager->refreshAppArea();
        }
    }

    void handleRemoteSelection(MenuItem* item) {
        if (!item) return;
        if (item->id == 101) {
            showRemoteView();
        }
    }

    void showRemoteView() {
        if (!menuWindow || !remoteWindow) return;
        uiManager->hidePage(menuWindow);
        uiManager->showPage(remoteWindow);
        if (appManager) {
            appManager->setGlobalEscEnabled(false);
        }
        uiManager->refresh();
        sendEspNowCommand("C_ST");
    }

    void showMenuView() {
        if (!menuWindow || !remoteWindow) return;
        uiManager->hidePage(remoteWindow);
        uiManager->showPage(menuWindow);
        uiManager->refresh();
    }

private:
    void initEspNow() {
        if (espNowInitialized) {
            return;
        }
        WiFi.mode(WIFI_STA);
        esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
        if (esp_now_init() != ESP_OK) {
            return;
        }
        esp_now_peer_info_t peerInfo = {};
        for (int i = 0; i < 6; i++) {
            peerInfo.peer_addr[i] = broadcastAddress[i];
        }
        peerInfo.channel = 1;
        peerInfo.encrypt = false;
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            return;
        }
        espNowInitialized = true;
    }

    void sendEspNowCommand(const char* cmd) {
        if (!cmd) return;
        initEspNow();
        if (!espNowInitialized) return;
        size_t len = std::strlen(cmd);
        if (len == 0) return;
        esp_now_send(broadcastAddress, reinterpret_cast<const uint8_t*>(cmd), len);
    }

    void handleRemoteKeyEvent(const KeyEvent& event) {
        if (!event.text.isEmpty()) {
            char key = event.text.charAt(0);
            if (key >= 'a' && key <= 'z') {
                key = static_cast<char>(key - 'a' + 'A');
            }
            if (key == 'E') {
                sendEspNowCommand("C_FD");
                return;
            }
            if (key == 'A') {
                sendEspNowCommand("C_LS");
                return;
            }
            if (key == 'S') {
                sendEspNowCommand("C_BK");
                return;
            }
            if (key == 'D') {
                sendEspNowCommand("C_RS");
                return;
            }
            if (key == 'K') {
                sendEspNowCommand("C_TL");
                return;
            }
            if (key == 'L') {
                sendEspNowCommand("C_TR");
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
            sendEspNowCommand("C_ST");
        }
    }
};

