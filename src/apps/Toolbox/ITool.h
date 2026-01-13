#pragma once

#include "ui/UIManager.h"
#include "system/EventSystem.h"

class AppManager;
class TipManager;

struct ToolServices {
    UIManager* uiManager;
    AppManager* appManager;
    TipManager* tipManager;
    void (*sendCommand)(const char* cmd, void* ctx);
    void* sendCommandCtx;

    void send(const char* cmd) const {
        if (!sendCommand || !cmd) return;
        sendCommand(cmd, sendCommandCtx);
    }
};

class ITool {
public:
    virtual ~ITool() {}

    virtual const char* getMenuText() const = 0;
    virtual int getMenuItemId() const = 0;

    virtual void setManagers(UIManager* uiManager, AppManager* appManager) = 0;
    virtual void setup() = 0;
    virtual void loop(const ToolServices& services) = 0;
    virtual void onKeyEvent(const KeyEvent& event, const ToolServices& services) = 0;

    virtual void ensureCreated(UIManager* uiManager) = 0;
    virtual UIWindow* getWindow() const = 0;

    virtual bool usesEspNow() const { return false; }
    virtual const char* getEnterCommand() const { return nullptr; }
    virtual const char* getEnterTipLine1() const { return nullptr; }
    virtual const char* getEnterTipLine2() const { return nullptr; }

    virtual void onEnter(const ToolServices& services) { (void)services; }
    virtual void onExit(const ToolServices& services) { (void)services; }

    virtual void handleKeyEvent(const KeyEvent& event, const ToolServices& services) { (void)event; (void)services; }
    virtual void update(const ToolServices& services) { (void)services; }
};

class ToolBase : public ITool {
public:
    void setManagers(UIManager* manager, AppManager* managerApp) override {
        uiManager = manager;
        appManager = managerApp;
    }

    void setup() override {
        if (!uiManager) return;
        ensureCreated(uiManager);
    }

    void loop(const ToolServices& services) override {
        update(services);
    }

    void onKeyEvent(const KeyEvent& event, const ToolServices& services) override {
        handleKeyEvent(event, services);
    }

    void ensureCreated(UIManager* manager) final {
        if (!manager) return;
        uiManager = manager;
        UIWidget* existingWindow = manager->getWidget(getWindowWidgetId());
        if (existingWindow && existingWindow->getType() == WIDGET_WINDOW) {
            window = static_cast<UIWindow*>(existingWindow);
            bindWidgets(manager);
            return;
        }
        resetPointers();
        createWidgets(manager);
        if (window) {
            manager->setWidgetTreeVisible(window, false);
        }
    }

    UIWindow* getWindow() const final {
        return window;
    }

protected:
    UIWindow* window = nullptr;
    UIManager* uiManager = nullptr;
    AppManager* appManager = nullptr;

    virtual int getWindowWidgetId() const = 0;
    virtual void resetPointers() = 0;
    virtual void bindWidgets(UIManager* manager) = 0;
    virtual void createWidgets(UIManager* manager) = 0;
};

class ImagePanelTool : public ToolBase {
public:
    void handleKeyEvent(const KeyEvent& event, const ToolServices& services) override {
        (void)event;
        (void)services;
    }

protected:
    UIImage* image = nullptr;

    virtual int getImageWidgetId() const = 0;
    virtual int getWindowX() const { return 30; }
    virtual int getWindowY() const { return 20; }
    virtual int getWindowW() const { return 154; }
    virtual int getWindowH() const { return 108; }
    virtual const char* getWindowTitle() const = 0;
    virtual const char* getWindowName() const = 0;
    virtual int getImageX() const { return 11; }
    virtual int getImageY() const { return 11; }
    virtual int getImageW() const { return 144; }
    virtual int getImageH() const { return 98; }
    virtual const uint8_t* getImageData() const = 0;
    virtual size_t getImageDataSize() const = 0;
    virtual const char* getImageName() const = 0;

    void resetPointers() override {
        window = nullptr;
        image = nullptr;
    }

    void bindWidgets(UIManager* manager) override {
        UIWidget* existingWindow = manager->getWidget(getWindowWidgetId());
        window = existingWindow ? static_cast<UIWindow*>(existingWindow) : nullptr;
        UIWidget* existingImage = manager->getWidget(getImageWidgetId());
        image = existingImage ? static_cast<UIImage*>(existingImage) : nullptr;
    }

    void createWidgets(UIManager* manager) override {
        window = manager->createWindow(
            getWindowWidgetId(),
            getWindowX(),
            getWindowY(),
            getWindowW(),
            getWindowH(),
            getWindowTitle(),
            getWindowName()
        );
        image = manager->createImage(
            getImageWidgetId(),
            getImageX(),
            getImageY(),
            getImageW(),
            getImageH(),
            getImageData(),
            getImageDataSize(),
            getImageName(),
            window
        );
    }
};
