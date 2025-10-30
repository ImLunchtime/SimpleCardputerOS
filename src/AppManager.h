#pragma once
#include <M5Cardputer.h>
#include "App.h"
#include "EventSystem.h"
#include "UIManager.h"

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
    AppInfo* apps[10];      // 最多支持10个应用
    int appCount;
    App* currentApp;
    App* launcherApp;
    EventSystem* eventSystem;
    UIManager* globalUIManager;  // 全局UI管理器
    
public:
    AppManager(EventSystem* events) : appCount(0), currentApp(nullptr), launcherApp(nullptr), eventSystem(events) {
        for (int i = 0; i < 10; i++) {
            apps[i] = nullptr;
        }
        globalUIManager = new UIManager();  // 创建全局UI管理器
    }
    
    ~AppManager() {
        clear();
        delete globalUIManager;
    }
    
    // 获取全局UI管理器
    UIManager* getUIManager() {
        return globalUIManager;
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
    }
    
    // 处理键盘事件
    void handleKeyEvent(const KeyEvent& event) {
        // 全局ESC键处理：如果当前不是启动器应用，ESC键退出到启动器
        if (event.esc && currentApp && currentApp != launcherApp) {
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