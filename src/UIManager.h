#pragma once
#include <M5Cardputer.h>
#include "UIWidget.h"
#include "EventSystem.h"

class UIManager {
private:
    LGFX_Device* display;
    UIWidget* widgets[20];  // 最多支持20个控件
    int widgetCount;
    int currentFocus;
    
    // 焦点管理
    int focusableWidgets[20];  // 可聚焦控件的索引
    int focusableCount;
    
    // 多层窗口管理
    UIWidget* backgroundWidgets[20];  // 背景层控件（启动器）
    int backgroundWidgetCount;
    UIWidget* foregroundWidgets[20];  // 前景层控件（当前应用）
    int foregroundWidgetCount;
    bool hasBackgroundLayer;  // 是否有背景层
    
public:
    UIManager() : display(&M5Cardputer.Display), widgetCount(0), currentFocus(-1), focusableCount(0), 
                  backgroundWidgetCount(0), foregroundWidgetCount(0), hasBackgroundLayer(false) {
        for (int i = 0; i < 20; i++) {
            widgets[i] = nullptr;
            focusableWidgets[i] = -1;
            backgroundWidgets[i] = nullptr;
            foregroundWidgets[i] = nullptr;
        }
    }
    
    ~UIManager() {
        clear();
    }
    
    // 控件管理
    void addWidget(UIWidget* widget) {
        if (widgetCount < 20 && widget != nullptr) {
            widgets[widgetCount] = widget;
            
            // 如果有背景层，新控件添加到前景层
            if (hasBackgroundLayer && foregroundWidgetCount < 20) {
                foregroundWidgets[foregroundWidgetCount] = widget;
                foregroundWidgetCount++;
                
                // 对于前景层控件，只有在没有其他前景层可聚焦控件时才设置焦点
                if (widget->isFocusable()) {
                    // 检查是否已有前景层可聚焦控件
                    bool hasForegroundFocusable = false;
                    for (int i = 0; i < foregroundWidgetCount - 1; i++) {
                        if (foregroundWidgets[i] && foregroundWidgets[i]->isFocusable()) {
                            hasForegroundFocusable = true;
                            break;
                        }
                    }
                    
                    // 如果这是第一个前景层可聚焦控件，清除背景层焦点并设置为当前焦点
                    if (!hasForegroundFocusable) {
                        // 清除所有背景层控件的焦点
                        for (int i = 0; i < backgroundWidgetCount; i++) {
                            if (backgroundWidgets[i]) {
                                backgroundWidgets[i]->setFocused(false);
                            }
                        }
                        
                        // 重置焦点列表并添加这个控件
                        focusableCount = 1;
                        focusableWidgets[0] = widgetCount;
                        currentFocus = 0;
                        widget->setFocused(true);
                    } else {
                        // 添加到焦点列表但不设置焦点
                        focusableWidgets[focusableCount] = widgetCount;
                        focusableCount++;
                    }
                }
            } else {
                // 没有背景层时的原始逻辑
                if (widget->isFocusable()) {
                    focusableWidgets[focusableCount] = widgetCount;
                    focusableCount++;
                    
                    // 如果是第一个可聚焦控件，设置为当前焦点
                    if (currentFocus == -1) {
                        currentFocus = 0;
                        widget->setFocused(true);
                    }
                }
            }
            
            widgetCount++;
        }
    }
    
    UIWidget* getWidget(int id) {
        for (int i = 0; i < widgetCount; i++) {
            if (widgets[i] && widgets[i]->getId() == id) {
                return widgets[i];
            }
        }
        return nullptr;
    }
    
    void removeWidget(int id) {
        for (int i = 0; i < widgetCount; i++) {
            if (widgets[i] && widgets[i]->getId() == id) {
                // 如果是可聚焦控件，从焦点列表中移除
                if (widgets[i]->isFocusable()) {
                    removeFocusableWidget(i);
                }
                
                delete widgets[i];
                
                // 移动后面的控件向前
                for (int j = i; j < widgetCount - 1; j++) {
                    widgets[j] = widgets[j + 1];
                }
                widgets[widgetCount - 1] = nullptr;
                widgetCount--;
                break;
            }
        }
    }
    
    void clear() {
        for (int i = 0; i < widgetCount; i++) {
            if (widgets[i]) {
                delete widgets[i];
                widgets[i] = nullptr;
            }
        }
        widgetCount = 0;
        focusableCount = 0;
        currentFocus = -1;
        
        for (int i = 0; i < 20; i++) {
            focusableWidgets[i] = -1;
        }
    }
    
    // 清除前景层（应用窗口）
    void clearForeground() {
        // 只清除前景层控件
        for (int i = 0; i < foregroundWidgetCount; i++) {
            if (foregroundWidgets[i]) {
                // 从主控件列表中移除
                removeFromMainList(foregroundWidgets[i]);
                delete foregroundWidgets[i];
                foregroundWidgets[i] = nullptr;
            }
        }
        foregroundWidgetCount = 0;
        
        // 重建焦点列表（只包含背景层的可聚焦控件）
        rebuildFocusListForBackground();
    }
    
    // 保存当前控件到背景层
    void saveToBackground() {
        for (int i = 0; i < widgetCount; i++) {
            if (widgets[i] && backgroundWidgetCount < 20) {
                backgroundWidgets[backgroundWidgetCount] = widgets[i];
                backgroundWidgetCount++;
            }
        }
        hasBackgroundLayer = true;
    }
    
    // 焦点管理
    void nextFocus() {
        if (focusableCount == 0) return;
        
        // 清除当前焦点
        if (currentFocus >= 0 && currentFocus < focusableCount) {
            int widgetIndex = focusableWidgets[currentFocus];
            if (widgetIndex >= 0 && widgets[widgetIndex]) {
                widgets[widgetIndex]->setFocused(false);
                widgets[widgetIndex]->onFocusChanged(false);
            }
        }
        
        // 移动到下一个焦点
        currentFocus = (currentFocus + 1) % focusableCount;
        
        // 设置新焦点
        int widgetIndex = focusableWidgets[currentFocus];
        if (widgetIndex >= 0 && widgets[widgetIndex]) {
            widgets[widgetIndex]->setFocused(true);
            widgets[widgetIndex]->onFocusChanged(true);
        }
    }
    
    void previousFocus() {
        if (focusableCount == 0) return;
        
        // 清除当前焦点
        if (currentFocus >= 0 && currentFocus < focusableCount) {
            int widgetIndex = focusableWidgets[currentFocus];
            if (widgetIndex >= 0 && widgets[widgetIndex]) {
                widgets[widgetIndex]->setFocused(false);
                widgets[widgetIndex]->onFocusChanged(false);
            }
        }
        
        // 移动到上一个焦点
        currentFocus = (currentFocus - 1 + focusableCount) % focusableCount;
        
        // 设置新焦点
        int widgetIndex = focusableWidgets[currentFocus];
        if (widgetIndex >= 0 && widgets[widgetIndex]) {
            widgets[widgetIndex]->setFocused(true);
            widgets[widgetIndex]->onFocusChanged(true);
        }
    }
    
    UIWidget* getCurrentFocusedWidget() {
        if (currentFocus >= 0 && currentFocus < focusableCount) {
            int widgetIndex = focusableWidgets[currentFocus];
            if (widgetIndex >= 0 && widgets[widgetIndex]) {
                return widgets[widgetIndex];
            }
        }
        return nullptr;
    }
    
    // 事件处理
    bool handleKeyEvent(const KeyEvent& event) {
        UIWidget* focusedWidget = getCurrentFocusedWidget();
        
        // 处理Tab键切换焦点（总是允许Tab键切换焦点）
        if (event.tab) {
            nextFocus();
            return true;
        }
        
        // 如果当前聚焦的控件有二级焦点，优先处理二级焦点的键盘事件
        if (focusedWidget && focusedWidget->hasSecondaryFocus()) {
            // 对于菜单控件，方向键由控件自己处理
            if (event.up || event.down || event.left || event.right || event.enter) {
                return focusedWidget->handleKeyEvent(event);
            }
        }
        
        // 将事件传递给当前聚焦的控件
        if (focusedWidget) {
            return focusedWidget->handleKeyEvent(event);
        }
        
        return false;
    }
    
    // 渲染管理
    void clearScreen() {
        display->fillScreen(TFT_BLACK);
    }
    
    void drawAll() {
        // 先绘制背景层
        if (hasBackgroundLayer) {
            for (int i = 0; i < backgroundWidgetCount; i++) {
                if (backgroundWidgets[i] && backgroundWidgets[i]->isVisible()) {
                    backgroundWidgets[i]->draw(display);
                }
            }
        }
        
        // 再绘制前景层
        for (int i = 0; i < foregroundWidgetCount; i++) {
            if (foregroundWidgets[i] && foregroundWidgets[i]->isVisible()) {
                foregroundWidgets[i]->draw(display);
            }
        }
        
        // 如果没有分层，使用原来的方式
        if (!hasBackgroundLayer && foregroundWidgetCount == 0) {
            for (int i = 0; i < widgetCount; i++) {
                if (widgets[i] && widgets[i]->isVisible()) {
                    widgets[i]->draw(display);
                }
            }
        }
    }
    
    // 强制刷新屏幕 - 清屏并重绘所有控件
    void refresh() {
        clearScreen();
        drawAll();
    }
    
    // 应用切换管理
    void switchToApp() {
        // 如果是第一次切换（启动器 -> 应用），保存启动器到背景层
        if (!hasBackgroundLayer && widgetCount > 0) {
            saveToBackground();
        }
        
        // 清除前景层（如果有的话）
        if (foregroundWidgetCount > 0) {
            clearForeground();
        }
        
        // 清屏准备绘制新应用界面
        clearScreen();
    }
    
    void switchToLauncher() {
        // 清除前景层
        if (foregroundWidgetCount > 0) {
            clearForeground();
        }
        
        // 清屏并重绘背景层
        clearScreen();
        drawAll();
    }
    
    void finishAppSetup() {
        // 应用设置完成后刷新界面
        if (hasBackgroundLayer && foregroundWidgetCount > 0) {
            // 重建前景层的焦点列表，确保只有前景层控件可以被聚焦
            rebuildFocusListForForeground();
        }
        drawAll();
    }
    
    void drawWidget(int id) {
        UIWidget* widget = getWidget(id);
        if (widget && widget->isVisible()) {
            widget->draw(display);
        }
    }
    
    // 便利方法
    UILabel* createLabel(int id, int x, int y, const String& text, const String& name = "") {
        UILabel* label = new UILabel(id, x, y, text, name);
        addWidget(label);
        return label;
    }
    
    UIButton* createButton(int id, int x, int y, int width, int height, const String& text, const String& name = "") {
        UIButton* button = new UIButton(id, x, y, width, height, text, name);
        addWidget(button);
        return button;
    }
    
    UIInput* createInput(int id, int x, int y, int width, int height, const String& placeholder = "", const String& name = "") {
        UIInput* input = new UIInput(id, x, y, width, height, placeholder, name);
        addWidget(input);
        return input;
    }
    
    UIWindow* createWindow(int id, int x, int y, int width, int height, const String& title = "", const String& name = "") {
        UIWindow* window = new UIWindow(id, x, y, width, height, title, name);
        addWidget(window);
        return window;
    }
    
    UIMenuList* createMenuList(int id, int x, int y, int width, int height, const String& name = "", int itemHeight = 16) {
        UIMenuList* menu = new UIMenuList(id, x, y, width, height, name, itemHeight);
        addWidget(menu);
        return menu;
    }
    
    UIMenuGrid* createMenuGrid(int id, int x, int y, int width, int height, int columns, int rows, const String& name = "") {
        UIMenuGrid* menu = new UIMenuGrid(id, x, y, width, height, columns, rows, name);
        addWidget(menu);
        return menu;
    }
    
private:
    void removeFocusableWidget(int widgetIndex) {
        // 从可聚焦列表中移除
        for (int i = 0; i < focusableCount; i++) {
            if (focusableWidgets[i] == widgetIndex) {
                // 移动后面的索引向前
                for (int j = i; j < focusableCount - 1; j++) {
                    focusableWidgets[j] = focusableWidgets[j + 1];
                }
                focusableWidgets[focusableCount - 1] = -1;
                focusableCount--;
                
                // 调整当前焦点
                if (currentFocus >= focusableCount) {
                    currentFocus = focusableCount > 0 ? 0 : -1;
                }
                break;
            }
        }
        
        // 更新所有大于被移除索引的焦点索引
        for (int i = 0; i < focusableCount; i++) {
            if (focusableWidgets[i] > widgetIndex) {
                focusableWidgets[i]--;
            }
        }
    }
    
    // 从主控件列表中移除指定控件
    void removeFromMainList(UIWidget* widget) {
        for (int i = 0; i < widgetCount; i++) {
            if (widgets[i] == widget) {
                // 如果是可聚焦控件，从焦点列表中移除
                if (widget->isFocusable()) {
                    removeFocusableWidget(i);
                }
                
                // 移动后面的控件向前
                for (int j = i; j < widgetCount - 1; j++) {
                    widgets[j] = widgets[j + 1];
                }
                widgets[widgetCount - 1] = nullptr;
                widgetCount--;
                break;
            }
        }
    }
    
    // 重建背景层的焦点列表
    void rebuildFocusListForBackground() {
        focusableCount = 0;
        currentFocus = -1;
        
        // 重新扫描背景层的可聚焦控件
        for (int i = 0; i < backgroundWidgetCount; i++) {
            if (backgroundWidgets[i] && backgroundWidgets[i]->isFocusable()) {
                // 找到对应的主列表索引
                for (int j = 0; j < widgetCount; j++) {
                    if (widgets[j] == backgroundWidgets[i]) {
                        focusableWidgets[focusableCount] = j;
                        focusableCount++;
                        
                        // 设置第一个为当前焦点
                        if (currentFocus == -1) {
                            currentFocus = 0;
                            backgroundWidgets[i]->setFocused(true);
                        }
                        break;
                    }
                }
            }
        }
    }
    
    // 重建前景层的焦点列表
    void rebuildFocusListForForeground() {
        focusableCount = 0;
        currentFocus = -1;
        
        // 清除所有控件的焦点状态
        for (int i = 0; i < widgetCount; i++) {
            if (widgets[i]) {
                widgets[i]->setFocused(false);
            }
        }
        
        // 重新扫描前景层的可聚焦控件
        for (int i = 0; i < foregroundWidgetCount; i++) {
            if (foregroundWidgets[i] && foregroundWidgets[i]->isFocusable()) {
                // 找到对应的主列表索引
                for (int j = 0; j < widgetCount; j++) {
                    if (widgets[j] == foregroundWidgets[i]) {
                        focusableWidgets[focusableCount] = j;
                        focusableCount++;
                        
                        // 设置第一个为当前焦点
                        if (currentFocus == -1) {
                            currentFocus = 0;
                            foregroundWidgets[i]->setFocused(true);
                        }
                        break;
                    }
                }
            }
        }
    }
};