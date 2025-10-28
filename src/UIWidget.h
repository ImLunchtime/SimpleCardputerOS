#pragma once
#include <M5Cardputer.h>
#include "EventSystem.h"

// UI控件类型枚举
enum UIWidgetType {
    WIDGET_LABEL,
    WIDGET_BUTTON,
    WIDGET_INPUT,
    WIDGET_WINDOW
};

// UI控件基类
class UIWidget {
protected:
    int id;
    UIWidgetType type;
    int x, y, width, height;
    String name;
    bool visible;
    bool focusable;
    bool focused;
    
public:
    UIWidget(int _id, UIWidgetType _type, int _x, int _y, int _w, int _h, const String& _name, bool _focusable = false)
        : id(_id), type(_type), x(_x), y(_y), width(_w), height(_h), name(_name), 
          visible(true), focusable(_focusable), focused(false) {}
    
    virtual ~UIWidget() {}
    
    // 基本属性访问
    int getId() const { return id; }
    UIWidgetType getType() const { return type; }
    String getName() const { return name; }
    bool isVisible() const { return visible; }
    bool isFocusable() const { return focusable; }
    bool isFocused() const { return focused; }
    
    void setVisible(bool _visible) { visible = _visible; }
    void setFocused(bool _focused) { focused = _focused; }
    
    // 位置和大小
    void setPosition(int _x, int _y) { x = _x; y = _y; }
    void setSize(int _w, int _h) { width = _w; height = _h; }
    void getBounds(int& _x, int& _y, int& _w, int& _h) const {
        _x = x; _y = y; _w = width; _h = height;
    }
    
    // 纯虚函数，子类必须实现
    virtual void draw(LGFX_Device* display) = 0;
    virtual bool handleKeyEvent(const KeyEvent& event) = 0;
    virtual void onFocusChanged(bool hasFocus) {}
};

// 标签控件
class UILabel : public UIWidget {
private:
    String text;
    uint16_t textColor;
    
public:
    UILabel(int id, int x, int y, const String& text, const String& name = "")
        : UIWidget(id, WIDGET_LABEL, x, y, text.length() * 6, 8, name, false),
          text(text), textColor(TFT_WHITE) {}
    
    void setText(const String& newText) { 
        text = newText; 
        width = text.length() * 6; // 更新宽度
    }
    
    void setTextColor(uint16_t color) { textColor = color; }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        display->setTextColor(textColor);
        display->setTextSize(1);
        display->setCursor(x, y);
        display->print(text);
    }
    
    bool handleKeyEvent(const KeyEvent& event) override {
        return false; // 标签不处理键盘事件
    }
};

// 按钮控件
class UIButton : public UIWidget {
private:
    String text;
    uint16_t borderColor;
    uint16_t textColor;
    
public:
    UIButton(int id, int x, int y, int width, int height, const String& text, const String& name = "")
        : UIWidget(id, WIDGET_BUTTON, x, y, width, height, name, true),
          text(text), borderColor(TFT_BLUE), textColor(TFT_WHITE) {}
    
    void setText(const String& newText) { text = newText; }
    void setBorderColor(uint16_t color) { borderColor = color; }
    void setTextColor(uint16_t color) { textColor = color; }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        // 绘制黑色背景
        display->fillRect(x, y, width, height, TFT_BLACK);
        
        // 绘制边框
        display->drawRect(x, y, width, height, borderColor);
        
        // 如果聚焦，绘制黄色外框
        if (focused) {
            display->drawRect(x - 1, y - 1, width + 2, height + 2, TFT_YELLOW);
            display->drawRect(x - 2, y - 2, width + 4, height + 4, TFT_YELLOW);
        }
        
        // 计算文本居中位置
        display->setTextSize(1);
        int textWidth = text.length() * 6;
        int textHeight = 8;
        int textX = x + (width - textWidth) / 2;
        int textY = y + (height - textHeight) / 2;
        
        // 绘制居中文本
        display->setTextColor(textColor);
        display->setCursor(textX, textY);
        display->print(text);
    }
    
    bool handleKeyEvent(const KeyEvent& event) override {
        if (!focused || !visible) return false;
        
        // 处理回车键
        if (event.enter) {
            onButtonClick();
            return true;
        }
        
        return false;
    }
    
    virtual void onButtonClick() {
        // 子类可以重写此方法来处理按钮点击
    }
};

// 输入框控件
class UIInput : public UIWidget {
private:
    String text;
    String placeholder;
    uint16_t borderColor;
    uint16_t textColor;
    int cursorPos;
    
public:
    UIInput(int id, int x, int y, int width, int height, const String& placeholder = "", const String& name = "")
        : UIWidget(id, WIDGET_INPUT, x, y, width, height, name, true),
          text(""), placeholder(placeholder), borderColor(TFT_WHITE), textColor(TFT_WHITE), cursorPos(0) {}
    
    void setText(const String& newText) { 
        text = newText; 
        cursorPos = text.length();
    }
    
    String getText() const { return text; }
    void setPlaceholder(const String& newPlaceholder) { placeholder = newPlaceholder; }
    void setBorderColor(uint16_t color) { borderColor = color; }
    void setTextColor(uint16_t color) { textColor = color; }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        // 绘制黑色背景
        display->fillRect(x, y, width, height, TFT_BLACK);
        
        // 绘制边框
        display->drawRect(x, y, width, height, borderColor);
        
        // 如果聚焦，绘制黄色外框
        if (focused) {
            display->drawRect(x - 1, y - 1, width + 2, height + 2, TFT_YELLOW);
            display->drawRect(x - 2, y - 2, width + 4, height + 4, TFT_YELLOW);
        }
        
        // 绘制文本或占位符
        display->setTextSize(1);
        display->setTextColor(textColor);
        
        String displayText = text.isEmpty() ? placeholder : text;
        if (text.isEmpty() && !placeholder.isEmpty()) {
            display->setTextColor(TFT_DARKGREY); // 占位符使用灰色
        }
        
        display->setCursor(x + 5, y + (height - 8) / 2);
        display->print(displayText);
        
        // 如果聚焦，绘制光标
        if (focused && !text.isEmpty()) {
            int cursorX = x + 5 + cursorPos * 6;
            int cursorY = y + (height - 8) / 2;
            display->drawLine(cursorX, cursorY, cursorX, cursorY + 8, TFT_WHITE);
        }
    }
    
    bool handleKeyEvent(const KeyEvent& event) override {
        if (!focused || !visible) return false;
        
        // 处理删除键
        if (event.del && text.length() > 0) {
            text.remove(text.length() - 1);
            cursorPos = text.length();
            return true;
        }
        
        // 处理文本输入
        if (!event.text.isEmpty()) {
            text += event.text;
            cursorPos = text.length();
            return true;
        }
        
        return false;
    }
    
    virtual void onTextChanged(const String& newText) {
        // 子类可以重写此方法来处理文本变化
    }
};

// 窗口控件
class UIWindow : public UIWidget {
private:
    String title;
    uint16_t borderColor;
    
public:
    UIWindow(int id, int x, int y, int width, int height, const String& title = "", const String& name = "")
        : UIWidget(id, WIDGET_WINDOW, x, y, width, height, name, false),
          title(title), borderColor(TFT_WHITE) {}
    
    void setTitle(const String& newTitle) { title = newTitle; }
    void setBorderColor(uint16_t color) { borderColor = color; }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        // 绘制黑色背景
        display->fillRect(x, y, width, height, TFT_BLACK);
        
        // 绘制边框
        display->drawRect(x, y, width, height, borderColor);
        
        // 如果有标题，绘制标题栏
        if (!title.isEmpty()) {
            display->fillRect(x + 1, y + 1, width - 2, 12, TFT_DARKGREY);
            display->setTextColor(TFT_WHITE);
            display->setTextSize(1);
            display->setCursor(x + 5, y + 3);
            display->print(title);
        }
    }
    
    bool handleKeyEvent(const KeyEvent& event) override {
        return false; // 窗口本身不处理键盘事件
    }
};