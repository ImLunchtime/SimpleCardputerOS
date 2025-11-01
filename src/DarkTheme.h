#pragma once
#include "ThemeManager.h"

// 深色主题实现
class DarkTheme : public Theme {
public:
    void drawLabel(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextColor(TFT_LIGHTGREY);  // 使用浅灰色文本
        params.display->setTextSize(1);
        params.display->setCursor(params.x, params.y);
        params.display->print(params.text);
    }
    
    void drawButton(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        // 绘制深灰色背景
        params.display->fillRect(params.x, params.y, params.width, params.height, TFT_DARKGREY);
        
        // 绘制浅灰色边框
        params.display->drawRect(params.x, params.y, params.width, params.height, TFT_LIGHTGREY);
        
        // 如果聚焦，绘制蓝色外框
        if (params.focused) {
            params.display->drawRect(params.x - 1, params.y - 1, params.width + 2, params.height + 2, TFT_CYAN);
            params.display->drawRect(params.x - 2, params.y - 2, params.width + 4, params.height + 4, TFT_CYAN);
        }
        
        // 计算文本居中位置
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextSize(1);
        int textWidth = params.text.length() * 6;
        int textHeight = 8;
        int textX = params.x + (params.width - textWidth) / 2;
        int textY = params.y + (params.height - textHeight) / 2;
        
        // 绘制居中文本
        params.display->setTextColor(TFT_WHITE);
        params.display->setCursor(textX, textY);
        params.display->print(params.text);
    }
    
    void drawWindow(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        // 绘制深灰色背景
        params.display->fillRect(params.x, params.y, params.width, params.height, 0x2104);  // 深灰色
        
        // 绘制浅灰色边框
        params.display->drawRect(params.x, params.y, params.width, params.height, TFT_LIGHTGREY);
        
        // 如果有标题，绘制标题栏
        if (!params.text.isEmpty()) {
            // 绘制标题栏背景
            params.display->fillRect(params.x + 1, params.y + 1, params.width - 2, 14, 0x4208);  // 稍浅的灰色
            
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(TFT_WHITE);
            params.display->setTextSize(1);
            params.display->setCursor(params.x + 5, params.y + 3);
            params.display->print(params.text);
        }
    }
    
    void drawSlider(const SliderDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        // 绘制标签
        if (!params.label.isEmpty()) {
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(TFT_LIGHTGREY);
            params.display->setTextSize(1);
            params.display->setCursor(params.x, params.y - 12);
            params.display->print(params.label);
        }
        
        // 计算滑块轨道
        int trackY = params.y + params.height / 2 - 2;
        int trackHeight = 4;
        
        // 绘制轨道背景
        params.display->fillRect(params.x, trackY, params.width, trackHeight, 0x4208);  // 深灰色轨道
        
        // 计算滑块位置
        int thumbWidth = 8;
        int thumbHeight = params.height;
        int range = params.maxValue - params.minValue;
        int thumbX = params.x;
        if (range > 0) {
            thumbX = params.x + ((params.currentValue - params.minValue) * (params.width - thumbWidth)) / range;
        }
        
        // 绘制滑块
        uint16_t thumbColor = params.focused ? TFT_CYAN : TFT_LIGHTGREY;
        params.display->fillRect(thumbX, params.y, thumbWidth, thumbHeight, thumbColor);
        
        // 显示数值
        if (params.showValue) {
            String valueStr = String(params.currentValue);
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(TFT_LIGHTGREY);
            params.display->setTextSize(1);
            params.display->setCursor(params.x + params.width + 5, params.y + 2);
            params.display->print(valueStr);
        }
    }
    
    void drawMenuBorder(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        // 绘制菜单边框
        params.display->drawRect(params.x, params.y, params.width, params.height, TFT_LIGHTGREY);
        
        // 如果聚焦，绘制焦点边框
        if (params.focused) {
            params.display->drawRect(params.x - 1, params.y - 1, params.width + 2, params.height + 2, TFT_CYAN);
        }
    }
    
    void drawMenuItem(const MenuItemDrawParams& params) override {
        if (!params.display) return;
        
        // 绘制背景
        uint16_t bgColor = params.selected ? TFT_CYAN : 0x2104;  // 选中时用青色，否则用深灰色
        params.display->fillRect(params.x, params.y, params.width, params.height, bgColor);
        
        // 设置文本颜色
        uint16_t textColor = TFT_LIGHTGREY;
        if (!params.enabled) {
            textColor = 0x4208;  // 禁用时用更深的灰色
        } else if (params.selected) {
            textColor = TFT_BLACK;  // 选中时使用黑色文本以便在青色背景上显示
        }
        
        // 绘制文本
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextColor(textColor);
        params.display->setTextSize(1);
        params.display->setCursor(params.x + 2, params.y + 2);
        params.display->print(params.text);
    }
    
    void clearArea(LGFX_Device* display, int x, int y, int width, int height) override {
        if (display) {
            display->fillRect(x, y, width, height, TFT_BLACK);
        }
    }
    
    String getThemeName() const override {
        return "Dark";
    }
    
    String getThemeDescription() const override {
        return "Dark theme with grey tones and cyan accents";
    }
};

// 全局深色主题实例
extern DarkTheme darkTheme;