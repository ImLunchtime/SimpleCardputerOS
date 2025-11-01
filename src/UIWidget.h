#pragma once
#include <M5Cardputer.h>
#include <SD.h>
#include "EventSystem.h"
#include "ThemeManager.h"

// UI控件类型枚举
enum UIWidgetType {
    WIDGET_LABEL,
    WIDGET_BUTTON,
    WIDGET_WINDOW,
    WIDGET_MENU_LIST,
    WIDGET_MENU_GRID,
    WIDGET_SLIDER,
    WIDGET_IMAGE
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
    
    // 局部重绘支持
    virtual void clearArea(LGFX_Device* display) {
        // 默认实现：用黑色填充控件区域
        display->fillRect(x, y, width, height, TFT_BLACK);
    }
    
    virtual void drawPartial(LGFX_Device* display) {
        // 默认实现：先清除区域，再绘制
        clearArea(display);
        draw(display);
    }
    
    // 应用窗口局部清屏支持
    virtual void clearAppArea(LGFX_Device* display) {
        // 对于应用窗口，清除整个应用区域（除启动器外的区域）
        if (type == WIDGET_WINDOW) {
            display->fillRect(x, y, width, height, TFT_BLACK);
        } else {
            // 对于其他控件，使用标准清屏
            clearArea(display);
        }
    }
    
    virtual void drawAppPartial(LGFX_Device* display) {
        // 应用级局部重绘：先清除应用区域，再绘制
        clearAppArea(display);
        draw(display);
    }
    
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
    
    String getText() const { return text; }  // 添加getText方法
    
    void setTextColor(uint16_t color) { textColor = color; }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        Theme* theme = getCurrentTheme();
        if (theme) {
            ThemeDrawParams params;
            params.display = display;
            params.x = x;
            params.y = y;
            params.width = width;
            params.height = height;
            params.visible = visible;
            params.text = text;
            params.textColor = textColor;
            
            theme->drawLabel(params);
        } else {
            // 回退到原始绘制方式
            display->setFont(&fonts::efontCN_12);
            display->setTextColor(textColor);
            display->setTextSize(1);
            display->setCursor(x, y);
            display->print(text);
        }
    }
    
    void drawPartial(LGFX_Device* display) override {
        if (!visible) return;
        
        // 局部重绘：先清除文本区域，再绘制新文本
        clearArea(display);
        draw(display);
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
        
        Theme* theme = getCurrentTheme();
        if (theme) {
            ThemeDrawParams params;
            params.display = display;
            params.x = x;
            params.y = y;
            params.width = width;
            params.height = height;
            params.visible = visible;
            params.focused = focused;
            params.text = text;
            params.textColor = textColor;
            params.borderColor = borderColor;
            params.backgroundColor = TFT_BLACK;
            
            theme->drawButton(params);
        } else {
            // 回退到原始绘制方式
            display->fillRect(x, y, width, height, TFT_BLACK);
            display->drawRect(x, y, width, height, borderColor);
            
            if (focused) {
                display->drawRect(x - 1, y - 1, width + 2, height + 2, TFT_YELLOW);
                display->drawRect(x - 2, y - 2, width + 4, height + 4, TFT_YELLOW);
            }
            
            display->setFont(&fonts::efontCN_12);
            display->setTextSize(1);
            int textWidth = text.length() * 6;
            int textHeight = 8;
            int textX = x + (width - textWidth) / 2;
            int textY = y + (height - textHeight) / 2;
            
            display->setTextColor(textColor);
            display->setCursor(textX, textY);
            display->print(text);
        }
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
        
        Theme* theme = getCurrentTheme();
        if (theme) {
            ThemeDrawParams params;
            params.display = display;
            params.x = x;
            params.y = y;
            params.width = width;
            params.height = height;
            params.visible = visible;
            params.text = title;
            params.textColor = TFT_WHITE;
            params.borderColor = borderColor;
            params.backgroundColor = TFT_BLACK;
            
            theme->drawWindow(params);
        } else {
            // 回退到原始绘制方式
            display->fillRect(x, y, width, height, TFT_BLACK);
            display->drawRect(x, y, width, height, borderColor);
            
            if (!title.isEmpty()) {
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(TFT_WHITE);
                display->setTextSize(1);
                display->setCursor(x + 5, y + 3);
                display->print(title);
            }
        }
    }
    
    void drawPartial(LGFX_Device* display) override {
        if (!visible) return;
        
        // 窗口的局部重绘：先清除整个窗口区域，再重绘
        clearArea(display);
        draw(display);
    }
    
    void clearAppArea(LGFX_Device* display) override {
        if (!visible) return;
        
        // 应用窗口清屏：清除整个窗口区域
        display->fillRect(x, y, width, height, TFT_BLACK);
    }
    
    void drawAppPartial(LGFX_Device* display) override {
        if (!visible) return;
        
        // 应用窗口局部重绘：先清除应用区域，再重绘
        clearAppArea(display);
        draw(display);
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
        selectedIndex = 1;
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
        // 使用主题系统绘制菜单边框
        Theme* currentTheme = getCurrentTheme();
        if (currentTheme) {
            ThemeDrawParams params;
            params.x = x;
            params.y = y;
            params.width = width;
            params.height = height;
            params.focused = focused;
            params.borderColor = borderColor;
            
            currentTheme->drawMenuBorder(params);
        } else {
            // 回退到原始绘制方法
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
    }
};

// 滑块控件
class UISlider : public UIWidget {
private:
    int minValue;
    int maxValue;
    int currentValue;
    uint16_t trackColor;
    uint16_t thumbColor;
    uint16_t focusColor;
    String label;
    bool showValue;
    
public:
    UISlider(int id, int x, int y, int width, int height, int min, int max, int initial, const String& _label = "", const String& name = "")
        : UIWidget(id, WIDGET_SLIDER, x, y, width, height, name, true),
          minValue(min), maxValue(max), currentValue(initial), 
          trackColor(TFT_DARKGREY), thumbColor(TFT_WHITE), focusColor(TFT_YELLOW),
          label(_label), showValue(true) {
        
        // 确保初始值在范围内
        if (currentValue < minValue) currentValue = minValue;
        if (currentValue > maxValue) currentValue = maxValue;
    }
    
    void setValue(int value) {
        if (value < minValue) value = minValue;
        if (value > maxValue) value = maxValue;
        if (currentValue != value) {
            currentValue = value;
            onValueChanged(currentValue);
        }
    }
    
    int getValue() const { return currentValue; }
    void setRange(int min, int max) { 
        minValue = min; 
        maxValue = max; 
        if (currentValue < minValue) setValue(minValue);
        if (currentValue > maxValue) setValue(maxValue);
    }
    
    void setColors(uint16_t track, uint16_t thumb, uint16_t focus) {
        trackColor = track;
        thumbColor = thumb;
        focusColor = focus;
    }
    
    void setShowValue(bool show) { showValue = show; }
    void setLabel(const String& newLabel) { label = newLabel; }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        // 使用主题系统绘制滑块
        Theme* currentTheme = getCurrentTheme();
        if (currentTheme) {
            SliderDrawParams params;
            params.display = display;
            params.x = x;
            params.y = y;
            params.width = width;
            params.height = height;
            params.minValue = minValue;
            params.maxValue = maxValue;
            params.currentValue = currentValue;
            params.label = label;
            params.showValue = showValue;
            params.focused = focused;
            params.trackColor = trackColor;
            params.thumbColor = thumbColor;
            
            currentTheme->drawSlider(params);
        } else {
            // 回退到原始绘制方法
            // 绘制标签
            if (label.length() > 0) {
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(TFT_WHITE);
                display->setTextSize(1);
                display->setCursor(x, y - 12);
                display->print(label);
            }
            
            // 计算滑块轨道区域
            int trackY = y + (height - 4) / 2;
            int trackHeight = 4;
            
            // 绘制轨道
            uint16_t borderColor = focused ? focusColor : TFT_WHITE;
            display->drawRect(x, trackY, width, trackHeight, borderColor);
            display->fillRect(x + 1, trackY + 1, width - 2, trackHeight - 2, trackColor);
            
            // 计算滑块位置
            int range = maxValue - minValue;
            int thumbX = x;
            if (range > 0) {
                thumbX = x + ((currentValue - minValue) * (width - 8)) / range;
            }
            
            // 绘制滑块
            uint16_t currentThumbColor = focused ? focusColor : thumbColor;
            display->fillRect(thumbX, y, 8, height, currentThumbColor);
            display->drawRect(thumbX, y, 8, height, TFT_BLACK);
            
            // 显示数值
            if (showValue) {
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(TFT_WHITE);
                display->setTextSize(1);
                String valueText = String(currentValue);
                int textWidth = valueText.length() * 6;
                display->setCursor(x + width - textWidth, y + height + 2);
                display->print(valueText);
            }
        }
    }
    
    bool handleKeyEvent(const KeyEvent& event) override {
        if (!focused) return false;
        
        bool handled = false;
        int step = (maxValue - minValue) / 20; // 5%步长
        if (step < 1) step = 1;
        
        if (event.left) {
            setValue(currentValue - step);
            handled = true;
        } else if (event.right) {
            setValue(currentValue + step);
            handled = true;
        }
        
        return handled;
    }
    
    virtual void onValueChanged(int newValue) {
        // 子类可以重写此方法来处理值变化
    }
};

// 列表菜单控件
class UIMenuList : public UIMenu {
private:
    int itemHeight;
    int scrollOffset;
    int visibleItems;
    
    // 文字裁剪辅助方法
    String clipText(const String& text, int maxWidth) {
        if (text.length() == 0) return text;
        
        // 计算可用宽度（减去左右边距和省略号宽度）
        int availableWidth = maxWidth - 8;  // 左边距4px + 右边距4px
        int ellipsisWidth = 18;  // "..." 大约18像素宽（6像素/字符 * 3字符）
        
        // 如果文本很短，直接返回
        if (text.length() * 6 <= availableWidth) {
            return text;
        }
        
        // 计算可以显示的字符数（为省略号留出空间）
        int maxChars = (availableWidth - ellipsisWidth) / 6;
        if (maxChars <= 0) {
            return "...";  // 如果空间太小，只显示省略号
        }
        
        // 裁剪文本并添加省略号
        return text.substring(0, maxChars) + "...";
    }
    
public:
    UIMenuList(int id, int x, int y, int width, int height, const String& name = "", int _itemHeight = 14)
        : UIMenu(id, WIDGET_MENU_LIST, x, y, width, height, name),
          itemHeight(_itemHeight), scrollOffset(0) {
        visibleItems = (height - 4) / itemHeight;  // 减去边框的高度
    }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        drawMenuBorder(display);
        
        // 绘制菜单项
        int startY = y;
        int drawCount = min(visibleItems, itemCount - scrollOffset);
        
        for (int i = 0; i < drawCount; i++) {
            int itemIndex = scrollOffset + i;
            if (itemIndex >= itemCount || !items[itemIndex]) continue;
            
            MenuItem* item = items[itemIndex];
            int itemY = startY + i * itemHeight;
            
            // 使用主题系统绘制菜单项
            Theme* currentTheme = getCurrentTheme();
            if (currentTheme) {
                MenuItemDrawParams params;
                params.display = display;
                params.x = x + 1;
                params.y = itemY;
                params.width = width - 2;
                params.height = itemHeight;
                params.text = clipText(item->text, width - 2);
                params.selected = (focused && itemIndex == selectedIndex);
                params.enabled = item->enabled;
                params.selectedColor = selectedColor;
                params.textColor = textColor;
                params.disabledColor = disabledColor;
                
                currentTheme->drawMenuItem(params);
            } else {
                // 回退到原始绘制方法
                // 绘制选中背景
                if (focused && itemIndex == selectedIndex) {
                    display->fillRect(x + 1, itemY, width - 2, itemHeight, selectedColor);
                }
                
                // 绘制文本（使用裁剪后的文本）
                uint16_t color = item->enabled ? textColor : disabledColor;
                if (focused && itemIndex == selectedIndex) {
                    color = TFT_BLACK;  // 选中项使用黑色文字
                }
                
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(color);
                display->setTextSize(1);
                display->setCursor(x + 4, itemY + (itemHeight - 8) / 2);
                
                // 使用裁剪后的文本
                String clippedText = clipText(item->text, width - 2);
                display->print(clippedText);
            }
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
                 
                 display->setFont(&fonts::efontCN_12);
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

// UIImage类 - 用于显示PNG图片
class UIImage : public UIWidget {
private:
    const uint8_t* imageData;    // 图片数据指针
    size_t imageDataSize;        // 图片数据大小
    String filePath;             // SD卡文件路径
    bool useFile;                // 是否使用文件模式
    float scaleX, scaleY;        // 缩放比例
    bool maintainAspectRatio;    // 是否保持宽高比
    
public:
    // 构造函数 - 从数组加载
    UIImage(int id, int x, int y, int width, int height, const uint8_t* data, size_t dataSize, const String& name = "")
        : UIWidget(id, WIDGET_IMAGE, x, y, width, height, name, false),
          imageData(data), imageDataSize(dataSize), useFile(false),
          scaleX(1.0f), scaleY(1.0f), maintainAspectRatio(true) {
    }
    
    // 构造函数 - 从SD卡文件加载
    UIImage(int id, int x, int y, int width, int height, const String& file, const String& name = "")
        : UIWidget(id, WIDGET_IMAGE, x, y, width, height, name, false),
          imageData(nullptr), imageDataSize(0), filePath(file), useFile(true),
          scaleX(1.0f), scaleY(1.0f), maintainAspectRatio(true) {
    }
    
    void setImageData(const uint8_t* data, size_t dataSize) {
        imageData = data;
        imageDataSize = dataSize;
        useFile = false;
    }
    
    void setImageFile(const String& file) {
        filePath = file;
        useFile = true;
    }
    
    void setScale(float x, float y) {
        scaleX = x;
        scaleY = y;
    }
    
    void setMaintainAspectRatio(bool maintain) {
        maintainAspectRatio = maintain;
    }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        if (useFile && filePath.length() > 0) {
            // 从SD卡文件绘制 - 使用简化的方法
            display->drawPngFile(filePath.c_str(), x, y);
        } else if (imageData && imageDataSize > 0) {
            // 从数组绘制
            display->drawPng(imageData, imageDataSize, x, y, width, height, 0, 0,
                           maintainAspectRatio ? 0 : scaleX,
                           maintainAspectRatio ? 0 : scaleY);
        }
    }
    
    bool handleKeyEvent(const KeyEvent& event) override {
        // 图片不处理键盘事件
        return false;
    }
};