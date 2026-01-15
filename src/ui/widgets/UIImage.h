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
    float zoom;
    int offsetX;
    int offsetY;
public:
    UIImage(int id, int x, int y, int width, int height, const uint8_t* data, size_t dataSize, const String& name = "")
        : UIWidget(id, WIDGET_IMAGE, x, y, width, height, name, false),
          imageData(data), imageDataSize(dataSize), useFile(false),
          scaleX(1.0f), scaleY(1.0f), maintainAspectRatio(true),
          zoom(1.0f), offsetX(0), offsetY(0) {
    }
    UIImage(int id, int x, int y, int width, int height, const String& file, const String& name = "")
        : UIWidget(id, WIDGET_IMAGE, x, y, width, height, name, false),
          imageData(nullptr), imageDataSize(0), filePath(file), useFile(true),
          scaleX(1.0f), scaleY(1.0f), maintainAspectRatio(true),
          zoom(1.0f), offsetX(0), offsetY(0) {
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
    void setZoom(float z) {
        if (z < 0.25f) z = 0.25f;
        if (z > 4.0f) z = 4.0f;
        if (zoom == z) return;
        zoom = z;
        if (zoom == 1.0f && offsetX == 0 && offsetY == 0) {
            maintainAspectRatio = true;
            scaleX = 1.0f;
            scaleY = 1.0f;
        } else {
            maintainAspectRatio = false;
            scaleX = zoom;
            scaleY = zoom;
        }
        invalidate();
    }
    float getZoom() const {
        return zoom;
    }
    void resetTransform() {
        zoom = 1.0f;
        offsetX = 0;
        offsetY = 0;
        maintainAspectRatio = true;
        scaleX = 1.0f;
        scaleY = 1.0f;
        invalidate();
    }
    void panBy(int dx, int dy) {
        if (dx == 0 && dy == 0) return;
        offsetX += dx;
        offsetY += dy;
        if (zoom != 1.0f || offsetX != 0 || offsetY != 0) {
            if (zoom < 0.25f) zoom = 0.25f;
            if (zoom > 4.0f) zoom = 4.0f;
            maintainAspectRatio = false;
            scaleX = zoom;
            scaleY = zoom;
        } else {
            maintainAspectRatio = true;
            scaleX = 1.0f;
            scaleY = 1.0f;
        }
        invalidate();
    }
    void setOffset(int xOff, int yOff) {
        offsetX = xOff;
        offsetY = yOff;
        if (zoom != 1.0f || offsetX != 0 || offsetY != 0) {
            if (zoom < 0.25f) zoom = 0.25f;
            if (zoom > 4.0f) zoom = 4.0f;
            maintainAspectRatio = false;
            scaleX = zoom;
            scaleY = zoom;
        } else {
            maintainAspectRatio = true;
            scaleX = 1.0f;
            scaleY = 1.0f;
        }
        invalidate();
    }
    void getOffset(int& xOff, int& yOff) const {
        xOff = offsetX;
        yOff = offsetY;
    }
    void draw(LovyanGFX* display) override {
        if (!visible) return;
        int absX = getAbsoluteX();
        int absY = getAbsoluteY();
        if (useFile && filePath.length() > 0) {
            printf("[UIImage] draw from file \"%s\" at (%d,%d) offset(%d,%d) zoom=%.2f\n",
                   filePath.c_str(), absX, absY, offsetX, offsetY, zoom);
            File f = SD.open(filePath, FILE_READ);
            if (!f) {
                printf("[UIImage] failed to open file on SD\n");
            } else {
                size_t fileSize = f.size();
                printf("[UIImage] file opened, size=%u\n", (unsigned int)fileSize);
                if (fileSize > 0) {
                    uint8_t* buffer = (uint8_t*)malloc(fileSize);
                    if (!buffer) {
                        printf("[UIImage] malloc failed for size=%u\n", (unsigned int)fileSize);
                    } else {
                        size_t readBytes = f.read(buffer, fileSize);
                        printf("[UIImage] read %u bytes from file\n", (unsigned int)readBytes);
                        if (readBytes > 0) {
                            bool ok = display->drawPng(buffer, readBytes,
                                                       absX, absY,
                                                       width, height,
                                                       offsetX, offsetY,
                                                       maintainAspectRatio ? 0 : scaleX,
                                                       maintainAspectRatio ? 0 : scaleY);
                            printf("[UIImage] drawPng from file buffer result=%d\n", ok ? 1 : 0);
                        } else {
                            printf("[UIImage] read returned 0 bytes\n");
                        }
                        free(buffer);
                    }
                } else {
                    printf("[UIImage] file size is 0\n");
                }
                f.close();
            }
        } else if (imageData && imageDataSize > 0) {
            printf("[UIImage] draw from memory data size=%u at (%d,%d) offset(%d,%d) zoom=%.2f\n",
                   (unsigned int)imageDataSize, absX, absY, offsetX, offsetY, zoom);
            display->drawPng(imageData, imageDataSize,
                             absX, absY,
                             width, height,
                             offsetX, offsetY,
                             maintainAspectRatio ? 0 : scaleX,
                             maintainAspectRatio ? 0 : scaleY);
        }
    }
    bool handleKeyEvent(const KeyEvent& event) override {
        return false;
    }
};
