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
        LIST_MENU_ID = 3,
        GRID_MENU_ID = 4,
        WINDOW_ID = 5
    };
    
    // 控件引用
    UILabel* titleLabel;
    UILabel* statusLabel;
    UIMenuList* listMenu;
    UIMenuGrid* gridMenu;
    UIWindow* mainWindow;
    
    // 自定义菜单类来处理菜单项选择
    class CustomMenuList : public UIMenuList {
    public:
        CustomMenuList(int id, int x, int y, int width, int height, const String& name, int itemHeight, NewTestApp* app)
            : UIMenuList(id, x, y, width, height, name, itemHeight), parentApp(app) {}
        
        void onItemSelected(MenuItem* item) override {
            parentApp->handleListMenuSelection(item);
        }
        
    private:
        NewTestApp* parentApp;
    };
    
    class CustomMenuGrid : public UIMenuGrid {
    public:
        CustomMenuGrid(int id, int x, int y, int width, int height, int columns, int rows, const String& name, NewTestApp* app)
            : UIMenuGrid(id, x, y, width, height, columns, rows, name), parentApp(app) {}
        
        void onItemSelected(MenuItem* item) override {
            parentApp->handleGridMenuSelection(item);
        }
        
    private:
        NewTestApp* parentApp;
    };

public:
    NewTestApp(EventSystem* events) : eventSystem(events) {}

    void setup() override {
        // 创建主窗口 - 适配240x135屏幕
        mainWindow = uiManager.createWindow(WINDOW_ID, 2, 2, 236, 131, "Menu Test", "MainWindow");
        
        // 创建标题标签 - 缩小字体和位置
        titleLabel = uiManager.createLabel(TITLE_LABEL_ID, 8, 18, "Menu Test", "Title");
        
        // 创建状态标签 - 简化文本
        statusLabel = uiManager.createLabel(STATUS_LABEL_ID, 8, 30, "Tab: switch, Arrows: navigate", "Status");
        
        // 创建列表菜单 - 调整尺寸适配小屏幕
        listMenu = new CustomMenuList(LIST_MENU_ID, 8, 45, 100, 80, "ListMenu", 16, this);
        uiManager.addWidget(listMenu);
        
        // 添加列表菜单项 - 减少项目数量
        listMenu->addItem("File", 101);
        listMenu->addItem("Edit", 102);
        listMenu->addItem("View", 103);
        listMenu->addItem("Tools", 104);
        listMenu->addItem("Exit", 105);
        
        // 创建网格菜单 - 调整尺寸和网格布局
        gridMenu = new CustomMenuGrid(GRID_MENU_ID, 120, 45, 110, 80, 2, 3, "GridMenu", this);
        uiManager.addWidget(gridMenu);
        
        // 添加网格菜单项 - 适配2x3网格
        gridMenu->addItem("App1", 201);
        gridMenu->addItem("App2", 202);
        gridMenu->addItem("Game", 203);
        gridMenu->addItem("Music", 204);
        gridMenu->addItem("Photo", 205);
        gridMenu->addItem("Set", 206);
        
        // 设置菜单颜色
        listMenu->setColors(TFT_CYAN, TFT_YELLOW, TFT_WHITE, TFT_DARKGREY);
        gridMenu->setColors(TFT_MAGENTA, TFT_GREEN, TFT_WHITE, TFT_DARKGREY);
        
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

    void handleListMenuSelection(MenuItem* item) {
        String message = "List Menu: Selected '" + item->text + "' (ID: " + String(item->id) + ")";
        statusLabel->setText(message);
        
        // 根据选择的菜单项执行不同操作
        switch (item->id) {
            case 101: // File
                // 处理文件菜单
                break;
            case 102: // Edit
                // 处理编辑菜单
                break;
            case 103: // View
                // 处理视图菜单
                break;
            case 104: // Tools
                // 处理工具菜单
                break;
            case 105: // Exit
                // 处理退出菜单
                break;
        }
    }
    
    void handleGridMenuSelection(MenuItem* item) {
        String message = "Grid Menu: Selected '" + item->text + "' (ID: " + String(item->id) + ")";
        statusLabel->setText(message);
        
        // 根据选择的菜单项执行不同操作
        switch (item->id) {
            case 201: // App1
                // 启动应用1
                break;
            case 202: // App2
                // 启动应用2
                break;
            case 203: // Game
                // 启动游戏
                break;
            case 204: // Music
                // 启动音乐播放器
                break;
            case 205: // Photo
                // 启动照片查看器
                break;
            case 206: // Set
                // 启动设置
                break;
        }
    }

private:
    void updateStatus() {
        UIWidget* focusedWidget = uiManager.getCurrentFocusedWidget();
        if (focusedWidget) {
            String status = "Focus: " + focusedWidget->getName();
            if (focusedWidget->hasSecondaryFocus()) {
                status = "Navigate: " + focusedWidget->getName();
            }
            statusLabel->setText(status);
        } else {
            statusLabel->setText("No focus");
        }
    }

    void drawInterface() {
        uiManager.clearScreen();
        uiManager.drawAll();
    }
};