#pragma once
#include <M5Cardputer.h>
#include "WidgetBase.h"
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
        if (event.enter) {
            onButtonClick();
            return true;
        }
        return false;
    }
    virtual void onButtonClick() {}
};
