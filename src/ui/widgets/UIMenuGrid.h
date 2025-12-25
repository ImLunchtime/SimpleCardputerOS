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
public:
    UIMenuGrid(int id, int x, int y, int width, int height, int _columns, int _rows, const String& name = "")
        : UIMenu(id, WIDGET_MENU_GRID, x, y, width, height, name),
          columns(_columns), rows(_rows), selectedRow(0), selectedCol(0) {
        itemWidth = (width - 4) / columns;
        itemHeight = (height - 4) / rows;
    }
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        drawMenuBorder(display);
        int absX = getAbsoluteX();
        int absY = getAbsoluteY();
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < columns; col++) {
                int itemIndex = row * columns + col;
                if (itemIndex >= itemCount || !items[itemIndex]) continue;
                MenuItem* item = items[itemIndex];
                int itemX = absX + 2 + col * itemWidth;
                int itemY = absY + 2 + row * itemHeight;
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
