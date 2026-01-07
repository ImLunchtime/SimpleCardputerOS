#pragma once

#include "system/App.h"
#include "ui/UIManager.h"
#include "system/EventSystem.h"
#include "system/AppManager.h"
#include "ITool.h"
#include "RoverRemoteTool.h"
#include "CapGNSSTool.h"
#include "BusSimulatorTool.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

class ToolboxApp : public App {
private:
    EventSystem* eventSystem;

    enum ControlIds {
        MENU_WINDOW_ID = 1,
        MENU_LIST_ID = 2,
        MENU_STATUS_LABEL_ID = 3
    };

    UILabel* menuStatusLabel;
    UIMenuList* toolMenu;
    UIWindow* menuWindow;

    ITool* currentTool;
    static const int kToolCount = 3;
    ITool* tools[kToolCount];
    RoverRemoteTool roverRemoteTool;
    CapGNSSTool gnssTool;
    BusSimulatorTool busSimTool;

    bool espNowInitialized;
    uint8_t broadcastAddress[6];

    class ToolMenuList : public UIMenuList {
    public:
        ToolMenuList(int id, int x, int y, int width, int height, const char* name, ToolboxApp* app)
            : UIMenuList(id, x, y, width, height, name), parentApp(app) {}

        void onItemSelected(MenuItem* item) override {
            parentApp->handleToolSelection(item);
        }

    private:
        ToolboxApp* parentApp;
    };

public:
    ToolboxApp(EventSystem* events)
        : eventSystem(events),
          menuStatusLabel(nullptr),
          toolMenu(nullptr),
          menuWindow(nullptr),
          currentTool(nullptr),
          roverRemoteTool(),
          gnssTool(),
          busSimTool(),
          espNowInitialized(false) {
        tools[0] = &roverRemoteTool;
        tools[1] = &gnssTool;
        tools[2] = &busSimTool;
        currentTool = tools[0];
        broadcastAddress[0] = 0xFF;
        broadcastAddress[1] = 0xFF;
        broadcastAddress[2] = 0xFF;
        broadcastAddress[3] = 0xFF;
        broadcastAddress[4] = 0xFF;
        broadcastAddress[5] = 0xFF;
    }

    void setup() override {
        menuWindow = uiManager->createWindow(MENU_WINDOW_ID, 30, 20, 180, 100, "Tool", "ToolMenuWindow");

        menuStatusLabel = uiManager->createLabel(MENU_STATUS_LABEL_ID, 15, 25, "Select a tool:", "ToolStatus", menuWindow);

        toolMenu = new ToolMenuList(MENU_LIST_ID, 15, 40, 140, 50, "ToolList", this);
        toolMenu->setParent(menuWindow);
        uiManager->addWidget(toolMenu);

        for (int i = 0; i < kToolCount; i++) {
            ITool* tool = tools[i];
            if (tool) {
                toolMenu->addItem(tool->getMenuText(), tool->getMenuItemId());
                tool->ensureCreated(uiManager);
                UIWindow* window = tool->getWindow();
                if (window) {
                    uiManager->setWidgetTreeVisible(window, false);
                }
            }
        }
        toolMenu->setColors(TFT_GREEN, TFT_YELLOW, TFT_WHITE, TFT_DARKGREY);

        uiManager->nextFocus();
    }

    void loop() override {
        if (currentTool) {
            currentTool->ensureCreated(uiManager);
            UIWindow* toolWindow = currentTool->getWindow();
            if (toolWindow && toolWindow->isVisible()) {
                currentTool->update();
            }
        }
    }

    void onKeyEvent(const KeyEvent& event) override {
        if (event.esc) {
            if (currentTool) {
                currentTool->ensureCreated(uiManager);
                UIWindow* toolWindow = currentTool->getWindow();
                if (toolWindow && toolWindow->isVisible()) {
                    deinitEspNow();
                    showMenuView();
                    if (appManager) {
                        appManager->setGlobalEscEnabled(true);
                    }
                    return;
                }
            }
        }
        if (currentTool) {
            currentTool->ensureCreated(uiManager);
            UIWindow* toolWindow = currentTool->getWindow();
            if (toolWindow && toolWindow->isVisible()) {
                handleToolKeyEvent(event);
                return;
            }
        }
        if (uiManager->handleKeyEvent(event)) {
            uiManager->refreshAppArea();
        }
    }

    void handleToolSelection(MenuItem* item) {
        if (!item) return;
        ITool* selected = nullptr;
        for (int i = 0; i < kToolCount; i++) {
            ITool* tool = tools[i];
            if (tool && tool->getMenuItemId() == item->id) {
                selected = tool;
                break;
            }
        }
        if (selected) {
            currentTool = selected;
            showToolView();
        }
    }

    void showToolView() {
        if (!menuWindow || !currentTool) return;
        currentTool->ensureCreated(uiManager);
        UIWindow* toolWindow = currentTool->getWindow();
        if (!toolWindow) return;
        uiManager->hidePage(menuWindow);
        uiManager->showPage(toolWindow);  
        if (appManager) {
            appManager->setGlobalEscEnabled(false);
        }
        uiManager->refresh();
        TipManager* tip = appManager ? appManager->getTipManager() : nullptr;
        if (tip) {
            tip->showTwoLabel("Initializing device", "Please wait...", 800, false);
        }
        sendEspNowCommand("C_ST");
    }

    void showMenuView() {
        if (!menuWindow || !currentTool) return;
        currentTool->ensureCreated(uiManager);
        UIWindow* toolWindow = currentTool->getWindow();
        if (!toolWindow) return;
        uiManager->hidePage(toolWindow);
        uiManager->showPage(menuWindow);
        uiManager->refresh();
    }

private:
    static void sendEspNowCommandStatic(const char* cmd, void* ctx) {
        if (!ctx) return;
        ToolboxApp* app = static_cast<ToolboxApp*>(ctx);
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

    void handleToolKeyEvent(const KeyEvent& event) {
        if (!currentTool) {
            return;
        }
        currentTool->handleKeyEvent(event, &ToolboxApp::sendEspNowCommandStatic, this);
    }
};
