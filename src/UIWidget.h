#pragma once
#include <M5Cardputer.h>
#include "EventSystem.h"

// UI控件类型枚举
enum UIWidgetType {
    WIDGET_LABEL,
    WIDGET_BUTTON,
    WIDGET_INPUT,
    WIDGET_WINDOW,
    WIDGET_MENU_LIST,
    WIDGET_MENU_GRID
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
    
    // 菜单控件需要的二级焦点支持
    virtual bool hasSecondaryFocus() const { return false; }
    virtual bool handleSecondaryKeyEvent(const KeyEvent& event) { return false; }
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

// 菜单项结构
struct MenuItem {
    String text;
    int id;
    bool enabled;
    
    MenuItem(const String& _text, int _id, bool _enabled = true)
        : text(_text), id(_id), enabled(_enabled) {}
};

// 菜单基类
class UIMenu : public UIWidget {
protected:
    MenuItem* items[20];  // 最多20个菜单项
    int itemCount;
    int selectedIndex;
    bool secondaryFocus;  // 是否处于二级焦点状态
    uint16_t borderColor;
    uint16_t selectedColor;
    uint16_t textColor;
    uint16_t disabledColor;
    
public:
    UIMenu(int id, UIWidgetType type, int x, int y, int width, int height, const String& name)
        : UIWidget(id, type, x, y, width, height, name, true),
          itemCount(0), selectedIndex(0), secondaryFocus(false),
          borderColor(TFT_WHITE), selectedColor(TFT_YELLOW), 
          textColor(TFT_WHITE), disabledColor(TFT_DARKGREY) {
        for (int i = 0; i < 20; i++) {
            items[i] = nullptr;
        }
    }
    
    virtual ~UIMenu() {
        clear();
    }
    
    void addItem(const String& text, int itemId, bool enabled = true) {
        if (itemCount < 20) {
            items[itemCount] = new MenuItem(text, itemId, enabled);
            itemCount++;
        }
    }
    
    void removeItem(int itemId) {
        for (int i = 0; i < itemCount; i++) {
            if (items[i] && items[i]->id == itemId) {
                delete items[i];
                // 移动后面的项目向前
                for (int j = i; j < itemCount - 1; j++) {
                    items[j] = items[j + 1];
                }
                items[itemCount - 1] = nullptr;
                itemCount--;
                
                // 调整选中索引
                if (selectedIndex >= itemCount && itemCount > 0) {
                    selectedIndex = itemCount - 1;
                } else if (itemCount == 0) {
                    selectedIndex = 0;
                }
                break;
            }
        }
    }
    
    void clear() {
        for (int i = 0; i < itemCount; i++) {
            if (items[i]) {
                delete items[i];
                items[i] = nullptr;
            }
        }
        itemCount = 0;
        selectedIndex = 0;
    }
    
    MenuItem* getSelectedItem() {
        if (selectedIndex >= 0 && selectedIndex < itemCount && items[selectedIndex]) {
            return items[selectedIndex];
        }
        return nullptr;
    }
    
    void setColors(uint16_t border, uint16_t selected, uint16_t text, uint16_t disabled) {
        borderColor = border;
        selectedColor = selected;
        textColor = text;
        disabledColor = disabled;
    }
    
    // 重写基类方法
    bool hasSecondaryFocus() const override { return focused; }  // 只要有一级焦点就启用二级焦点
    
    void onFocusChanged(bool hasFocus) override {
        // 不需要手动管理secondaryFocus，因为它现在直接依赖于focused状态
    }
    
    bool handleKeyEvent(const KeyEvent& event) override {
        if (!focused || !visible || itemCount == 0) return false;
        
        // 现在直接处理方向键和回车键，因为有一级焦点就自动启用二级焦点
        return handleSecondaryKeyEvent(event);
    }
    
    virtual void onItemSelected(MenuItem* item) {
        // 子类可以重写此方法来处理菜单项选择
    }
    
protected:
    void drawMenuBorder(LGFX_Device* display) {
        // 绘制背景
        display->fillRect(x, y, width, height, TFT_BLACK);
        
        // 绘制边框
        display->drawRect(x, y, width, height, borderColor);
        
        // 如果聚焦，绘制焦点框
        if (focused) {
            uint16_t focusColor = TFT_YELLOW;  // 一级焦点使用黄色
            display->drawRect(x - 1, y - 1, width + 2, height + 2, focusColor);
            display->drawRect(x - 2, y - 2, width + 4, height + 4, focusColor);
        }
    }
};

// 列表菜单控件
class UIMenuList : public UIMenu {
private:
    int itemHeight;
    int scrollOffset;
    int visibleItems;
    
public:
    UIMenuList(int id, int x, int y, int width, int height, const String& name = "", int _itemHeight = 16)
        : UIMenu(id, WIDGET_MENU_LIST, x, y, width, height, name),
          itemHeight(_itemHeight), scrollOffset(0) {
        visibleItems = (height - 4) / itemHeight;  // 减去边框的高度
    }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        drawMenuBorder(display);
        
        // 绘制菜单项
        int startY = y + 2;
        int drawCount = min(visibleItems, itemCount - scrollOffset);
        
        for (int i = 0; i < drawCount; i++) {
            int itemIndex = scrollOffset + i;
            if (itemIndex >= itemCount || !items[itemIndex]) continue;
            
            MenuItem* item = items[itemIndex];
            int itemY = startY + i * itemHeight;
            
            // 绘制选中背景
            if (focused && itemIndex == selectedIndex) {
                display->fillRect(x + 1, itemY, width - 2, itemHeight, selectedColor);
            }
            
            // 绘制文本
            uint16_t color = item->enabled ? textColor : disabledColor;
            if (focused && itemIndex == selectedIndex) {
                color = TFT_BLACK;  // 选中项使用黑色文字
            }
            
            display->setTextColor(color);
            display->setTextSize(1);
            display->setCursor(x + 4, itemY + (itemHeight - 8) / 2);
            display->print(item->text);
        }
    }
    
    bool handleSecondaryKeyEvent(const KeyEvent& event) override {
        if (itemCount == 0) return false;
        
        // 处理上下方向键
        if (event.up) {
            if (selectedIndex > 0) {
                selectedIndex--;
                // 调整滚动偏移
                if (selectedIndex < scrollOffset) {
                    scrollOffset = selectedIndex;
                }
            }
            return true;
        }
        
        if (event.down) {
            if (selectedIndex < itemCount - 1) {
                selectedIndex++;
                // 调整滚动偏移
                if (selectedIndex >= scrollOffset + visibleItems) {
                    scrollOffset = selectedIndex - visibleItems + 1;
                }
            }
            return true;
        }
        
        // 处理回车键选择项目
        if (event.enter) {
            MenuItem* item = getSelectedItem();
            if (item && item->enabled) {
                onItemSelected(item);
                secondaryFocus = false;  // 选择后退出二级焦点
            }
            return true;
        }
        
        return false;
    }
};

// 网格菜单控件
class UIMenuGrid : public UIMenu {
private:
    int columns;
    int rows;
    int itemWidth;
    int itemHeight;
    int selectedRow;
    int selectedCol;
    
public:
    UIMenuGrid(int id, int x, int y, int width, int height, int _columns, int _rows, const String& name = "")
        : UIMenu(id, WIDGET_MENU_GRID, x, y, width, height, name),
          columns(_columns), rows(_rows), selectedRow(0), selectedCol(0) {
        itemWidth = (width - 4) / columns;  // 减去边框
        itemHeight = (height - 4) / rows;
    }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        drawMenuBorder(display);
        
        // 绘制网格项
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < columns; col++) {
                int itemIndex = row * columns + col;
                if (itemIndex >= itemCount || !items[itemIndex]) continue;
                
                MenuItem* item = items[itemIndex];
                int itemX = x + 2 + col * itemWidth;
                int itemY = y + 2 + row * itemHeight;
                
                // 绘制选中背景
                if (focused && row == selectedRow && col == selectedCol) {
                    display->fillRect(itemX, itemY, itemWidth, itemHeight, selectedColor);
                }
                
                // 绘制边框
                display->drawRect(itemX, itemY, itemWidth, itemHeight, TFT_DARKGREY);
                
                // 绘制文本
                uint16_t color = item->enabled ? textColor : disabledColor;
                if (focused && row == selectedRow && col == selectedCol) {
                    color = TFT_BLACK;
                }
                
                display->setTextColor(color);
                display->setTextSize(1);
                
                // 计算文本居中位置
                int textWidth = item->text.length() * 6;
                int textX = itemX + (itemWidth - textWidth) / 2;
                int textY = itemY + (itemHeight - 8) / 2;
                
                display->setCursor(textX, textY);
                display->print(item->text);
            }
        }
    }
    
    bool handleSecondaryKeyEvent(const KeyEvent& event) override {
        if (itemCount == 0) return false;
        
        // 处理方向键
        if (event.up && selectedRow > 0) {
            selectedRow--;
            updateSelectedIndex();
            return true;
        }
        
        if (event.down && selectedRow < rows - 1) {
            int newIndex = (selectedRow + 1) * columns + selectedCol;
            if (newIndex < itemCount) {
                selectedRow++;
                updateSelectedIndex();
            }
            return true;
        }
        
        if (event.left && selectedCol > 0) {
            selectedCol--;
            updateSelectedIndex();
            return true;
        }
        
        if (event.right && selectedCol < columns - 1) {
            int newIndex = selectedRow * columns + (selectedCol + 1);
            if (newIndex < itemCount) {
                selectedCol++;
                updateSelectedIndex();
            }
            return true;
        }
        
        // 处理回车键选择项目
        if (event.enter) {
            MenuItem* item = getSelectedItem();
            if (item && item->enabled) {
                onItemSelected(item);
                secondaryFocus = false;
            }
            return true;
        }
        
        return false;
    }
    
private:
    void updateSelectedIndex() {
        selectedIndex = selectedRow * columns + selectedCol;
        if (selectedIndex >= itemCount) {
            selectedIndex = itemCount - 1;
            selectedRow = selectedIndex / columns;
            selectedCol = selectedIndex % columns;
        }
    }
};