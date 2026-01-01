#pragma once

#include "system/App.h"
#include "ui/UIManager.h"
#include "system/EventSystem.h"
#include "system/AppManager.h"
#include "IRemoteController.h"
#include "RoverRemoteController.h"
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
        MENU_STATUS_LABEL_ID = 3
    };

    UILabel* menuStatusLabel;
    UIMenuList* remoteMenu;
    UIWindow* menuWindow;

    IRemoteController* currentRemote;
    static const int kRemoteCount = 1;
    IRemoteController* remotes[kRemoteCount];
    RoverRemoteController roverRemote;

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
          currentRemote(nullptr),
          roverRemote(),
          espNowInitialized(false) {
        remotes[0] = &roverRemote;
        currentRemote = remotes[0];
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

        for (int i = 0; i < kRemoteCount; i++) {
            IRemoteController* controller = remotes[i];
            if (controller) {
                remoteMenu->addItem(controller->getMenuText(), controller->getMenuItemId());
                controller->ensureCreated(uiManager);
                UIWindow* window = controller->getWindow();
                if (window) {
                    uiManager->setWidgetTreeVisible(window, false);
                }
            }
        }
        remoteMenu->setColors(TFT_GREEN, TFT_YELLOW, TFT_WHITE, TFT_DARKGREY);

        uiManager->nextFocus();
    }

    void loop() override {
    }

    void onKeyEvent(const KeyEvent& event) override {
        if (event.esc) {
            if (currentRemote) {
                currentRemote->ensureCreated(uiManager);
                UIWindow* remoteWindow = currentRemote->getWindow();
                if (remoteWindow && remoteWindow->isVisible()) {
                    deinitEspNow();
                    showMenuView();
                    if (appManager) {
                        appManager->setGlobalEscEnabled(true);
                    }
                    return;
                }
            }
        }
        if (currentRemote) {
            currentRemote->ensureCreated(uiManager);
            UIWindow* remoteWindow = currentRemote->getWindow();
            if (remoteWindow && remoteWindow->isVisible()) {
                handleRemoteKeyEvent(event);
                return;
            }
        }
        if (uiManager->handleKeyEvent(event)) {
            uiManager->refreshAppArea();
        }
    }

    void handleRemoteSelection(MenuItem* item) {
        if (!item) return;
        IRemoteController* selected = nullptr;
        for (int i = 0; i < kRemoteCount; i++) {
            IRemoteController* controller = remotes[i];
            if (controller && controller->getMenuItemId() == item->id) {
                selected = controller;
                break;
            }
        }
        if (selected) {
            currentRemote = selected;
            showRemoteView();
        }
    }

    void showRemoteView() {
        if (!menuWindow || !currentRemote) return;
        currentRemote->ensureCreated(uiManager);
        UIWindow* remoteWindow = currentRemote->getWindow();
        if (!remoteWindow) return;
        uiManager->hidePage(menuWindow);
        uiManager->showPage(remoteWindow);
        if (appManager) {
            appManager->setGlobalEscEnabled(false);
        }
        uiManager->refresh();
        sendEspNowCommand("C_ST");
    }

    void showMenuView() {
        if (!menuWindow || !currentRemote) return;
        currentRemote->ensureCreated(uiManager);
        UIWindow* remoteWindow = currentRemote->getWindow();
        if (!remoteWindow) return;
        uiManager->hidePage(remoteWindow);
        uiManager->showPage(menuWindow);
        uiManager->refresh();
    }

private:
    static void sendEspNowCommandStatic(const char* cmd, void* ctx) {
        if (!ctx) return;
        RemoteApp* app = static_cast<RemoteApp*>(ctx);
        app->sendEspNowCommand(cmd);
    }

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

    void deinitEspNow() {
        if (!espNowInitialized) {
            return;
        }
        esp_now_deinit();
        WiFi.mode(WIFI_OFF);
        espNowInitialized = false;
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
        if (!currentRemote) {
            return;
        }
        currentRemote->handleKeyEvent(event, &RemoteApp::sendEspNowCommandStatic, this);
    }
};
