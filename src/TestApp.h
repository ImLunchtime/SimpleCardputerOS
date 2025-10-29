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
        TITLE_LABEL_ID = 1,
        STATUS_LABEL_ID = 2,
        COUNTER_LABEL_ID = 3,
        INCREMENT_BUTTON_ID = 4,
        RESET_BUTTON_ID = 5,
        BACK_BUTTON_ID = 6,
        WINDOW_ID = 7
    };
    
    // 控件引用
    UILabel* titleLabel;
    UILabel* statusLabel;
    UILabel* counterLabel;
    UIButton* incrementButton;
    UIButton* resetButton;
    UIButton* backButton;
    UIWindow* mainWindow;
    
    int counter;
    
    // 自定义按钮类
    class TestButton : public UIButton {
    public:
        TestButton(int id, int x, int y, int width, int height, const String& text, const String& name, TestApp* app)
            : UIButton(id, x, y, width, height, text, name), parentApp(app) {}
        
        void onButtonClick() override {
            parentApp->handleButtonClick(getId());
        }
        
    private:
        TestApp* parentApp;
    };

public:
    TestApp(EventSystem* events) 
        : eventSystem(events), counter(0) {}

    void setup() override {
        // 创建主窗口
        mainWindow = uiManager->createWindow(WINDOW_ID, 2, 2, 236, 131, "Test App", "MainWindow");
        
        // 创建标题标签
        titleLabel = uiManager->createLabel(TITLE_LABEL_ID, 8, 18, "Test Application", "Title");
        
        // 创建状态标签
        statusLabel = uiManager->createLabel(STATUS_LABEL_ID, 8, 30, "Counter test app", "Status");
        
        // 创建计数器标签
        counterLabel = uiManager->createLabel(COUNTER_LABEL_ID, 8, 50, "Counter: 0", "Counter");
        
        // 创建增加按钮
        incrementButton = new TestButton(INCREMENT_BUTTON_ID, 8, 70, 60, 20, "+1", "IncrementButton", this);
        uiManager->addWidget(incrementButton);
        
        // 创建重置按钮
        resetButton = new TestButton(RESET_BUTTON_ID, 80, 70, 60, 20, "Reset", "ResetButton", this);
        uiManager->addWidget(resetButton);
        
        // 创建返回按钮
        backButton = new TestButton(BACK_BUTTON_ID, 150, 70, 60, 20, "Back", "BackButton", this);
        uiManager->addWidget(backButton);
    }

    void loop() override {
        // 测试相关逻辑可以在这里实现
    }

    void onKeyEvent(const KeyEvent& event) override {
        // 将事件传递给UI管理器处理
        if (uiManager->handleKeyEvent(event)) {
            drawInterface();
        }
    }
    
    void handleButtonClick(int buttonId) {
        switch (buttonId) {
            case INCREMENT_BUTTON_ID:
                counter++;
                counterLabel->setText("Counter: " + String(counter));
                statusLabel->setText("Incremented to " + String(counter));
                break;
                
            case RESET_BUTTON_ID:
                counter = 0;
                counterLabel->setText("Counter: 0");
                statusLabel->setText("Counter reset");
                break;
                
            case BACK_BUTTON_ID:
                returnToLauncher();
                break;
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