#pragma once
#include <M5Cardputer.h>
#include "UIMenu.h"
class UIMenuGrid : public UIMenu {
private:
    int columns;
    int rows;
    int itemWidth;
    int itemHeight;
    int selectedRow;
    int selectedCol;
    int scrollRowOffset;
    float scrollPixel;
    float targetScrollPixel;
    uint32_t lastAnimMs;
    bool animating;
public:
    UIMenuGrid(int id, int x, int y, int width, int height, int _columns, int _rows, const String& name = "")
        : UIMenu(id, WIDGET_MENU_GRID, x, y, width, height, name),
          columns(_columns), rows(_rows),
          selectedRow(0), selectedCol(0),
          scrollRowOffset(0), scrollPixel(0.0f),
          targetScrollPixel(0.0f), lastAnimMs(0),
          animating(false) {
        itemWidth = (width - 4) / columns;
        itemHeight = (height - 4) / rows;
    }
private:
    int totalRows() const {
        if (columns <= 0) return 0;
        if (itemCount <= 0) return 0;
        return (itemCount + columns - 1) / columns;
    }
    void setScrollRowOffsetAnimated(int newOffset) {
        if (newOffset < 0) newOffset = 0;
        int maxTop = totalRows() - rows;
        if (maxTop < 0) maxTop = 0;
        if (newOffset > maxTop) newOffset = maxTop;
        float current = animating ? scrollPixel : ((float)scrollRowOffset * (float)itemHeight);
        scrollRowOffset = newOffset;
        targetScrollPixel = (float)scrollRowOffset * (float)itemHeight;
        scrollPixel = current;
        animating = true;
        lastAnimMs = 0;
        invalidate();
    }
public:
    void draw(LovyanGFX* display) override {
        if (!visible) return;
        drawMenuBorder(display);
        int absX = getAbsoluteX();
        int absY = getAbsoluteY();
        float sp = animating ? scrollPixel : ((float)scrollRowOffset * (float)itemHeight);
        if (sp < 0.0f) sp = 0.0f;
        int maxTop = totalRows() - rows;
        if (maxTop < 0) maxTop = 0;
        float maxSp = (float)maxTop * (float)itemHeight;
        if (sp > maxSp) sp = maxSp;
        int firstRow = itemHeight > 0 ? (int)(sp / (float)itemHeight) : 0;
        int yOffset = itemHeight > 0 ? -(int)(sp - (float)firstRow * (float)itemHeight) : 0;
        int drawRows = rows + 1;
        for (int row = 0; row < drawRows; row++) {
            for (int col = 0; col < columns; col++) {
                int globalRow = firstRow + row;
                int itemIndex = globalRow * columns + col;
                if (itemIndex >= itemCount || !items[itemIndex]) continue;
                MenuItem* item = items[itemIndex];
                int itemX = absX + 2 + col * itemWidth;
                int itemY = absY + 2 + yOffset + row * itemHeight;
                Theme* currentTheme = getCurrentTheme();
                if (currentTheme) {
                    GridMenuItemDrawParams params;
                    params.display = display;
                    params.x = itemX;
                    params.y = itemY;
                    params.width = itemWidth;
                    params.height = itemHeight;
                    params.text = (item->imageData || item->useFileImage) ? String("") : item->text;
                    params.selected = (globalRow == selectedRow && col == selectedCol);
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
                    if (focused && globalRow == selectedRow && col == selectedCol) {
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
                        if (focused && globalRow == selectedRow && col == selectedCol) {
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
    bool update(uint32_t nowMs) override {
        if (!visible || itemHeight <= 0) return false;
        if (!animating) return false;
        if (lastAnimMs == 0) {
            lastAnimMs = nowMs;
            return true;
        }
        float dt = (float)(nowMs - lastAnimMs) / 1000.0f;
        lastAnimMs = nowMs;
        if (dt <= 0.0f) return false;
        float diff = targetScrollPixel - scrollPixel;
        float absDiff = diff >= 0.0f ? diff : -diff;
        if (absDiff < 0.5f) {
            scrollPixel = targetScrollPixel;
            animating = false;
            return true;
        }
        float speed = 80.0f;
        float maxStep = speed * dt;
        if (maxStep > absDiff) maxStep = absDiff;
        scrollPixel += (diff >= 0.0f ? maxStep : -maxStep);
        return true;
    }
    bool handleSecondaryKeyEvent(const KeyEvent& event) override {
        if (itemCount == 0) return false;
        int total = totalRows();
        if (total <= 0) total = 1;
        if (event.up) {
            if (selectedRow > 0) {
                selectedRow--;
                updateSelectedIndex();
                if (selectedRow < scrollRowOffset) {
                    setScrollRowOffsetAnimated(selectedRow);
                }
            }
            return true;
        }
        if (event.down) {
            if (selectedRow < total - 1) {
                selectedRow++;
                updateSelectedIndex();
                if (selectedRow >= scrollRowOffset + rows) {
                    setScrollRowOffsetAnimated(selectedRow - rows + 1);
                }
            }
            return true;
        }
        if (event.left && selectedCol > 0) {
            selectedCol--;
            updateSelectedIndex();
            return true;
        }
        if (event.right) {
            if (selectedCol < columns - 1) {
                int newIndex = selectedRow * columns + (selectedCol + 1);
                if (newIndex < itemCount) {
                    selectedCol++;
                    updateSelectedIndex();
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
private:
    void updateSelectedIndex() {
        selectedIndex = selectedRow * columns + selectedCol;
        if (selectedIndex >= itemCount) {
            selectedIndex = itemCount - 1;
            selectedRow = selectedIndex / columns;
            selectedCol = selectedIndex % columns;
        }
        invalidate();
    }
};
