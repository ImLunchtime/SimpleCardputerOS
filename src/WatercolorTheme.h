#pragma once
#include "ThemeManager.h"
#include "NinePatch.h"
#include "watercolor_window_ninepatch.h"
#include "watercolor_button_ninepatch.h"

// 水彩主题实现
class WatercolorTheme : public Theme {
private:
    NinePatchSet windowSet;
    NinePatchMetrics windowMetrics;
    NinePatchSet buttonSet;
    NinePatchMetrics buttonMetrics;

    static const uint16_t WC_TEXT = 0x0000;       // 黑色文本
    static const uint16_t WC_BG    = 0xFFFF;       // 白色背景
    static const uint16_t WC_ACCENT= 0x5CDF;       // 水彩青绿点缀
    static const uint16_t WC_BORDER= 0x753F;       // 深色边框/标题色

public:
    WatercolorTheme() {
        windowSet = makeNinePatch_watercolor_window();
        windowMetrics = NinePatchMetrics::fromSet(windowSet);

        buttonSet = makeNinePatch_watercolor_button();
        buttonMetrics = NinePatchMetrics::fromSet(buttonSet);
    }

    void drawLabel(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextColor(WC_TEXT);
        params.display->setTextSize(1);
        params.display->setCursor(params.x, params.y);
        params.display->print(params.text);
    }

    void drawButton(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;

        // 使用水彩按钮九宫格绘制
        NinePatchRenderer::drawWindow(
            params.display,
            buttonSet,
            params.x, params.y, params.width, params.height,
            buttonMetrics,
            NinePatchFillMode::Tile,
            NinePatchFillMode::Tile);

        // 聚焦时加一圈强调边框
        if (params.focused) {
            params.display->drawRect(params.x - 1, params.y - 1, params.width + 2, params.height + 2, WC_ACCENT);
        }

        // 文本居中
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextSize(1);
        int textWidth = params.text.length() * 6;
        int textHeight = 8;
        int textX = params.x + (params.width - textWidth) / 2;
        int textY = params.y + (params.height - textHeight) / 2;
        params.display->setTextColor(WC_TEXT);
        params.display->setCursor(textX, textY);
        params.display->print(params.text);
    }

    void drawWindow(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;

        // 使用水彩窗口九宫格绘制窗口
        NinePatchRenderer::drawWindow(
            params.display,
            windowSet,
            params.x, params.y, params.width, params.height,
            windowMetrics,
            NinePatchFillMode::Tile,
            NinePatchFillMode::Tile);

        // 标题（左上内边距稍微偏移）
        if (!params.text.isEmpty()) {
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(TFT_WHITE);
            params.display->setTextSize(1);
            params.display->setCursor(params.x + 8, params.y + 6);
            params.display->print(params.text);
        }
    }

    void drawSlider(const SliderDrawParams& params) override {
        if (!params.visible || !params.display) return;

        // 标签在上方
        if (!params.label.isEmpty()) {
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(WC_TEXT);
            params.display->setTextSize(1);
            params.display->setCursor(params.x, params.y - 12);
            params.display->print(params.label);
        }

        // 水彩风轨道与滑块
        int trackY = params.y + params.height / 2 - 2;
        params.display->fillRect(params.x, trackY, params.width, 4, WC_ACCENT);

        int thumbWidth = 8;
        int range = params.maxValue - params.minValue;
        int thumbX = params.x;
        if (range > 0) {
            thumbX = params.x + ((params.currentValue - params.minValue) * (params.width - thumbWidth)) / range;
        }
        uint16_t thumbColor = params.focused ? WC_BORDER : WC_TEXT;
        params.display->fillRect(thumbX, params.y, thumbWidth, params.height, thumbColor);

        if (params.showValue) {
            String valueStr = String(params.currentValue);
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(WC_TEXT);
            params.display->setTextSize(1);
            params.display->setCursor(params.x + params.width + 5, params.y + 2);
            params.display->print(valueStr);
        }
    }

    void drawMenuBorder(const ThemeDrawParams& params) override {
        if (!params.visible || !params.display) return;
        // 柔和背景 + 边框
        params.display->fillRect(params.x, params.y, params.width, params.height, WC_BG);
        params.display->drawRect(params.x, params.y, params.width, params.height, WC_BORDER);
        if (params.focused) {
            params.display->drawRect(params.x - 1, params.y - 1, params.width + 2, params.height + 2, WC_ACCENT);
        }
    }

    void drawMenuItem(const MenuItemDrawParams& params) override {
        if (!params.display) return;
        uint16_t bg = params.selected ? WC_ACCENT : WC_BG;
        uint16_t txt = params.enabled ? WC_TEXT : TFT_DARKGREY;
        if (params.selected) txt = TFT_BLACK;
        params.display->fillRect(params.x, params.y, params.width, params.height, bg);
        params.display->setFont(&fonts::efontCN_12);
        params.display->setTextColor(txt);
        params.display->setTextSize(1);
        params.display->setCursor(params.x + 2, params.y + 2);
        params.display->print(params.text);
    }

    void drawGridMenuItem(const GridMenuItemDrawParams& params) override {
        if (!params.display) return;

        // 使用水彩按钮九宫格作为网格项背景
        NinePatchRenderer::drawWindow(
            params.display,
            buttonSet,
            params.x, params.y, params.width, params.height,
            buttonMetrics,
            NinePatchFillMode::Tile,
            NinePatchFillMode::Tile);

        // 选中且有焦点时，叠加高亮边框
        if (params.focused && params.selected) {
            params.display->drawRect(params.x - 1, params.y - 1, params.width + 2, params.height + 2, TFT_WHITE);
        } else {
            // 常态边框
            params.display->drawRect(params.x, params.y, params.width, params.height, WC_BORDER);
        }

        // 文本居中绘制
        if (!params.text.isEmpty()) {
            uint16_t txt = params.enabled ? (params.focused && params.selected ? TFT_BLACK : WC_TEXT) : TFT_DARKGREY;
            int textWidth = params.text.length() * 6;
            int textX = params.x + (params.width - textWidth) / 2;
            int textY = params.y + (params.height - 8) / 2;
            params.display->setFont(&fonts::efontCN_12);
            params.display->setTextColor(txt);
            params.display->setTextSize(1);
            params.display->setCursor(textX, textY);
            params.display->print(params.text);
        }
    }

    void clearArea(LGFX_Device* display, int x, int y, int width, int height) override {
        if (display) {
            display->fillRect(x, y, width, height, WC_BG);
        }
    }

    String getThemeName() const override { return "Watercolor"; }
    String getThemeDescription() const override { return "Soft watercolor UI with nine-patch windows"; }
};