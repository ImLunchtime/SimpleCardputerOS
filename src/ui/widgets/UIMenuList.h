#pragma once
#include <M5Cardputer.h>
#include "UIMenu.h"
#include <smooth_ui_toolkit.hpp>
class UIMenuList : public UIMenu {
private:
    int itemHeight;
    int scrollOffset;
    int visibleItems;
    float scrollPixel;
    float targetScrollPixel;
    bool animating;
    smooth_ui_toolkit::AnimateValue scrollAnim;
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
    void setScrollOffsetAnimated(int newScrollOffset) {
        float current = animating ? scrollAnim.directValue() : ((float)scrollOffset * (float)itemHeight);
        scrollOffset = newScrollOffset;
        targetScrollPixel = (float)scrollOffset * (float)itemHeight;
        scrollAnim.teleport(current);
        auto& spring = scrollAnim.springOptions();
        spring.visualDuration = 1.2f;
        spring.bounce = 0.2f;
        scrollAnim.move(targetScrollPixel);
        scrollPixel = current;
        animating = true;
        invalidate();
    }
public:
    UIMenuList(int id, int x, int y, int width, int height, const String& name = "")
        : UIMenu(id, WIDGET_MENU_LIST, x, y, width, height, name),
          itemHeight(18), scrollOffset(0), scrollPixel(0.0f), targetScrollPixel(0.0f), animating(false) {
        visibleItems = (height - 4) / itemHeight;
    }
    void draw(LovyanGFX* display) override {
        if (!visible) return;
        drawMenuBorder(display);
        int absX = getAbsoluteX();
        int absY = getAbsoluteY();
        int contentX = absX + 1;
        int contentY = absY + 1;
        int contentW = width - 2;
        int contentH = height - 2;

        float sp = animating ? scrollPixel : ((float)scrollOffset * (float)itemHeight);
        if (sp < 0.0f) sp = 0.0f;
        int maxTopIndex = max(0, itemCount - visibleItems);
        float maxSp = (float)maxTopIndex * (float)itemHeight;
        if (sp > maxSp) sp = maxSp;

        int firstIndex = itemHeight > 0 ? (int)(sp / (float)itemHeight) : 0;
        int yOffset = itemHeight > 0 ? -(int)(sp - (float)firstIndex * (float)itemHeight) : 0;
        int drawCount = min(itemCount - firstIndex, visibleItems + 2);

        for (int i = 0; i < drawCount; i++) {
            int itemIndex = firstIndex + i;
            if (itemIndex >= itemCount || itemIndex < 0 || !items[itemIndex]) continue;
            MenuItem* item = items[itemIndex];
            int itemY = contentY + yOffset + i * itemHeight;
            if (itemY + itemHeight < contentY || itemY > contentY + contentH) continue;
            Theme* currentTheme = getCurrentTheme();
            if (currentTheme) {
                MenuItemDrawParams params;
                params.display = display;
                params.x = contentX;
                params.y = itemY;
                params.width = contentW;
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
                    display->fillRect(contentX, itemY, contentW, itemHeight, selectedColor);
                }
                uint16_t color = item->enabled ? textColor : disabledColor;
                if (focused && itemIndex == selectedIndex) {
                    color = TFT_BLACK;
                }
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(color);
                display->setTextSize(1);
                display->setCursor(contentX + 3, itemY + (itemHeight - 8) / 2);
                String clippedText = clipText(item->text, width - 2);
                display->print(clippedText);
            }
        }
    }
    bool update(uint32_t nowMs) override {
        if (!visible || itemHeight <= 0) return false;
        if (!animating) return false;
        float currentTimeS = (float)nowMs / 1000.0f;
        scrollAnim.update(currentTimeS);
        float newValue = scrollAnim.directValue();
        scrollPixel = newValue;
        float diff = targetScrollPixel - scrollPixel;
        float absDiff = diff >= 0.0f ? diff : -diff;
        if (absDiff < 0.5f || scrollAnim.currentPlayingState() == smooth_ui_toolkit::AnimateState::Completed) {
            scrollPixel = targetScrollPixel;
            animating = false;
        }
        return true;
    }
    bool handleSecondaryKeyEvent(const KeyEvent& event) override {
        if (itemCount == 0) return false;
        if (event.up) {
            if (selectedIndex > 0) {
                selectedIndex--;
                if (selectedIndex < scrollOffset) {
                    setScrollOffsetAnimated(selectedIndex);
                }
                invalidate();
            }
            return true;
        }
        if (event.down) {
            if (selectedIndex < itemCount - 1) {
                selectedIndex++;
                if (selectedIndex >= scrollOffset + visibleItems) {
                    setScrollOffsetAnimated(selectedIndex - visibleItems + 1);
                }
                invalidate();
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
