#pragma once
#include "App.h"
#include "UIManager.h"
#include "EventSystem.h"
#include "AppManager.h"

class SettingsApp : public App {
private:
    EventSystem* eventSystem;
    
    // 控件ID定义
    enum ControlIds {
        TITLE_LABEL_ID = 1,
        STATUS_LABEL_ID = 2,
        MENU_LIST_ID = 3,
        BACK_BUTTON_ID = 4,
        WINDOW_ID = 5
    };
    
    // 控件引用
    UILabel* titleLabel;
    UILabel* statusLabel;
    UIMenuList* settingsMenu;
    UIButton* backButton;
    UIWindow* mainWindow;
    
    // 自定义设置菜单
    class SettingsMenuList : public UIMenuList {
    public:
        SettingsMenuList(int id, int x, int y, int width, int height, const String& name, int itemHeight, SettingsApp* app)
            : UIMenuList(id, x, y, width, height, name, itemHeight), parentApp(app) {}
        
        void onItemSelected(MenuItem* item) override {
            parentApp->handleSettingSelection(item);
        }
        
    private:
        SettingsApp* parentApp;
    };
    
    // 自定义返回按钮
    class BackButton : public UIButton {
    public:
        BackButton(int id, int x, int y, int width, int height, const String& text, const String& name, SettingsApp* app)
            : UIButton(id, x, y, width, height, text, name), parentApp(app) {}
        
        void onButtonClick() override {
            parentApp->returnToLauncher();
        }
        
    private:
        SettingsApp* parentApp;
    };

public:
    SettingsApp(EventSystem* events) 
        : eventSystem(events) {}

    void setup() override {
        // 创建主窗口
        mainWindow = uiManager->createWindow(WINDOW_ID, 2, 2, 236, 131, "Settings", "MainWindow");
        
        // 创建标题标签
        titleLabel = uiManager->createLabel(TITLE_LABEL_ID, 8, 18, "Settings", "Title");
        
        // 创建状态标签
        statusLabel = uiManager->createLabel(STATUS_LABEL_ID, 8, 30, "Select a setting to configure", "Status");
        
        // 创建设置菜单
        settingsMenu = new SettingsMenuList(MENU_LIST_ID, 8, 45, 150, 60, "SettingsMenu", 15, this);
        uiManager->addWidget(settingsMenu);
        
        // 添加设置项
        settingsMenu->addItem("Display", 101);
        settingsMenu->addItem("Sound", 102);
        settingsMenu->addItem("Network", 103);
        settingsMenu->addItem("About", 104);
        
        // 创建返回按钮
        backButton = new BackButton(BACK_BUTTON_ID, 170, 90, 60, 20, "Back", "BackButton", this);
        uiManager->addWidget(backButton);
        
        // 设置菜单颜色
        settingsMenu->setColors(TFT_GREEN, TFT_YELLOW, TFT_WHITE, TFT_DARKGREY);
    }

    void loop() override {
        // 设置相关逻辑可以在这里实现
    }

    void onKeyEvent(const KeyEvent& event) override {
        // 将事件传递给UI管理器处理
        if (uiManager->handleKeyEvent(event)) {
            updateStatus();
            drawInterface();
        }
    }
    
    void handleSettingSelection(MenuItem* item) {
        String message = "Selected: " + item->text;
        statusLabel->setText(message);
        
        // 根据选择的设置项执行不同操作
        switch (item->id) {
            case 101: // Display
                statusLabel->setText("Display settings (placeholder)");
                break;
            case 102: // Sound
                statusLabel->setText("Sound settings (placeholder)");
                break;
            case 103: // Network
                statusLabel->setText("Network settings (placeholder)");
                break;
            case 104: // About
                statusLabel->setText("CardputerOS v1.0 (placeholder)");
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
    void updateStatus() {
        UIWidget* focusedWidget = uiManager->getCurrentFocusedWidget();
        if (focusedWidget) {
            String status = "Focus: " + focusedWidget->getName();
            if (focusedWidget->hasSecondaryFocus()) {
                status = "Navigate: " + focusedWidget->getName();
            }
            statusLabel->setText(status);
        } else {
            statusLabel->setText("Select a setting to configure");
        }
    }

    void drawInterface() {
        uiManager->refresh();  // 使用新的refresh方法确保完整刷新
    }
};