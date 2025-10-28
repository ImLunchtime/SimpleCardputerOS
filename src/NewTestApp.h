#pragma once
#include "App.h"
#include "UIManager.h"
#include "EventSystem.h"

class NewTestApp : public App {
private:
    UIManager uiManager;
    EventSystem* eventSystem;
    
    // 控件ID定义
    enum ControlIds {
        TITLE_LABEL_ID = 1,
        STATUS_LABEL_ID = 2,
        BUTTON1_ID = 3,
        BUTTON2_ID = 4,
        BUTTON3_ID = 5,
        INPUT_ID = 6,
        WINDOW_ID = 7
    };
    
    // 控件引用（便于访问）
    UILabel* titleLabel;
    UILabel* statusLabel;
    UIButton* button1;
    UIButton* button2;
    UIButton* button3;
    UIInput* inputField;
    UIWindow* mainWindow;

public:
    NewTestApp(EventSystem* events) : eventSystem(events) {}

    void setup() override {
        // 创建主窗口
        mainWindow = uiManager.createWindow(WINDOW_ID, 10, 10, 220, 110, "CardputerOS Test", "MainWindow");
        
        // 创建标题标签
        titleLabel = uiManager.createLabel(TITLE_LABEL_ID, 20, 30, "CardputerOS UI Library Demo", "Title");
        
        // 创建状态标签
        statusLabel = uiManager.createLabel(STATUS_LABEL_ID, 20, 45, "Focus: None", "Status");
        
        // 创建按钮
        button1 = uiManager.createButton(BUTTON1_ID, 20, 60, 60, 20, "Button1", "Button1");
        button2 = uiManager.createButton(BUTTON2_ID, 90, 60, 60, 20, "Button2", "Button2");
        button3 = uiManager.createButton(BUTTON3_ID, 160, 60, 60, 20, "Button3", "Button3");
        
        // 创建输入框
        inputField = uiManager.createInput(INPUT_ID, 20, 90, 200, 20, "Enter text here...", "Input");
        
        // 设置按钮点击处理
        setupButtonHandlers();
        
        // 初始绘制
        drawInterface();
    }

    void loop() override {
        // 主循环逻辑（如果需要）
    }

    void onKeyEvent(const KeyEvent& event) override {
        // 将事件传递给UI管理器处理
        if (uiManager.handleKeyEvent(event)) {
            // 如果事件被处理，更新状态并重绘
            updateStatus();
            drawInterface();
        }
    }

private:
    void setupButtonHandlers() {
        // 创建自定义按钮类来处理点击事件
        class CustomButton1 : public UIButton {
        public:
            CustomButton1(int id, int x, int y, int width, int height, const String& text, const String& name, NewTestApp* app)
                : UIButton(id, x, y, width, height, text, name), parentApp(app) {}
            
            void onButtonClick() override {
                parentApp->handleButton1Click();
            }
            
        private:
            NewTestApp* parentApp;
        };
        
        class CustomButton2 : public UIButton {
        public:
            CustomButton2(int id, int x, int y, int width, int height, const String& text, const String& name, NewTestApp* app)
                : UIButton(id, x, y, width, height, text, name), parentApp(app) {}
            
            void onButtonClick() override {
                parentApp->handleButton2Click();
            }
            
        private:
            NewTestApp* parentApp;
        };
        
        class CustomButton3 : public UIButton {
        public:
            CustomButton3(int id, int x, int y, int width, int height, const String& text, const String& name, NewTestApp* app)
                : UIButton(id, x, y, width, height, text, name), parentApp(app) {}
            
            void onButtonClick() override {
                parentApp->handleButton3Click();
            }
            
        private:
            NewTestApp* parentApp;
        };
        
        // 替换默认按钮为自定义按钮
        uiManager.removeWidget(BUTTON1_ID);
        uiManager.removeWidget(BUTTON2_ID);
        uiManager.removeWidget(BUTTON3_ID);
        
        button1 = new CustomButton1(BUTTON1_ID, 20, 60, 60, 20, "Button1", "Button1", this);
        button2 = new CustomButton2(BUTTON2_ID, 90, 60, 60, 20, "Button2", "Button2", this);
        button3 = new CustomButton3(BUTTON3_ID, 160, 60, 60, 20, "Button3", "Button3", this);
        
        uiManager.addWidget(button1);
        uiManager.addWidget(button2);
        uiManager.addWidget(button3);
    }
    
    void handleButton1Click() {
        titleLabel->setText("Button 1 Clicked!");
        drawInterface();
    }
    
    void handleButton2Click() {
        titleLabel->setText("Button 2 Clicked!");
        drawInterface();
    }
    
    void handleButton3Click() {
        titleLabel->setText("Button 3 Clicked!");
        drawInterface();
    }
    
    void updateStatus() {
        UIWidget* focusedWidget = uiManager.getCurrentFocusedWidget();
        if (focusedWidget) {
            String status = "Focus: " + focusedWidget->getName();
            if (focusedWidget->getType() == WIDGET_INPUT) {
                UIInput* input = static_cast<UIInput*>(focusedWidget);
                status += " [" + input->getText() + "]";
            }
            statusLabel->setText(status);
        } else {
            statusLabel->setText("Focus: None");
        }
    }
    
    void drawInterface() {
        uiManager.clearScreen();
        uiManager.drawAll();
    }
};