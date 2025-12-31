#pragma once
#include "system/App.h"
#include "ui/UIManager.h"
#include "system/EventSystem.h"
#include "system/AppManager.h"
#include <M5Cardputer.h>
#include "themes/ThemeManager.h"

extern int g_displayBrightness;

class SettingsApp : public App {
private:
    EventSystem* eventSystem;
    
    // 控件ID定义
    enum ControlIds {
        STATUS_LABEL_ID = 2,
        MENU_LIST_ID = 3,
        WINDOW_ID = 5,
        BRIGHTNESS_WINDOW_ID = 10,
        BRIGHTNESS_SLIDER_ID = 11,
        BRIGHTNESS_LABEL_ID = 12,
        THEME_WINDOW_ID = 20,
        THEME_PREVIEW_LABEL_ID = 21,
        THEME_MENU_ID = 22,
        ABOUT_WINDOW_ID = 30,
        ABOUT_TITLE_LABEL_ID = 31,
        ABOUT_LINE1_LABEL_ID = 32,
        ABOUT_LINE2_LABEL_ID = 33
    };
    
    // 控件引用
    UILabel* statusLabel;
    UIMenuList* settingsMenu;
    UIWindow* mainWindow;
    UIWindow* brightnessWindow;
    UILabel* brightnessLabel;
    UISlider* brightnessSlider;
    UIWindow* themeWindow;
    UILabel* themePreviewLabel;
    UIMenuList* themeMenu;
    UIWindow* aboutWindow;
    UILabel* aboutTitleLabel;
    UILabel* aboutLine1Label;
    UILabel* aboutLine2Label;
    
    // 自定义设置菜单
    class SettingsMenuList : public UIMenuList {
    public:
        SettingsMenuList(int id, int x, int y, int width, int height, const String& name, SettingsApp* app)
            : UIMenuList(id, x, y, width, height, name), parentApp(app) {}
        
        void onItemSelected(MenuItem* item) override {
            parentApp->handleSettingSelection(item);
        }
        
    private:
        SettingsApp* parentApp;
    };

    class BrightnessSlider : public UISlider {
    public:
        BrightnessSlider(int id, int x, int y, int width, int height, const String& name)
            : UISlider(id, x, y, width, height, 5, 100, 50, "Brightness", name) {}
        void onValueChanged(int newValue) override {
            int brightness = (newValue * 255) / 100;
            if (brightness < 0) brightness = 0;
            if (brightness > 255) brightness = 255;
            M5Cardputer.Display.setBrightness(brightness);
            g_displayBrightness = brightness;
        }
    };

    class ThemeMenuList : public UIMenuList {
    public:
        ThemeMenuList(int id, int x, int y, int width, int height, const String& name, SettingsApp* app)
            : UIMenuList(id, x, y, width, height, name), parentApp(app) {}

        void onItemSelected(MenuItem* item) override {
            parentApp->handleThemeSelection(item);
        }

    private:
        SettingsApp* parentApp;
    };

public:
    SettingsApp(EventSystem* events) 
        : eventSystem(events),
          statusLabel(nullptr),
          settingsMenu(nullptr),
          mainWindow(nullptr),
          brightnessWindow(nullptr),
          brightnessLabel(nullptr),
          brightnessSlider(nullptr),
          themeWindow(nullptr),
          themePreviewLabel(nullptr),
          themeMenu(nullptr),
          aboutWindow(nullptr),
          aboutTitleLabel(nullptr),
          aboutLine1Label(nullptr),
          aboutLine2Label(nullptr) {}

    void setup() override {
        mainWindow = uiManager->createWindow(WINDOW_ID, 30, 20, 150, 100, "Settings", "MainWindow");
        mainWindow->setChildOffset(-30, -20);
        
        statusLabel = uiManager->createLabel(STATUS_LABEL_ID, 35, 35, "Select a setting", "Status", mainWindow);
        
        settingsMenu = new SettingsMenuList(MENU_LIST_ID, 35, 50, 130, 70, "SettingsMenu", this);
        settingsMenu->setParent(mainWindow);
        uiManager->addWidget(settingsMenu);
        
        settingsMenu->addItem("Brightness", 101);
        settingsMenu->addItem("Theme", 102);
        settingsMenu->addItem("About", 103);
        
        settingsMenu->setColors(TFT_GREEN, TFT_YELLOW, TFT_WHITE, TFT_DARKGREY);

        brightnessWindow = uiManager->createWindow(BRIGHTNESS_WINDOW_ID, 30, 20, 180, 100, "Brightness", "BrightnessWindow");
        brightnessWindow->setChildOffset(-30, -20);
        brightnessLabel = uiManager->createLabel(BRIGHTNESS_LABEL_ID, 40, 35, "Brightness", "BrightnessLabel", brightnessWindow);

        int sliderX = 40;
        int sliderY = 55;
        int sliderW = 120;
        int sliderH = 20;
        BrightnessSlider* slider = new BrightnessSlider(BRIGHTNESS_SLIDER_ID, sliderX, sliderY, sliderW, sliderH, "BrightnessSlider");
        int initialPercent = (g_displayBrightness * 100) / 255;
        if (initialPercent < 5) initialPercent = 5;
        if (initialPercent > 100) initialPercent = 100;
        slider->setValue(initialPercent);
        slider->setParent(brightnessWindow);
        uiManager->addWidget(slider);
        brightnessSlider = slider;

        uiManager->setWidgetTreeVisible(brightnessWindow, false);

        themeWindow = uiManager->createWindow(THEME_WINDOW_ID, 30, 20, 180, 100, "Theme", "ThemeWindow");
        themeWindow->setChildOffset(-30, -20);
        themePreviewLabel = uiManager->createLabel(THEME_PREVIEW_LABEL_ID, 35, 35, "Current: Default", "ThemePreview", themeWindow);

        themeMenu = new ThemeMenuList(THEME_MENU_ID, 35, 50, 150, 45, "ThemeMenu", this);
        themeMenu->setParent(themeWindow);
        uiManager->addWidget(themeMenu);

        if (globalThemeManager) {
            for (int i = 0; i < globalThemeManager->getThemeCount(); i++) {
                Theme* theme = globalThemeManager->getTheme(i);
                if (theme) {
                    themeMenu->addItem(theme->getThemeName(), 200 + i);
                }
            }
        }

        themeMenu->setColors(TFT_BLUE, TFT_CYAN, TFT_WHITE, TFT_DARKGREY);

        uiManager->setWidgetTreeVisible(themeWindow, false);

        aboutWindow = uiManager->createWindow(ABOUT_WINDOW_ID, 30, 20, 180, 100, "About", "AboutWindow");
        aboutWindow->setChildOffset(-30, -20);
        aboutTitleLabel = uiManager->createLabel(ABOUT_TITLE_LABEL_ID, 35, 40, "CardputerOS", "AboutTitle", aboutWindow);
        aboutLine1Label = uiManager->createLabel(ABOUT_LINE1_LABEL_ID, 35, 55, "Using Kiwifruit framework", "AboutLine1", aboutWindow);
        aboutLine2Label = uiManager->createLabel(ABOUT_LINE2_LABEL_ID, 35, 70, "Kiwifruit v5.78", "AboutLine2", aboutWindow);

        uiManager->setWidgetTreeVisible(aboutWindow, false);

        uiManager->nextFocus();
    }

    void loop() override {
    }

    void onKeyEvent(const KeyEvent& event) override {
        if (event.esc) {
            if (brightnessWindow && brightnessWindow->isVisible()) {
                showMenuView();
                if (appManager) {
                    appManager->setGlobalEscEnabled(true);
                }
                return;
            }
            if (themeWindow && themeWindow->isVisible()) {
                showMenuView();
                if (appManager) {
                    appManager->setGlobalEscEnabled(true);
                }
                return;
            }
            if (aboutWindow && aboutWindow->isVisible()) {
                showMenuView();
                if (appManager) {
                    appManager->setGlobalEscEnabled(true);
                }
                return;
            }
        }

        if ((brightnessWindow && brightnessWindow->isVisible()) ||
            (themeWindow && themeWindow->isVisible())) {
            if (uiManager->handleKeyEvent(event)) {
                uiManager->refreshAppArea();
            }
            return;
        }

        if (uiManager->handleKeyEvent(event)) {
            updateStatus();
            uiManager->refreshAppArea();
        }
    }
    
    void handleSettingSelection(MenuItem* item) {
        if (!item) return;
        String message = "Selected: " + item->text;
        statusLabel->setText(message);
        
        switch (item->id) {
            case 101:
                showBrightnessView();
                if (appManager) {
                    appManager->setGlobalEscEnabled(false);
                }
                break;
            case 102:
                showThemeView();
                if (appManager) {
                    appManager->setGlobalEscEnabled(false);
                }
                break;
            case 103:
                showAboutView();
                if (appManager) {
                    appManager->setGlobalEscEnabled(false);
                }
                break;
        }
    }

    void handleThemeSelection(MenuItem* item) {
        if (!item || !globalThemeManager) return;
        for (int i = 0; i < globalThemeManager->getThemeCount(); i++) {
            Theme* theme = globalThemeManager->getTheme(i);
            if (theme && theme->getThemeName() == item->text) {
                globalThemeManager->setCurrentTheme(i);
                updateCurrentThemeStatus();
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
            if (focusedWidget->hasSecondaryFocus()) {
                status = "Navigate: " + focusedWidget->getName();
            }
            statusLabel->setText(status);
        } else {
            statusLabel->setText("Select a setting");
        }
    }

    void updateCurrentThemeStatus() {
        if (!globalThemeManager) return;
        Theme* currentTheme = globalThemeManager->getCurrentTheme();
        if (!themePreviewLabel) return;
        if (currentTheme) {
            String currentStatus = "Current: " + currentTheme->getThemeName();
            themePreviewLabel->setText(currentStatus);
        } else {
            themePreviewLabel->setText("Current: No theme active");
        }
    }

    void showBrightnessView() {
        if (!mainWindow || !brightnessWindow) return;
        uiManager->hidePage(mainWindow);
        if (themeWindow) {
            uiManager->hidePage(themeWindow);
        }
        uiManager->showPage(brightnessWindow);
        uiManager->rebuildForegroundFocus();
        uiManager->refresh();
    }

    void showThemeView() {
        if (!mainWindow || !themeWindow) return;
        uiManager->hidePage(mainWindow);
        if (brightnessWindow) {
            uiManager->hidePage(brightnessWindow);
        }
        if (aboutWindow) {
            uiManager->hidePage(aboutWindow);
        }
        uiManager->showPage(themeWindow);
        uiManager->rebuildForegroundFocus();
        uiManager->refresh();
    }

    void showAboutView() {
        if (!mainWindow || !aboutWindow) return;
        uiManager->hidePage(mainWindow);
        if (brightnessWindow) {
            uiManager->hidePage(brightnessWindow);
        }
        if (themeWindow) {
            uiManager->hidePage(themeWindow);
        }
        uiManager->showPage(aboutWindow);
        uiManager->rebuildForegroundFocus();
        uiManager->refresh();
    }

    void showMenuView() {
        if (!mainWindow) return;
        if (brightnessWindow) {
            uiManager->hidePage(brightnessWindow);
        }
        if (themeWindow) {
            uiManager->hidePage(themeWindow);
        }
        if (aboutWindow) {
            uiManager->hidePage(aboutWindow);
        }
        uiManager->showPage(mainWindow);
        uiManager->rebuildForegroundFocus();
        uiManager->refresh();
    }

    void drawInterface() {
        uiManager->smartRefresh();
    }
};
