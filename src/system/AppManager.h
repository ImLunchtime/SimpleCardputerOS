#pragma once
#include <M5Cardputer.h>
#include "system/App.h"
#include "system/EventSystem.h"
#include "ui/UIManager.h"
#include "system/SDFileManager.h"

// 应用信息结构
struct AppInfo {
    String name;        // 应用名称
    String displayName; // 显示名称
    App* instance;      // 应用实例
    bool isLauncher;    // 是否为启动器应用
    
    AppInfo(const String& _name, const String& _displayName, App* _instance, bool _isLauncher = false)
        : name(_name), displayName(_displayName), instance(_instance), isLauncher(_isLauncher) {}
};

class AppManager {
private:
    AppInfo* apps[10];
    int appCount;
    App* currentApp;
    App* launcherApp;
    EventSystem* eventSystem;
    UIManager* globalUIManager;
    SDFileManager* globalSDManager;
    bool globalEscEnabled;
    static constexpr int BRIGHTNESS_PANEL_ID = -2000;
    static constexpr int BRIGHTNESS_SLIDER_ID = -2001;
    class BrightnessSlider : public UISlider {
    public:
        BrightnessSlider(int id, int x, int y, int width, int height, const String& name)
            : UISlider(id, x, y, width, height, 5, 100, 50, "Brightness", name) {}
        void onValueChanged(int newValue) override {
            int brightness = (newValue * 255) / 100;
            if (brightness < 0) brightness = 0;
            if (brightness > 255) brightness = 255;
            M5Cardputer.Display.setBrightness(brightness);
        }
    };
    void ensureBrightnessMenuWidgets() {
        if (!globalUIManager) return;
        UIWidget* panelWidget = globalUIManager->getWidget(BRIGHTNESS_PANEL_ID);
        UIPanel* panel = panelWidget ? static_cast<UIPanel*>(panelWidget) : nullptr;
        UIWidget* sliderWidget = globalUIManager->getWidget(BRIGHTNESS_SLIDER_ID);
        UISlider* sliderBase = sliderWidget ? static_cast<UISlider*>(sliderWidget) : nullptr;
        if (!panel) {
            int screenW = M5Cardputer.Display.width();
            int screenH = M5Cardputer.Display.height();
            if (screenW <= 0) screenW = 240;
            if (screenH <= 0) screenH = 135;
            int panelW = screenW - 60;
            if (panelW < 100) panelW = screenW - 20;
            int panelH = 40;
            int panelX = (screenW - panelW) / 2;
            int panelY = (screenH - panelH) / 2;
            panel = globalUIManager->createPanel(BRIGHTNESS_PANEL_ID, panelX, panelY, panelW, panelH, "BrightnessPanel");
            panel->setBackgroundColor(TFT_BLACK);
            panel->setBorderColor(TFT_WHITE);
        }
        if (!sliderBase && panel) {
            int panelX, panelY, panelW, panelH;
            panel->getBounds(panelX, panelY, panelW, panelH);
            int sliderW = panelW - 20;
            int sliderH = panelH - 16;
            if (sliderH < 12) sliderH = panelH - 8;
            if (sliderH < 8) sliderH = panelH;
            int sliderX = 10;
            int sliderY = (panelH - sliderH) / 2;
            BrightnessSlider* slider = new BrightnessSlider(BRIGHTNESS_SLIDER_ID, sliderX, sliderY, sliderW, sliderH, "BrightnessSlider");
            slider->setParent(panel);
            globalUIManager->addWidget(slider);
            slider->onValueChanged(slider->getValue());
        }
        panelWidget = globalUIManager->getWidget(BRIGHTNESS_PANEL_ID);
        sliderWidget = globalUIManager->getWidget(BRIGHTNESS_SLIDER_ID);
        panel = panelWidget ? static_cast<UIPanel*>(panelWidget) : nullptr;
        sliderBase = sliderWidget ? static_cast<UISlider*>(sliderWidget) : nullptr;
        if (panel) {
            globalUIManager->ensureForeground(panel);
        }
        if (sliderBase) {
            globalUIManager->ensureForeground(sliderBase);
        }
        if (panel) {
            globalUIManager->setWidgetTreeVisible(panel, false);
        }
    }
    bool handleBrightnessMenuKeyEvent(const KeyEvent& event) {
        if (!globalUIManager) return false;
        UIWidget* panelWidget = globalUIManager->getWidget(BRIGHTNESS_PANEL_ID);
        UIPanel* panel = panelWidget ? static_cast<UIPanel*>(panelWidget) : nullptr;
        UIWidget* sliderWidget = globalUIManager->getWidget(BRIGHTNESS_SLIDER_ID);
        UISlider* slider = sliderWidget ? static_cast<UISlider*>(sliderWidget) : nullptr;
        bool panelVisible = panel && panel->isVisible();
        if (event.opt) {
            if (!panel || !slider) {
                ensureBrightnessMenuWidgets();
                panelWidget = globalUIManager->getWidget(BRIGHTNESS_PANEL_ID);
                panel = panelWidget ? static_cast<UIPanel*>(panelWidget) : nullptr;
                sliderWidget = globalUIManager->getWidget(BRIGHTNESS_SLIDER_ID);
                slider = sliderWidget ? static_cast<UISlider*>(sliderWidget) : nullptr;
            }
            if (panel) {
                globalUIManager->ensureForeground(panel);
            }
            if (slider) {
                globalUIManager->ensureForeground(slider);
            }
            if (panel) {
                bool currentlyVisible = panel->isVisible();
                globalUIManager->setWidgetTreeVisible(panel, !currentlyVisible);
                globalUIManager->rebuildForegroundFocus();
                globalUIManager->smartRefresh();
            }
            return true;
        }
        if (panelVisible && slider) {
            bool handled = false;
            int value = slider->getValue();
            if (event.left) {
                value -= 5;
                if (value < 5) value = 5;
                slider->setValue(value);
                handled = true;
            } else if (event.right) {
                value += 5;
                if (value > 100) value = 100;
                slider->setValue(value);
                handled = true;
            }
            if (event.enter || event.esc) {
                globalUIManager->setWidgetTreeVisible(panel, false);
                globalUIManager->rebuildForegroundFocus();
                globalUIManager->smartRefresh();
                handled = true;
            }
            if (handled) {
                return true;
            }
        }
        return false;
    }
    
public:
    AppManager(EventSystem* events) : appCount(0), currentApp(nullptr), launcherApp(nullptr), eventSystem(events), globalEscEnabled(true) {
        for (int i = 0; i < 10; i++) {
            apps[i] = nullptr;
        }
        globalUIManager = new UIManager();
        globalSDManager = new SDFileManager();
    }
    
    ~AppManager() {
        clear();
        delete globalUIManager;
        delete globalSDManager;
    }
    
    UIManager* getUIManager() {
        return globalUIManager;
    }

    SDFileManager* getSDFileManager() {
        return globalSDManager;
    }

    void setGlobalEscEnabled(bool enabled) {
        globalEscEnabled = enabled;
    }

    bool isGlobalEscEnabled() const {
        return globalEscEnabled;
    }

    bool initializeSD() {
        return globalSDManager ? globalSDManager->initialize() : false;
    }
    
    // 注册应用
    bool registerApp(const String& name, const String& displayName, App* app, bool isLauncher = false) {
        if (appCount >= 10 || app == nullptr) {
            return false;
        }
        
        // 设置应用的管理器引用
        app->setManagers(globalUIManager, this);
        
        apps[appCount] = new AppInfo(name, displayName, app, isLauncher);
        
        // 如果是启动器应用，记录引用
        if (isLauncher) {
            launcherApp = app;
        }
        
        appCount++;
        return true;
    }
    
    // 获取应用数量（不包括启动器）
    int getAppCount() const {
        int count = 0;
        for (int i = 0; i < appCount; i++) {
            if (apps[i] && !apps[i]->isLauncher) {
                count++;
            }
        }
        return count;
    }
    
    // 获取应用列表（不包括启动器）
    void getAppList(AppInfo** appList, int& count) const {
        count = 0;
        for (int i = 0; i < appCount; i++) {
            if (apps[i] && !apps[i]->isLauncher) {
                appList[count] = apps[i];
                count++;
            }
        }
    }
    
    // 根据名称查找应用
    AppInfo* findApp(const String& name) const {
        for (int i = 0; i < appCount; i++) {
            if (apps[i] && apps[i]->name == name) {
                return apps[i];
            }
        }
        return nullptr;
    }
    
    // 启动应用
    bool launchApp(const String& name) {
        AppInfo* appInfo = findApp(name);
        if (appInfo && appInfo->instance) {
            // 使用全局UI管理器切换到新应用
            globalUIManager->switchToApp();
            currentApp = appInfo->instance;
            currentApp->setup();
            globalUIManager->finishAppSetup();
            return true;
        }
        return false;
    }
    
    // 返回启动器
    void returnToLauncher() {
        if (launcherApp) {
            // 使用全局UI管理器切换到启动器（保持背景层）
            globalUIManager->switchToLauncher();
            currentApp = launcherApp;
            globalEscEnabled = true;
            // 不需要重新setup，因为启动器窗口已经在背景层
        }
    }
    
    // 获取当前应用
    App* getCurrentApp() const {
        return currentApp;
    }
    
    // 更新当前应用
    void update() {
        if (currentApp) {
            currentApp->loop();
        }
        if (globalUIManager) {
            globalUIManager->tick();
        }
    }
    
    // 处理键盘事件
    void handleKeyEvent(const KeyEvent& event) {
        if (handleBrightnessMenuKeyEvent(event)) {
            return;
        }
        // 全局ESC键处理：如果当前不是启动器应用，ESC键退出到启动器
        if (event.esc && globalEscEnabled && currentApp && currentApp != launcherApp) {
            returnToLauncher();
            return;
        }
        
        if (currentApp) {
            currentApp->onKeyEvent(event);
        }
    }
    
    // 初始化（启动启动器）
    void initialize() {
        if (launcherApp) {
            // 使用全局UI管理器初始化启动器
            globalUIManager->switchToApp();
            currentApp = launcherApp;
            currentApp->setup();
            globalUIManager->finishAppSetup();
        }
    }
    
private:
    void clear() {
        for (int i = 0; i < appCount; i++) {
            if (apps[i]) {
                delete apps[i];
                apps[i] = nullptr;
            }
        }
        appCount = 0;
        currentApp = nullptr;
        launcherApp = nullptr;
    }
};
