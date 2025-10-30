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
        STATUS_LABEL_ID = 2,
        MENU_LIST_ID = 3,
        WINDOW_ID = 5
    };
    
    // 控件引用
    UILabel* statusLabel;
    UIMenuList* settingsMenu;
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

public:
    SettingsApp(EventSystem* events) 
        : eventSystem(events) {}

    void setup() override {
        // 创建主窗口 - 更小的窗口位于指定位置
        mainWindow = uiManager->createWindow(WINDOW_ID, 30, 20, 150, 100, "Settings", "MainWindow");
        
        // 创建状态标签
        statusLabel = uiManager->createLabel(STATUS_LABEL_ID, 35, 35, "Select a setting", "Status");
        
        // 创建设置菜单
        settingsMenu = new SettingsMenuList(MENU_LIST_ID, 35, 45, 120, 40, "SettingsMenu", 10, this);
        uiManager->addWidget(settingsMenu);
        
        // 添加设置项
        settingsMenu->addItem("Display", 101);
        settingsMenu->addItem("Sound", 102);
        settingsMenu->addItem("Network", 103);
        settingsMenu->addItem("About", 104);
        
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
            statusLabel->setText("Select a setting");
        }
    }

    void drawInterface() {
        uiManager->refresh();  // 使用新的refresh方法确保完整刷新
    }
};