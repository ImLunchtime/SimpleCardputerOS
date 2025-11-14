#pragma once
#include "ThemeManager.h"

// Windows 98主题实现 - 经典立体效果
class Windows98Theme : public Theme {
private:
    // Windows 98经典颜色定义
    static const uint16_t WIN98_BUTTON_FACE = 0xC618;     // 亮灰色 (192,192,192)
    static const uint16_t WIN98_BUTTON_HIGHLIGHT = 0xFFFF; // 白色高光
    static const uint16_t WIN98_BUTTON_SHADOW = 0x8410;    // 深灰色阴影 (128,128,128)
    static const uint16_t WIN98_BUTTON_DARK_SHADOW = 0x0000; // 黑色深阴影
    static const uint16_t WIN98_WINDOW_FRAME = 0xC618;     // 窗口框架色
    static const uint16_t WIN98_ACTIVE_CAPTION = 0x001F;   // 活动标题栏蓝色
    static const uint16_t WIN98_INACTIVE_CAPTION = 0x8410; // 非活动标题栏灰色
    static const uint16_t WIN98_WINDOW_TEXT = 0x0000;      // 黑色文本
    static const uint16_t WIN98_CAPTION_TEXT = 0xFFFF;     // 白色标题文本
    static const uint16_t WIN98_MENU_BAR = 0xC618;         // 菜单栏灰色
    static const uint16_t WIN98_MENU_HIGHLIGHT = 0x001F;   // 菜单高亮蓝色
    static const uint16_t WIN98_WINDOW_BACKGROUND = 0xC618; // 窗口背景灰色
    static const uint16_t WIN98_EDIT_BACKGROUND = 0xFFFF;  // 编辑框白色背景

    // 绘制立体边框的辅助函数
    void drawRaisedBorder(LGFX_Device* display, int x, int y, int width, int height) {
        // 外层高光 (左上)
        display->drawFastHLine(x, y, width - 1, WIN98_BUTTON_HIGHLIGHT);
        display->drawFastVLine(x, y, height - 1, WIN98_BUTTON_HIGHLIGHT);
        
        // 外层阴影 (右下)
        display->drawFastHLine(x + 1, y + height - 1, width - 1, WIN98_BUTTON_DARK_SHADOW);
        display->drawFastVLine(x + width - 1, y + 1, height - 1, WIN98_BUTTON_DARK_SHADOW);
        
        // 内层高光 (左上)
        display->drawFastHLine(x + 1, y + 1, width - 3, WIN98_BUTTON_FACE);
        display->drawFastVLine(x + 1, y + 1, height - 3, WIN98_BUTTON_FACE);
        
        // 内层阴影 (右下)
        display->drawFastHLine(x + 2, y + height - 2, width - 3, WIN98_BUTTON_SHADOW);
        display->drawFastVLine(x + width - 2, y + 2, height - 3, WIN98_BUTTON_SHADOW);
    }

    void drawSunkenBorder(LGFX_Device* display, int x, int y, int width, int height) {
        // 外层阴影 (左上)
        display->drawFastHLine(x, y, width - 1, WIN98_BUTTON_DARK_SHADOW);
        display->drawFastVLine(x, y, height - 1, WIN98_BUTTON_DARK_SHADOW);
        
        // 外层高光 (右下)
        display->drawFastHLine(x + 1, y + height - 1, width - 1, WIN98_BUTTON_HIGHLIGHT);
        display->drawFastVLine(x + width - 1, y + 1, height - 1, WIN98_BUTTON_HIGHLIGHT);
        
        // 内层阴影 (左上)
        display->drawFastHLine(x + 1, y + 1, width - 3, WIN98_BUTTON_SHADOW);
        display->drawFastVLine(x + 1, y + 1, height - 3, WIN98_BUTTON_SHADOW);
        
        // 内层高光 (右下)
        display->drawFastHLine(x + 2, y + height - 2, width - 3, WIN98_BUTTON_FACE);
        display->drawFastVLine(x + width - 2, y + 2, height - 3, WIN98_BUTTON_FACE);
    }

public:
    void drawLabel(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextColor(WIN98_WINDOW_TEXT);
        params.display->setTextSize(1);
        params.display->setCursor(params.x, params.y);
        params.display->print(params.text);
    }
    
    void drawButton(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        // 绘制按钮背景
        params.display->fillRect(params.x + 2, params.y + 2, params.width - 4, params.height - 4, WIN98_BUTTON_FACE);
        
        // 绘制立体边框
        if (params.focused) {
            // 聚焦时绘制按下效果
            drawSunkenBorder(params.display, params.x, params.y, params.width, params.height);
            
            // 绘制聚焦虚线框
            for (int i = 4; i < params.width - 4; i += 2) {
                params.display->drawPixel(params.x + i, params.y + 4, WIN98_WINDOW_TEXT);
                params.display->drawPixel(params.x + i, params.y + params.height - 5, WIN98_WINDOW_TEXT);
            }
            for (int i = 4; i < params.height - 4; i += 2) {
                params.display->drawPixel(params.x + 4, params.y + i, WIN98_WINDOW_TEXT);
                params.display->drawPixel(params.x + params.width - 5, params.y + i, WIN98_WINDOW_TEXT);
            }
        } else {
            // 正常状态绘制凸起效果
            drawRaisedBorder(params.display, params.x, params.y, params.width, params.height);
        }
        
        if (params.imageData || params.useFile) {
            int imgW = 0, imgH = 0;
            bool ok = false;
            if (params.imageData && params.imageDataSize > 24) ok = pngGetSize(params.imageData, params.imageDataSize, imgW, imgH);
            else if (params.useFile && params.filePath.length() > 0) ok = pngFileGetSize(params.filePath, imgW, imgH);
            if (ok) {
                int maxW = params.width - 6;
                int maxH = params.height - 6;
                if (params.imageData) {
                    float sx = (float)maxW / (float)imgW;
                    float sy = (float)maxH / (float)imgH;
                    float scale = sx < sy ? sx : sy;
                    if (scale > 1.0f) scale = 1.0f;
                    int dw = (int)(imgW * scale);
                    int dh = (int)(imgH * scale);
                    int cx = params.x + (params.width - dw) / 2;
                    int cy = params.y + (params.height - dh) / 2;
                    if (params.focused) { cx += 1; cy += 1; }
                    params.display->drawPng(params.imageData, params.imageDataSize, cx, cy, dw, dh, 0, 0, scale, scale);
                } else {
                    int cx = params.x + (params.width - imgW) / 2;
                    int cy = params.y + (params.height - imgH) / 2;
                    if (params.focused) { cx += 1; cy += 1; }
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
            if (params.focused) { textX += 1; textY += 1; }
            params.display->setTextColor(WIN98_WINDOW_TEXT);
            params.display->setCursor(textX, textY);
            params.display->print(params.text);
        }
    }
    
    void drawWindow(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        // 绘制窗口背景
        params.display->fillRect(params.x + 2, params.y + 15, params.width - 3, params.height - 17, WIN98_WINDOW_BACKGROUND);
        
        // 绘制标题栏
        params.display->fillRect(params.x + 2, params.y + 2, params.width - 4, 14, WIN98_ACTIVE_CAPTION);
        
        // 绘制窗口边框
        drawRaisedBorder(params.display, params.x, params.y, params.width, params.height);
        
        // 绘制标题栏文本
        if (!params.text.isEmpty()) {
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(WIN98_CAPTION_TEXT);
            params.display->setTextSize(1);
            params.display->setCursor(params.x + 6, params.y + 2);
            params.display->print(params.text);
        }
    }
    
    void drawSlider(const SliderDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        // 绘制滑块轨道 (凹陷效果)
        int trackY = params.y + params.height / 2 - 2;
        params.display->fillRect(params.x, trackY, params.width, 4, WIN98_BUTTON_SHADOW);
        drawSunkenBorder(params.display, params.x, trackY, params.width, 4);
        
        // 计算滑块位置
        int range = params.maxValue - params.minValue;
        int sliderPos = params.x;
        if (range > 0) {
            sliderPos = params.x + ((params.currentValue - params.minValue) * (params.width - 16)) / range;
        }
        
        // 绘制滑块 (凸起效果)
        params.display->fillRect(sliderPos, params.y, 16, params.height, WIN98_BUTTON_FACE);
        drawRaisedBorder(params.display, sliderPos, params.y, 16, params.height);
        
        // 绘制滑块中央的纹理线
        for (int i = 2; i < 14; i += 3) {
            params.display->drawFastVLine(sliderPos + i, params.y + 3, params.height - 6, WIN98_BUTTON_SHADOW);
            params.display->drawFastVLine(sliderPos + i + 1, params.y + 3, params.height - 6, WIN98_BUTTON_HIGHLIGHT);
        }
    }
    
    void drawMenuBorder(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        // 绘制菜单背景为白色
        params.display->fillRect(params.x + 2, params.y + 2, params.width - 4, params.height - 4, WIN98_EDIT_BACKGROUND);
        
        // 绘制菜单边框
        drawRaisedBorder(params.display, params.x, params.y, params.width, params.height);
    }
    
    void drawMenuItem(const MenuItemDrawParams& params) override {
        if (!params.display) return;
        
        uint16_t bgColor = WIN98_EDIT_BACKGROUND;  // 默认白色背景
        uint16_t textColor = WIN98_WINDOW_TEXT;    // 黑色文本
        
        // 如果选中，使用高亮颜色
        if (params.selected) {
            bgColor = WIN98_MENU_HIGHLIGHT;        // 蓝色高亮背景
            textColor = WIN98_CAPTION_TEXT;        // 白色文本
        }
        
        // 绘制菜单项背景（无边框）
        params.display->fillRect(params.x, params.y, params.width, params.height, bgColor);
        
        // 绘制菜单项文本
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextColor(textColor);
        params.display->setTextSize(1);
        params.display->setCursor(params.x + 4, params.y + (params.height - 10) / 2);
        params.display->print(params.text);
    }
    
    void drawGridMenuItem(const GridMenuItemDrawParams& params) override {
        if (!params.display) return;
        if (params.width <= 0 || params.height <= 0) return;
        
        // 绘制网格项背景
        uint16_t bgColor = WIN98_EDIT_BACKGROUND;  // 默认白色背景
        
        if (params.focused && params.selected) {
            // 选中状态：蓝色高亮背景
            bgColor = WIN98_MENU_HIGHLIGHT;
        } else if (!params.enabled) {
            // 禁用状态：浅灰色背景
            bgColor = WIN98_BUTTON_FACE;
        }
        
        // 绘制背景
        params.display->fillRect(params.x + 2, params.y + 2, 
                               params.width - 4, params.height - 4, bgColor);
        
        // 绘制3D边框效果
        if (params.focused && params.selected) {
            // 选中时绘制凹陷效果
            drawSunkenBorder(params.display, params.x, params.y, params.width, params.height);
        } else {
            // 正常状态绘制凸起效果
            drawRaisedBorder(params.display, params.x, params.y, params.width, params.height);
        }
        
        if (params.imageData || params.useFile) {
            int imgW = 0, imgH = 0;
            bool ok = false;
            if (params.imageData && params.imageDataSize > 24) ok = pngGetSize(params.imageData, params.imageDataSize, imgW, imgH);
            else if (params.useFile && params.filePath.length() > 0) ok = pngFileGetSize(params.filePath, imgW, imgH);
            if (ok) {
                int maxW = params.width - 6;
                int maxH = params.height - 6;
                if (params.imageData) {
                    float sx = (float)maxW / (float)imgW;
                    float sy = (float)maxH / (float)imgH;
                    float scale = sx < sy ? sx : sy;
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
        } else if (!params.text.isEmpty()) {
            uint16_t textColor = WIN98_WINDOW_TEXT;
            if (!params.enabled) textColor = WIN98_BUTTON_SHADOW;
            else if (params.focused && params.selected) textColor = WIN98_CAPTION_TEXT;
            params.display->setTextColor(textColor);
            params.display->setTextSize(1);
            int textWidth = params.text.length() * 6;
            int textX = params.x + (params.width - textWidth) / 2;
            int textY = params.y + (params.height - 8) / 2;
            params.display->setCursor(textX, textY);
            params.display->print(params.text);
        }
    }
    
    void clearArea(LGFX_Device* display, int x, int y, int width, int height) override {
        if (!display) return;
        display->fillRect(x, y, width, height, WIN98_WINDOW_BACKGROUND);
    }
    
    String getThemeName() const override {
        return "Windows 98";
    }
    
    String getThemeDescription() const override {
        return "Classic Windows 98 theme with 3D beveled controls";
    }
};

// 全局Windows 98主题实例
extern Windows98Theme windows98Theme;