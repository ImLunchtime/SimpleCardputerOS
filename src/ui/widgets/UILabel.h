#pragma once
#include <M5Cardputer.h>
#include "WidgetBase.h"
class UILabel : public UIWidget {
private:
    String text;
    uint16_t textColor;
public:
    UILabel(int id, int x, int y, const String& text, const String& name = "")
        : UIWidget(id, WIDGET_LABEL, x, y, text.length() * 6, 8, name, false),
          text(text), textColor(TFT_WHITE) {}
    void setText(const String& newText) {
        text = newText;
        width = text.length() * 6;
    }
    String getText() const { return text; }
    void setTextColor(uint16_t color) { textColor = color; }
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
            params.text = text;
            params.textColor = textColor;
            theme->drawLabel(params);
        } else {
            display->setFont(&fonts::efontCN_12);
            display->setTextColor(textColor);
            display->setTextSize(1);
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            display->setCursor(absX, absY);
            display->print(text);
        }
    }
    void drawPartial(LGFX_Device* display) override {
        if (!visible) return;
        clearArea(display);
        draw(display);
    }
    bool handleKeyEvent(const KeyEvent& event) override {
        return false;
    }
};
