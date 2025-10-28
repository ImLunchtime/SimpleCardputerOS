#pragma once
#include <M5Cardputer.h>

class Graphics {
private:
    LGFX_Device* display;

public:
    Graphics() : display(&M5Cardputer.Display) {}
    
    void clear() {
        display->fillScreen(TFT_BLACK);
    }
    
    void drawLabel(int x, int y, const String& text) {
        // 只绘制文本
        display->setTextColor(TFT_WHITE);
        display->setTextSize(1);
        display->setCursor(x, y);
        display->print(text);
    }
    
    void drawWindow(int x, int y, int width, int height, uint16_t borderColor = TFT_WHITE, bool focused = false) {
        // 绘制黑色背景
        display->fillRect(x, y, width, height, TFT_BLACK);
        
        // 绘制边框
        display->drawRect(x, y, width, height, borderColor);
        
        // 如果聚焦，绘制黄色外框
        if (focused) {
            display->drawRect(x - 1, y - 1, width + 2, height + 2, TFT_YELLOW);
            display->drawRect(x - 2, y - 2, width + 4, height + 4, TFT_YELLOW);
        }
    }
    
    void drawButton(int x, int y, int width, int height, const String& text, uint16_t borderColor = TFT_BLUE, bool focused = false) {
        // 绘制黑色背景
        display->fillRect(x, y, width, height, TFT_BLACK);
        
        // 绘制边框
        display->drawRect(x, y, width, height, borderColor);
        
        // 如果聚焦，绘制黄色外框
        if (focused) {
            display->drawRect(x - 1, y - 1, width + 2, height + 2, TFT_YELLOW);
            display->drawRect(x - 2, y - 2, width + 4, height + 4, TFT_YELLOW);
        }
        
        // 计算文本居中位置
        display->setTextSize(1);
        int textWidth = text.length() * 6; // 估算文本宽度
        int textHeight = 8; // 文本高度
        int textX = x + (width - textWidth) / 2;
        int textY = y + (height - textHeight) / 2;
        
        // 绘制居中文本
        display->setTextColor(TFT_WHITE);
        display->setCursor(textX, textY);
        display->print(text);
    }
};