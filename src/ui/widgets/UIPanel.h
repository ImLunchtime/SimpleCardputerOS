#pragma once
#include <M5Cardputer.h>
#include "WidgetBase.h"
class UIPanel : public UIWidget {
private:
    uint16_t borderColor;
    uint16_t backgroundColor;
    int childOffsetX;
    int childOffsetY;
public:
    UIPanel(int id, int x, int y, int width, int height, const String& name = "")
        : UIWidget(id, WIDGET_PANEL, x, y, width, height, name, false),
          borderColor(TFT_WHITE), backgroundColor(TFT_BLACK), childOffsetX(0), childOffsetY(0) {}
    int getChildOffsetX() const override { return childOffsetX; }
    int getChildOffsetY() const override { return childOffsetY; }
    void setChildOffset(int ox, int oy) { childOffsetX = ox; childOffsetY = oy; }
    void setBorderColor(uint16_t color) { borderColor = color; }
    void setBackgroundColor(uint16_t color) { backgroundColor = color; }
    void draw(LovyanGFX* display) override {
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
            params.borderColor = borderColor;
            params.backgroundColor = backgroundColor;
            theme->drawPanel(params);
        } else {
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            display->fillRect(absX, absY, width, height, backgroundColor);
            display->drawRect(absX, absY, width, height, borderColor);
        }
    }
    void drawPartial(LovyanGFX* display) override {
        if (!visible) return;
        draw(display);
    }
    void clearAppArea(LovyanGFX* display) override {
        if (!visible) return;
        return;
    }
    void drawAppPartial(LovyanGFX* display) override {
        if (!visible) return;
        draw(display);
    }
    bool handleKeyEvent(const KeyEvent& event) override {
        return false;
    }
};

