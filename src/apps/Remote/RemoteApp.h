#pragma once

#include "system/App.h"
#include "ui/UIManager.h"
#include "system/EventSystem.h"
#include "system/AppManager.h"
#include "assets/remote_rover_keyboard_tips.h"

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

    class RemoteMenuList : public UIMenuList {
    public:
        RemoteMenuList(int id, int x, int y, int width, int height, const String& name, int itemHeight, RemoteApp* app)
            : UIMenuList(id, x, y, width, height, name, itemHeight), parentApp(app) {}

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
          tipsImage(nullptr) {}

    void setup() override {
        menuWindow = uiManager->createWindow(MENU_WINDOW_ID, 30, 20, 180, 100, "Remote", "RemoteMenuWindow");

        menuStatusLabel = uiManager->createLabel(MENU_STATUS_LABEL_ID, 15, 25, "Select a remote:", "RemoteStatus", menuWindow);

        remoteMenu = new RemoteMenuList(MENU_LIST_ID, 15, 40, 140, 50, "RemoteList", 12, this);
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
    }

    void showMenuView() {
        if (!menuWindow || !remoteWindow) return;
        uiManager->hidePage(remoteWindow);
        uiManager->showPage(menuWindow);
        uiManager->refresh();
    }
};

