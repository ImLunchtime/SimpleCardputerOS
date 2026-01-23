#pragma once
#include <M5Cardputer.h>
#include "system/App.h"
#include "system/EventSystem.h"
#include "ui/UIManager.h"
#include "system/SDFileManager.h"
#include "system/SleepManager.h"
#include "system/TipManager.h"
#include "themes/ThemeManager.h"
#include <mooncake.h>

extern int g_displayBrightness;
extern ThemeManager* globalThemeManager;

// 应用信息结构
struct AppInfo {
    String name;        // 应用名称
    String displayName; // 显示名称
    App* instance;      // 应用实例
    bool isLauncher;    // 是否为启动器应用
    int id;             // Mooncake ID
    
    AppInfo(const String& _name, const String& _displayName, App* _instance, bool _isLauncher = false, int _id = -1)
        : name(_name), displayName(_displayName), instance(_instance), isLauncher(_isLauncher), id(_id) {}
};

class AppManager {
private:
    mooncake::Mooncake mooncake;
    AppInfo* apps[10];
    int appCount;
    App* currentApp;
    int currentAppID;
    App* launcherApp;
    int launcherAppID;
    EventSystem* eventSystem;
    UIManager* globalUIManager;
    SDFileManager* globalSDManager;
    bool globalEscEnabled;
    TipManager* tipManager;
    
public:
    AppManager(EventSystem* events) : appCount(0), currentApp(nullptr), currentAppID(-1), launcherApp(nullptr), launcherAppID(-1), eventSystem(events), globalEscEnabled(true), tipManager(nullptr) {
        for (int i = 0; i < 10; i++) {
            apps[i] = nullptr;
        }
        globalUIManager = new UIManager();
        globalSDManager = new SDFileManager();
        tipManager = new TipManager(globalUIManager);
    }
    
    ~AppManager() {
        clear();
        delete globalUIManager;
        delete globalSDManager;
        delete tipManager;
    }
    
    UIManager* getUIManager() {
        return globalUIManager;
    }

    TipManager* getTipManager() {
        return tipManager;
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

    bool loadSystemConfig() {
        SDFileManager* fm = getSDFileManager();
        if (!fm || !fm->isInitialized()) {
            return false;
        }

        const String path = "/scos_config.json";
        if (!fm->exists(path)) {
            return false;
        }

        String content = fm->readFile(path);
        if (content.length() == 0) {
            return false;
        }

        int newBrightness = g_displayBrightness;
        String brightnessKey = "\"brightness\"";
        int bPos = content.indexOf(brightnessKey);
        if (bPos >= 0) {
            int colonPos = content.indexOf(':', bPos + brightnessKey.length());
            if (colonPos >= 0) {
                int valueStart = colonPos + 1;
                while (valueStart < (int)content.length() && (content[valueStart] == ' ' || content[valueStart] == '\t')) {
                    valueStart++;
                }
                int valueEnd = valueStart;
                while (valueEnd < (int)content.length() && content[valueEnd] >= '0' && content[valueEnd] <= '9') {
                    valueEnd++;
                }
                if (valueEnd > valueStart) {
                    String numStr = content.substring(valueStart, valueEnd);
                    int parsed = numStr.toInt();
                    if (parsed >= 0 && parsed <= 255) {
                        newBrightness = parsed;
                    }
                }
            }
        }

        g_displayBrightness = newBrightness;
        M5Cardputer.Display.setBrightness(g_displayBrightness);

        String themeKey = "\"theme\"";
        int tPos = content.indexOf(themeKey);
        if (tPos >= 0 && globalThemeManager) {
            int colonPos = content.indexOf(':', tPos + themeKey.length());
            if (colonPos >= 0) {
                int quoteStart = content.indexOf('"', colonPos + 1);
                if (quoteStart >= 0) {
                    int quoteEnd = content.indexOf('"', quoteStart + 1);
                    if (quoteEnd > quoteStart) {
                        String themeName = content.substring(quoteStart + 1, quoteEnd);
                        themeName.trim();
                        if (themeName.length() > 0) {
                            globalThemeManager->setCurrentTheme(themeName);
                        }
                    }
                }
            }
        }

        return true;
    }

    bool saveSystemConfig() {
        SDFileManager* fm = getSDFileManager();
        if (!fm || !fm->isInitialized()) {
            return false;
        }

        String themeName = "";
        if (globalThemeManager) {
            Theme* currentTheme = globalThemeManager->getCurrentTheme();
            if (currentTheme) {
                themeName = currentTheme->getThemeName();
            }
        }

        String content = "{\n";
        content += "  \"brightness\": " + String(g_displayBrightness) + ",\n";
        content += "  \"theme\": \"" + themeName + "\"\n";
        content += "}\n";

        return fm->writeFile("/scos_config.json", content);
    }
    
    // 注册应用
    bool registerApp(const String& name, const String& displayName, App* app, bool isLauncher = false) {
        if (appCount >= 10 || app == nullptr) {
            return false;
        }
        
        // 设置应用的管理器引用
        app->setManagers(globalUIManager, this);
        app->setAppInfo().name = name.c_str();
        
        // Mooncake takes ownership
        int id = mooncake.installApp(std::unique_ptr<mooncake::AppAbility>(app));
        
        apps[appCount] = new AppInfo(name, displayName, app, isLauncher, id);
        
        // 如果是启动器应用，记录引用
        if (isLauncher) {
            launcherApp = app;
            launcherAppID = id;
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
            
            // 如果已经在运行其他应用，先关闭
            if (currentAppID != -1 && currentAppID != launcherAppID) {
                mooncake.closeApp(currentAppID);
            }
            
            // 使用全局UI管理器切换到新应用
            globalUIManager->switchToApp();
            
            mooncake.openApp(appInfo->id);
            currentApp = appInfo->instance;
            currentAppID = appInfo->id;
            
            globalUIManager->finishAppSetup();
            return true;
        }
        return false;
    }
    
    // 返回启动器
    void returnToLauncher() {
        if (launcherApp) {
            if (currentAppID != -1 && currentAppID != launcherAppID) {
                mooncake.closeApp(currentAppID);
            }
            
            // 使用全局UI管理器切换到启动器（保持背景层）
            globalUIManager->switchToLauncher();
            
            // 启动器通常一直运行或重新打开
             // 这里我们确保它被打开
             if (mooncake.getAppCurrentState(launcherAppID) == mooncake::AppAbility::StateSleeping) {
                 mooncake.openApp(launcherAppID);
             }
            
            currentApp = launcherApp;
            currentAppID = launcherAppID;
            globalEscEnabled = true;
        }
    }
    
    // 获取当前应用
    App* getCurrentApp() const {
        return currentApp;
    }
    
    // 更新当前应用
    void update() {
        mooncake.update();
        
        if (globalUIManager && !globalSleepManager.isSleeping()) {
            globalUIManager->tick();
        }
        if (tipManager && !globalSleepManager.isSleeping()) {
            tipManager->update();
        }
    }
    
    // 处理键盘事件
    void handleKeyEvent(const KeyEvent& event) {
        if (globalSleepManager.isSleeping()) {
            return;
        }
        if (tipManager && tipManager->handleKeyEvent(event)) {
            return;
        }
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
            
            mooncake.openApp(launcherAppID);
            currentApp = launcherApp;
            currentAppID = launcherAppID;
            
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
        currentAppID = -1;
        launcherApp = nullptr;
        launcherAppID = -1;
        // Mooncake destructor will clean up apps
    }
};
