#pragma once
#include <M5Cardputer.h>
#include "UIMenu.h"
#include <smooth_ui_toolkit.hpp>
class UIMenuGrid : public UIMenu {
private:
    int columns;
    int rows;
    int itemWidth;
    int itemHeight;
    int selectedRow;
    int selectedCol;
    smooth_ui_toolkit::SmoothSelectorMenu* smoothMenu;
public:
    UIMenuGrid(int id, int x, int y, int width, int height, int _columns, int _rows, const String& name = "")
        : UIMenu(id, WIDGET_MENU_GRID, x, y, width, height, name),
          columns(_columns), rows(_rows),
          selectedRow(0), selectedCol(0),
          smoothMenu(nullptr) {
        itemWidth = (width - 4) / columns;
        itemHeight = (height - 4) / rows;
    }
    ~UIMenuGrid() override {
        if (smoothMenu) {
            delete smoothMenu;
            smoothMenu = nullptr;
        }
    }
    void addItem(const String& text, int itemId, bool enabled = true) {
        UIMenu::addItem(text, itemId, enabled);
        rebuildSmoothMenuLayout();
    }
    void addImageItem(const uint8_t* data, size_t dataSize, int itemId, bool enabled = true) {
        UIMenu::addImageItem(data, dataSize, itemId, enabled);
        rebuildSmoothMenuLayout();
    }
    void addImageItemFromFile(const String& filePath, int itemId, bool enabled = true) {
        UIMenu::addImageItemFromFile(filePath, itemId, enabled);
        rebuildSmoothMenuLayout();
    }
    void removeItem(int itemId) {
        UIMenu::removeItem(itemId);
        rebuildSmoothMenuLayout();
    }
    void clear() {
        UIMenu::clear();
        rebuildSmoothMenuLayout();
    }
private:
    int totalRows() const {
        if (columns <= 0) return 0;
        if (itemCount <= 0) return 0;
        return (itemCount + columns - 1) / columns;
    }
    void rebuildSmoothMenuLayout() {
        if (smoothMenu) {
            delete smoothMenu;
            smoothMenu = nullptr;
        }
        if (columns <= 0 || rows <= 0 || itemWidth <= 0 || itemHeight <= 0 || itemCount <= 0) {
            return;
        }
        smoothMenu = new smooth_ui_toolkit::SmoothSelectorMenu();
        smooth_ui_toolkit::SmoothSelectorMenu::Config_t cfg;
        cfg.moveInLoop = false;
        cfg.cameraSize = smooth_ui_toolkit::Vector2(0.0f, 0.0f);
        cfg.readInputInterval = 16;
        cfg.renderInterval = 16;
        smoothMenu->setConfig(cfg);
        for (int i = 0; i < itemCount; i++) {
            int col = i % columns;
            int row = i / columns;
            smooth_ui_toolkit::SmoothSelectorMenu::Option_t option;
            option.keyframe = smooth_ui_toolkit::Vector4((float)(col * itemWidth),
                                                        (float)(row * itemHeight),
                                                        (float)itemWidth,
                                                        (float)itemHeight);
            option.userData = nullptr;
            smoothMenu->addOption(option);
        }
        if (selectedIndex < 0) selectedIndex = 0;
        if (selectedIndex >= itemCount) selectedIndex = itemCount - 1;
        selectedRow = selectedIndex / columns;
        selectedCol = selectedIndex % columns;
        const auto& opt = smoothMenu->getOptionList()[selectedIndex];
        smoothMenu->getSelectorPostion().teleport(opt.keyframe.x, opt.keyframe.y);
        smoothMenu->getSelectorShape().teleport(opt.keyframe.width, opt.keyframe.height);
    }
public:
    void draw(LovyanGFX* display) override {
        if (!visible) return;
        drawMenuBorder(display);
        int absX = getAbsoluteX();
        int absY = getAbsoluteY();
        int contentX = absX + 2;
        int contentY = absY + 2;
        int contentW = width - 4;
        int contentH = height - 4;
        int centerX = contentX + (contentW - itemWidth) / 2;
        int centerY = contentY + (contentH - itemHeight) / 2;
        smooth_ui_toolkit::Vector2 selectorPos(0.0f, 0.0f);
        const std::vector<smooth_ui_toolkit::SmoothSelectorMenu::Option_t>* optionList = nullptr;
        if (itemCount > 0 && (!smoothMenu || (int)smoothMenu->getOptionList().size() != itemCount)) {
            rebuildSmoothMenuLayout();
        }
        if (smoothMenu) {
            selectorPos = smoothMenu->getSelectorPostion();
            optionList = &smoothMenu->getOptionList();
        }
        for (int i = 0; i < itemCount; i++) {
            if (!items[i]) continue;
            const smooth_ui_toolkit::Vector4 keyframe =
                (optionList && i < (int)optionList->size())
                    ? (*optionList)[i].keyframe
                    : smooth_ui_toolkit::Vector4((float)((i % columns) * itemWidth),
                                                 (float)((i / columns) * itemHeight),
                                                 (float)itemWidth,
                                                 (float)itemHeight);
            int itemX = centerX + (int)(keyframe.x - selectorPos.x);
            int itemY = centerY + (int)(keyframe.y - selectorPos.y);
            if (itemX + itemWidth < contentX || itemX > contentX + contentW) continue;
            if (itemY + itemHeight < contentY || itemY > contentY + contentH) continue;
            MenuItem* item = items[i];
                Theme* currentTheme = getCurrentTheme();
                if (currentTheme) {
                    GridMenuItemDrawParams params;
                    params.display = display;
                    params.x = itemX;
                    params.y = itemY;
                    params.width = itemWidth;
                    params.height = itemHeight;
                    params.text = (item->imageData || item->useFileImage) ? String("") : item->text;
                    params.selected = (focused && i == selectedIndex);
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
                    if (focused && i == selectedIndex) {
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
                                if (imgW == 32 && imgH == 32) {
                                    scale = 1.0f;
                                }
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
                        if (focused && i == selectedIndex) {
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
    bool update(uint32_t nowMs) override {
        if (!visible || itemHeight <= 0 || itemCount == 0) return false;
        if (!smoothMenu || (int)smoothMenu->getOptionList().size() != itemCount) {
            rebuildSmoothMenuLayout();
            if (!smoothMenu) return false;
        }
        smoothMenu->update(nowMs);
        bool selectorMoving = !smoothMenu->getSelectorPostion().done();
        bool shapeChanging = !smoothMenu->getSelectorShape().done();
        return selectorMoving || shapeChanging;
    }
    bool handleSecondaryKeyEvent(const KeyEvent& event) override {
        if (itemCount == 0) return false;
        if (!smoothMenu) {
            rebuildSmoothMenuLayout();
        }
        int total = totalRows();
        if (total <= 0) total = 1;
        if (selectedIndex < 0) selectedIndex = 0;
        if (selectedIndex >= itemCount) selectedIndex = itemCount - 1;
        selectedRow = selectedIndex / columns;
        selectedCol = selectedIndex % columns;
        int newIndex = selectedIndex;
        if (event.up) {
            if (selectedRow > 0) {
                int candidate = (selectedRow - 1) * columns + selectedCol;
                if (candidate < itemCount) newIndex = candidate;
                else newIndex = itemCount - 1;
            }
        } else if (event.down) {
            if (selectedRow < total - 1) {
                int candidate = (selectedRow + 1) * columns + selectedCol;
                if (candidate < itemCount) newIndex = candidate;
                else newIndex = itemCount - 1;
            }
        } else if (event.left) {
            if (selectedCol > 0) {
                int candidate = selectedRow * columns + (selectedCol - 1);
                if (candidate < itemCount) newIndex = candidate;
            }
        } else if (event.right) {
            if (selectedCol < columns - 1) {
                int candidate = selectedRow * columns + (selectedCol + 1);
                if (candidate < itemCount) newIndex = candidate;
            }
        } else if (event.enter) {
            MenuItem* item = getSelectedItem();
            if (item && item->enabled) {
                onItemSelected(item);
                secondaryFocus = false;
            }
            return true;
        } else {
            return false;
        }

        if (newIndex != selectedIndex) {
            selectedIndex = newIndex;
            selectedRow = selectedIndex / columns;
            selectedCol = selectedIndex % columns;
            if (smoothMenu) {
                smoothMenu->moveTo(selectedIndex);
            }
            invalidate();
        }
        return true;
    }
};
