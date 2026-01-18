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
        if (z < 0.1f) z = 0.1f;
        if (z > 8.0f) z = 8.0f;
        if (zoom != z) {
            zoom = z;
            invalidate();
        }
    }
    float getZoom() const {
        return zoom;
    }
    void resetTransform() {
        zoom = 1.0f;
        offsetX = 0;
        offsetY = 0;
        invalidate();
    }
    void panBy(int dx, int dy) {
        if (dx == 0 && dy == 0) return;
        offsetX += dx;
        offsetY += dy;
        invalidate();
    }
    void setOffset(int xOff, int yOff) {
        offsetX = xOff;
        offsetY = yOff;
        invalidate();
    }
    void getOffset(int& xOff, int& yOff) const {
        xOff = offsetX;
        yOff = offsetY;
    }
    bool getImageSize(int& imgW, int& imgH) const {
        imgW = 0;
        imgH = 0;
        bool ok = false;
        if (useFile && filePath.length() > 0) {
            ok = pngFileGetSize(filePath, imgW, imgH);
        } else if (imageData && imageDataSize > 0) {
            ok = pngGetSize(imageData, imageDataSize, imgW, imgH);
        }
        if (!ok) {
            imgW = 0;
            imgH = 0;
        }
        return ok;
    }
    void draw(LovyanGFX* display) override {
        if (!visible) return;
        int absX = getAbsoluteX();
        int absY = getAbsoluteY();
        float effectiveZoom = zoom;
        if (effectiveZoom < 0.1f) effectiveZoom = 0.1f;
        if (effectiveZoom > 8.0f) effectiveZoom = 8.0f;
        if (useFile && filePath.length() > 0) {
            printf("[UIImage] draw from file \"%s\" at (%d,%d) offset(%d,%d) zoom=%.2f\n",
                   filePath.c_str(), absX, absY, offsetX, offsetY, effectiveZoom);
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
                            int imgW = 0;
                            int imgH = 0;
                            bool hasSize = pngGetSize(buffer, readBytes, imgW, imgH);
                            float scale = 1.0f;
                            if (hasSize && imgW > 0 && imgH > 0) {
                                float baseScaleX = (float)width / (float)imgW;
                                float baseScaleY = (float)height / (float)imgH;
                                float baseScale = baseScaleX < baseScaleY ? baseScaleX : baseScaleY;
                                scale = baseScale * effectiveZoom;
                            } else {
                                scale = effectiveZoom;
                            }
                            bool ok = display->drawPng(buffer, readBytes,
                                                       absX, absY,
                                                       width, height,
                                                       offsetX, offsetY,
                                                       scale, scale);
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
                   (unsigned int)imageDataSize, absX, absY, offsetX, offsetY, effectiveZoom);
            int imgW = 0;
            int imgH = 0;
            bool hasSize = pngGetSize(imageData, imageDataSize, imgW, imgH);
            float scale = 1.0f;
            if (hasSize && imgW > 0 && imgH > 0) {
                float baseScaleX = (float)width / (float)imgW;
                float baseScaleY = (float)height / (float)imgH;
                float baseScale = baseScaleX < baseScaleY ? baseScaleX : baseScaleY;
                scale = baseScale * effectiveZoom;
            } else {
                scale = effectiveZoom;
            }
            display->drawPng(imageData, imageDataSize,
                             absX, absY,
                             width, height,
                             offsetX, offsetY,
                             scale, scale);
        }
    }
    bool handleKeyEvent(const KeyEvent& event) override {
        return false;
    }
};
