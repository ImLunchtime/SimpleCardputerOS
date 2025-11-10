#pragma once
#include "App.h"
#include "UIManager.h"
#include "EventSystem.h"
#include "AppManager.h"
#include "BatteryManager.h"

class LauncherApp : public App {
private:
    EventSystem* eventSystem;
    BatteryManager batteryManager;  // 电池信息管理器
    
    // 控件ID定义
    enum ControlIds {
        TITLE_LABEL_ID = 1,
        STATUS_LABEL_ID = 2,
        GRID_MENU_ID = 3,
        WINDOW_ID = 4,
        BATTERY_LABEL_ID = 5,  // 电池信息标签ID
        BATTERY_VOLTAGE_LABEL_ID = 6  // 电池电压标签ID
    };
    
    // 控件引用
    UILabel* statusLabel;
    UILabel* batteryLabel;  // 电池信息标签
    UILabel* batteryVoltageLabel;  // 电池电压标签
    UIMenuGrid* gridMenu;
    UIWindow* mainWindow;
    
    // 自定义网格菜单类来处理应用启动
    class LauncherMenuGrid : public UIMenuGrid {
    public:
        LauncherMenuGrid(int id, int x, int y, int width, int height, int columns, int rows, const String& name, LauncherApp* app)
            : UIMenuGrid(id, x, y, width, height, columns, rows, name), parentApp(app) {}
        
        void onItemSelected(MenuItem* item) override {
            parentApp->handleAppSelection(item);
        }
        
    private:
        LauncherApp* parentApp;
    };

public:
    LauncherApp(EventSystem* events) 
        : eventSystem(events) {}

    void setup() override {
        // 初始化电池管理器
        batteryManager.forceUpdate();
        
        // 创建主窗口 - 更小的窗口位于左上角
        mainWindow = uiManager->createWindow(WINDOW_ID, 5, 5, 150, 100, "Launcher", "MainWindow");
        
        // 创建电池信息标签 - 位于窗口右上角
        batteryLabel = uiManager->createLabel(BATTERY_LABEL_ID, 100, 10, batteryManager.getBatteryLevelString(), "BatteryInfo", mainWindow);
        batteryLabel->setTextColor(TFT_GREEN);
        
        // 创建电池电压标签 - 位于电池信息标签下方
        String voltageText = String(batteryManager.getBatteryVoltage()) + "mV";
        batteryVoltageLabel = uiManager->createLabel(BATTERY_VOLTAGE_LABEL_ID, 100, 20, voltageText, "BatteryVoltage", mainWindow);
        batteryVoltageLabel->setTextColor(TFT_CYAN);
        
        // 创建状态标签
        statusLabel = uiManager->createLabel(STATUS_LABEL_ID, 10, 25, "Select app", "Status", mainWindow);
        
        // 创建网格菜单 - 3x2网格布局以显示更多应用
        gridMenu = new LauncherMenuGrid(GRID_MENU_ID, 10, 40, 140, 60, 3, 2, "AppGrid", this);
        gridMenu->setParent(mainWindow);
        uiManager->addWidget(gridMenu);
        
        // 从应用管理器获取应用列表并添加到菜单
        loadAppsToMenu();
        
        // 设置菜单颜色
        gridMenu->setColors(TFT_BLUE, TFT_CYAN, TFT_WHITE, TFT_DARKGREY);
    }

    void loop() override {
        // 更新电池信息
        batteryManager.update();
        
        bool needsRedraw = false;
        
        // 更新电池电量标签显示
        String newBatteryText = batteryManager.getBatteryLevelString();
        if (batteryLabel->getText() != newBatteryText) {
            batteryLabel->setText(newBatteryText);
            // 根据电池电量设置颜色
            int level = batteryManager.getBatteryLevel();
            if (level > 50) {
                batteryLabel->setTextColor(TFT_GREEN);
            } else if (level > 20) {
                batteryLabel->setTextColor(TFT_YELLOW);
            } else {
                batteryLabel->setTextColor(TFT_RED);
            }
            // 使用局部重绘更新电池电量标签
            //uiManager->drawWidgetPartial(BATTERY_LABEL_ID);
            needsRedraw = true;
        }
        
        // 如果有前景层窗口，使用局部重绘避免闪烁
        if (needsRedraw && uiManager) {
            // 检查是否有前景层，如果有则使用局部重绘
            // 这里我们不需要调用drawInterface()，因为已经使用了局部重绘
        }
    }

    void onKeyEvent(const KeyEvent& event) override {
        // 将事件传递给UI管理器处理
        if (uiManager->handleKeyEvent(event)) {
            // 如果事件被处理，更新状态并重绘
            updateStatus();
            drawInterface();
        }
    }

    void handleAppSelection(MenuItem* item) {
        // 根据菜单项ID查找对应的应用
        AppInfo* appList[10];
        int count;
        appManager->getAppList(appList, count);
        
        for (int i = 0; i < count; i++) {
            if (appList[i] && item->id == (i + 100)) { // 应用ID从100开始
                String message = "Launching: " + appList[i]->displayName;
                statusLabel->setText(message);
                drawInterface();
                
                // 延迟一下让用户看到反馈
                delay(500);
                
                // 启动选中的应用
                appManager->launchApp(appList[i]->name);
                return;
            }
        }
        
        statusLabel->setText("App not found!");
    }

private:
    void loadAppsToMenu() {
        // 清空现有菜单项
        gridMenu->clear();
        
        // 从应用管理器获取应用列表
        AppInfo* appList[10];
        int count;
        appManager->getAppList(appList, count);
        
        // 添加应用到网格菜单
        for (int i = 0; i < count && i < 4; i++) { // 最多显示4个应用（2x2网格）
            if (appList[i]) {
                gridMenu->addItem(appList[i]->displayName, i + 100); // 应用ID从100开始
            }
        }
        
        // 如果没有应用，显示提示
        if (count == 0) {
            gridMenu->addItem("No Apps", 999, false); // 禁用状态
        }
    }
    
    void updateStatus() {
        UIWidget* focusedWidget = uiManager->getCurrentFocusedWidget();
        if (focusedWidget) {
            String status = "Focus: " + focusedWidget->getName();
            if (focusedWidget->hasSecondaryFocus()) {
                status = "Navigate: " + focusedWidget->getName();
            }
            statusLabel->setText(status);
        } else {
            statusLabel->setText("Select an app to launch");
        }
    }

    void drawInterface() {
        uiManager->refresh();  // 使用新的refresh方法确保完整刷新
    }
};