#pragma once

#include "system/App.h"
#include "ui/UIManager.h"
#include "system/AppManager.h"
#include "system/TipManager.h"
#include "apps/GNSS/CapGNSSTool.h"

class GNSSApp : public App {
private:
    EventSystem* eventSystem;
    CapGNSSTool* gnssTool;

public:
    explicit GNSSApp(EventSystem* events)
        : eventSystem(events), gnssTool(nullptr) {
        (void)eventSystem;
    }

    ~GNSSApp() {
        cleanup();
    }

    void setup() override {
        if (appManager) {
            appManager->setGlobalEscEnabled(false);
        }

        cleanup();

        gnssTool = new CapGNSSTool();
        gnssTool->setManagers(uiManager, appManager);
        gnssTool->setup();

        ToolServices services = buildToolServices();
        TipManager* tip = services.tipManager;
        const char* tip1 = gnssTool->getEnterTipLine1();
        const char* tip2 = gnssTool->getEnterTipLine2();
        if (tip && tip1) {
            tip->showTwoLabel(tip1, tip2 ? tip2 : "", 800, false);
        }
        gnssTool->onEnter(services);

        UIWindow* toolWindow = gnssTool->getWindow();
        if (toolWindow) {
            uiManager->showPage(toolWindow);
            uiManager->rebuildForegroundFocus();
            uiManager->refresh();
        }
    }

    void loop() override {
        if (!gnssTool) return;
        ToolServices services = buildToolServices();
        gnssTool->loop(services);
    }

    void onKeyEvent(const KeyEvent& event) override {
        if (event.esc) {
            cleanup();
            if (appManager) {
                appManager->returnToLauncher();
            }
            return;
        }

        if (gnssTool) {
            ToolServices services = buildToolServices();
            gnssTool->onKeyEvent(event, services);
        }

        if (uiManager && uiManager->handleKeyEvent(event)) {
            uiManager->refreshAppArea();
        }
    }

private:
    ToolServices buildToolServices() {
        ToolServices s{};
        s.uiManager = uiManager;
        s.appManager = appManager;
        s.tipManager = appManager ? appManager->getTipManager() : nullptr;
        s.sendCommand = nullptr;
        s.sendCommandCtx = nullptr;
        return s;
    }

    void cleanup() {
        if (appManager) {
            TipManager* tip = appManager->getTipManager();
            if (tip) {
                tip->closeTip();
            }
        }

        if (gnssTool) {
            ToolServices services = buildToolServices();
            gnssTool->onExit(services);
            if (uiManager) {
                UIWindow* toolWindow = gnssTool->getWindow();
                if (toolWindow) {
                    uiManager->setWidgetTreeVisible(toolWindow, false);
                    uiManager->removeWidgetTree(toolWindow);
                }
            }
            delete gnssTool;
            gnssTool = nullptr;
        }

        if (appManager) {
            appManager->setGlobalEscEnabled(true);
        }
    }
};

