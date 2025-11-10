#pragma once
#include "ThemeManager.h"
#include <cmath>
// 九宫格渲染
#include "NinePatch.h"
#include "windowdesign1_ninepatch.h"

// Frutiger Aero主题实现 - 现代白色主题，圆角设计，天蓝色渐变
class FrutigerAeroTheme : public Theme {
private:
    // Frutiger Aero颜色定义
    static const uint16_t AERO_WHITE = 0xFFFF;           // 纯白色
    static const uint16_t AERO_LIGHT_GRAY = 0xF7DE;     // 浅灰色 (248,248,248)
    static const uint16_t AERO_GRAY = 0xE71C;           // 中灰色 (224,224,224)
    static const uint16_t AERO_DARK_GRAY = 0xC618;      // 深灰色 (192,192,192)
    static const uint16_t AERO_SKY_BLUE = 0x87FF;       // 天蓝色 (135,206,255)
    static const uint16_t AERO_LIGHT_BLUE = 0xAEFD;     // 浅蓝色 (173,216,255)
    static const uint16_t AERO_BLUE_BORDER = 0x4A69;    // 蓝色边框 (70,130,180)
    static const uint16_t AERO_TEXT_BLACK = 0x0000;     // 黑色文本
    static const uint16_t AERO_TEXT_WHITE = 0xFFFF;     // 白色文本
    static const uint16_t AERO_SHADOW = 0xD69A;         // 阴影色 (210,210,210)
    
    // 圆角半径常量
    static const int CORNER_RADIUS = 8;
    static const int BUTTON_RADIUS = 16;  // 胶囊形按钮使用更大的圆角
    
    // 绘制渐变效果的辅助函数
    void drawVerticalGradient(LGFX_Device* display, int x, int y, int width, int height, 
                             uint16_t startColor, uint16_t endColor) {
        if (!display || width <= 0 || height <= 0) return;
        
        // 避免除零错误
        if (height <= 1) {
            display->fillRect(x, y, width, height, startColor);
            return;
        }
        
        for (int i = 0; i < height; i++) {
            // 安全的渐变比例计算
            float ratio = (float)i / (float)(height - 1);
            
            // 分离RGB分量
            uint8_t r1 = (startColor >> 11) & 0x1F;
            uint8_t g1 = (startColor >> 5) & 0x3F;
            uint8_t b1 = startColor & 0x1F;
            
            uint8_t r2 = (endColor >> 11) & 0x1F;
            uint8_t g2 = (endColor >> 5) & 0x3F;
            uint8_t b2 = endColor & 0x1F;
            
            // 插值计算
            uint8_t r = r1 + (uint8_t)((r2 - r1) * ratio);
            uint8_t g = g1 + (uint8_t)((g2 - g1) * ratio);
            uint8_t b = b1 + (uint8_t)((b2 - b1) * ratio);
            
            // 合成颜色
            uint16_t color = (r << 11) | (g << 5) | b;
            display->drawFastHLine(x, y + i, width, color);
        }
    }
    
    // 绘制胶囊形按钮
    void drawCapsuleButton(LGFX_Device* display, int x, int y, int width, int height, 
                          bool pressed = false) {
        if (!display || width <= 0 || height <= 0) return;
        
        // 计算安全的圆角半径
        int radius = min(BUTTON_RADIUS, min(width/2, height/2));
        
        // 背景色
        uint16_t bgColor = pressed ? AERO_LIGHT_GRAY : AERO_WHITE;
        
        // 绘制按钮背景
        display->fillRoundRect(x, y, width, height, radius, bgColor);
        
        // 绘制边框
        display->drawRoundRect(x, y, width, height, radius, AERO_BLUE_BORDER);
        
        // 简化的渐变效果 - 只在顶部绘制高光
        if (height > 4 && !pressed) {
            int highlightHeight = height / 3;
            display->fillRoundRect(x + 1, y + 1, width - 2, highlightHeight, 
                                 radius - 1, AERO_LIGHT_GRAY);
        }
    }

    // 九宫格资源（窗口皮肤）
    NinePatchSet windowNP;
    NinePatchMetrics windowMetrics;

public:
    FrutigerAeroTheme();

    void drawLabel(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        if (params.width <= 0 || params.height <= 0) return;
        
        // 清除背景
        params.display->fillRect(params.x, params.y, params.width, params.height, AERO_WHITE);
        
        // 绘制文本
        if (!params.text.isEmpty()) {
            params.display->setTextColor(AERO_TEXT_BLACK);
            params.display->setTextSize(1);
            params.display->setCursor(params.x + 4, params.y + (params.height - 8) / 2);
            params.display->print(params.text);
        }
    }
    
    void drawButton(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        if (params.width <= 0 || params.height <= 0) return;
        
        // 清除背景
        params.display->fillRect(params.x, params.y, params.width, params.height, AERO_WHITE);
        
        // 绘制胶囊形按钮
        drawCapsuleButton(params.display, params.x, params.y, params.width, params.height, params.focused);
        
        // 绘制按钮文本
        if (!params.text.isEmpty()) {
            params.display->setTextColor(AERO_TEXT_BLACK);
            params.display->setTextSize(1);
            
            // 计算文本居中位置
            int textWidth = params.text.length() * 6;  // 估算文本宽度
            int textX = params.x + (params.width - textWidth) / 2;
            int textY = params.y + (params.height - 8) / 2;
            
            params.display->setCursor(textX, textY);
            params.display->print(params.text);
        }
    }
    
    void drawWindow(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        if (params.width <= 0 || params.height <= 0) return;

        // 使用九宫格渲染窗口整体
        NinePatchRenderer::drawWindow(
            params.display,
            windowNP,
            params.x, params.y, params.width, params.height,
            windowMetrics,
            NinePatchFillMode::Tile,   // 边框平铺
            NinePatchFillMode::Tile    // 中心平铺
        );

        // 标题文本叠加（可选）
        if (!params.text.isEmpty()) {
            params.display->setTextColor(AERO_TEXT_BLACK);
            params.display->setTextSize(1);
            // 贴近上边缘的内边距
            int textX = params.x + 6;
            int textY = params.y + 4;
            params.display->setCursor(textX, textY);
            params.display->print(params.text);
        }
    }
    
    void drawSlider(const SliderDrawParams& params) override {
        if (!params.visible || !params.display) return;
        if (params.width <= 0 || params.height <= 0) return;
        
        // 清除背景
        params.display->fillRect(params.x, params.y, params.width, params.height, AERO_WHITE);
        
        // 绘制滑块轨道
        int trackY = params.y + params.height / 2 - 2;
        int trackHeight = 4;
        if (params.width > 20) {
            params.display->fillRoundRect(params.x + 10, trackY, params.width - 20, trackHeight, 
                                        2, AERO_GRAY);
        }
        
        // 计算滑块位置 - 添加安全检查
        int range = params.maxValue - params.minValue;
        if (range > 0 && params.width > 40) {
            int thumbX = params.x + 10 + ((params.currentValue - params.minValue) * (params.width - 40)) / range;
            
            // 绘制滑块
            params.display->fillCircle(thumbX, params.y + params.height / 2, 8, AERO_WHITE);
            params.display->drawCircle(thumbX, params.y + params.height / 2, 8, AERO_BLUE_BORDER);
            params.display->drawCircle(thumbX, params.y + params.height / 2, 7, AERO_BLUE_BORDER);
        }
        
        // 绘制标签和数值
        if (!params.label.isEmpty()) {
            params.display->setTextColor(AERO_TEXT_BLACK);
            params.display->setTextSize(1);
            params.display->setCursor(params.x, params.y);
            params.display->print(params.label);
        }
        
        if (params.showValue) {
            params.display->setTextColor(AERO_TEXT_BLACK);
            params.display->setTextSize(1);
            String valueStr = String(params.currentValue);
            int textWidth = valueStr.length() * 6;
            params.display->setCursor(params.x + params.width - textWidth, params.y);
            params.display->print(valueStr);
        }
    }
    
    void drawMenuBorder(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        if (params.width <= 0 || params.height <= 0) return;
        
        // 计算安全的圆角半径
        int radius = min(CORNER_RADIUS, min(params.width/2, params.height/2));
        
        // 绘制菜单背景 - 白色圆角矩形
        params.display->fillRoundRect(params.x, params.y, params.width, params.height, 
                                    radius, AERO_WHITE);
        
        // 绘制边框
        params.display->drawRoundRect(params.x, params.y, params.width, params.height, 
                                    radius, AERO_SHADOW);
    }
    
    void drawMenuItem(const MenuItemDrawParams& params) override {
        if (!params.enabled || !params.display) return;
        if (params.width <= 0 || params.height <= 0) return;
        
        // 绘制选中状态背景
        if (params.selected) {
            params.display->fillRect(params.x + 2, params.y, params.width - 4, params.height, 
                                   AERO_LIGHT_BLUE);
        } else {
            params.display->fillRect(params.x + 2, params.y, params.width - 4, params.height, 
                                   AERO_WHITE);
        }
        
        // 绘制菜单项文本
        if (!params.text.isEmpty()) {
            uint16_t textColor = params.enabled ? 
                (params.selected ? AERO_TEXT_BLACK : AERO_TEXT_BLACK) : AERO_DARK_GRAY;
            
            params.display->setTextColor(textColor);
            params.display->setTextSize(1);
            params.display->setCursor(params.x + 8, params.y + (params.height - 8) / 2);
            params.display->print(params.text);
        }
    }
    
    void drawGridMenuItem(const GridMenuItemDrawParams& params) override {
        if (!params.display) return;
        if (params.width <= 0 || params.height <= 0) return;
        
        // 计算安全的圆角半径
        int radius = min(6, min(params.width/4, params.height/4));
        
        // 绘制网格项背景
        uint16_t bgColor = AERO_WHITE;
        uint16_t borderColor = AERO_GRAY;
        
        if (params.focused && params.selected) {
            // 选中状态：浅蓝色背景，蓝色边框
            bgColor = AERO_LIGHT_BLUE;
            borderColor = AERO_BLUE_BORDER;
        } else if (!params.enabled) {
            // 禁用状态：浅灰色背景
            bgColor = AERO_LIGHT_GRAY;
            borderColor = AERO_GRAY;
        }
        
        // 绘制圆角背景
        params.display->fillRoundRect(params.x + 1, params.y + 1, 
                                    params.width - 2, params.height - 2, 
                                    radius, bgColor);
        
        // 绘制圆角边框
        params.display->drawRoundRect(params.x, params.y, 
                                    params.width, params.height, 
                                    radius, borderColor);
        
        // 如果选中且有焦点，绘制额外的高亮边框
        if (params.focused && params.selected) {
            params.display->drawRoundRect(params.x - 1, params.y - 1, 
                                        params.width + 2, params.height + 2, 
                                        radius + 1, AERO_BLUE_BORDER);
        }
        
        // 绘制文本
        if (!params.text.isEmpty()) {
            uint16_t textColor = AERO_TEXT_BLACK;
            if (!params.enabled) {
                textColor = AERO_DARK_GRAY;
            }
            
            params.display->setTextColor(textColor);
            params.display->setTextSize(1);
            
            // 计算文本居中位置
            int textWidth = params.text.length() * 6;
            int textX = params.x + (params.width - textWidth) / 2;
            int textY = params.y + (params.height - 8) / 2;
            
            params.display->setCursor(textX, textY);
            params.display->print(params.text);
        }
    }
    
    void clearArea(LGFX_Device* display, int x, int y, int width, int height) override {
        if (!display || width <= 0 || height <= 0) return;
        display->fillRect(x, y, width, height, AERO_WHITE);
    }
    
    String getThemeName() const override {
        return "Frutiger Aero";
    }
    
    String getThemeDescription() const override {
        return "Modern white theme with rounded corners and sky blue gradients";
    }
};

// 全局主题实例
extern FrutigerAeroTheme frutigerAeroTheme;