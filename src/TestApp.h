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
        BACK_BUTTON_ID = 4,
        WINDOW_ID = 5
    };
    
    // 控件引用
    UILabel* statusLabel;
    UILabel* infoLabel;
    UIButton* backButton;
    UIWindow* mainWindow;
    
    // 自定义返回按钮
    class BackButton : public UIButton {
    public:
        BackButton(int id, int x, int y, int width, int height, const String& text, const String& name, TestApp* app)
            : UIButton(id, x, y, width, height, text, name), parentApp(app) {}
        
        void onButtonClick() override {
            parentApp->returnToLauncher();
        }
        
    private:
        TestApp* parentApp;
    };

public:
    TestApp(EventSystem* events) 
        : eventSystem(events) {}

    void setup() override {
        // 创建主窗口 - 更小的窗口位于指定位置
        mainWindow = uiManager->createWindow(WINDOW_ID, 20, 20, 150, 100, "Test", "MainWindow");
        
        // 创建状态标签
        statusLabel = uiManager->createLabel(STATUS_LABEL_ID, 25, 35, "Test app ready", "Status");
        
        // 创建信息标签
        infoLabel = uiManager->createLabel(INFO_LABEL_ID, 25, 50, "Test app placeholder", "Info");
        
        // 创建返回按钮
        backButton = new BackButton(BACK_BUTTON_ID, 25, 85, 80, 20, "Back", "BackButton", this);
        uiManager->addWidget(backButton);
    }

    void loop() override {
        // nope
    }

    void onKeyEvent(const KeyEvent& event) override {
        // 将事件传递给UI管理器处理
        if (uiManager->handleKeyEvent(event)) {
            drawInterface();
        }
    }
    
    void returnToLauncher() {
        statusLabel->setText("Returning to launcher...");
        drawInterface();
        delay(300);
        appManager->returnToLauncher();
    }

private:
    void drawInterface() {
        uiManager->refresh();  // 使用新的refresh方法确保完整刷新
    }
};