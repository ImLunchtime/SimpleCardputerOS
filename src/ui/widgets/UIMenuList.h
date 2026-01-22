#pragma once
#include <M5Cardputer.h>
#include "UIMenu.h"
#include <smooth_ui_toolkit.hpp>
class UIMenuList : public UIMenu {
private:
    int itemHeight;
    int visibleItems;
    bool layoutDirty;
    smooth_ui_toolkit::SmoothSelectorMenu* smoothMenu;
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
    void rebuildSmoothMenuLayout() {
        int contentW = width - 2;
        int contentH = height - 2;
        
        // Lazy initialization
        if (!smoothMenu) {
            smoothMenu = new smooth_ui_toolkit::SmoothSelectorMenu();
            smooth_ui_toolkit::SmoothSelectorMenu::Config_t cfg;
            cfg.moveInLoop = false;
            cfg.cameraSize = smooth_ui_toolkit::Vector2((float)contentW, (float)contentH);
            cfg.readInputInterval = 16;
            cfg.renderInterval = 16;
            smoothMenu->setConfig(cfg);
        } else {
             // Just update camera size if needed
             smoothMenu->setCameraSize(contentW, contentH);
        }

        if (contentW <= 0 || contentH <= 0 || itemHeight <= 0) {
            return;
        }

        // Instead of deleting/recreating, we just clear options and re-add them if the count mismatches or just assume append?
        // But SmoothSelectorMenu doesn't have clearOptions(). We can't access _data directly as it is protected.
        // Wait, SmoothSelectorMenu has protected _data. We can't clear it easily without subclassing or recreating.
        // BUT, recreating is expensive.
        // Let's look at SmoothSelectorMenu.hpp again.
        // It has `getOptionList()` but no `clearOptionList()`.
        // So we MUST delete and recreate if we want to change options significantly?
        // Or we can add a method to SmoothSelectorMenu if we could modify library (user said NO).
        // So we are stuck with recreating.
        // BUT, we can avoid doing it on EVERY addItem.
        // We can just set a dirty flag.
        
        if (smoothMenu) {
             delete smoothMenu;
             smoothMenu = nullptr;
        }
        
        smoothMenu = new smooth_ui_toolkit::SmoothSelectorMenu();
        smooth_ui_toolkit::SmoothSelectorMenu::Config_t cfg;
        cfg.moveInLoop = false;
        cfg.cameraSize = smooth_ui_toolkit::Vector2((float)contentW, (float)contentH);
        cfg.readInputInterval = 16;
        cfg.renderInterval = 16;
        smoothMenu->setConfig(cfg);
        
        for (int i = 0; i < itemCount; i++) {
            smooth_ui_toolkit::SmoothSelectorMenu::Option_t option;
            option.keyframe =
                smooth_ui_toolkit::Vector4(0.0f, (float)(i * itemHeight), (float)contentW, (float)itemHeight);
            option.userData = nullptr;
            smoothMenu->addOption(option);
        }
        if (selectedIndex >= 0 && selectedIndex < itemCount) {
            smoothMenu->jumpTo(selectedIndex);
        }
        layoutDirty = false;
    }
public:
    UIMenuList(int id, int x, int y, int width, int height, const String& name = "")
        : UIMenu(id, WIDGET_MENU_LIST, x, y, width, height, name),
          itemHeight(18),
          smoothMenu(nullptr),
          layoutDirty(true) {
        visibleItems = (height - 4) / itemHeight;
        // Don't build immediately, wait for first update/draw
    }
    ~UIMenuList() override {
        if (smoothMenu) {
            delete smoothMenu;
            smoothMenu = nullptr;
        }
    }
    void addItem(const String& text, int itemId, bool enabled = true) {
        UIMenu::addItem(text, itemId, enabled);
        layoutDirty = true;
    }
    void addImageItem(const uint8_t* data, size_t dataSize, int itemId, bool enabled = true) {
        UIMenu::addImageItem(data, dataSize, itemId, enabled);
        layoutDirty = true;
    }
    void addImageItemFromFile(const String& filePath, int itemId, bool enabled = true) {
        UIMenu::addImageItemFromFile(filePath, itemId, enabled);
        layoutDirty = true;
    }
    void removeItem(int itemId) {
        UIMenu::removeItem(itemId);
        layoutDirty = true;
    }
    void clear() {
        UIMenu::clear();
        layoutDirty = true;
    }
    void draw(LovyanGFX* display) override {
        if (!visible) return;
        if (layoutDirty) rebuildSmoothMenuLayout();
        
        drawMenuBorder(display);
        int absX = getAbsoluteX();
        int absY = getAbsoluteY();
        int contentX = absX + 1;
        int contentY = absY + 1;
        int contentW = width - 2;
        int contentH = height - 2;
        smooth_ui_toolkit::Vector2 cameraOffset(0.0f, 0.0f);
        if (smoothMenu) {
            cameraOffset = smoothMenu->getCameraOffset();
        }
        int cameraY = (int)cameraOffset.y;
        for (int i = 0; i < itemCount; i++) {
            if (!items[i]) continue;
            MenuItem* item = items[i];
            int itemLocalY = i * itemHeight - cameraY;
            int itemY = contentY + itemLocalY;
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
                params.selected = (focused && i == selectedIndex);
                params.enabled = item->enabled;
                params.selectedColor = selectedColor;
                params.textColor = textColor;
                params.disabledColor = disabledColor;
                currentTheme->drawMenuItem(params);
            } else {
                if (focused && i == selectedIndex) {
                    display->fillRect(contentX, itemY, contentW, itemHeight, selectedColor);
                }
                uint16_t color = item->enabled ? textColor : disabledColor;
                if (focused && i == selectedIndex) {
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
        if (!visible || itemHeight <= 0 || itemCount == 0) return false;
        if (layoutDirty) rebuildSmoothMenuLayout();
        if (!smoothMenu) return false;
        smoothMenu->update(nowMs);
        bool selectorMoving = !smoothMenu->getSelectorPostion().done();
        bool shapeChanging = !smoothMenu->getSelectorShape().done();
        bool cameraMoving = !smoothMenu->getCamera().done();
        return selectorMoving || shapeChanging || cameraMoving;
    }
    bool handleSecondaryKeyEvent(const KeyEvent& event) override {
        if (itemCount == 0) return false;
        if (layoutDirty) rebuildSmoothMenuLayout();
        if (!smoothMenu) return false;
        if (event.up) {
            smoothMenu->goLast();
            selectedIndex = smoothMenu->getSelectedOptionIndex();
            invalidate();
            return true;
        }
        if (event.down) {
            smoothMenu->goNext();
            selectedIndex = smoothMenu->getSelectedOptionIndex();
            invalidate();
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
