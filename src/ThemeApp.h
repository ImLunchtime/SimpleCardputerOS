#pragma once
#include "App.h"
#include "UIManager.h"
#include "EventSystem.h"
#include "AppManager.h"
#include "ThemeManager.h"
#include "PrototypeTheme.h"
#include "DarkTheme.h"

class ThemeApp : public App {
private:
    EventSystem* eventSystem;
    
    // 控件ID定义
    enum ControlIds {
        STATUS_LABEL_ID = 2,
        THEME_MENU_ID = 3,
        WINDOW_ID = 5,
        PREVIEW_LABEL_ID = 6
    };
    
    // 控件引用
    UILabel* statusLabel;
    UILabel* previewLabel;
    UIMenuList* themeMenu;
    UIWindow* mainWindow;
    
    // 自定义主题菜单
    class ThemeMenuList : public UIMenuList {
    public:
        ThemeMenuList(int id, int x, int y, int width, int height, const String& name, int itemHeight, ThemeApp* app)
            : UIMenuList(id, x, y, width, height, name, itemHeight), parentApp(app) {}
        
        void onItemSelected(MenuItem* item) override {
            parentApp->handleThemeSelection(item);
        }
        
    private:
        ThemeApp* parentApp;
    };

public:
    ThemeApp(EventSystem* events) 
        : eventSystem(events) {}

    void setup() override {
        // 创建主窗口 - 更小的窗口位于指定位置，参考设置App的布局
        mainWindow = new UIWindow(1, 30, 20, 180, 100, "Theme Settings");
        uiManager->addWidget(mainWindow);
        // 矫正控件位置：当前控件整体偏右下约24px，向左上移动24px
        mainWindow->setChildOffset(-30, -30);
        
        // 创建状态标签（设置父为主窗口）
        statusLabel = new UILabel(2, 35, 35, "Theme Manager");
        statusLabel->setParent(mainWindow);
        uiManager->addWidget(statusLabel);
        
        // 创建当前主题预览标签（设置父为主窗口）
        previewLabel = new UILabel(3, 35, 45, "Current: Default");
        previewLabel->setParent(mainWindow);
        uiManager->addWidget(previewLabel);
        
        // 创建主题菜单 - 更紧凑的尺寸（设置父为主窗口）
        themeMenu = new ThemeMenuList(4, 35, 55, 150, 50, "ThemeMenu", 12, this);
        themeMenu->setParent(mainWindow);
        
        // 添加主题选项
        if (globalThemeManager) {
            for (int i = 0; i < globalThemeManager->getThemeCount(); i++) {
                Theme* theme = globalThemeManager->getTheme(i);
                if (theme) {
                    themeMenu->addItem(theme->getThemeName(), 100 + i);
                }
            }
        }
        
        // 设置菜单颜色 - 使用与设置App相似的配色
        themeMenu->setColors(TFT_BLUE, TFT_CYAN, TFT_WHITE, TFT_DARKGREY);
        
        uiManager->addWidget(themeMenu);
        
        updateCurrentThemeStatus();
    }

    void loop() override {
        // 主题相关逻辑可以在这里实现
    }

    void onKeyEvent(const KeyEvent& event) override {
        // 将事件传递给UI管理器处理
        if (uiManager->handleKeyEvent(event)) {
            updateStatus();
            // 使用局部刷新避免闪烁
            uiManager->refreshAppArea();
        }
    }
    
    void handleThemeSelection(MenuItem* item) {
        if (!item || !globalThemeManager) return;
        
        // 查找并设置选中的主题
        for (int i = 0; i < globalThemeManager->getThemeCount(); i++) {
            Theme* theme = globalThemeManager->getTheme(i);
            if (theme && theme->getThemeName() == item->text) {
                globalThemeManager->setCurrentTheme(i);
                updateCurrentThemeStatus();
                
                // 刷新所有UI元素以应用新主题
                if (uiManager) {
                    uiManager->refresh();
                }
                break;
            }
        }
    }

private:
    void updateStatus() {
        UIWidget* focusedWidget = uiManager->getCurrentFocusedWidget();
        if (focusedWidget) {
            String status = "Focus: " + focusedWidget->getName();
            // 只在不是主题选择状态时更新状态
            if (status.indexOf("ThemeMenu") == -1) {
                statusLabel->setText(status);
            }
        }
    }
    
    void updateCurrentThemeStatus() {
        if (!globalThemeManager) return;
        
        Theme* currentTheme = globalThemeManager->getCurrentTheme();
        if (currentTheme) {
            String currentStatus = "Current: " + currentTheme->getThemeName();
            previewLabel->setText(currentStatus);
        } else {
            previewLabel->setText("Current: No theme active");
        }
    }
};