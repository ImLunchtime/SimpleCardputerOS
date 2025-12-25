#pragma once
#include <M5Cardputer.h>
#include "WidgetBase.h"
class UIWindow : public UIWidget {
private:
    String title;
    uint16_t borderColor;
    int childOffsetX;
    int childOffsetY;
public:
    UIWindow(int id, int x, int y, int width, int height, const String& title = "", const String& name = "")
        : UIWidget(id, WIDGET_WINDOW, x, y, width, height, name, false),
          title(title), borderColor(TFT_WHITE), childOffsetX(-6), childOffsetY(-6) {}
    int getChildOffsetX() const override { return childOffsetX; }
    int getChildOffsetY() const override { return childOffsetY; }
    void setChildOffset(int ox, int oy) { childOffsetX = ox; childOffsetY = oy; }
    void setTitle(const String& newTitle) { title = newTitle; }
    void setBorderColor(uint16_t color) { borderColor = color; }
    void draw(LGFX_Device* display) override {
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
            params.text = title;
            params.textColor = TFT_WHITE;
            params.borderColor = borderColor;
            params.backgroundColor = TFT_BLACK;
            theme->drawWindow(params);
        } else {
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            display->fillRect(absX, absY, width, height, TFT_BLACK);
            display->drawRect(absX, absY, width, height, borderColor);
            if (!title.isEmpty()) {
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(TFT_WHITE);
                display->setTextSize(1);
                display->setCursor(absX + 5, absY + 3);
                display->print(title);
            }
        }
    }
    void drawPartial(LGFX_Device* display) override {
        if (!visible) return;
        draw(display);
    }
    void clearAppArea(LGFX_Device* display) override {
        if (!visible) return;
        return;
    }
    void drawAppPartial(LGFX_Device* display) override {
        if (!visible) return;
        draw(display);
    }
    bool handleKeyEvent(const KeyEvent& event) override {
        return false;
    }
};
