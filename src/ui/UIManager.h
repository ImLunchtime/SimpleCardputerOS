#pragma once
#include <M5Cardputer.h>
#include "UIWidget.h"
#include "system/EventSystem.h"
class UIManager {
private:
    LGFX_Device* display;
    UIWidget* widgets[20];
    int widgetCount;
    int currentFocus;
    int focusableWidgets[20];
    int focusableCount;
    UIWidget* backgroundWidgets[20];
    int backgroundWidgetCount;
    UIWidget* foregroundWidgets[20];
    int foregroundWidgetCount;
    bool hasBackgroundLayer;
    UIScreen* rootScreen;
    uint32_t lastAnimationRedrawMs;
public:
    UIManager();
    ~UIManager();
    void addWidget(UIWidget* widget);
    UIWidget* getWidget(int id);
    UIScreen* getRootScreen() const;
    void removeWidget(int id);
    void clear();
    void clearForeground();
    void saveToBackground();
    void nextFocus();
    void previousFocus();
    UIWidget* getCurrentFocusedWidget();
    bool handleKeyEvent(const KeyEvent& event);
    void clearScreen();
    void drawAll();
    void refresh();
    void switchToApp();
    void switchToLauncher();
    void finishAppSetup();
    void drawWidget(int id);
    void drawWidgetPartial(int id);
    void drawForegroundPartial();
    void refreshAppArea();
    void smartRefresh();
    void tick();
    UILabel* createLabel(int id, int x, int y, const String& text, const String& name = "", UIWidget* parent = nullptr);
    UIButton* createButton(int id, int x, int y, int width, int height, const String& text, const String& name = "", UIWidget* parent = nullptr);
    UIButton* createImageButton(int id, int x, int y, int width, int height, const uint8_t* imageData, size_t dataSize, const String& name = "", UIWidget* parent = nullptr);
    UIButton* createImageButtonFromFile(int id, int x, int y, int width, int height, const String& filePath, const String& name = "", UIWidget* parent = nullptr);
    UIWindow* createWindow(int id, int x, int y, int width, int height, const String& title = "", const String& name = "", UIWidget* parent = nullptr);
    UIMenuList* createMenuList(int id, int x, int y, int width, int height, const String& name = "", int itemHeight = 16, UIWidget* parent = nullptr);
    UIMenuGrid* createMenuGrid(int id, int x, int y, int width, int height, int columns, int rows, const String& name = "", UIWidget* parent = nullptr);
    UIImage* createImage(int id, int x, int y, int width, int height, const uint8_t* imageData, size_t dataSize, const String& name = "", UIWidget* parent = nullptr);
private:
    void removeFocusableWidget(int widgetIndex);
    void removeFromMainList(UIWidget* widget);
    void rebuildFocusListForBackground();
    void rebuildFocusListForForeground();
    bool computeClipRect(UIWidget* widget, int& outX, int& outY, int& outW, int& outH);
    void drawWidgetClipped(UIWidget* widget, bool partial);
    void drawWidgetClippedWithExtra(UIWidget* widget, bool partial, int clipX, int clipY, int clipW, int clipH);
    bool flushDirtyInAppArea();
    bool flushDirtyInRoot();
};
