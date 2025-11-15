#ifndef APP_H
#define APP_H

#include "system/EventSystem.h"

// 前向声明
class UIManager;
class AppManager;

class App {
protected:
    UIManager* uiManager;      // 全局UI管理器引用
    AppManager* appManager;    // App管理器引用
    
public:
    App() : uiManager(nullptr), appManager(nullptr) {}
    virtual ~App() {}
    
    // 设置管理器引用
    void setManagers(UIManager* ui, AppManager* app) {
        uiManager = ui;
        appManager = app;
    }
    
    // App生命周期方法
    virtual void setup() = 0;           // 初始化
    virtual void loop() = 0;            // 主循环
    virtual void onKeyEvent(const KeyEvent& event) = 0;  // 处理键盘事件
};

#endif