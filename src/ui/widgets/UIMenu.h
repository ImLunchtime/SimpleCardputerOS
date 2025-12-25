#pragma once
#include <M5Cardputer.h>
#include "WidgetBase.h"
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
class UIMenu : public UIWidget {
protected:
    MenuItem* items[20];
    int itemCount;
    int selectedIndex;
    bool secondaryFocus;
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
            invalidate();
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
            invalidate();
        }
    }
    void addImageItemFromFile(const String& filePath, int itemId, bool enabled = true) {
        if (itemCount < 20) {
            MenuItem* m = new MenuItem(String(""), itemId, enabled);
            m->imageFilePath = filePath;
            m->useFileImage = true;
            items[itemCount] = m;
            itemCount++;
            invalidate();
        }
    }
    void removeItem(int itemId) {
        for (int i = 0; i < itemCount; i++) {
            if (items[i] && items[i]->id == itemId) {
                delete items[i];
                for (int j = i; j < itemCount - 1; j++) {
                    items[j] = items[j + 1];
                }
                items[itemCount - 1] = nullptr;
                itemCount--;
                if (selectedIndex >= itemCount && itemCount > 0) {
                    selectedIndex = itemCount - 1;
                } else if (itemCount == 0) {
                    selectedIndex = 0;
                }
                invalidate();
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
        invalidate();
    }
    MenuItem* getSelectedItem() {
        if (selectedIndex >= 0 && selectedIndex < itemCount && items[selectedIndex]) {
            return items[selectedIndex];
        }
        return nullptr;
    }
    void setColors(uint16_t border, uint16_t selected, uint16_t text, uint16_t disabled) {
        if (borderColor == border && selectedColor == selected && textColor == text && disabledColor == disabled) return;
        borderColor = border;
        selectedColor = selected;
        textColor = text;
        disabledColor = disabled;
        invalidate();
    }
    bool hasSecondaryFocus() const override { return focused; }
    void onFocusChanged(bool hasFocus) override {}
    bool handleKeyEvent(const KeyEvent& event) override {
        if (!focused || !visible || itemCount == 0) return false;
        return handleSecondaryKeyEvent(event);
    }
    virtual void onItemSelected(MenuItem* item) {}
protected:
    void drawMenuBorder(LGFX_Device* display) {
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
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            display->fillRect(absX, absY, width, height, TFT_BLACK);
            display->drawRect(absX, absY, width, height, borderColor);
            if (focused) {
                uint16_t focusColor = TFT_YELLOW;
                if (width > 2 && height > 2) display->drawRect(absX + 1, absY + 1, width - 2, height - 2, focusColor);
                if (width > 4 && height > 4) display->drawRect(absX + 2, absY + 2, width - 4, height - 4, focusColor);
            }
        }
    }
};
