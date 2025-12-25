#pragma once
#include <M5Cardputer.h>
#include <SD.h>
#include "system/EventSystem.h"
#include "themes/ThemeManager.h"

// UI控件类型枚举
enum UIWidgetType {
    WIDGET_LABEL,
    WIDGET_BUTTON,
    WIDGET_WINDOW,
    WIDGET_MENU_LIST,
    WIDGET_MENU_GRID,
    WIDGET_SLIDER,
    WIDGET_IMAGE,
    WIDGET_SCREEN
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
    UIWidget* parent;
    
public:
    UIWidget(int _id, UIWidgetType _type, int _x, int _y, int _w, int _h, const String& _name, bool _focusable = false)
        : id(_id), type(_type), x(_x), y(_y), width(_w), height(_h), name(_name), 
          visible(true), focusable(_focusable), focused(false), parent(nullptr) {}
    
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
    void setParent(UIWidget* p) { parent = p; }
    UIWidget* getParent() const { return parent; }
    
    // 位置和大小
    void setPosition(int _x, int _y) { x = _x; y = _y; }
    void setSize(int _w, int _h) { width = _w; height = _h; }
    void getBounds(int& _x, int& _y, int& _w, int& _h) const {
        _x = x; _y = y; _w = width; _h = height;
    }
    // 父容器对子元素的偏移（默认无偏移）
    virtual int getChildOffsetX() const { return 0; }
    virtual int getChildOffsetY() const { return 0; }

    // 绝对坐标计算：包含父容器对子元素的偏移
    int getAbsoluteX() const { return parent ? parent->getAbsoluteX() + parent->getChildOffsetX() + x : x; }
    int getAbsoluteY() const { return parent ? parent->getAbsoluteY() + parent->getChildOffsetY() + y : y; }
    void getAbsoluteBounds(int& _x, int& _y, int& _w, int& _h) const {
        _x = getAbsoluteX();
        _y = getAbsoluteY();
        _w = width;
        _h = height;
    }
    
    // 纯虚函数，子类必须实现
    virtual void draw(LGFX_Device* display) = 0;
    virtual bool handleKeyEvent(const KeyEvent& event) = 0;
    virtual void onFocusChanged(bool hasFocus) {}
    
    // 局部重绘支持
    virtual void clearArea(LGFX_Device* display) {
        // 默认实现：用黑色填充控件区域（使用绝对坐标）
        int ax, ay, aw, ah;
        getAbsoluteBounds(ax, ay, aw, ah);
        display->fillRect(ax, ay, aw, ah, TFT_BLACK);
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
            int ax, ay, aw, ah;
            getAbsoluteBounds(ax, ay, aw, ah);
            display->fillRect(ax, ay, aw, ah, TFT_BLACK);
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
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            params.x = absX;
            params.y = absY;
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
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            display->setCursor(absX, absY);
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
    const uint8_t* imageData;
    size_t imageDataSize;
    String imageFilePath;
    bool useFileImage;
    
public:
    UIButton(int id, int x, int y, int width, int height, const String& text, const String& name = "")
        : UIWidget(id, WIDGET_BUTTON, x, y, width, height, name, true),
          text(text), borderColor(TFT_BLUE), textColor(TFT_WHITE),
          imageData(nullptr), imageDataSize(0), imageFilePath(""), useFileImage(false) {}
    
    void setText(const String& newText) { text = newText; }
    void setBorderColor(uint16_t color) { borderColor = color; }
    void setTextColor(uint16_t color) { textColor = color; }
    void setImageData(const uint8_t* data, size_t dataSize) { imageData = data; imageDataSize = dataSize; useFileImage = false; }
    void setImageFile(const String& file) { imageFilePath = file; useFileImage = true; }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        Theme* theme = getCurrentTheme();
        if (theme) {
            ThemeDrawParams params;
            params.display = display;
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            params.x = absX;
            params.y = absY;
            params.width = width;
            params.height = height;
            params.visible = visible;
            params.focused = focused;
            params.text = (imageData || useFileImage) ? String("") : text;
            params.textColor = textColor;
            params.borderColor = borderColor;
            params.backgroundColor = TFT_BLACK;
            params.imageData = imageData;
            params.imageDataSize = imageDataSize;
            params.filePath = imageFilePath;
            params.useFile = useFileImage;
            
            theme->drawButton(params);
        } else {
            // 回退到原始绘制方式
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            display->fillRect(absX, absY, width, height, TFT_BLACK);
            display->drawRect(absX, absY, width, height, borderColor);
            
            if (focused) {
                display->drawRect(absX - 1, absY - 1, width + 2, height + 2, TFT_YELLOW);
                display->drawRect(absX - 2, absY - 2, width + 4, height + 4, TFT_YELLOW);
            }
            
            if (!(imageData || useFileImage)) {
                display->setFont(&fonts::efontCN_12);
                display->setTextSize(1);
                int textWidth = text.length() * 6;
                int textHeight = 8;
                int textX = absX + (width - textWidth) / 2;
                int textY = absY + (height - textHeight) / 2;
                display->setTextColor(textColor);
                display->setCursor(textX, textY);
                display->print(text);
            } else {
                int imgW = 0, imgH = 0;
                bool ok = false;
                if (imageData && imageDataSize > 24) {
                    ok = pngGetSize(imageData, imageDataSize, imgW, imgH);
                } else if (useFileImage && imageFilePath.length() > 0) {
                    ok = pngFileGetSize(imageFilePath, imgW, imgH);
                }
                if (ok) {
                    int maxW = width - 2;
                    int maxH = height - 2;
                    if (imageData) {
                        float scale = 1.0f;
                        // if (imgW > 0 && imgH > 0) {
                        //     float sx = (float)maxW / (float)imgW;
                        //     float sy = (float)maxH / (float)imgH;
                        //     scale = sx < sy ? sx : sy;
                        //     if (scale > 1.0f) scale = 1.0f;
                        // }
                        int dw = (int)(imgW * scale);
                        int dh = (int)(imgH * scale);
                        int cx = absX + (width - dw) / 2;
                        int cy = absY + (height - dh) / 2;
                        display->drawPng(imageData, imageDataSize, cx, cy, dw, dh, 0, 0, scale, scale);
                    } else {
                        int cx = absX + (width - imgW) / 2;
                        int cy = absY + (height - imgH) / 2;
                        display->drawPngFile(imageFilePath.c_str(), cx, cy);
                    }
                }
            }
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
    int childOffsetX;
    int childOffsetY;
    
public:
    UIWindow(int id, int x, int y, int width, int height, const String& title = "", const String& name = "")
        : UIWidget(id, WIDGET_WINDOW, x, y, width, height, name, false),
          title(title), borderColor(TFT_WHITE), childOffsetX(-6), childOffsetY(-6) {}

    // 窗口对子元素应用统一偏移：默认左上角移动约6像素，可配置
    int getChildOffsetX() const override { return childOffsetX; }
    int getChildOffsetY() const override { return childOffsetY; }
    void setChildOffset(int ox, int oy) { childOffsetX = ox; childOffsetY = oy; }
    
    void setTitle(const String& newTitle) { title = newTitle; }
    void setBorderColor(uint16_t color) { borderColor = color; }
    
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        
        Theme* theme = getCurrentTheme();
        if (theme) {
            ThemeDrawParams params;
            params.display = display;
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            params.x = absX;
            params.y = absY;
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
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            display->fillRect(absX, absY, width, height, TFT_BLACK);
            display->drawRect(absX, absY, width, height, borderColor);
            
            if (!title.isEmpty()) {
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(TFT_WHITE);
                display->setTextSize(1);
                display->setCursor(absX + 5, absY + 3);
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

// 屏幕根容器，不可聚焦，不绘制内容，仅用于作为父对象
class UIScreen : public UIWidget {
public:
    UIScreen(int id, int width, int height, const String& name = "")
        : UIWidget(id, WIDGET_SCREEN, 0, 0, width, height, name, false) {}

    void draw(LGFX_Device* display) override {
        // 根屏幕不绘制任何东西
    }

    bool handleKeyEvent(const KeyEvent& event) override { return false; }
};

// 菜单项结构
struct MenuItem {
    String text;
    int id;
    bool enabled;
    const uint8_t* imageData;
    size_t imageDataSize;
    String imageFilePath;
    bool useFileImage;
    
    MenuItem(const String& _text, int _id, bool _enabled = true)
        : text(_text), id(_id), enabled(_enabled), imageData(nullptr), imageDataSize(0), imageFilePath(""), useFileImage(false) {}
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

    void addImageItem(const uint8_t* data, size_t dataSize, int itemId, bool enabled = true) {
        if (itemCount < 20) {
            MenuItem* m = new MenuItem(String(""), itemId, enabled);
            m->imageData = data;
            m->imageDataSize = dataSize;
            m->useFileImage = false;
            items[itemCount] = m;
            itemCount++;
        }
    }

    void addImageItemFromFile(const String& filePath, int itemId, bool enabled = true) {
        if (itemCount < 20) {
            MenuItem* m = new MenuItem(String(""), itemId, enabled);
            m->imageFilePath = filePath;
            m->useFileImage = true;
            items[itemCount] = m;
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
        // 使用主题系统绘制菜单边框
        Theme* currentTheme = getCurrentTheme();
        if (currentTheme) {
            ThemeDrawParams params;
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            params.x = absX;
            params.y = absY;
            params.width = width;
            params.height = height;
            params.focused = focused;
            params.borderColor = borderColor;
            
            currentTheme->drawMenuBorder(params);
        } else {
            // 回退到原始绘制方法
            // 绘制背景
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            display->fillRect(absX, absY, width, height, TFT_BLACK);
            
            // 绘制边框
            display->drawRect(absX, absY, width, height, borderColor);
            
            // 如果聚焦，绘制焦点框
            if (focused) {
                uint16_t focusColor = TFT_YELLOW;  // 一级焦点使用黄色
                display->drawRect(absX - 1, absY - 1, width + 2, height + 2, focusColor);
                display->drawRect(absX - 2, absY - 2, width + 4, height + 4, focusColor);
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
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            params.x = absX;
            params.y = absY;
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
                int absX = getAbsoluteX();
                int absY = getAbsoluteY();
                display->setCursor(absX, absY - 12);
                display->print(label);
            }
            
            // 计算滑块轨道区域
            int absX2 = getAbsoluteX();
            int absY2 = getAbsoluteY();
            int trackY = absY2 + (height - 4) / 2;
            int trackHeight = 4;
            
            // 绘制轨道
            uint16_t borderColor = focused ? focusColor : TFT_WHITE;
            display->drawRect(absX2, trackY, width, trackHeight, borderColor);
            display->fillRect(absX2 + 1, trackY + 1, width - 2, trackHeight - 2, trackColor);
            
            // 计算滑块位置
            int range = maxValue - minValue;
            int thumbX = absX2;
            if (range > 0) {
                thumbX = absX2 + ((currentValue - minValue) * (width - 8)) / range;
            }
            
            // 绘制滑块
            uint16_t currentThumbColor = focused ? focusColor : thumbColor;
            display->fillRect(thumbX, absY2, 8, height, currentThumbColor);
            display->drawRect(thumbX, absY2, 8, height, TFT_BLACK);
            
            // 显示数值
            if (showValue) {
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(TFT_WHITE);
                display->setTextSize(1);
                String valueText = String(currentValue);
                int textWidth = valueText.length() * 6;
                display->setCursor(absX2 + width - textWidth, absY2 + height + 2);
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
        int absX = getAbsoluteX();
        int absY = getAbsoluteY();
        int startY = absY;
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
                params.x = absX + 1;
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
                    display->fillRect(absX + 1, itemY, width - 2, itemHeight, selectedColor);
                }
                
                // 绘制文本（使用裁剪后的文本）
                uint16_t color = item->enabled ? textColor : disabledColor;
                if (focused && itemIndex == selectedIndex) {
                    color = TFT_BLACK;  // 选中项使用黑色文字
                }
                
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(color);
                display->setTextSize(1);
                display->setCursor(absX + 4, itemY + (itemHeight - 8) / 2);
                
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
        int absX = getAbsoluteX();
        int absY = getAbsoluteY();
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < columns; col++) {
                int itemIndex = row * columns + col;
                if (itemIndex >= itemCount || !items[itemIndex]) continue;
                
                MenuItem* item = items[itemIndex];
                int itemX = absX + 2 + col * itemWidth;
                int itemY = absY + 2 + row * itemHeight;
                
                // 使用主题系统绘制网格菜单项
                Theme* currentTheme = getCurrentTheme();
                if (currentTheme) {
                    GridMenuItemDrawParams params;
                    params.display = display;
                    params.x = itemX;
                    params.y = itemY;
                    params.width = itemWidth;
                    params.height = itemHeight;
                    params.text = (item->imageData || item->useFileImage) ? String("") : item->text;
                    params.selected = (row == selectedRow && col == selectedCol);
                    params.enabled = item->enabled;
                    params.focused = focused;
                    params.textColor = textColor;
                    params.selectedColor = selectedColor;
                    params.disabledColor = disabledColor;
                    params.backgroundColor = TFT_BLACK;
                    params.borderColor = TFT_DARKGREY;
                    params.imageData = item->imageData;
                    params.imageDataSize = item->imageDataSize;
                    params.filePath = item->imageFilePath;
                    params.useFile = item->useFileImage;
                    
                    currentTheme->drawGridMenuItem(params);
                } else {
                    // 回退到原始绘制方法
                    if (focused && row == selectedRow && col == selectedCol) {
                        display->fillRect(itemX, itemY, itemWidth, itemHeight, selectedColor);
                    }
                    display->drawRect(itemX, itemY, itemWidth, itemHeight, TFT_DARKGREY);
                    if (item->imageData || item->useFileImage) {
                        int imgW = 0, imgH = 0;
                        bool ok = false;
                        if (item->imageData && item->imageDataSize > 24) ok = pngGetSize(item->imageData, item->imageDataSize, imgW, imgH);
                        else if (item->useFileImage && item->imageFilePath.length() > 0) ok = pngFileGetSize(item->imageFilePath, imgW, imgH);
                        if (ok) {
                            int maxW = itemWidth - 4;
                            int maxH = itemHeight - 4;
                            if (item->imageData) {
                                float sx = (float)maxW / (float)imgW;
                                float sy = (float)maxH / (float)imgH;
                                float scale = sx < sy ? sx : sy;
                                if (scale > 1.0f) scale = 1.0f;
                                int dw = (int)(imgW * scale);
                                int dh = (int)(imgH * scale);
                                int cx = itemX + (itemWidth - dw) / 2;
                                int cy = itemY + (itemHeight - dh) / 2;
                                display->drawPng(item->imageData, item->imageDataSize, cx, cy, dw, dh, 0, 0, scale, scale);
                            } else {
                                int cx = itemX + (itemWidth - imgW) / 2;
                                int cy = itemY + (itemHeight - imgH) / 2;
                                display->drawPngFile(item->imageFilePath.c_str(), cx, cy);
                            }
                        }
                    } else {
                        uint16_t color = item->enabled ? textColor : disabledColor;
                        if (focused && row == selectedRow && col == selectedCol) {
                            color = TFT_BLACK;
                        }
                        display->setFont(&fonts::efontCN_12);
                        display->setTextColor(color);
                        display->setTextSize(1);
                        int textWidth = item->text.length() * 6;
                        int textX = itemX + (itemWidth - textWidth) / 2;
                        int textY = itemY + (itemHeight - 8) / 2;
                        display->setCursor(textX, textY);
                        display->print(item->text);
                    }
                }
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
        
        int absX = getAbsoluteX();
        int absY = getAbsoluteY();
        if (useFile && filePath.length() > 0) {
            // 从SD卡文件绘制 - 使用简化的方法
            display->drawPngFile(filePath.c_str(), absX, absY);
        } else if (imageData && imageDataSize > 0) {
            // 从数组绘制
            display->drawPng(imageData, imageDataSize, absX, absY, width, height, 0, 0,
                           maintainAspectRatio ? 0 : scaleX,
                           maintainAspectRatio ? 0 : scaleY);
        }
    }
    
    bool handleKeyEvent(const KeyEvent& event) override {
        // 图片不处理键盘事件
        return false;
    }
};