#ifndef APP_H
#define APP_H

#include "Graphics.h"
#include "EventSystem.h"

class App {
public:
    virtual ~App() {}
    
    // App生命周期方法
    virtual void setup() = 0;           // 初始化
    virtual void loop() = 0;            // 主循环
    virtual void onKeyEvent(const KeyEvent& event) = 0;  // 处理键盘事件
};

class AppSystem {
private:
    App* currentApp;
    
public:
    AppSystem() : currentApp(nullptr) {}
    
    // 运行App
    void runApp(App* app) {
        currentApp = app;
        if (currentApp) {
            currentApp->setup();
        }
    }
    
    // 更新当前App
    void update() {
        if (currentApp) {
            currentApp->loop();
        }
    }
    
    // 处理键盘事件
    void handleKeyEvent(const KeyEvent& event) {
        if (currentApp) {
            currentApp->onKeyEvent(event);
        }
    }
};

#endif