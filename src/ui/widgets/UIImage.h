#pragma once
#include <M5Cardputer.h>
#include <SD.h>
#include "WidgetBase.h"
class UIImage : public UIWidget {
private:
    const uint8_t* imageData;
    size_t imageDataSize;
    String filePath;
    bool useFile;
    float scaleX, scaleY;
    bool maintainAspectRatio;
public:
    UIImage(int id, int x, int y, int width, int height, const uint8_t* data, size_t dataSize, const String& name = "")
        : UIWidget(id, WIDGET_IMAGE, x, y, width, height, name, false),
          imageData(data), imageDataSize(dataSize), useFile(false),
          scaleX(1.0f), scaleY(1.0f), maintainAspectRatio(true) {
    }
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
            display->drawPngFile(filePath.c_str(), absX, absY);
        } else if (imageData && imageDataSize > 0) {
            display->drawPng(imageData, imageDataSize, absX, absY, width, height, 0, 0,
                           maintainAspectRatio ? 0 : scaleX,
                           maintainAspectRatio ? 0 : scaleY);
        }
    }
    bool handleKeyEvent(const KeyEvent& event) override {
        return false;
    }
};
