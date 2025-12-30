#pragma once
#include "ThemeManager.h"

class OrangeTheme : public Theme {
private:
    static const uint16_t ORANGE = 0xFD20;
    static const uint16_t ORANGE_DIM = 0xCA60;

    void drawChamferedRect(LovyanGFX* display, int x, int y, int width, int height, uint16_t borderColor, uint16_t fillColor) const {
        if (!display || width <= 2 || height <= 2) return;
        int x1 = x + width - 1;
        int y1 = y + height - 1;
        int r = 4;
        if (r * 2 > width) r = width / 2;
        if (r * 2 > height) r = height / 2;

        for (int yy = 0; yy < height; ++yy) {
            int yAbs = y + yy;
            int left = x + 1;
            int right = x1 - 1;
            if (yy < r) {
                int offset = r - yy;
                left = x + offset;
                right = x1 - offset;
            } else if (yy > height - 1 - r) {
                int offset = r - (height - 1 - yy);
                left = x + offset;
                right = x1 - offset;
            }
            if (left <= right) {
                display->drawFastHLine(left, yAbs, right - left + 1, fillColor);
            }
        }

        display->drawLine(x + r, y, x1 - r, y, borderColor);
        display->drawLine(x1, y + r, x1, y1 - r, borderColor);
        display->drawLine(x + r, y1, x1 - r, y1, borderColor);
        display->drawLine(x, y + r, x, y1 - r, borderColor);

        display->drawLine(x + r, y, x, y + r, borderColor);
        display->drawLine(x1 - r, y, x1, y + r, borderColor);
        display->drawLine(x1, y1 - r, x1 - r, y1, borderColor);
        display->drawLine(x, y1 - r, x + r, y1, borderColor);
    }

public:
    void drawLabel(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextColor(params.textColor);
        params.display->setTextSize(1);
        params.display->setCursor(params.x, params.y);
        params.display->print(params.text);
    }

    void drawButton(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;

        uint16_t bg = params.focused ? ORANGE : TFT_BLACK;
        uint16_t border = ORANGE;
        params.display->fillRect(params.x, params.y, params.width, params.height, bg);
        params.display->drawRect(params.x, params.y, params.width, params.height, border);

        if (params.imageData || params.useFile) {
            int imgW = 0, imgH = 0;
            bool ok = false;
            if (params.imageData && params.imageDataSize > 24) ok = pngGetSize(params.imageData, params.imageDataSize, imgW, imgH);
            else if (params.useFile && params.filePath.length() > 0) ok = pngFileGetSize(params.filePath, imgW, imgH);
            if (ok) {
                int maxW = params.width - 4;
                int maxH = params.height - 4;
                if (params.imageData) {
                    float sx = (float)maxW / (float)imgW;
                    float sy = (float)maxH / (float)imgH;
                    float scale = sx < sy ? sx : sy;
                    if (imgW == 32 && imgH == 32) {
                        scale = 1.0f;
                    }
                    if (scale > 1.0f) scale = 1.0f;
                    int dw = (int)(imgW * scale);
                    int dh = (int)(imgH * scale);
                    int cx = params.x + (params.width - dw) / 2;
                    int cy = params.y + (params.height - dh) / 2;
                    params.display->drawPng(params.imageData, params.imageDataSize, cx, cy, dw, dh, 0, 0, scale, scale);
                } else {
                    int cx = params.x + (params.width - imgW) / 2;
                    int cy = params.y + (params.height - imgH) / 2;
                    params.display->drawPngFile(params.filePath.c_str(), cx, cy);
                }
            }
        } else {
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextSize(1);
            int textWidth = params.text.length() * 6;
            int textHeight = 8;
            int textX = params.x + (params.width - textWidth) / 2;
            int textY = params.y + (params.height - textHeight) / 2;
            uint16_t textColor = params.focused ? TFT_BLACK : TFT_WHITE;
            params.display->setTextColor(textColor);
            params.display->setCursor(textX, textY);
            params.display->print(params.text);
        }
    }

    void drawWindow(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        drawChamferedRect(params.display, params.x, params.y, params.width, params.height, ORANGE, TFT_BLACK);
        if (!params.text.isEmpty()) {
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(TFT_WHITE);
            params.display->setTextSize(1);
            params.display->setCursor(params.x + 6, params.y + 4);
            params.display->print(params.text);
        }
    }

    void drawPanel(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        params.display->fillRect(params.x, params.y, params.width, params.height, TFT_BLACK);
        params.display->drawRect(params.x, params.y, params.width, params.height, ORANGE_DIM);
    }

    void drawSlider(const SliderDrawParams& params) override {
        if (!params.visible || !params.display) return;

        if (!params.label.isEmpty()) {
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(TFT_WHITE);
            params.display->setTextSize(1);
            params.display->setCursor(params.x, params.y - 12);
            params.display->print(params.label);
        }

        int trackY = params.y + params.height / 2 - 2;
        int trackHeight = 4;
        params.display->fillRect(params.x, trackY, params.width, trackHeight, TFT_DARKGREY);

        int thumbWidth = 8;
        int thumbHeight = params.height;
        int range = params.maxValue - params.minValue;
        int thumbX = params.x;
        if (range > 0) {
            thumbX = params.x + ((params.currentValue - params.minValue) * (params.width - thumbWidth)) / range;
        }

        uint16_t thumbColor = params.focused ? ORANGE : ORANGE_DIM;
        params.display->fillRect(thumbX, params.y, thumbWidth, thumbHeight, thumbColor);

        if (params.showValue) {
            String valueStr = String(params.currentValue);
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(TFT_WHITE);
            params.display->setTextSize(1);
            params.display->setCursor(params.x + params.width + 5, params.y + 2);
            params.display->print(valueStr);
        }
    }

    void drawMenuBorder(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        params.display->fillRect(params.x, params.y, params.width, params.height, TFT_BLACK);
        params.display->drawRect(params.x, params.y, params.width, params.height, ORANGE);
    }

    void drawMenuItem(const MenuItemDrawParams& params) override {
        if (!params.display) return;
        int margin = 1;
        int itemX = params.x + margin;
        int itemY = params.y + margin;
        int itemW = params.width - margin * 2;
        int itemH = params.height - margin * 2;
        if (itemW <= 4 || itemH <= 4) return;
        uint16_t borderColor = params.selected ? ORANGE_DIM : ORANGE;
        uint16_t bgColor = params.selected ? ORANGE : TFT_BLACK;
        uint16_t textColor = params.enabled ? TFT_WHITE : params.disabledColor;
        if (params.selected && params.enabled) {
            textColor = TFT_BLACK;
        }
        drawChamferedRect(params.display, itemX, itemY, itemW, itemH, borderColor, bgColor);
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextColor(textColor);
        params.display->setTextSize(1);
        int textWidth = params.text.length() * 6;
        int textX = itemX + 4;
        int textY = itemY + (itemH - 8) / 2 - 1;
        params.display->setCursor(textX, textY);
        params.display->print(params.text);
    }

    void drawGridMenuItem(const GridMenuItemDrawParams& params) override {
        if (!params.display) return;
        if (params.width <= 0 || params.height <= 0) return;

        int margin = 1;
        int itemX = params.x + margin;
        int itemY = params.y + margin;
        int itemW = params.width - margin * 2;
        int itemH = params.height - margin * 2;
        if (itemW <= 4 || itemH <= 4) return;

        uint16_t borderColor = params.selected ? ORANGE_DIM : ORANGE;
        uint16_t bgColor = params.selected ? ORANGE : TFT_BLACK;
        uint16_t textColor = params.enabled ? TFT_WHITE : params.disabledColor;
        if (params.selected && params.enabled) {
            textColor = TFT_BLACK;
        }

        drawChamferedRect(params.display, itemX, itemY, itemW, itemH, borderColor, bgColor);

        if (params.imageData || params.useFile) {
            int imgW = 0, imgH = 0;
            bool ok = false;
            if (params.imageData && params.imageDataSize > 24) ok = pngGetSize(params.imageData, params.imageDataSize, imgW, imgH);
            else if (params.useFile && params.filePath.length() > 0) ok = pngFileGetSize(params.filePath, imgW, imgH);
            if (ok) {
                int maxW = itemW - 4;
                int maxH = itemH - 4;
                if (params.imageData) {
                    float sx = (float)maxW / (float)imgW;
                    float sy = (float)maxH / (float)imgH;
                    float scale = sx < sy ? sx : sy;
                    if (imgW == 32 && imgH == 32) {
                        scale = 1.0f;
                    }
                    if (scale > 1.0f) scale = 1.0f;
                    int dw = (int)(imgW * scale);
                    int dh = (int)(imgH * scale);
                    int cx = itemX + (itemW - dw) / 2;
                    int cy = itemY + (itemH - dh) / 2;
                    params.display->drawPng(params.imageData, params.imageDataSize, cx, cy, dw, dh, 0, 0, scale, scale);
                } else {
                    int cx = itemX + (itemW - imgW) / 2;
                    int cy = itemY + (itemH - imgH) / 2;
                    params.display->drawPngFile(params.filePath.c_str(), cx, cy);
                }
            }
        } else if (!params.text.isEmpty()) {
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(textColor);
            params.display->setTextSize(1);
            int textWidth = params.text.length() * 6;
            int textX = itemX + (itemW - textWidth) / 2;
            int textY = itemY + (itemH - 8) / 2;
            params.display->setCursor(textX, textY);
            params.display->print(params.text);
        }
    }

    void clearArea(LovyanGFX* display, int x, int y, int width, int height) override {
        if (display) {
            display->fillRect(x, y, width, height, TFT_BLACK);
        }
    }

    String getThemeName() const override {
        return "Orange";
    }

    String getThemeDescription() const override {
        return "Orange accent theme with chamfered windows and dark background";
    }
};
