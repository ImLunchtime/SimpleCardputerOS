#pragma once
#include "ThemeManager.h"

// 原型主题实现
class PrototypeTheme : public Theme {
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
        
        // 绘制黑色背景
        params.display->fillRect(params.x, params.y, params.width, params.height, params.backgroundColor);
        
        // 绘制边框
        params.display->drawRect(params.x, params.y, params.width, params.height, params.borderColor);
        
        // 如果聚焦，绘制黄色外框
        if (params.focused) {
            params.display->drawRect(params.x - 1, params.y - 1, params.width + 2, params.height + 2, TFT_YELLOW);
            params.display->drawRect(params.x - 2, params.y - 2, params.width + 4, params.height + 4, TFT_YELLOW);
        }
        
        // 计算文本居中位置
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextSize(1);
        int textWidth = params.text.length() * 6;
        int textHeight = 8;
        int textX = params.x + (params.width - textWidth) / 2;
        int textY = params.y + (params.height - textHeight) / 2;
        
        // 绘制居中文本
        params.display->setTextColor(params.textColor);
        params.display->setCursor(textX, textY);
        params.display->print(params.text);
    }
    
    void drawWindow(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        // 绘制黑色背景
        params.display->fillRect(params.x, params.y, params.width, params.height, params.backgroundColor);
        
        // 绘制边框
        params.display->drawRect(params.x, params.y, params.width, params.height, params.borderColor);
        
        // 如果有标题，绘制标题栏
        if (!params.text.isEmpty()) {
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(params.textColor);
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
            params.display->setTextColor(params.textColor);
            params.display->setTextSize(1);
            params.display->setCursor(params.x, params.y - 12);
            params.display->print(params.label);
        }
        
        // 计算滑块轨道
        int trackY = params.y + params.height / 2 - 2;
        int trackHeight = 4;
        
        // 绘制轨道背景
        params.display->fillRect(params.x, trackY, params.width, trackHeight, params.trackColor);
        
        // 计算滑块位置
        int thumbWidth = 8;
        int thumbHeight = params.height;
        int range = params.maxValue - params.minValue;
        int thumbX = params.x;
        if (range > 0) {
            thumbX = params.x + ((params.currentValue - params.minValue) * (params.width - thumbWidth)) / range;
        }
        
        // 绘制滑块
        uint16_t thumbColor = params.focused ? TFT_YELLOW : params.thumbColor;
        params.display->fillRect(thumbX, params.y, thumbWidth, thumbHeight, thumbColor);
        
        // 显示数值
        if (params.showValue) {
            String valueStr = String(params.currentValue);
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(params.textColor);
            params.display->setTextSize(1);
            params.display->setCursor(params.x + params.width + 5, params.y + 2);
            params.display->print(valueStr);
        }
    }
    
    void drawMenuBorder(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        
        // 绘制菜单边框
        params.display->drawRect(params.x, params.y, params.width, params.height, params.borderColor);
        
        // 如果聚焦，绘制焦点边框
        if (params.focused) {
            params.display->drawRect(params.x - 1, params.y - 1, params.width + 2, params.height + 2, TFT_YELLOW);
        }
    }
    
    void drawMenuItem(const MenuItemDrawParams& params) override {
        if (!params.display) return;
        
        // 绘制背景
        uint16_t bgColor = params.selected ? params.selectedColor : params.backgroundColor;
        params.display->fillRect(params.x, params.y, params.width, params.height, bgColor);
        
        // 设置文本颜色
        uint16_t textColor = params.textColor;
        if (!params.enabled) {
            textColor = params.disabledColor;
        } else if (params.selected) {
            textColor = TFT_BLACK;  // 选中时使用黑色文本以便在黄色背景上显示
        }
        
        // 绘制文本
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextColor(textColor);
        params.display->setTextSize(1);
        params.display->setCursor(params.x + 2, params.y + 2);
        params.display->print(params.text);
    }
    
    void drawGridMenuItem(const GridMenuItemDrawParams& params) override {
        if (!params.display) return;
        if (params.width <= 0 || params.height <= 0) return;
        
        // 绘制网格项背景
        uint16_t bgColor = TFT_BLACK;  // 默认黑色背景
        uint16_t borderColor = TFT_WHITE;  // 白色边框
        
        if (params.focused && params.selected) {
            // 选中状态：黄色背景
            bgColor = TFT_YELLOW;
            borderColor = TFT_WHITE;
        } else if (!params.enabled) {
            // 禁用状态：深灰色背景
            bgColor = TFT_DARKGREY;
            borderColor = TFT_DARKGREY;
        }
        
        // 绘制背景
        params.display->fillRect(params.x + 1, params.y + 1, 
                               params.width - 2, params.height - 2, bgColor);
        
        // 绘制边框
        params.display->drawRect(params.x, params.y, 
                               params.width, params.height, borderColor);
        
        // 如果选中且有焦点，绘制额外的高亮边框
        if (params.focused && params.selected) {
            params.display->drawRect(params.x - 1, params.y - 1, 
                                   params.width + 2, params.height + 2, TFT_YELLOW);
        }
        
        // 绘制文本
        if (!params.text.isEmpty()) {
            uint16_t textColor = TFT_WHITE;
            if (!params.enabled) {
                textColor = TFT_DARKGREY;
            } else if (params.focused && params.selected) {
                textColor = TFT_BLACK;  // 选中时使用黑色文本
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
        if (display) {
            display->fillRect(x, y, width, height, TFT_BLACK);
        }
    }
    
    String getThemeName() const override {
        return "Prototype";
    }
    
    String getThemeDescription() const override {
        return "Prototype system theme with classic appearance";
    }
};

// 全局原型主题实例
extern PrototypeTheme prototypeTheme;