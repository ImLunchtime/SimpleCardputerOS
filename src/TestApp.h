#pragma once
#include "App.h"
#include "UIManager.h"
#include "EventSystem.h"
#include "AppManager.h"

class TestApp : public App {
private:
    EventSystem* eventSystem;
    
    // 控件ID定义
    enum ControlIds {
        STATUS_LABEL_ID = 2,
        INFO_LABEL_ID = 3,
        WINDOW_ID = 5
    };
    
    // 控件引用
    UILabel* statusLabel;
    UILabel* infoLabel;
    UIWindow* mainWindow;
    
public:
    TestApp(EventSystem* events) 
        : eventSystem(events) {}

    void setup() override {
        // 创建主窗口 - 更小的窗口位于指定位置
        mainWindow = uiManager->createWindow(WINDOW_ID, 30, 20, 150, 100, "Test", "MainWindow");
        
        // 创建状态标签
        statusLabel = uiManager->createLabel(STATUS_LABEL_ID, 35, 35, "Test app ready", "Status");
        
        // 创建信息标签
        infoLabel = uiManager->createLabel(INFO_LABEL_ID, 35, 50, "Press ESC to exit", "Info");
    }

    void loop() override {
        // nope
    }

    void onKeyEvent(const KeyEvent& event) override {
        // 将事件传递给UI管理器处理
        if (uiManager->handleKeyEvent(event)) {
            // 使用局部刷新避免闪烁
            uiManager->refreshAppArea();
        }
    }

private:
    void drawInterface() {
        // 使用智能刷新，根据是否有前景层选择合适的刷新方式
        uiManager->smartRefresh();
    }
};