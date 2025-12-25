#pragma once
#include <M5Cardputer.h>
#include "UIMenu.h"
class UIMenuList : public UIMenu {
private:
    int itemHeight;
    int scrollOffset;
    int visibleItems;
    String clipText(const String& text, int maxWidth) {
        if (text.length() == 0) return text;
        int availableWidth = maxWidth - 8;
        int ellipsisWidth = 18;
        if (text.length() * 6 <= availableWidth) {
            return text;
        }
        int maxChars = (availableWidth - ellipsisWidth) / 6;
        if (maxChars <= 0) {
            return "...";
        }
        return text.substring(0, maxChars) + "...";
    }
public:
    UIMenuList(int id, int x, int y, int width, int height, const String& name = "", int _itemHeight = 14)
        : UIMenu(id, WIDGET_MENU_LIST, x, y, width, height, name),
          itemHeight(_itemHeight), scrollOffset(0) {
        visibleItems = (height - 4) / itemHeight;
    }
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        drawMenuBorder(display);
        int absX = getAbsoluteX();
        int absY = getAbsoluteY();
        int startY = absY;
        int drawCount = min(visibleItems, itemCount - scrollOffset);
        for (int i = 0; i < drawCount; i++) {
            int itemIndex = scrollOffset + i;
            if (itemIndex >= itemCount || !items[itemIndex]) continue;
            MenuItem* item = items[itemIndex];
            int itemY = startY + i * itemHeight;
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
                if (focused && itemIndex == selectedIndex) {
                    display->fillRect(absX + 1, itemY, width - 2, itemHeight, selectedColor);
                }
                uint16_t color = item->enabled ? textColor : disabledColor;
                if (focused && itemIndex == selectedIndex) {
                    color = TFT_BLACK;
                }
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(color);
                display->setTextSize(1);
                display->setCursor(absX + 4, itemY + (itemHeight - 8) / 2);
                String clippedText = clipText(item->text, width - 2);
                display->print(clippedText);
            }
        }
    }
    bool handleSecondaryKeyEvent(const KeyEvent& event) override {
        if (itemCount == 0) return false;
        if (event.up) {
            if (selectedIndex > 0) {
                selectedIndex--;
                if (selectedIndex < scrollOffset) {
                    scrollOffset = selectedIndex;
                }
            }
            return true;
        }
        if (event.down) {
            if (selectedIndex < itemCount - 1) {
                selectedIndex++;
                if (selectedIndex >= scrollOffset + visibleItems) {
                    scrollOffset = selectedIndex - visibleItems + 1;
                }
            }
            return true;
        }
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
};
