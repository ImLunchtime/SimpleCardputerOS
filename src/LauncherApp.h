#pragma once
#include "App.h"
#include "UIManager.h"
#include "EventSystem.h"
#include "AppManager.h"

class LauncherApp : public App {
private:
    EventSystem* eventSystem;
    
    // 控件ID定义
    enum ControlIds {
        TITLE_LABEL_ID = 1,
        STATUS_LABEL_ID = 2,
        GRID_MENU_ID = 3,
        WINDOW_ID = 4
    };
    
    // 控件引用
    UILabel* titleLabel;
    UILabel* statusLabel;
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
        // 创建主窗口 - 适配240x135屏幕
        mainWindow = uiManager->createWindow(WINDOW_ID, 2, 2, 236, 131, "App Launcher", "MainWindow");
        
        // 创建标题标签
        titleLabel = uiManager->createLabel(TITLE_LABEL_ID, 8, 18, "App Launcher", "Title");
        
        // 创建状态标签
        statusLabel = uiManager->createLabel(STATUS_LABEL_ID, 8, 30, "Select an app to launch", "Status");
        
        // 创建网格菜单 - 3x2网格布局适配小屏幕
        gridMenu = new LauncherMenuGrid(GRID_MENU_ID, 8, 45, 220, 80, 3, 2, "AppGrid", this);
        uiManager->addWidget(gridMenu);
        
        // 从应用管理器获取应用列表并添加到菜单
        loadAppsToMenu();
        
        // 设置菜单颜色
        gridMenu->setColors(TFT_BLUE, TFT_CYAN, TFT_WHITE, TFT_DARKGREY);
    }

    void loop() override {
        // 主循环逻辑（如果需要）
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
        for (int i = 0; i < count && i < 6; i++) { // 最多显示6个应用（3x2网格）
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