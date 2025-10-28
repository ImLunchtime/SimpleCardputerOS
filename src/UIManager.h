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
    
public:
    UIManager() : display(&M5Cardputer.Display), widgetCount(0), currentFocus(-1), focusableCount(0) {
        for (int i = 0; i < 20; i++) {
            widgets[i] = nullptr;
            focusableWidgets[i] = -1;
        }
    }
    
    ~UIManager() {
        clear();
    }
    
    // 控件管理
    void addWidget(UIWidget* widget) {
        if (widgetCount < 20 && widget != nullptr) {
            widgets[widgetCount] = widget;
            
            // 如果是可聚焦控件，添加到焦点列表
            if (widget->isFocusable()) {
                focusableWidgets[focusableCount] = widgetCount;
                focusableCount++;
                
                // 如果是第一个可聚焦控件，设置为当前焦点
                if (currentFocus == -1) {
                    currentFocus = 0;
                    widget->setFocused(true);
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
        // 处理Tab键切换焦点
        if (event.tab) {
            nextFocus();
            return true;
        }
        
        // 将事件传递给当前聚焦的控件
        UIWidget* focusedWidget = getCurrentFocusedWidget();
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
        for (int i = 0; i < widgetCount; i++) {
            if (widgets[i] && widgets[i]->isVisible()) {
                widgets[i]->draw(display);
            }
        }
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
};